#include "AssetActionNamingCheck.h"
#include "AssetRegistryModule.h"
#include "ObjectTools.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "WidgetBlueprint.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/UserDefinedStruct.h"
#include "AssetToolsModule.h"
#include "ISourceControlModule.h"
#include "FileHelpers.h"


AssetActionNamingCheck::AssetActionNamingCheck()
{
	NamingPatterns = GetDefaultPatterns();

	NamingPatterns.Sort([](const NamingPattern& A, const NamingPattern& B)
	{
		return A.Class->IsChildOf(B.Class);
	});
}

void AssetActionNamingCheck::ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId)
{
	for (FAssetInfo& Asset : Assets)
	{
		FString name = Asset.Data.AssetName.ToString();
		FString suggested_name = GetNameForAsset(name, Asset.Data.GetClass());
		
		if(!name.Equals(suggested_name))
		{
			Asset.ActionResults.Add(AssignedId, suggested_name);
		}
	}
}

void AssetActionNamingCheck::ExecuteAction(TArray<FAssetData> Assets)
{
	for (FAssetData& Asset : Assets) 
	{
		FString name = Asset.AssetName.ToString();
		FString suggested_name = GetNameForAsset(name, Asset.GetClass());

		if (!name.Equals(suggested_name))
		{
			TArray<UPackage*> FilesToSave;
			
			TSet<UPackage*> ObjectsUserRefusedToFullyLoad;
			FText ErrorMessage;

			ObjectTools::FPackageGroupName PGN;
			PGN.ObjectName = suggested_name;
			PGN.GroupName = TEXT("");
			PGN.PackageName = Asset.PackagePath.ToString() / suggested_name;

			UObject* Object = Asset.GetAsset();
			UPackage* OldPackage = Object->GetOutermost();

			bool WasRooted = false;
			if(!OldPackage->IsRooted())
			{
				OldPackage->AddToRoot();
				WasRooted = true;
			}
			
			bool Result = ObjectTools::RenameSingleObject(Object, PGN, ObjectsUserRefusedToFullyLoad, ErrorMessage, nullptr, true);

			if(!Result)
			{
				FNotificationInfo Notification(ErrorMessage);
				Notification.ExpireDuration = 3.0f;
				FSlateNotificationManager::Get().AddNotification(Notification);
			}
			else
			{
				FilesToSave.Add(Object->GetOutermost());
				FilesToSave.Add(OldPackage);
			}

			if (WasRooted)
			{
				OldPackage->RemoveFromRoot();
			}

			if (FilesToSave.Num() > 0)
			{
				FEditorFileUtils::PromptForCheckoutAndSave(FilesToSave, false, false, nullptr, true);
				ISourceControlModule::Get().QueueStatusUpdate(FilesToSave);
			}

			if (Result)
			{
				UObjectRedirector* Redirector = LoadObject<UObjectRedirector>(UObjectRedirector::StaticClass(), *Asset.PackageName.ToString());

				if (Redirector != nullptr)
				{
					FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
					AssetToolsModule.Get().FixupReferencers({ Redirector });
				}
			}
		}
	}
}

TArray<NamingPattern> AssetActionNamingCheck::GetDefaultPatterns()
{
	TArray<NamingPattern> Patterns;
	
	Patterns.Add({ UWidgetBlueprint::StaticClass(), "WBP_", "" });

	Patterns.Add({ UUserDefinedStruct::StaticClass(), "F", "" });
	Patterns.Add({ UUserDefinedEnum::StaticClass(), "E", "" });

	Patterns.Add({ UMaterialInstanceConstant::StaticClass(), "MI_", "" });
	Patterns.Add({ UMaterial::StaticClass(), "M_", "" });

	Patterns.Add({ UStaticMesh::StaticClass(), "SM_", "" });

	return Patterns;
}

FString AssetActionNamingCheck::GetNameForAsset(FString Name, UClass* Class)
{
	NamingPattern* Pattern = nullptr;
	
	for(NamingPattern& Check : NamingPatterns)
	{
		if(Check.Class == Class)
		{
			Pattern = &Check;
			break;
		}
	}

	//TODO improve class detection
	/*if (Pattern == nullptr) 
	{
		for (NamingPattern& Check : NamingPatterns)
		{
			if (Class->IsChildOf(Check.Class))
			{
				Pattern = &Check;
				break;
			}
		}
	}*/

	if (Pattern == nullptr)
	{
		return Name;
	}
	else 
	{
		FString result = Name;
		if (!Name.Mid(0, Pattern->Prefix.Len()).Equals(Pattern->Prefix)) result = Pattern->Prefix + result;
		//TODO add suffix check
		return result;
	}
}

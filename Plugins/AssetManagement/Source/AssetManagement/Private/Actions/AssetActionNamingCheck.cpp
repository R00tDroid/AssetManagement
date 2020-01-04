#include "AssetActionNamingCheck.h"
#include "AssetRegistryModule.h"
#include "ObjectTools.h"
#include "NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "WidgetBlueprint.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/UserDefinedStruct.h"


AssetActionNamingCheck::AssetActionNamingCheck()
{
	NamingPatterns.Empty();
	NamingPatterns.Add({ UWidgetBlueprint::StaticClass(), "WBP_", "" });
	
	NamingPatterns.Add({ UUserDefinedStruct::StaticClass(), "F", "" });
	NamingPatterns.Add({ UUserDefinedEnum::StaticClass(), "E", "" });

	NamingPatterns.Add({ UMaterialInstanceConstant::StaticClass(), "MI_", "" });
	NamingPatterns.Add({ UMaterial::StaticClass(), "M_", "" });

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
			TSet<UPackage*> ObjectsUserRefusedToFullyLoad;
			FText ErrorMessage;

			ObjectTools::FPackageGroupName PGN;
			PGN.ObjectName = suggested_name;
			PGN.GroupName = TEXT("");
			PGN.PackageName = Asset.PackagePath.ToString() / suggested_name;

			UPackage* OldPackage = Asset.GetAsset()->GetOutermost();

			bool rooted = false;
			if(!OldPackage->IsRooted())
			{
				OldPackage->AddToRoot();
				rooted = true;
			}
			
			bool result = ObjectTools::RenameSingleObject(Asset.GetAsset(), PGN, ObjectsUserRefusedToFullyLoad, ErrorMessage, {}, false);

			if(!result)
			{
				FNotificationInfo Notification(ErrorMessage);
				Notification.ExpireDuration = 3.0f;
				FSlateNotificationManager::Get().AddNotification(Notification);
			}

			if (rooted)
			{
				OldPackage->RemoveFromRoot();
			}

			if(result) ObjectTools::CleanupAfterSuccessfulDelete({ OldPackage });
		}
	}
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

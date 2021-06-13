#include "AssetActionNamingCheck.h"

#include "AssetMagementConfig.h"
#include "AssetMagementCore.h"
#include "ObjectTools.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "FileHelpers.h"
#include "ISourceControlModule.h"
#include "Dom/JsonValue.h"
#include "JsonSerializer.h"
#include "JsonWriter.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/UserDefinedStruct.h"
#include "AssetToolsModule.h"
#include "WidgetBlueprint.h"
#include "Animation/AnimBlueprint.h"

AssetActionNamingCheck::AssetActionNamingCheck()
{
	OnConfigChanged();
	OnConfigChangedHandle = AssetManagerConfig::OnConfigChanged.AddRaw(this, &AssetActionNamingCheck::OnConfigChanged);
}

AssetActionNamingCheck::~AssetActionNamingCheck()
{
	AssetManagerConfig::OnConfigChanged.Remove(OnConfigChangedHandle);
}

void AssetActionNamingCheck::OnConfigChanged()
{
	FString JsonData = AssetManagerConfig::Get().GetString("Actions", "NamingPatterns", "");
	if (!JsonData.IsEmpty())
	{
		NamingPatterns = JsonToNamingPatterns(JsonData);
	}
	else
	{
		NamingPatterns = GetDefaultPatterns();
		JsonData = NamingPatternsToJson(NamingPatterns);
		AssetManagerConfig::Get().SetString("Actions", "NamingPatterns", JsonData);
		AssetManagerConfig::OnConfigChanged.Broadcast();
	}

	NamingPatterns.Sort([](const FNamingPattern& A, const FNamingPattern& B)
	{
		return A.Class->IsChildOf(B.Class);
	});

	AssetManager* manager = AssetManager::Get();
	if (manager != nullptr)
	{
		manager->Get()->RequestRescan();
	}
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

TArray<FNamingPattern> AssetActionNamingCheck::GetDefaultPatterns()
{
	TArray<FNamingPattern> Patterns;

	Patterns.Add({ UBlueprint::StaticClass(), "BP_", "" });
	Patterns.Add({ UAnimBlueprint::StaticClass(), "", "_AnimBP" });
	Patterns.Add({ UWidgetBlueprint::StaticClass(), "WBP_", "" });

	Patterns.Add({ UUserDefinedStruct::StaticClass(), "F", "" });
	Patterns.Add({ UUserDefinedEnum::StaticClass(), "E", "" });

	Patterns.Add({ UMaterialInstanceConstant::StaticClass(), "MI_", "" });
	Patterns.Add({ UMaterial::StaticClass(), "M_", "" });

	Patterns.Add({ UStaticMesh::StaticClass(), "SM_", "" });

	return Patterns;
}

FString AssetActionNamingCheck::NamingPatternsToJson(const TArray<FNamingPattern>& Patterns)
{
	TArray<TSharedPtr<FJsonObject>> PatternObjects;
	TArray<TSharedPtr<FJsonValue>> PatternValues;
	for(const FNamingPattern& Pattern : Patterns)
	{
		TSharedPtr<FJsonObject> Object = MakeShareable(new FJsonObject);
		PatternObjects.Add(Object);
		Object->SetStringField("Class", Pattern.Class->GetPathName());
		Object->SetStringField("Prefix", Pattern.Prefix);
		Object->SetStringField("Suffix", Pattern.Suffix);

		PatternValues.Add(MakeShareable(new FJsonValueObject(Object)));
	}

	TSharedRef<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetArrayField("Patterns", PatternValues);
	
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject, Writer);
	
	return OutputString;
}

TArray<FNamingPattern> AssetActionNamingCheck::JsonToNamingPatterns(const FString& InputString)
{
	TArray<FNamingPattern> Patterns = {};
	
	TSharedPtr<FJsonObject> RootObject;
	
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InputString);
	FJsonSerializer::Deserialize(Reader, RootObject);

	if (RootObject.IsValid())
	{
		TArray<TSharedPtr<FJsonValue>> Array = RootObject->GetArrayField("Patterns");
		for(TSharedPtr<FJsonValue>& Value : Array)
		{
			const TSharedPtr<FJsonObject>* Object;
			if (Value->TryGetObject(Object))
			{
				FString ClassName, Prefix, Suffix;
				Object->Get()->TryGetStringField("Class", ClassName);
				Object->Get()->TryGetStringField("Prefix", Prefix);
				Object->Get()->TryGetStringField("Suffix", Suffix);

				UClass* Class = nullptr;
				if (!ClassName.IsEmpty()) Class = LoadClass<UObject>(nullptr, *ClassName);

				if (Class != nullptr)
				{
					Patterns.Add({ Class, Prefix, Suffix });
				}
			}
		}
	}
	
	return Patterns;
}

FString AssetActionNamingCheck::GetNameForAsset(FString Name, UClass* Class)
{
	FNamingPattern* Pattern = nullptr;
	
	for(FNamingPattern& Check : NamingPatterns)
	{
		if (Class == Check.Class || Class->IsChildOf(Check.Class))
		{
			Pattern = &Check;
			break;
		}
	}

	if (Pattern == nullptr)
	{
		return Name;
	}
	else 
	{
		FString result = Name;
		if (!Pattern->Prefix.IsEmpty() && !Name.Mid(0, Pattern->Prefix.Len()).Equals(Pattern->Prefix)) result = Pattern->Prefix + result;
		if (!Pattern->Suffix.IsEmpty() && !Name.Mid(Name.Len() - Pattern->Suffix.Len()).Equals(Pattern->Suffix)) result = result + Pattern->Suffix;
		return result;
	}
}

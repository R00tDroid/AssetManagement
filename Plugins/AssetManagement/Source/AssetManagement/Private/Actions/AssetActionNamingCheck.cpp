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
#include "Animation/BlendSpace.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/Rig.h"
#include "Particles/ParticleSystem.h"

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
		FString suggested_name = GetNameForAsset(name, Asset.Data.GetClass(), Asset.Data.GetAsset());
		
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
		FString suggested_name = GetNameForAsset(name, Asset.GetClass(), Asset.GetAsset());

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

	Patterns.Add({ UBlueprint::StaticClass(), {}, "BP_", "" });
	Patterns.Add({ UBlueprint::StaticClass(), {{ "BlueprintType", EClassPropertyType::CPT_Byte, FString::FromInt(EBlueprintType::BPTYPE_FunctionLibrary) }}, "BPFL_", "" });
	Patterns.Add({ UBlueprint::StaticClass(), {{ "BlueprintType", EClassPropertyType::CPT_Byte, FString::FromInt(EBlueprintType::BPTYPE_Interface) }}, "BPI_", "" });
	Patterns.Add({ UBlueprint::StaticClass(), {{ "BlueprintType", EClassPropertyType::CPT_Byte, FString::FromInt(EBlueprintType::BPTYPE_MacroLibrary) }}, "BPML_", "" });
	Patterns.Add({ UAnimBlueprint::StaticClass(), {}, "ABP_", "" });
	Patterns.Add({ UWidgetBlueprint::StaticClass(), {}, "WBP_", "" });

	Patterns.Add({ UUserDefinedStruct::StaticClass(), {}, "F", "" });
	Patterns.Add({ UUserDefinedEnum::StaticClass(), {}, "E", "" });

	Patterns.Add({ UMaterialInstanceConstant::StaticClass(), {}, "MI_", "" });
	Patterns.Add({ UMaterial::StaticClass(), {}, "M_", "" });
	
	Patterns.Add({ UStaticMesh::StaticClass(), {}, "SM_", "" });
	Patterns.Add({ USkeletalMesh::StaticClass(), {}, "SK_", "" });

	//Patterns.Add({ UTexture::StaticClass(), {}, "T_", "_?" });
	//Patterns.Add({ URenderTarget::StaticClass(), {}, "RT_", "" });
	//Patterns.Add({ UMediaTexture::StaticClass(), {}, "MT_", "" });

	Patterns.Add({ UParticleSystem::StaticClass(), {}, "PS_", "" });

	/*Patterns.Add({ UAimOffset::StaticClass(), {}, "AO_", "" });
	Patterns.Add({ UAimOffset1D::StaticClass(), {}, "AO_", "" });
	Patterns.Add({ UAnimationComposite::StaticClass(), {}, "AC_", "" });
	Patterns.Add({ UAnimationMontage::StaticClass(), {}, "AM_", "" });
	Patterns.Add({ UAnimationSequence::StaticClass(), {}, "A_", "" });*/
	Patterns.Add({ UBlendSpace::StaticClass(), {}, "BS_", "" });
	Patterns.Add({ UBlendSpace1D::StaticClass(), {}, "BS_", "" });

	//Patterns.Add({ ULevelSequence::StaticClass(), {}, "LS_", "" });
	Patterns.Add({ URig::StaticClass(), {}, "Rig_", "" });
	Patterns.Add({ USkeleton::StaticClass(), {}, "Skel_", "" });
	
	return Patterns;
}

FString AssetActionNamingCheck::NamingPatternsToJson(const TArray<FNamingPattern>& Patterns)
{
	if (Patterns.Num() == 0)
	{
		return "";
	}

	TArray<TSharedPtr<FJsonValue>> PatternValues;
	for(const FNamingPattern& Pattern : Patterns)
	{
		TSharedPtr<FJsonObject> Object = MakeShareable(new FJsonObject);
		Object->SetStringField("Class", Pattern.Class->GetPathName());
		Object->SetStringField("Prefix", Pattern.Prefix);
		Object->SetStringField("Suffix", Pattern.Suffix);

		TArray<TSharedPtr<FJsonValue>> PropertyList;
		for (const FPropertyFilter& Filter : Pattern.ClassProperties)
		{
			TSharedPtr<FJsonObject> FilterObject = MakeShareable(new FJsonObject);
			FilterObject->SetStringField("Property", Filter.PropertyName);
			FilterObject->SetNumberField("Type", (int)Filter.PropertyType);
			FilterObject->SetStringField("Value", Filter.ExpectedValue);
			PropertyList.Add(MakeShareable(new FJsonValueObject(FilterObject)));
		}
		Object->SetArrayField("Properties", PropertyList);

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
				TArray<FPropertyFilter> PropertyFilters;
				
				Object->Get()->TryGetStringField("Class", ClassName);
				Object->Get()->TryGetStringField("Prefix", Prefix);
				Object->Get()->TryGetStringField("Suffix", Suffix);

				const TArray<TSharedPtr<FJsonValue>>* FilterArray;
				if (Object->Get()->TryGetArrayField("Properties", FilterArray))
				{
					for (TSharedPtr<FJsonValue> Filter : (*FilterArray))
					{
						const TSharedPtr<FJsonObject>* FilterObject;
						if (Filter->TryGetObject(FilterObject))
						{
							FString PropertyName, ExpectedValue;
							int Type;
							FilterObject->Get()->TryGetStringField("Property", PropertyName);
							FilterObject->Get()->TryGetNumberField("Type", Type);
							FilterObject->Get()->TryGetStringField("Value", ExpectedValue);

							if (!PropertyName.IsEmpty() && !ExpectedValue.IsEmpty())
							{
								PropertyFilters.Add({ PropertyName, static_cast<EClassPropertyType>(Type), ExpectedValue });
							}
						}
					}
				}

				UClass* Class = nullptr;
				if (!ClassName.IsEmpty()) Class = LoadClass<UObject>(nullptr, *ClassName);

				if (Class != nullptr)
				{
					Patterns.Add({ Class, PropertyFilters, Prefix, Suffix });
				}
			}
		}
	}
	
	return Patterns;
}

#define FindPropertyValue(PropType, TargetVar, PropClass, PropName, PropObject) U##PropType##Property* Prop = FindField<U##PropType##Property>(PropClass, PropName); if (Prop != nullptr) { TargetVar = Prop->GetPropertyValue_InContainer(PropObject); }

FString GetObjectProperty(UClass* Class, UObject* Object, FString PropertyName, EClassPropertyType PropertyType)
{
	switch (PropertyType)
	{
		case EClassPropertyType::CPT_String:
		{
			FString Out = "";
			FindPropertyValue(Str, Out, Class, *PropertyName, Object);
			return Out;
		}
		case EClassPropertyType::CPT_Byte:
		{
			unsigned char Out = 0;
			FindPropertyValue(Byte, Out, Class, *PropertyName, Object);
			return FString::FromInt(Out);
		}
		case EClassPropertyType::CPT_Int32:
		{
			int32 Out = 0;
			FindPropertyValue(Int, Out, Class, *PropertyName, Object);
			return FString::FromInt(Out);
		}
		case EClassPropertyType::CPT_Float:
		{
			float Out = 0;
			FindPropertyValue(Float, Out, Class, *PropertyName, Object);
			return FString::SanitizeFloat(Out);
		}
	}

	return "";
}

FString AssetActionNamingCheck::GetNameForAsset(FString Name, UClass* Class, UObject* Object)
{
	FNamingPattern* Pattern = nullptr;

	for(FNamingPattern& Check : NamingPatterns)
	{
		if (Class == Check.Class || Class->IsChildOf(Check.Class))
		{
			bool Valid = true;

			for( FPropertyFilter& PropertyFilter : Check.ClassProperties)
			{
				FString Value = GetObjectProperty(Class, Object, PropertyFilter.PropertyName, PropertyFilter.PropertyType);
				if (Value != PropertyFilter.ExpectedValue)
				{
					Valid = false;
					break;
				}
			}
			
			if (Valid) 
			{
				Pattern = &Check;
				break;
			}
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

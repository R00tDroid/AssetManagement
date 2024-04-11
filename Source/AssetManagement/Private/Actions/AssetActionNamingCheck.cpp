#include "AssetActionNamingCheck.h"

#include "AssetManagementConfig.h"
#include "AssetManagementCore.h"
#include "ObjectTools.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "FileHelpers.h"
#include "ISourceControlModule.h"
#include "Dom/JsonValue.h"

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
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTargetCube.h"
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

#define NAMED_OBJECT(CLASS_NAME) TSoftClassPtr<UObject>(FSoftObjectPath(TEXT("/Script/" #CLASS_NAME))).Get()
#define STATIC_OBJECT(CLASS_NAME) CLASS_NAME::StaticClass()

#define DEF_BASEBEG(CLASS) Patterns.Add({ CLASS
#define DEF_BASEEND() })
#define DEF_PREFIX(CLASS, PREFIX) DEF_BASEBEG(CLASS), {}, PREFIX, "" DEF_BASEEND()
#define DEF_SUFFIX(CLASS, SUFFIX) DEF_BASEBEG(CLASS), {}, "", SUFFIX DEF_BASEEND()
#define DEF_PREFIX_SUFFIX(CLASS, PREFIX, SUFFIX) DEF_BASEBEG(CLASS), {}, PREFIX, SUFFIX DEF_BASEEND()

TArray<FNamingPattern> AssetActionNamingCheck::GetDefaultPatterns()
{
    TArray<FNamingPattern> Patterns;

    DEF_PREFIX(STATIC_OBJECT(UBlueprint), "BP_");
    Patterns.Add({ UBlueprint::StaticClass(), {{ "BlueprintType", EClassPropertyType::CPT_Byte, FString::FromInt(EBlueprintType::BPTYPE_FunctionLibrary) }}, "BPFL_", "" });
    Patterns.Add({ UBlueprint::StaticClass(), {{ "BlueprintType", EClassPropertyType::CPT_Byte, FString::FromInt(EBlueprintType::BPTYPE_Interface) }}, "BPI_", "" });
    Patterns.Add({ UBlueprint::StaticClass(), {{ "BlueprintType", EClassPropertyType::CPT_Byte, FString::FromInt(EBlueprintType::BPTYPE_MacroLibrary) }}, "BPML_", "" });
    DEF_PREFIX(STATIC_OBJECT(UAnimBlueprint), "ABP_");
    DEF_PREFIX(STATIC_OBJECT(UWidgetBlueprint), "WBP_");

    DEF_PREFIX(STATIC_OBJECT(UUserDefinedStruct), "F");
    DEF_PREFIX(STATIC_OBJECT(UUserDefinedEnum), "E");

    DEF_PREFIX(STATIC_OBJECT(UMaterialInstanceConstant), "MI_");
    DEF_PREFIX(STATIC_OBJECT(UMaterial), "M_");
    
    DEF_PREFIX(STATIC_OBJECT(UStaticMesh), "SM_");
    DEF_PREFIX(STATIC_OBJECT(USkeletalMesh), "SK_");

    DEF_PREFIX(STATIC_OBJECT(UTexture), "T_");
    DEF_PREFIX(STATIC_OBJECT(UTextureRenderTarget2D), "RT_");
    DEF_PREFIX(STATIC_OBJECT(UTextureRenderTargetCube), "RTC_");
    DEF_PREFIX(NAMED_OBJECT(MediaAssets.MediaTexture), "MT_");
    DEF_PREFIX(NAMED_OBJECT(MediaAssets.MediaPlayer), "MP_");

    DEF_PREFIX(STATIC_OBJECT(UParticleSystem), "PS_");

    DEF_PREFIX(NAMED_OBJECT(Engine.AimOffsetBlendSpace), "AO_");
    DEF_PREFIX(NAMED_OBJECT(Engine.AimOffsetBlendSpace1D), "AO_");
    DEF_PREFIX(NAMED_OBJECT(Engine.AnimComposite), "AC_");
    DEF_PREFIX(NAMED_OBJECT(Engine.AnimMontage), "AM_");
    DEF_PREFIX(NAMED_OBJECT(Engine.AnimSequence), "A_");
    DEF_PREFIX(STATIC_OBJECT(UBlendSpace), "BS_");
    DEF_PREFIX(STATIC_OBJECT(UBlendSpace1D), "BS_");

    DEF_PREFIX(STATIC_OBJECT(URig), "Rig_");
    DEF_PREFIX(STATIC_OBJECT(USkeleton), "Skel_");

    DEF_PREFIX(NAMED_OBJECT(Engine.Font), "Font_");
    
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

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 26
    #define FindPropertyValue(PropType, TargetVar, PropClass, PropName, PropObject) U##PropType##Property* Prop = FindField<U##PropType##Property>(PropClass, PropName); if (Prop != nullptr) { TargetVar = Prop->GetPropertyValue_InContainer(PropObject); }
#else
    #define FindPropertyValue(PropType, TargetVar, PropClass, PropName, PropObject) F##PropType##Property* Prop = FindFProperty<F##PropType##Property>(PropClass, PropName); if (Prop != nullptr) { TargetVar = Prop->GetPropertyValue_InContainer(PropObject); }
#endif

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

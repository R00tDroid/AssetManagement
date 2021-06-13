#include "ProjectSettingsEditor.h"
#include "AssetMagementConfig.h"
#include "AssetManagementModule.h"
#include "Actions/AssetActionNamingCheck.h"

TMap<TSubclassOf<UObject>, FNamingConventionList> ConvertNamingConventions(const TArray<FNamingPattern>& In)
{
    TMap<TSubclassOf<UObject>, FNamingConventionList> Out;
    
    for(const FNamingPattern& Naming : In)
    {
        if (!Out.Contains(Naming.Class))
        {
            Out.Add(Naming.Class, {});
        }
        
        Out[Naming.Class].Conventions.Add({ Naming.ClassProperties, Naming.Prefix, Naming.Suffix });
    }

    return Out;
}

TArray<FNamingPattern> ConvertNamingConventions(const TMap<TSubclassOf<UObject>, FNamingConventionList>& In)
{
    TArray<FNamingPattern> Out;

    for (const auto& ListIt : In)
    {
        for (const auto FilterIt : ListIt.Value.Conventions)
        Out.Add({ ListIt.Key, FilterIt.PropertyFilters, FilterIt.Prefix, FilterIt.Suffix });
    }

    return Out;
}

UProjectSettingsEditor& UProjectSettingsEditor::Get()
{
    if (IsInGameThread())
    {
        FAssetManagementModule& EditorModule = FModuleManager::LoadModuleChecked<FAssetManagementModule>("SREditor");
        return *EditorModule.GetSettingsEditor();
    }
    else
    {
        FAssetManagementModule& EditorModule = FModuleManager::GetModuleChecked<FAssetManagementModule>("SREditor");
        return *EditorModule.GetSettingsEditor();
    }
}

void UProjectSettingsEditor::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
    SaveConfig();

    auto CurrentProperty = PropertyChangedEvent.PropertyChain.GetHead();

    if (CurrentProperty != nullptr)
    {
        if (CurrentProperty->GetValue() != nullptr)
        {
            const FName& PropertyName(CurrentProperty->GetValue()->GetFName());

            if (PropertyName == GET_MEMBER_NAME_CHECKED(UProjectSettingsEditor, NamingConventions))
            {
                AssetManagerConfig::OnConfigChanged.Broadcast();
            }
        }
    }


    Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void UProjectSettingsEditor::SaveConfig()
{
    AssetManagerConfig::Get().SetUseProjectSettings(SettingStorage == EProjectSettingStorage::PSS_ProjectGlobal);
    
    TArray<FNamingPattern> Patterns = ConvertNamingConventions(NamingConventions);
    FString JsonData = AssetActionNamingCheck::NamingPatternsToJson(Patterns);
    AssetManagerConfig::Get().SetString("Actions", "NamingPatterns", JsonData);
}

void UProjectSettingsEditor::LoadConfig()
{
    FString JsonData = AssetManagerConfig::Get().GetString("Actions", "NamingPatterns", "");
    if (!JsonData.IsEmpty())
    {
        TArray<FNamingPattern> Patterns = AssetActionNamingCheck::JsonToNamingPatterns(JsonData);
        NamingConventions = ConvertNamingConventions(Patterns);
    }
}

void UProjectSettingsEditor::PostInitProperties()
{
    Super::PostInitProperties();
    SettingStorage = AssetManagerConfig::Get().UsesProjectSettings() ? EProjectSettingStorage::PSS_ProjectGlobal : EProjectSettingStorage::PSS_PerUser;

    LoadConfig();
    AssetManagerConfig::OnConfigChanged.AddUObject(this, &UProjectSettingsEditor::LoadConfig);

    SaveConfig();
}

#if WITH_EDITOR
bool UProjectSettingsEditor::CanEditChange(const FProperty* InProperty) const
{
    return true;
}
#endif

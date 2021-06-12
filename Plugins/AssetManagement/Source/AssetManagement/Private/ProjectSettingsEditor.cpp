#include "ProjectSettingsEditor.h"
#include "AssetMagementConfig.h"
#include "AssetManagementModule.h"
#include "Actions/AssetActionNamingCheck.h"

TMap<UClass*, FNamingConvention> ConvertNamingConventions(const TArray<FNamingPattern>& In)
{
    TMap<UClass*, FNamingConvention> Out;
	
	for(const FNamingPattern& Naming : In)
	{
        Out.Add(Naming.Class, { Naming.Prefix, Naming.Suffix });
	}

    return Out;
}

TArray<FNamingPattern> ConvertNamingConventions(const TMap<UClass*, FNamingConvention>& In)
{
    TArray<FNamingPattern> Out;

    for (const auto& It : In)
    {
        Out.Add({It.Key, It.Value.Prefix, It.Value.Suffix });
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
    Save();

    Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void UProjectSettingsEditor::Save()
{
    AssetManagerConfig::Get().SetUseProjectSettings(SettingStorage == EProjectSettingStorage::PSS_ProjectGlobal);
}

void UProjectSettingsEditor::PostInitProperties()
{
    Super::PostInitProperties();
    SettingStorage = AssetManagerConfig::Get().UsesProjectSettings() ? EProjectSettingStorage::PSS_ProjectGlobal : EProjectSettingStorage::PSS_PerUser;

    Save();
}

#if WITH_EDITOR
bool UProjectSettingsEditor::CanEditChange(const FProperty* InProperty) const
{
    return true;
}
#endif

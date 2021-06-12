#include "ProjectSettingsEditor.h"
#include "AssetMagementConfig.h"
#include "AssetManagementModule.h"

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

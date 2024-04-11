#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetMagementCore.h"
#include "ProjectSettingsEditor.h"

DECLARE_LOG_CATEGORY_EXTERN(AssetManagementLog, Log, All);

class FAssetManagementModule : public IModuleInterface
{
public:
    void StartupModule() override;
    void ShutdownModule() override;

    UProjectSettingsEditor* GetSettingsEditor();

private:
    TSharedPtr<FExtender> MainMenuExtender;
    TSharedPtr<AssetManager> Manager;
    UProjectSettingsEditor* SettingsEditor = nullptr;
};

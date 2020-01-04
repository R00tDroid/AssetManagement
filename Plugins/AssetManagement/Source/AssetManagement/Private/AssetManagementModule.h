#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetMagementCore.h"

DECLARE_LOG_CATEGORY_EXTERN(AssetManagementLog, Log, All);

class FAssetManagementModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<FExtender> MainMenuExtender;
	TSharedPtr<AssetManager> manager;
};

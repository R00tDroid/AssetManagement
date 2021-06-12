#include "AssetManagementModule.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "AssetManagementWidget.h"
#include "AssetManagementCommands.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "FAssetManagementModule"

DEFINE_LOG_CATEGORY(AssetManagementLog);

void FAssetManagementModule::StartupModule()
{
	AssetManagementCommands::Register();
	AssetManagementCommands::BindCommands();

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.OnTabManagerChanged().AddLambda([]()
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		TSharedPtr<FTabManager> tab_manager = LevelEditorModule.GetLevelEditorTabManager();

		tab_manager->RegisterTabSpawner(assetmanager_tab, FOnSpawnTab::CreateStatic([](const FSpawnTabArgs&)
		{
			TSharedRef<SDockTab> tab = SNew(SDockTab);
			TSharedRef<SWidget> content = SNew(SWidgetAssetManagement);
			tab->SetContent(content);
			return tab;
		})).SetDisplayName(NSLOCTEXT("AssetManager", "TabTitle", "Asset Manager")).SetAutoGenerateMenuEntry(false);
	});

	MainMenuExtender = MakeShareable(new FExtender);
	MainMenuExtender->AddMenuBarExtension("Window", EExtensionHook::After, AssetManagementCommands::menu_commands, FMenuBarExtensionDelegate::CreateStatic(&AssetManagementCommands::BuildMenu));
	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MainMenuExtender);

	Manager = MakeShareable(new AssetManager());
	Manager->Create();

	SettingsEditor = GetMutableDefault<UProjectSettingsEditor>();
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
			"Editor", "Advanced", "Advanced Asset Management",
			LOCTEXT("RuntimeSettingsName", "Advanced Asset Management"),
			LOCTEXT("RuntimeSettingsDescription", "Configure Advanced Asset Management"),
			SettingsEditor
		);
	}
}

void FAssetManagementModule::ShutdownModule()
{
	Manager->Destroy();
	Manager.Reset();
	
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FTabManager> tab_manager = LevelEditorModule.GetLevelEditorTabManager();

	if (tab_manager.IsValid())
	{
		tab_manager->UnregisterTabSpawner(assetmanager_tab);
	}
}

UProjectSettingsEditor* FAssetManagementModule::GetSettingsEditor()
{
	return SettingsEditor;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAssetManagementModule, AssetManagement)
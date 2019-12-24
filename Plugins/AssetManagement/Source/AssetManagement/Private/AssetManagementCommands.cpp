#include "AssetManagementCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Modules/ModuleManager.h"
#include "EditorStyleSet.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "DataWiseEditorModule"

TSharedPtr<FUICommandList> AssetManagementCommands::menu_commands;


AssetManagementCommands::AssetManagementCommands() : TCommands<AssetManagementCommands>("CompileBtn", LOCTEXT("CompileBtnDesc", ""), "MainFrame", FEditorStyle::GetStyleSetName())
{

}

void AssetManagementCommands::RegisterCommands()
{
	UI_COMMAND(open_assetmanager, "Asset Manager", "", EUserInterfaceActionType::Button, FInputChord());
}


void AssetManagementCommands::BindCommands()
{
	menu_commands = MakeShareable(new FUICommandList);
	const AssetManagementCommands& Commands = AssetManagementCommands::Get();

	FUICommandList& menu_actions = *menu_commands;
	menu_actions.MapAction(Commands.open_assetmanager, FExecuteAction::CreateLambda([]() { FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor").GetLevelEditorTabManager()->InvokeTab(assetmanager_tab); }));
	
}

void AssetManagementCommands::BuildMenu(FMenuBarBuilder& MenuBuilder)
{
	MenuBuilder.AddPullDownMenu(LOCTEXT("", "Asset Tools"), LOCTEXT("", "Open analytics tools"), FNewMenuDelegate::CreateStatic(&AssetManagementCommands::MakeMenu), "Asset Tools");
}

void AssetManagementCommands::MakeMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(AssetManagementCommands::Get().open_assetmanager);
}

#undef LOCTEXT_NAMESPACE

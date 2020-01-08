#include "AssetManagementCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Modules/ModuleManager.h"
#include "EditorStyleSet.h"
#include "LevelEditor.h"
#include "AssetMagementCore.h"

#define LOCTEXT_NAMESPACE "AssetManagementModule"

TSharedPtr<FUICommandList> AssetManagementCommands::menu_commands;


AssetManagementCommands::AssetManagementCommands() : TCommands<AssetManagementCommands>("", FText(), "", FEditorStyle::GetStyleSetName())
{

}

void AssetManagementCommands::RegisterCommands()
{
	UI_COMMAND(open_assetmanager, "Asset Manager", "", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(execute_fixredirectors, "Fix all redirectors", "", EUserInterfaceActionType::Button, FInputChord());
}


void AssetManagementCommands::BindCommands()
{
	menu_commands = MakeShareable(new FUICommandList);
	const AssetManagementCommands& Commands = AssetManagementCommands::Get();

	FUICommandList& menu_actions = *menu_commands;
	menu_actions.MapAction(Commands.open_assetmanager, FExecuteAction::CreateLambda([]() { FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor").GetLevelEditorTabManager()->InvokeTab(assetmanager_tab); }));
	menu_actions.MapAction(Commands.execute_fixredirectors, FExecuteAction::CreateLambda([]()
	{
		AssetManager* manager = AssetManager::Get();
		
		if (manager != nullptr)
		{
			manager->FixAllRedirectors();
		}
	}));
	
}

void AssetManagementCommands::BuildMenu(FMenuBarBuilder& MenuBuilder)
{
	MenuBuilder.AddPullDownMenu(FText::FromString("Asset Tools"), FText::FromString("Open asset tools"), FNewMenuDelegate::CreateStatic(&AssetManagementCommands::MakeMenu), "Asset Tools");
}

void AssetManagementCommands::MakeMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(AssetManagementCommands::Get().open_assetmanager);
	MenuBuilder.AddMenuEntry(AssetManagementCommands::Get().execute_fixredirectors);
}

#undef LOCTEXT_NAMESPACE

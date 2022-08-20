#pragma once
#include "Framework/Commands/Commands.h"


const FName assetmanager_tab = FName(TEXT("AssetManagerTab"));

class AssetManagementCommands : public TCommands<AssetManagementCommands>
{
private:

    friend class TCommands<AssetManagementCommands>;

    AssetManagementCommands();

public:

    void RegisterCommands() override;

    static void BindCommands();

    static void BuildMenu(FMenuBarBuilder& MenuBuilder);
    static void MakeMenu(FMenuBuilder& MenuBuilder);

    static TSharedPtr<FUICommandList> menu_commands;

private:
    TSharedPtr<FUICommandInfo> open_assetmanager;
    TSharedPtr<FUICommandInfo> execute_fixredirectors;
};
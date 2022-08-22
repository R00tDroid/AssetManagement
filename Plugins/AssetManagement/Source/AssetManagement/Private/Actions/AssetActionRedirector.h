#pragma once
#include "../AssetAction.h"

class AssetActionRedirector: public IAssetAction
{
public:
    void ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId) override;
    void ExecuteAction(TArray<FAssetData> Assets) override;
    FString GetTooltipHeading() override { return "Redirector"; }
    FString GetTooltipContent() override { return "This asset redirects it's reference to another asset.\n\nRedirects to: {Asset}\n\nClick to fix redirection"; }
    FString GetFilterName() override { return "Redirectors"; }
    FString GetApplyAllTag() override { return "Fix all redirectors"; }
    FString GetButtonStyleName() override { return "Action.Redirector"; }
};

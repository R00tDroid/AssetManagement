#pragma once
#include "../AssetAction.h"

class AssetActionUnusedCheck : public IAssetAction
{
public:
	void ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId) override;
	void ExecuteAction(TArray<FAssetData> Assets) override;
	FString GetTooltipHeading() override { return "Unused Asset"; }
	FString GetTooltipContent() override { return "This asset is not used by a playable level.\n\nClick to delete"; }
};

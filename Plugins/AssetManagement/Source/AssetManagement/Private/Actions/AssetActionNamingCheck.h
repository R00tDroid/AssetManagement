#pragma once
#include "AssetAction.h"

class AssetActionNamingCheck: public IAssetAction
{
public:
	void ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId) override;
	void ExecuteAction(TArray<FAssetData> Assets) override;
	FString GetTooltipHeading() override { return "Improper naming"; }
	FString GetTooltipContent() override { return "The name of this asset does not follow the defined format.\nSuggested asset name: {Asset}.\n\nClick to apply naming"; }
};

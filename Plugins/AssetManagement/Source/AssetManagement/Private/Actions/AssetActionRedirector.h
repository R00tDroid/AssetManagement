#pragma once
#include "AssetAction.h"

class AssetActionRedirector: public IAssetAction
{
public:
	void ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId) override;
	void ExecuteAction(TArray<FAssetData> Assets) override;
};

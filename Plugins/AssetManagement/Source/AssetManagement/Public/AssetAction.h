#pragma once
#include "AssetData.h"

struct FAssetInfo
{
	FAssetData Data;
	TMap<uint16, FString> ActionResults;
};

class IAssetAction
{
public:
	virtual void ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId) = 0;

	virtual void ExecuteAction(TArray<FAssetData> Assets) = 0;
};

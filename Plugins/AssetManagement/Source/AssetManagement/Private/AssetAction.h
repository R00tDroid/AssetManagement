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
    virtual ~IAssetAction() = default;
    virtual void ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId) = 0;

    virtual void ExecuteAction(TArray<FAssetData> Assets) = 0;

    virtual FString GetTooltipHeading() = 0;
    virtual FString GetTooltipContent() = 0; //Use {Asset} for asset specific data

    virtual FString GetFilterName() = 0;
    virtual FString GetApplyAllTag() = 0;
};

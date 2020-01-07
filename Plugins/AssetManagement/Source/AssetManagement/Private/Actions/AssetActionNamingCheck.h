#pragma once
#include "../AssetAction.h"

struct NamingPattern
{
	UClass* Class;
	FString Prefix;
	FString Suffix;
};

class AssetActionNamingCheck: public IAssetAction
{
public:
	AssetActionNamingCheck();
	
	void ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId) override;
	void ExecuteAction(TArray<FAssetData> Assets) override;
	FString GetTooltipHeading() override { return "Improper naming"; }
	FString GetTooltipContent() override { return "The name of this asset does not follow the defined format.\nSuggested asset name: {Asset}.\n\nClick to apply naming"; }
	FString GetFilterName() override { return "Naming conventions"; }
	FString GetApplyAllTag() override { return "Apply all naming conventions"; }

private:
	TArray<NamingPattern> NamingPatterns;

	FString GetNameForAsset(FString Name, UClass* Class);
};

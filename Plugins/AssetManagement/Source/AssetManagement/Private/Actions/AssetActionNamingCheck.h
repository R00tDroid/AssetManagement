#pragma once
#include "../AssetAction.h"
#include "AssetActionNamingCheck.generated.h"


USTRUCT(BlueprintType)
struct FNamingPattern
{
	GENERATED_BODY();

public:
	UPROPERTY()
	UClass* Class;

	UPROPERTY()
	FString Prefix;

	UPROPERTY()
	FString Suffix;
};

class AssetActionNamingCheck : public IAssetAction
{
public:
	AssetActionNamingCheck();
	virtual ~AssetActionNamingCheck() override;
	
	void OnConfigChanged();
	
	void ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId) override;
	void ExecuteAction(TArray<FAssetData> Assets) override;
	FString GetTooltipHeading() override { return "Improper naming"; }
	FString GetTooltipContent() override { return "The name of this asset does not follow the defined format.\nSuggested asset name: {Asset}.\n\nClick to apply naming"; }
	FString GetFilterName() override { return "Naming conventions"; }
	FString GetApplyAllTag() override { return "Apply all naming conventions"; }

	static TArray<FNamingPattern> GetDefaultPatterns();

	static FString NamingPatternsToJson(const TArray<FNamingPattern>&);
	static TArray<FNamingPattern> JsonToNamingPatterns(const FString&);

private:
	TArray<FNamingPattern> NamingPatterns;

	FString GetNameForAsset(FString Name, UClass* Class);

	FDelegateHandle OnConfigChangedHandle;
};

#pragma once
#include "../AssetAction.h"
#include "AssetActionNamingCheck.generated.h"

UENUM()
enum class EClassPropertyType : uint8
{
    CPT_String    UMETA(DisplayName = "String"),
    CPT_Byte      UMETA(DisplayName = "Byte"),
    CPT_Int32     UMETA(DisplayName = "Int32"),
    CPT_Float     UMETA(DisplayName = "Float")
};

USTRUCT(BlueprintType)
struct FPropertyFilter
{
    GENERATED_BODY();

    UPROPERTY()
    FString PropertyName;

    UPROPERTY()
    EClassPropertyType PropertyType;

    UPROPERTY()
    FString ExpectedValue;
};

USTRUCT(BlueprintType)
struct FNamingPattern
{
    GENERATED_BODY();

    UPROPERTY()
    UClass* Class;

    UPROPERTY()
    TArray<FPropertyFilter> ClassProperties;

    UPROPERTY()
    FString Prefix;

    UPROPERTY()
    FString Suffix;
};

class AssetActionNamingCheck : public IAssetAction
{
public:
    AssetActionNamingCheck();
    ~AssetActionNamingCheck() override;
    
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

    FString GetNameForAsset(FString Name, UClass* Class, UObject* Object);

    FDelegateHandle OnConfigChangedHandle;
};

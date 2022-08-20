#pragma once
#include "CoreMinimal.h"
#include "Actions/AssetActionNamingCheck.h"
#include "ProjectSettingsEditor.generated.h"

UENUM()
enum class EProjectSettingStorage : uint8
{
    PSS_ProjectGlobal    UMETA(DisplayName = "Project"),
    PSS_PerUser          UMETA(DisplayName = "Per user")
};

USTRUCT()
struct FNamingConvention
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    TArray<FPropertyFilter> PropertyFilters;
    
    UPROPERTY(EditAnywhere)
    FString Prefix;

    UPROPERTY(EditAnywhere)
    FString Suffix;
};

USTRUCT()
struct FNamingConventionList
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    TArray<FNamingConvention> Conventions;
};

UCLASS()
class UProjectSettingsEditor : public UObject
{
    GENERATED_BODY()

public:
    static UProjectSettingsEditor& Get();

    virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;

private:
    UPROPERTY(EditAnywhere, Category = Settings, meta = (
        ToolTip = "Where the settings will be saved."),
        DisplayName = "Settings storage")
    EProjectSettingStorage SettingStorage = EProjectSettingStorage::PSS_PerUser;

    //TODO rescan assets when changed
    UPROPERTY(EditAnywhere, Category = Assets, meta = (
        DisplayName = "Assets naming conventions", ShowOnlyInnerProperties))
    TMap<TSubclassOf<UObject>, FNamingConventionList> NamingConventions;

    //TODO rescan assets when changed
    //TODO warning user if no levels are set
    UPROPERTY(EditAnywhere, Category = Assets, meta = (
        DisplayName = "Playable levels", ShowOnlyInnerProperties))
    TArray<UWorld*> PlayableLevels;

    void SaveConfig();
    void LoadConfig();
    
    void PostInitProperties() override;
#if WITH_EDITOR
    bool CanEditChange(const FProperty* InProperty) const override;
#endif
};
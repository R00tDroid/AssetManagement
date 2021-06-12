#pragma once
#include "CoreMinimal.h"
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
    FString Prefix;

    UPROPERTY(EditAnywhere)
    FString Suffix;
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

    UPROPERTY(EditAnywhere, Category = Assets, meta = (
        DisplayName = "Assets naming conventions", ShowOnlyInnerProperties))
    TMap<TSubclassOf<UObject>, FNamingConvention> NamingConventions;

    UPROPERTY(EditAnywhere, Category = Assets, meta = (
        DisplayName = "Playable levels", ShowOnlyInnerProperties))
    TArray<UWorld*> PlayableLevels;

    void SaveConfig();
    void LoadConfig();
	
    virtual void PostInitProperties() override;
#if WITH_EDITOR
    virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif
};
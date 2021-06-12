#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectSettingsEditor.generated.h"

UENUM()
enum class EProjectSettingStorage : uint8
{
	PSS_ProjectGlobal    UMETA(DisplayName = "Project"),
	PSS_PerUser          UMETA(DisplayName = "Per user")
};

UCLASS(config = EditorPerProjectUserSettings)
class UProjectSettingsEditor : public UObject
{
    GENERATED_BODY()

public:
    static UProjectSettingsEditor& Get();

    virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;

private:
    UPROPERTY(config, EditAnywhere, Category = Settings, meta = (
        ToolTip = "Where the settings will be saved."),
        DisplayName = "Settings storage")
    EProjectSettingStorage SettingStorage = EProjectSettingStorage::PSS_PerUser;

    UPROPERTY(config, EditAnywhere, Category = Assets, meta = (
        DisplayName = "Assets naming converntions"))
    TMap<UClass*, FString> NamingConventions;

    void Save();
    virtual void PostInitProperties() override;
#if WITH_EDITOR
    virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif
};
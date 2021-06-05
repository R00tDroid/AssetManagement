#pragma once
#include "AssetAction.h"

class AssetManagerConfig
{
public:
	static AssetManagerConfig& Get();
	void Load();

	bool UsesProjectSettings();
	void SetUseProjectSettings(bool);
	
	bool GetBool(FString Section, FString Key, bool DefaultValue);
	void SetBool(FString Section, FString Key, bool Value);

	int GetInt(FString Section, FString Key, int DefaultValue);
	void SetInt(FString Section, FString Key, int Value);

private:
	FString GetPluginConfig();
	FString GetProjectConfig();
	FString GetConfig();

	bool UsingProjectConfig = false;

	static AssetManagerConfig instance;
};
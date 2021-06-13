#include "AssetMagementConfig.h"
#include "Misc/ConfigCacheIni.h"
#include "FileManager.h"
#include "IPluginManager.h"

AssetManagerConfig AssetManagerConfig::instance;
AssetManagerConfig::FOnConfigChanged AssetManagerConfig::OnConfigChanged;

AssetManagerConfig& AssetManagerConfig::Get()
{
    instance.Load();
    return instance;
}

void AssetManagerConfig::Load()
{
    if (!loaded) 
    {
        loaded = true;
        bool res = GConfig->GetBool(TEXT("Global"), TEXT("UseProjectSettings"), UsingProjectConfig, GetProjectConfig());
        if (!res) UsingProjectConfig = false;
    }
}

bool AssetManagerConfig::UsesProjectSettings()
{
    return UsingProjectConfig;
}

void AssetManagerConfig::SetUseProjectSettings(bool Enable)
{
    if (UsingProjectConfig != Enable)
    {
        UsingProjectConfig = Enable;

        GConfig->SetBool(TEXT("Global"), TEXT("UseProjectSettings"), Enable, GetProjectConfig());
        GConfig->Flush(false, GetProjectConfig());

        OnConfigChanged.Broadcast();
    }
}

bool AssetManagerConfig::GetBool(FString Section, FString Key, bool DefaultValue)
{
    bool Value;
    bool res = GConfig->GetBool(*Section, *Key, Value, GetConfig());
    if (!res) { SetBool(Section, Key, DefaultValue); return DefaultValue; }

    return Value;
}

void AssetManagerConfig::SetBool(FString Section, FString Key, bool Value)
{
    GConfig->SetBool(*Section, *Key, Value, GetConfig());
    GConfig->Flush(false, GetConfig());
}

int AssetManagerConfig::GetInt(FString Section, FString Key, int DefaultValue)
{
    int Value;
    bool res = GConfig->GetInt(*Section, *Key, Value, GetConfig());
    if (!res) { SetInt(Section, Key, DefaultValue); return DefaultValue; }

    return Value;
}

void AssetManagerConfig::SetInt(FString Section, FString Key, int Value)
{
    GConfig->SetInt(*Section, *Key, Value, GetConfig());
    GConfig->Flush(false, GetConfig());
}

FString AssetManagerConfig::GetString(FString Section, FString Key, FString DefaultValue)
{
    FString Value;
    bool res = GConfig->GetString(*Section, *Key, Value, GetConfig());
    if (!res) { SetString(Section, Key, DefaultValue); return DefaultValue; }

    return Value;
}

void AssetManagerConfig::SetString(FString Section, FString Key, FString Value)
{
    GConfig->SetString(*Section, *Key, *Value, GetConfig());
    GConfig->Flush(false, GetConfig());
}

FString AssetManagerConfig::GetPluginConfig()
{
    FString path = "";
    TSharedPtr<IPlugin> mod = IPluginManager::Get().FindPlugin(TEXT("AssetManagement"));
    if(mod.IsValid())
    {
        path = mod->GetBaseDir();
    }

    if (!path.IsEmpty()) 
    {
        path = FPaths::Combine(path, FString("Config"), FString("AdvancedAssetManagement.ini"));
        path = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*path);
    }
    
    return path;
}

FString AssetManagerConfig::GetProjectConfig()
{
    FString path = FPaths::Combine(FPaths::ProjectConfigDir(), FString("AdvancedAssetManagement.ini"));
    path = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*path);
    return path;
}

FString AssetManagerConfig::GetConfig()
{
    if(UsesProjectSettings())
    {
        return GetProjectConfig();
    }
    else
    {
        return GetPluginConfig();
    }
}

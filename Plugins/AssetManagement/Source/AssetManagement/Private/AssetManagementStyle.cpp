#include "AssetManagementStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FAssetManagementStyle::Instance = nullptr;

void FAssetManagementStyle::Init()
{
    if (!Instance.IsValid())
    {
        Instance = MakeShareable(new FSlateStyleSet(FName("AssetManagementStyle")));
        FSlateStyleRegistry::RegisterSlateStyle(*Instance);
    }

    Instance->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
    Instance->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

    IPluginManager& PluginManager = IPluginManager::Get();
    TSharedPtr<IPlugin> Plugin = PluginManager.FindPlugin("AssetManagement");

    FString ResourceRoot = FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir()) / TEXT("/Resources/");

    Instance->Set("Action.Naming", new FSlateImageBrush(FName(*(ResourceRoot + "IconNaming.png")), FVector2D(25, 25)));
    Instance->Set("Action.Redirector", new FSlateImageBrush(FName(*(ResourceRoot + "IconRedirector.png")), FVector2D(25, 25)));
    Instance->Set("Action.Unused", new FSlateImageBrush(FName(*(ResourceRoot + "IconUnused.png")), FVector2D(25, 25)));

    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
    }
}

void FAssetManagementStyle::Release()
{
    FSlateStyleRegistry::UnRegisterSlateStyle(*Instance);
    Instance.Reset();
}

const ISlateStyle& FAssetManagementStyle::Get()
{
    return *Instance;
}

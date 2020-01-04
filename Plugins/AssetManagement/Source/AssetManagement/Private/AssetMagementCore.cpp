#include "AssetMagementCore.h"
#include "AssetManagementModule.h"
#include "AssetRegistryModule.h"
#include "Actions/AssetActionUnusedCheck.h"
#include "Actions/AssetActionRedirector.h"
#include "Actions/AssetActionNamingCheck.h"

AssetManager* instance_ = nullptr;

AssetManager::FOnAssetListUpdated AssetManager::OnAssetListUpdated;

void AssetManager::Create()
{
	if (instance_ != nullptr) UE_LOG(AssetManagementLog, Fatal, TEXT("AssetManager already started"))
	instance_ = this;

	AssetActions.Add(MakeShareable(new AssetActionUnusedCheck()));
	AssetActions.Add(MakeShareable(new AssetActionNamingCheck()));
	AssetActions.Add(MakeShareable(new AssetActionRedirector()));
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	if (AssetRegistry.IsLoadingAssets())
	{
		AssetRegistry.OnFilesLoaded().AddSP(this, &AssetManager::BindToAssetRegistry);
	}
	else
	{
		BindToAssetRegistry();
	}
}

void AssetManager::Destroy()
{
	for(TSharedPtr<IAssetAction>& Action : AssetActions)
	{
		Action.Reset();
	}

	AssetActions.Empty();
	
	instance_ = nullptr;
}

TArray<FAssetInfo> AssetManager::GetAssets()
{
	AssetLock.Lock();
	TArray<FAssetInfo> res = Assets;
	AssetLock.Unlock();

	return res;
}

TArray<IAssetAction*> AssetManager::GetActions()
{
	TArray<IAssetAction*> actions;
	for(TSharedPtr<IAssetAction>& Action : AssetActions)
	{
		actions.Add(Action.Get());
	}

	return actions;
}

void AssetManager::RequestRescan()
{
	ScanAssets();
}

//TODO implement asset update handling
void AssetManager::OnAssetAdded(const FAssetData&)
{
}

void AssetManager::OnAssetUpdated(const FAssetData&)
{
}

void AssetManager::OnAssetRenamed(const FAssetData&, const FString&)
{
}

void AssetManager::OnAssetRemoved(const FAssetData&)
{
}

void AssetManager::BindToAssetRegistry()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	AssetRegistry.OnAssetAdded().AddSP(this, &AssetManager::OnAssetAdded);
	AssetRegistry.OnAssetRemoved().AddSP(this, &AssetManager::OnAssetUpdated);
	AssetRegistry.OnAssetRemoved().AddSP(this, &AssetManager::OnAssetRemoved);
	AssetRegistry.OnAssetRenamed().AddSP(this, &AssetManager::OnAssetRenamed);

	ScanAssets();
}

void AssetManager::RequestActionExecution(int ActionId, TArray<FAssetData> Assets)
{
	if(AssetActions.IsValidIndex(ActionId))
	{
		AssetActions[ActionId]->ExecuteAction(Assets);
	}
}

void AssetManager::ScanAssets() //TODO perform scan on worker thread
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> RawAssets;
	bool res = AssetRegistry.GetAllAssets(RawAssets, true);

	TArray<FAssetData> Worlds;
	res = AssetRegistry.GetAssetsByClass(UWorld::StaticClass()->GetFName(), Worlds, true);

	for (int32 i = 0; i < Worlds.Num(); i++)
	{
		FAssetData& Asset = Worlds[i];

		if (!Asset.PackageName.ToString().StartsWith("/Game/", ESearchCase::IgnoreCase))
		{
			if (Worlds.IsValidIndex(i))
			{
				Worlds.RemoveAt(i);
				i--;
			}
		}
	}

	for (int32 i = 0; i < RawAssets.Num(); i++)
	{
		FAssetData& Asset = RawAssets[i];

		if (!Asset.PackageName.ToString().StartsWith("/Game/", ESearchCase::IgnoreCase))
		{
			if (RawAssets.IsValidIndex(i))
			{
				RawAssets.RemoveAt(i);
				i--;
			}
		}
	}

	TArray<FAssetInfo> NewAssets;

	for(FAssetData Asset : RawAssets)
	{
		NewAssets.Add({ Asset, {} });
	}

	uint16 id = 0;
	for (TSharedPtr<IAssetAction>& Action : AssetActions)
	{
		Action->ScanAssets(NewAssets, id);
		id++;
	}

	for (int i = 0; i < NewAssets.Num(); i++)
	{
		if (NewAssets[i].ActionResults.Num() == 0) 
		{
			NewAssets.RemoveAt(i);
			i--;
		}
	}

	AssetLock.Lock();
	Assets = NewAssets;
	AssetLock.Unlock();
	
	OnAssetListUpdated.ExecuteIfBound();
}

AssetManager* AssetManager::Get()
{
	return instance_;
}

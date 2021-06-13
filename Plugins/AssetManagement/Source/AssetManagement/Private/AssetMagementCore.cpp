#include "AssetMagementCore.h"
#include "AssetManagementModule.h"
#include "AssetRegistryModule.h"
#include "Actions/AssetActionUnusedCheck.h"
#include "Actions/AssetActionRedirector.h"
#include "Actions/AssetActionNamingCheck.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Editor.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "AssetToolsModule.h"
#include "AssetMagementConfig.h"

AssetManager* instance_ = nullptr;

AssetManager::FOnAssetListUpdated AssetManager::OnAssetListUpdated;

void AssetManager::Create()
{
	if (instance_ != nullptr) UE_LOG(AssetManagementLog, Fatal, TEXT("AssetManager already started"))
	instance_ = this;

	AssetManagerConfig::Get().Load();

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

void AssetManager::OnAssetAdded(const FAssetData& Asset)
{
	bool found = false;
	AssetLock.Lock();
	for(FAssetInfo& info : Assets)
	{
		if (info.Data == Asset) { found = true; break; }
	}
	AssetLock.Unlock();
	if (found) return;
	
	//TODO execute on separate thread
	TArray<FAssetInfo> NewAssets = {{Asset, {}}};
	ProcessAssets(NewAssets);

	if (NewAssets.Num() > 0)
	{
		AssetLock.Lock();
		for (FAssetInfo& info : NewAssets)
		{
			Assets.Add(info);
		}
		PrepareAssetList();
		AssetLock.Unlock();
		OnAssetListUpdated.ExecuteIfBound();
	}
}

void AssetManager::OnAssetUpdated(const FAssetData&)
{
	RequestRescan(); //TODO redo dependency scan
}

void AssetManager::OnAssetRenamed(const FAssetData&, const FString&)
{
	RequestRescan(); //TODO improve renamed asset handling
}

void AssetManager::OnAssetRemoved(const FAssetData& Asset)
{
	bool changed = false;
	AssetLock.Lock();
	for (int i = 0; i < Assets.Num(); i++)
	{
		if (Assets[i].Data == Asset) 
		{
			Assets.RemoveAt(i);
			i--;
			changed = true;
		}
	}
	if (changed) PrepareAssetList();
	AssetLock.Unlock();
	
	if(changed) OnAssetListUpdated.ExecuteIfBound();
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

void AssetManager::RequestActionExecution(int ActionId, TArray<FAssetData> ActionAssets)
{
	FWorldContext* PIEWorldContext = GEditor->GetPIEWorldContext();
	if (PIEWorldContext)
	{
		FNotificationInfo Notification(FText::FromString("Can not modify assets while Play In Editor is active"));
		Notification.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Notification);

		return;
	}
	
	if(AssetActions.IsValidIndex(ActionId))
	{
		AssetActions[ActionId]->ExecuteAction(ActionAssets);
	}
}

void AssetManager::FixAllRedirectors()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter filter;
	filter.ClassNames.Add(UObjectRedirector::StaticClass()->GetFName());
	filter.bRecursivePaths = true;
	filter.PackagePaths.Add("/Game");

	TArray<FAssetData> Redirectors;
	AssetRegistry.GetAssets(filter, Redirectors);
	
	TArray<UObjectRedirector*> Objects;

	for (int32 i = 0; i < Redirectors.Num(); i++)
	{
		FAssetData& Asset = Redirectors[i];

		FString asset_name = FPaths::GetBaseFilename(Asset.PackageName.ToString());

		if (Asset.AssetName.ToString().Equals(asset_name))
		{
			Objects.AddUnique(static_cast<UObjectRedirector*>(Asset.GetAsset()));
		}
	}
	
	if(Objects.Num() == 0)
	{
		FNotificationInfo Notification(FText::FromString("No redirectors found"));
		Notification.ExpireDuration = 2.0f;
		FSlateNotificationManager::Get().AddNotification(Notification);
	}
	else 
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		AssetToolsModule.Get().FixupReferencers(Objects);

		FNotificationInfo Notification(FText::FromString("Fixed " + FString::FromInt(Objects.Num()) +  " redirector(s)"));
		Notification.ExpireDuration = 2.0f;
		FSlateNotificationManager::Get().AddNotification(Notification);
	}
}

void AssetManager::ScanAssets() //TODO perform scan on worker thread
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> RawAssets;
	bool res = AssetRegistry.GetAllAssets(RawAssets, true);

	TArray<FAssetInfo> NewAssets;

	for (FAssetData Asset : RawAssets)
	{
		NewAssets.Add({ Asset, {} });
	}

	ProcessAssets(NewAssets);

	AssetLock.Lock();
	Assets = NewAssets;
	PrepareAssetList();
	AssetLock.Unlock();
	
	OnAssetListUpdated.ExecuteIfBound();
}

void AssetManager::ProcessAssets(TArray<FAssetInfo>& NewAssets)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	
	TArray<FAssetData> Worlds;
	AssetRegistry.GetAssetsByClass(UWorld::StaticClass()->GetFName(), Worlds, true);

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

	for (int32 i = 0; i < NewAssets.Num(); i++)
	{
		FAssetData& Asset = NewAssets[i].Data;

		if (!Asset.PackageName.ToString().StartsWith("/Game/", ESearchCase::IgnoreCase))
		{
			NewAssets.RemoveAt(i);
			i--;
			continue;
		}

		FString asset_name = FPaths::GetBaseFilename(Asset.PackageName.ToString());
		
		if(!Asset.AssetName.ToString().Equals(asset_name))
		{
			NewAssets.RemoveAt(i);
			i--;
			continue;
		}
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
}

void AssetManager::PrepareAssetList()
{
	Assets.Sort([](const FAssetInfo& A, const FAssetInfo& B)
	{
		return A.Data.PackageName < B.Data.PackageName;
	});
}

AssetManager* AssetManager::Get()
{
	return instance_;
}

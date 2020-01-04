#pragma once
#include "AssetAction.h"

class AssetManager : public TSharedFromThis<AssetManager>
{
public:
	static AssetManager* Get();
	
	void Create();
	void Destroy();

	DECLARE_DELEGATE(FOnAssetListUpdated)
	static FOnAssetListUpdated OnAssetListUpdated;

	TArray<FAssetInfo> GetAssets();
	TArray<IAssetAction*> GetActions();

	void RequestRescan();

	void OnAssetAdded(const FAssetData&);
	void OnAssetUpdated(const FAssetData&);
	void OnAssetRenamed(const FAssetData&, const FString&);
	void OnAssetRemoved(const FAssetData&);

	void BindToAssetRegistry();

	void RequestActionExecution(int ActionId, TArray<FAssetData> Assets);

private:
	void ScanAssets();

	TArray<TSharedPtr<IAssetAction>> AssetActions;
	
	TArray<FAssetInfo> Assets;
	FCriticalSection AssetLock;
};

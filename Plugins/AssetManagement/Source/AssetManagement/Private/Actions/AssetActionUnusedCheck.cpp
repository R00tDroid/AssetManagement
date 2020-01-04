#include "AssetActionUnusedCheck.h"
#include "AssetRegistryModule.h"

void AssetActionUnusedCheck::ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> Worlds;
	AssetRegistry.GetAssetsByClass(UWorld::StaticClass()->GetFName(), Worlds, true);
	
	TMap<FAssetData, uint16> References;

	for (FAssetData& World : Worlds)
	{
		TArray<FAssetData> ToSearch = { World };
		TArray<FAssetData> Searched;

		while (ToSearch.Num() > 0)
		{
			FAssetData Asset = ToSearch[0];
			ToSearch.RemoveAt(0);
			Searched.Add(Asset);

			if (!References.Contains(Asset))
			{
				References.Add(Asset, 1);
			}
			else
			{
				References[Asset]++;
			}

			TArray<FName> Dependencies;
			AssetRegistry.GetDependencies(Asset.PackageName, Dependencies);

			for (FName& Dependency : Dependencies)
			{
				for (FAssetInfo& Link : Assets)
				{
					if (Link.Data.PackageName.IsEqual(Dependency))
					{
						if (!Searched.Contains(Link.Data) && !ToSearch.Contains(Link.Data))
						{
							ToSearch.Add(Link.Data);
						}

						break;
					}
				}
			}
		}
	}

	for (FAssetInfo& Asset : Assets)
	{
		if(!References.Contains(Asset.Data))
		{
			Asset.ActionResults.Add(AssignedId, "");
		}
	}
}

void AssetActionUnusedCheck::ExecuteAction(TArray<FAssetData> Assets)
{
	//TODO implement asset deletion
}

#include "AssetActionRedirector.h"
#include "AssetRegistryModule.h"

void AssetActionRedirector::ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId)
{
	for (FAssetInfo& Asset : Assets)
	{
		if(Asset.Data.IsRedirector())
		{
			Asset.ActionResults.Add(AssignedId, "");
		}
	}
}

void AssetActionRedirector::ExecuteAction(TArray<FAssetData> Assets)
{
	//TODO implement redirector fixup
}

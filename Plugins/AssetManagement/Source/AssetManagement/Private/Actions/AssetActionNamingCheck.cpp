#include "AssetActionNamingCheck.h"
#include "AssetRegistryModule.h"

void AssetActionNamingCheck::ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId)
{
	for (FAssetInfo& Asset : Assets)
	{
		FString name = Asset.Data.AssetName.ToString();
		FString suggested_name = name; //TODO implement naming check
		
		if(!name.Equals(suggested_name))
		{
			Asset.ActionResults.Add(AssignedId, suggested_name);
		}
	}
}

void AssetActionNamingCheck::ExecuteAction(TArray<FAssetData> Assets)
{
	//TODO implement asset renaming
}

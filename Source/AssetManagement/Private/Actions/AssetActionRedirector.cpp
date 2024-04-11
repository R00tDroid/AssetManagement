#include "AssetActionRedirector.h"
#include "AssetToolsModule.h"

void AssetActionRedirector::ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId)
{
    for (FAssetInfo& Asset : Assets)
    {
        if(Asset.Data.IsRedirector() && Asset.Data.GetClass() == UObjectRedirector::StaticClass())
        {    
            UObjectRedirector* Redirector = StaticCast<UObjectRedirector*>(Asset.Data.GetAsset());
            FString path = Redirector->DestinationObject->GetPathName();

            int32 index;
            if (path.FindLastChar('.', index))
            {
                path.RemoveAt(index, path.Len() - index);
            }
            
            Asset.ActionResults.Add(AssignedId, path);
        }
    }
}

void AssetActionRedirector::ExecuteAction(TArray<FAssetData> Assets)
{
    TArray<UObjectRedirector*> Redirectors;
    
    for (FAssetData& Asset : Assets)
    {
        UObjectRedirector* Redirector = CastChecked<UObjectRedirector>(Asset.GetAsset());
        Redirectors.Add(Redirector);
    }
    
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
    AssetToolsModule.Get().FixupReferencers(Redirectors);
}

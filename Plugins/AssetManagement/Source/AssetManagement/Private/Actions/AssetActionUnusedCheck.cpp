#include "AssetActionUnusedCheck.h"
#include "AssetRegistryModule.h"
#include "ObjectTools.h"

void AssetActionUnusedCheck::ScanAssets(TArray<FAssetInfo>& Assets, uint16 AssignedId)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> Worlds;
    AssetRegistry.GetAssetsByClass(UWorld::StaticClass()->GetFName(), Worlds, true);
    
    TMap<FName, uint16> References;

    for (FAssetData& World : Worlds)
    {
        TArray<FAssetData> ToSearch = { World };
        TArray<FAssetData> Searched;

        while (ToSearch.Num() > 0)
        {
            FAssetData Asset = ToSearch[0];
            ToSearch.RemoveAt(0);
            Searched.Add(Asset);

            if (!References.Contains(Asset.PackageName))
            {
                References.Add(Asset.PackageName, 1);
            }
            else
            {
                References[Asset.PackageName]++;
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
        if (!References.Contains(Asset.Data.PackageName) && Asset.Data.GetClass() != UObjectRedirector::StaticClass())
        {
            Asset.ActionResults.Add(AssignedId, "");
        }
    }
}

void AssetActionUnusedCheck::ExecuteAction(TArray<FAssetData> Assets)
{
    TArray<FAssetData> ToDelete;

    bool AllYes = false;
    bool AllNo = false;

    for (FAssetData& Asset : Assets) 
    {
        EAppReturnType::Type SelectedOption = EAppReturnType::No;
        if (!AllYes && !AllNo)
        {
            SelectedOption = FMessageDialog::Open(EAppMsgType::YesNoYesAllNoAll, EAppReturnType::NoAll, FText::FromString(FString("Are you sure you wish to delete the following file?\n") + Asset.AssetName.ToString()));
            if (SelectedOption == EAppReturnType::YesAll) AllYes = true;
            if (SelectedOption == EAppReturnType::NoAll) AllNo = true;
        }

        if (SelectedOption == EAppReturnType::Yes || AllYes) ToDelete.Add(Asset);
    }
    
    if (Assets.Num() > 0) ObjectTools::DeleteAssets(ToDelete, false); //TODO add confirmation toggle to config
}

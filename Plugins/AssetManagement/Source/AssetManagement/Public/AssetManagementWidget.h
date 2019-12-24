#pragma once
#include "ExtendedWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "AssetData.h"

class SWidgetAssetManagement : public SExtendedWidget
{
	SLATE_USER_ARGS(SWidgetAssetManagement)
	{ }

	SLATE_END_ARGS()

public:

	virtual void Construct(const FArguments& InArgs);
	virtual void Start() override;

	void BindToAssetRegistry();

	void OnAssetAdded(const FAssetData&);
	void OnAssetUpdated(const FAssetData&);
	void OnAssetRenamed(const FAssetData&, const FString&);
	void OnAssetRemoved(const FAssetData&);

	FReply RequestRescan();

private:

	void PopulateAssets();

	TArray<FAssetData> Assets;
	TSharedPtr<SVerticalBox> asset_list;
};

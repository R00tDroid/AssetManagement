#pragma once
#include "ExtendedWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "AssetData.h"
#include "SToolTip.h"
#include "Widgets/Layout/SSeparator.h"

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

class SActionToolTip : public SToolTip
{
public:

	SLATE_BEGIN_ARGS(SActionToolTip) { }
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		SToolTip::Construct(
			SToolTip::FArguments()
			.Content()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(heading, STextBlock)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2)
				[
					SNew(SSeparator)
					.Orientation(Orient_Horizontal)]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(content, STextBlock)
				]
			]
		);
	}

	void SetHeading(FString text)
	{
		heading->SetText(FText::FromString(text));
	}

	void SetContent(FString text)
	{
		content->SetText(FText::FromString(text));
	}

private:
	TSharedPtr<STextBlock> heading;
	TSharedPtr<STextBlock> content;
};

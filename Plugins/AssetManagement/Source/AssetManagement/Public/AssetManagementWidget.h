#pragma once
#include "ExtendedWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "SToolTip.h"
#include "Widgets/Layout/SSeparator.h"
#include "AssetAction.h"

class SWidgetAssetManagement : public SExtendedWidget
{
	SLATE_USER_ARGS(SWidgetAssetManagement)
	{ }

	SLATE_END_ARGS()

public:

	virtual void Construct(const FArguments& InArgs);
	virtual void Start() override;

	FReply RequestRescan();

private:

	void PopulateAssets();
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

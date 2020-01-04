#include "AssetManagementWidget.h"
#include "AssetManagementModule.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Async/Async.h"
#include "EditorStyle.h"
#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"
#include "Engine/World.h"
#include "Editor.h"

void SWidgetAssetManagement::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				.Padding(FMargin(5.0f))
				[
					SAssignNew(asset_list, SVerticalBox)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSeparator)
				.Orientation(Orient_Horizontal)
			]


			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				  .FillWidth(1.0f)
				  .Padding(FMargin(5.0f))
				  .VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				]

				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .Padding(FMargin(5.0f))
				[
					SNew(SButton)
					.OnClicked(FOnClicked::CreateSP(this, &SWidgetAssetManagement::RequestRescan))
					.VAlign(VAlign_Fill)
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
						.Text(FText::FromString("Rescan"))
					]
				]
			]
		]
	];
}

void SWidgetAssetManagement::Start()
{
	PopulateAssets();
	AssetManager::OnAssetListUpdated.BindSP(this, &SWidgetAssetManagement::PopulateAssets);
}

FReply SWidgetAssetManagement::RequestRescan()
{
	AssetManager* manager = AssetManager::Get();
	if (manager != nullptr)
	{
		manager->Get()->RequestRescan();
	}

	return FReply::Handled();
}

void SWidgetAssetManagement::PopulateAssets()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetInfo> Assets;
	TArray<IAssetAction*> AssetActions;
	
	AssetManager* manager = AssetManager::Get();
	if (manager != nullptr)
	{
		Assets = manager->GetAssets();
		AssetActions = manager->GetActions();
	}

	int difference = Assets.Num() - asset_list.Get()->GetChildren()->Num();
	int pre_count = asset_list.Get()->GetChildren()->Num();

	if (difference > 0)
	{
		for (int i = 0; i < FMath::Abs(difference); i++)
		{
			int index = pre_count + i;

			TSharedPtr<SHorizontalBox> ButtonContainer;

			SVerticalBox::FSlot& slot = asset_list->AddSlot();
			slot[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
				.FillWidth(1)
				.VAlign(EVerticalAlignment::VAlign_Center)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.Clipping(EWidgetClipping::ClipToBounds)
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2)
				.VAlign(EVerticalAlignment::VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(20)
				.HeightOverride(20)
				[
					SNew(SButton)
					.ForegroundColor(FSlateColor(FLinearColor(0, 0, 0, 0)))
				.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
				.ContentPadding(4.0f)
				.ForegroundColor(FSlateColor::UseForeground())
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("PropertyWindow.Button_Browse"))
				.ColorAndOpacity(FSlateColor::UseForeground())
				]
				]
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew(ButtonContainer, SHorizontalBox)
				]
				];

			slot.Padding(FMargin(4));

			for (int j = 0; j < AssetActions.Num(); j++)
			{
				SHorizontalBox::FSlot& button_slot = ButtonContainer->AddSlot();
				button_slot.Padding(2);
				
				button_slot[
					SNew(SButton)
					.ToolTip(SNew(SActionToolTip))
				];
			}
		}
	}
	else if (difference < 0)
	{
		for (int i = 0; i < std::abs(difference); i++)
		{
			asset_list->RemoveSlot(
				asset_list.Get()->GetChildren()->GetChildAt(asset_list.Get()->GetChildren()->Num() - 1));
		}
	}

	for (int i = 0; i < asset_list.Get()->GetChildren()->Num(); i++)
	{
		SHorizontalBox* container = static_cast<SHorizontalBox*>(&asset_list.Get()->GetChildren()->GetChildAt(i).Get());

		TSharedRef<STextBlock> name_label = StaticCastSharedRef<STextBlock>(container->GetChildren()->GetChildAt(0));
		TSharedRef<SBox> browse_button_container = StaticCastSharedRef<SBox>(container->GetChildren()->GetChildAt(1));
		TSharedRef<SHorizontalBox> button_container = StaticCastSharedRef<SHorizontalBox>(container->GetChildren()->GetChildAt(2));

		TSharedRef<SButton> browse_button = StaticCastSharedRef<SButton>(browse_button_container->GetChildren()->GetChildAt(0));

		for (int j = 0; j < AssetActions.Num(); j++)
		{
			TSharedRef<SButton> button = StaticCastSharedRef<SButton>(button_container->GetChildren()->GetChildAt(j));
			TSharedPtr<SActionToolTip> tooltip = StaticCastSharedPtr<SActionToolTip>(button->GetToolTip());

			bool enable = Assets[i].ActionResults.Contains(j);
			button->SetEnabled(enable);

			FString TooltipContent = AssetActions[j]->GetTooltipContent();
			if(enable) TooltipContent = TooltipContent.Replace(TEXT("{Asset}"), *Assets[i].ActionResults[j]);
			else TooltipContent = "";

			tooltip->SetHeading(AssetActions[j]->GetTooltipHeading());
			tooltip->SetContent(TooltipContent);
		}

		name_label->SetText(FText::FromString(Assets[i].Data.AssetName.ToString()));
		name_label->SetToolTipText(FText::FromString(Assets[i].Data.PackageName.ToString()));

		FAssetData target = Assets[i].Data;
		browse_button->SetOnClicked(FOnClicked::CreateLambda([target]()
		{
			TArray<FAssetData> AssetDataList;
			AssetDataList.Add(target);
			GEditor->SyncBrowserToObjects(AssetDataList);

			return FReply::Handled();
		}));
	}
}

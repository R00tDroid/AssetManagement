#include "AssetManagementWidget.h"
#include "AssetManagement.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SCheckBox.h"
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
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	if (AssetRegistry.IsLoadingAssets())
	{
		AssetRegistry.OnFilesLoaded().AddSP(this, &SWidgetAssetManagement::BindToAssetRegistry);
	}
	else
	{
		BindToAssetRegistry();
	}
}

void SWidgetAssetManagement::BindToAssetRegistry()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	AssetRegistry.OnAssetAdded().AddSP(this, &SWidgetAssetManagement::OnAssetAdded);
	AssetRegistry.OnAssetRemoved().AddSP(this, &SWidgetAssetManagement::OnAssetUpdated);
	AssetRegistry.OnAssetRemoved().AddSP(this, &SWidgetAssetManagement::OnAssetRemoved);
	AssetRegistry.OnAssetRenamed().AddSP(this, &SWidgetAssetManagement::OnAssetRenamed);
}

void SWidgetAssetManagement::OnAssetAdded(const FAssetData&)
{
}

void SWidgetAssetManagement::OnAssetUpdated(const FAssetData&)
{
}

void SWidgetAssetManagement::OnAssetRemoved(const FAssetData&)
{
}

void SWidgetAssetManagement::OnAssetRenamed(const FAssetData&, const FString&)
{
}

FReply SWidgetAssetManagement::RequestRescan()
{
	PopulateAssets();

	return FReply::Handled();
}

void SWidgetAssetManagement::PopulateAssets()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	Assets.Empty();
	bool res = AssetRegistry.GetAllAssets(Assets, true);

	TArray<FAssetData> Worlds;
	res = AssetRegistry.GetAssetsByClass(UWorld::StaticClass()->GetFName(), Worlds, true);

	for (int32 i = 0; i < Worlds.Num(); i++)
	{
		FAssetData& Asset = Worlds[i];

		if (!Asset.PackageName.ToString().StartsWith("/Game/", ESearchCase::IgnoreCase))
		{
			if (Worlds.IsValidIndex(i))
			{
				Worlds.RemoveAt(i);
				i--;
			}
		}
	}

	for (int32 i = 0; i < Assets.Num(); i++)
	{
		FAssetData& Asset = Assets[i];

		if (!Asset.PackageName.ToString().StartsWith("/Game/", ESearchCase::IgnoreCase))
		{
			if (Assets.IsValidIndex(i))
			{
				Assets.RemoveAt(i);
				i--;
			}
		}
	}

	TMap<FAssetData, uint16> References;

	for (FAssetData& World : Worlds)
	{
		TArray<FAssetData> ToSearch = {World};
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
			res = AssetRegistry.GetDependencies(Asset.PackageName, Dependencies);

			for (FName& Dependency : Dependencies)
			{
				for (FAssetData& Link : Assets)
				{
					if (Link.PackageName.IsEqual(Dependency))
					{
						if (!Searched.Contains(Link) && !ToSearch.Contains(Link))
						{
							ToSearch.Add(Link);
						}

						break;
					}
				}
			}
		}
	}


	Assets.Sort([References](const FAssetData& A, const FAssetData& B)
	{
		uint16 a_ref, b_ref;
		a_ref = 0;
		b_ref = 0;

		if (References.Contains(A)) a_ref = References[A];
		if (References.Contains(B)) b_ref = References[B];

		return a_ref < b_ref;
	});


	int difference = Assets.Num() - asset_list.Get()->GetChildren()->Num();
	int pre_count = asset_list.Get()->GetChildren()->Num();

	if (difference > 0)
	{
		for (int i = 0; i < FMath::Abs(difference); i++)
		{
			int index = pre_count + i;

			SVerticalBox::FSlot& slot = asset_list->AddSlot();
			slot
			[
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
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(2)
					[
						SNew(SButton)
						.ToolTip(SNew(SActionToolTip))
					]

					+ SHorizontalBox::Slot()
					.Padding(2)
					[
						SNew(SButton)
						.ToolTip(SNew(SActionToolTip))
					]

					+ SHorizontalBox::Slot()
					.Padding(2)
					[
						SNew(SButton)
						.ToolTip(SNew(SActionToolTip))
					]
				]
			];

			slot.Padding(FMargin(4));
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
		

		for (int j = 0; j < 3; j++)
		{
			TSharedRef<SButton> button = StaticCastSharedRef<SButton>(button_container->GetChildren()->GetChildAt(j));
			TSharedPtr<SActionToolTip> tooltip = StaticCastSharedPtr<SActionToolTip>(button->GetToolTip());

			if (j == 0)
			{
				tooltip->SetHeading("Unused asset");
				tooltip->SetContent("Click to delete resource");
			}
			else if (j == 1)
			{
				tooltip->SetHeading("Improper naming");
				tooltip->SetContent("Suggested asset name: " + Assets[i].AssetName.ToString() + "\nClick to apply naming");
			}
			else if (j == 2)
			{
				tooltip->SetHeading("Redirector");
				tooltip->SetContent("Click to fix redirection");
			}
		}

		name_label->SetText(FText::FromString(Assets[i].AssetName.ToString()));
		name_label->SetToolTipText(FText::FromString(Assets[i].PackageName.ToString()));

		FAssetData target = Assets[i];
		browse_button->SetOnClicked(FOnClicked::CreateLambda([target]()
		{
			TArray<FAssetData> AssetDataList;
			AssetDataList.Add(target);
			GEditor->SyncBrowserToObjects(AssetDataList);
			
			return FReply::Handled();
		}));

		uint16 refCount = 0;
		if (References.Contains(Assets[i]))
		{
			refCount = References[Assets[i]];
		}
	}
}

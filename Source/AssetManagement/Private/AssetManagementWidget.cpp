#include "AssetManagementWidget.h"
#include "AssetManagementModule.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Async/Async.h"
#include "EditorStyle.h"
#include "EditorStyleSet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "AssetMagementConfig.h"
#include "AssetManagementStyle.h"

#if ENGINE_MAJOR_VERSION == 5
#define GetCompatibleWidget GetAttachedWidget().ToSharedRef()
#else
#define GetCompatibleWidget GetWidget()
#endif

void SWidgetAssetManagement::Construct(const FArguments& InArgs)
{
    MaxAssetsInList = AssetManagerConfig::Get().GetInt("UI", "AssetListLimit", 200);
    
    TSharedPtr<SGridPanel> FilterGrid;
    
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
                .AutoWidth()
                .Padding(FMargin(5.0f))
                .VAlign(VAlign_Center)
                [
                    SAssignNew(FilterGrid, SGridPanel)
                ]

                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(FMargin(5.0f))
                .VAlign(VAlign_Center)

                + SHorizontalBox::Slot()
                  .AutoWidth()
                  .Padding(FMargin(5.0f))
                [
                    SNew(SButton)
                    .OnClicked(FOnClicked::CreateSP(this, &SWidgetAssetManagement::RequestRescan))
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12))
                        .Text(FText::FromString("Rescan"))
                    ]
                ]
            ]
        ]
    ];

    TArray<IAssetAction*> AssetActions;

    AssetManager* manager = AssetManager::Get();
    if (manager != nullptr)
    {
        AssetActions = manager->GetActions();
    }

    for (int i = 0; i < AssetActions.Num(); i++)
    {
        FilteredActions.Add(i);
        
        auto checkbox_slot = FilterGrid->AddSlot(0, i);
        checkbox_slot.VAlign(EVerticalAlignment::VAlign_Center);

        auto name_slot = FilterGrid->AddSlot(1, i);
        name_slot.VAlign(EVerticalAlignment::VAlign_Center);

        checkbox_slot
        [
            SNew(SCheckBox)
            .OnCheckStateChanged_Lambda([this, i](ECheckBoxState Checked)
            {
                if(Checked == ECheckBoxState::Checked)
                {
                    this->FilteredActions.AddUnique(i);
                    this->PopulateAssets();
                }
            else
                {
                    this->FilteredActions.Remove(i);
                    this->PopulateAssets();
                }
            })
        ];

        name_slot
        [
            SNew(STextBlock)
            .Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 8))
            .Text(FText::FromString(AssetActions[i]->GetFilterName()))
        ];

        TSharedRef<SCheckBox> checkbox = StaticCastSharedRef<SCheckBox>(checkbox_slot.GetCompatibleWidget);
        checkbox->SetIsChecked(ECheckBoxState::Checked);
        
        auto apply_all_slot = FilterGrid->AddSlot(2, i);
        checkbox_slot.VAlign(EVerticalAlignment::VAlign_Fill);
        apply_all_slot.Padding(10, 0, 0, 0);

        apply_all_slot
        [
            SNew(SButton)
            .OnClicked_Lambda([this, i]()
            {
                this->ApplyAll(i);
                return FReply::Handled();
            })
            [
                SNew(STextBlock)
                .Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 8))
                .Text(FText::FromString(AssetActions[i]->GetApplyAllTag()))
            ]
        ];
    }
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

    for (int i = 0; i < Assets.Num(); i++)
    {
        TArray<uint16> ActionIndices;
        Assets[i].ActionResults.GetKeys(ActionIndices);
        bool ShouldDisplay = false;
        for(uint16 Index : ActionIndices)
        {
            if(FilteredActions.Contains(Index))
            {
                ShouldDisplay = true;
                break;
            }
        }

        if(!ShouldDisplay)
        {
            Assets.RemoveAt(i);
            i--;
        }
    }

    while (Assets.Num() > MaxAssetsInList)
    {
        Assets.RemoveAt(Assets.Num() - 1);
    }

    ResizeList(Assets.Num(), AssetActions);

    UpdateAssetList(Assets, AssetActions);
}

void SWidgetAssetManagement::ApplyAll(int Index)
{
    TArray<FAssetInfo> Assets;
    TArray<FAssetData> ToApplyFor;

    AssetManager* manager = AssetManager::Get();
    if (manager != nullptr)
    {
        Assets = manager->GetAssets();
    }

    for (FAssetInfo& Asset : Assets)
    {
        if(Asset.ActionResults.Contains(Index))
        {
            ToApplyFor.Add(Asset.Data);
        }
    }

    if (Assets.Num() > 0)
    {
        manager->RequestActionExecution(Index, ToApplyFor);
    }
}

void SWidgetAssetManagement::ResizeList(int AmountOfAssets, TArray<IAssetAction*>& AssetActions)
{
    int difference = AmountOfAssets - asset_list.Get()->GetChildren()->Num();
    int pre_count = asset_list.Get()->GetChildren()->Num();

    if (difference > 0)
    {
        for (int i = 0; i < FMath::Abs(difference); i++)
        {
            int index = pre_count + i;

            TSharedPtr<SHorizontalBox> ButtonContainer;

            auto slot = asset_list->AddSlot();
            slot
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1)
                .VAlign(EVerticalAlignment::VAlign_Center)
                [
                    SNew(STextBlock)
                    .Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 8))
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
                auto button_slot = ButtonContainer->AddSlot();
                button_slot.Padding(2);
                
                button_slot[
                    SNew(SButton)
                    .ToolTip(SNew(SActionToolTip))
                    [
                        SNew(SImage)
                        .Image(FAssetManagementStyle::Get().GetBrush(FName(AssetActions[j]->GetButtonStyleName())))
                        .ColorAndOpacity(FSlateColor::UseForeground())
                    ]
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
}

void SWidgetAssetManagement::UpdateAssetList(TArray<FAssetInfo>& Assets, TArray<IAssetAction*>& AssetActions)
{
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

            FAssetData target = Assets[i].Data;
            int ActionId = j;
            button->SetOnClicked(FOnClicked::CreateLambda([ActionId, target]()
            {
                AssetManager* manager = AssetManager::Get();
                if (manager != nullptr) manager->RequestActionExecution(ActionId, { target });
                return FReply::Handled();
            }));
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

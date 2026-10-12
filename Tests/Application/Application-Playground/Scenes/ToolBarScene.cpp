#include <Core/Containers/SharedPtr.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/Spacer.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Elements/ToolBar.h>
#include <Application/Menus/ComboBox.h>
#include <Application/Menus/Menu.h>
#include <Application/Menus/MenuItem.h>
#include <Application/Style/UIStyle.h>

#include "PlaygroundScene.h"
#include "ScenePanel.h"

static TSharedPtr<FTextBlock> MakeReadout(const FPlaygroundFonts& Fonts, const FString& InText)
{
    FTextBlock::FDesc Desc;
    Desc.Text            = InText;
    Desc.Font            = Fonts.Monospace;
    Desc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    return FTextBlock::Create(Desc);
}

static TSharedPtr<FMenuItem> MakeItem(const FPlaygroundFonts& Fonts, const FString& Label, const TSharedPtr<FTextBlock>& Readout)
{
    FMenuItem::FDesc Desc;
    Desc.SetLabel(Label).SetFont(Fonts.Body);
    Desc.OnActivated = FOnMenuItemActivated::CreateLambda([Readout, Label]()
    {
        Readout->SetText(FString::Printf(TEXT("chose %s"), Label.Data()));
    });

    return FMenuItem::Create(Desc);
}

static TSharedPtr<FVisualElement> FrameToolBar(const TSharedPtr<FVisualElement>& ToolBar)
{
    FBorder::FDesc Desc;
    Desc.BackgroundColor = FUIStyle::GetDefault().Colors.PanelBackground;
    Desc.BorderColor     = FUIStyle::GetDefault().Colors.Border;
    Desc.BorderThickness = FUIStyle::GetDefault().Metrics.BorderThickness;
    Desc.CornerRadius    = FUIStyle::GetDefault().Metrics.CornerRadius;
    Desc.Content         = ToolBar;

    return FBorder::Create(Desc);
}

static TSharedPtr<FVisualElement> MakeMainToolBarRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, TEXT("nothing pressed yet"));

    FToolBar::FDesc Desc;
    Desc.Font           = Fonts.Body;
    Desc.bHasBackground = false;

    TSharedPtr<FToolBar> ToolBar = FToolBar::Create(Desc);

    const CHAR_T* Actions[] = { TEXT("New"), TEXT("Open"), TEXT("Save") };
    for (const CHAR_T* Action : Actions)
    {
        ToolBar->AddButton(
            FToolBarItemDesc().SetLabel(Action).SetToolTipText(FString::Printf(TEXT("%s the scene"), Action)),
            FOnClicked::CreateLambda([Readout, Action]()
            {
                Readout->SetText(FString::Printf(TEXT("pressed %s"), Action));
            }));
    }

    ToolBar->AddSeparator();

    const CHAR_T* Toggles[] = { TEXT("Grid"), TEXT("Wireframe"), TEXT("Stats") };
    for (const CHAR_T* Toggle : Toggles)
    {
        ToolBar->AddToggle(
            FToolBarItemDesc().SetLabel(Toggle).SetToolTipText(FString::Printf(TEXT("Show %s in the viewport"), Toggle)),
            ECheckBoxState::Unchecked,
            FOnCheckStateChanged::CreateLambda([Readout, Toggle](ECheckBoxState NewState)
            {
                Readout->SetText(FString::Printf(TEXT("%s is now %s"), Toggle, NewState == ECheckBoxState::Checked ? TEXT("on") : TEXT("off")));
            }));
    }

    ToolBar->AddSeparator();

    TSharedPtr<FMenu> ShadingMenu = FMenu::Create();
    ShadingMenu->AddItem(MakeItem(Fonts, TEXT("Lit"), Readout));
    ShadingMenu->AddItem(MakeItem(Fonts, TEXT("Unlit"), Readout));
    ShadingMenu->AddSeparator();
    ShadingMenu->AddItem(MakeItem(Fonts, TEXT("Base Color"), Readout));
    ShadingMenu->AddItem(MakeItem(Fonts, TEXT("Normals"), Readout));

    TSharedPtr<FMenu> SnapMenu = FMenu::Create();
    SnapMenu->AddItem(MakeItem(Fonts, TEXT("Off"), Readout));
    SnapMenu->AddItem(MakeItem(Fonts, TEXT("10 cm"), Readout));
    SnapMenu->AddItem(MakeItem(Fonts, TEXT("1 m"), Readout));

    ToolBar->AddDropDown(FToolBarItemDesc().SetLabel(TEXT("Shading")), ShadingMenu);
    ToolBar->AddDropDown(FToolBarItemDesc().SetLabel(TEXT("Snap")), SnapMenu);

    ToolBar->AddSeparator();

    FComboBox::FDesc ComboDesc;
    ComboDesc.SetOptions({ TEXT("Perspective"), TEXT("Top"), TEXT("Front"), TEXT("Side") }).SetFont(Fonts.Body);
    ComboDesc.SelectedIndex      = 0;
    ComboDesc.OnSelectionChanged = FOnComboSelectionChanged::CreateLambda([Readout](int32 SelectedIndex)
    {
        const CHAR_T* Views[] = { TEXT("Perspective"), TEXT("Top"), TEXT("Front"), TEXT("Side") };
        Readout->SetText(FString::Printf(TEXT("camera is now %s"), Views[Math::Clamp(SelectedIndex, 0, 3)]));
    });

    ToolBar->AddWidget(FComboBox::Create(ComboDesc));

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FrameToolBar(ToolBar)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeVerticalToolBarRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, TEXT("select is the tool in force"));

    FToolBar::FDesc Desc;
    Desc.Font           = Fonts.Body;
    Desc.Orientation    = EOrientation::Vertical;
    Desc.bHasBackground = false;

    TSharedPtr<FToolBar> ToolBar = FToolBar::Create(Desc);
    TSharedPtr<TArray<TSharedPtr<FToolBarButton>>> Tools = MakeSharedPtr<TArray<TSharedPtr<FToolBarButton>>>();

    const CHAR_T* ToolNames[] = { TEXT("Select"), TEXT("Move"), TEXT("Rotate"), TEXT("Scale") };
    for (const CHAR_T* ToolName : ToolNames)
    {
        TSharedPtr<FToolBarButton> Tool = ToolBar->AddToggle(
            FToolBarItemDesc().SetLabel(ToolName).SetToolTipText(FString::Printf(TEXT("The %s tool"), ToolName)),
            ECheckBoxState::Unchecked,
            FOnCheckStateChanged());

        Tools->Add(Tool);
    }

    for (const TSharedPtr<FToolBarButton>& Tool : *Tools)
    {
        TWeakPtr<FToolBarButton> WeakTool = Tool;
        Tool->SetOnStateChanged(FOnCheckStateChanged::CreateLambda([Tools, WeakTool, Readout](ECheckBoxState NewState)
        {
            TSharedPtr<FToolBarButton> LiveTool(WeakTool);
            if (!LiveTool || NewState != ECheckBoxState::Checked)
            {
                return;
            }

            for (const TSharedPtr<FToolBarButton>& Sibling : *Tools)
            {
                if (Sibling != LiveTool)
                {
                    Sibling->SetCheckState(ECheckBoxState::Unchecked);
                }
            }

            Readout->SetText(FString::Printf(TEXT("%s is the tool in force"), LiveTool->GetLabel().Data()));
        }));
    }

    (*Tools)[0]->SetCheckState(ECheckBoxState::Checked);

    ToolBar->AddSeparator();

    TSharedPtr<FToolBarButton> Retired = ToolBar->AddButton(
        FToolBarItemDesc().SetLabel(TEXT("Bake")).SetToolTipText(TEXT("Nothing to bake in the playground")),
        FOnClicked());

    Retired->SetEnabled(false);

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(FrameToolBar(ToolBar)).SetVerticalAlignment(EVerticalAlignment::Top);
    Row->AddSlot(Readout).SetPadding(FMargin(12, 4, 0, 0)).SetVerticalAlignment(EVerticalAlignment::Top);
    Row->AddSlot(FSpacer::Create(FIntVector2(0, 0))).SetFillCoefficient(1.0f);
    return Row;
}

FPlaygroundScene CreateToolBarScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = TEXT("Toolbar");
        Desc.Description = TEXT("Buttons that fire, toggles that latch and dropdowns that open menus, in groups divided by rules. Rest on one for its tip, and once a dropdown is open, hovering its sibling switches.");
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeMainToolBarRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = TEXT("Vertical strip");
        Desc.Description = TEXT("The same strip stacked downwards, holding a one-of-many tool group and an entry that is disabled.");
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeVerticalToolBarRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene(TEXT("Toolbar"), MakeSceneColumn(TEXT("Toolbar"), Fonts, Panels));
}

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

static TSharedPtr<FTextBlock> MakeReadout(const FPlaygroundFonts& Fonts, const String& InText)
{
    FTextBlock::FDesc Desc;
    Desc.Text            = InText;
    Desc.Font            = Fonts.Monospace;
    Desc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    return FTextBlock::Create(Desc);
}

static TSharedPtr<FMenuItem> MakeItem(const FPlaygroundFonts& Fonts, const String& Label, const TSharedPtr<FTextBlock>& Readout)
{
    FMenuItem::FDesc Desc;
    Desc.SetLabel(Label).SetFont(Fonts.Body);
    Desc.OnActivated = FOnMenuItemActivated::CreateLambda([Readout, Label]()
    {
        Readout->SetText(String::Printf("chose %s", Label.Data()));
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
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, "nothing pressed yet");

    FToolBar::FDesc Desc;
    Desc.Font           = Fonts.Body;
    Desc.bHasBackground = false;

    TSharedPtr<FToolBar> ToolBar = FToolBar::Create(Desc);

    const CHAR* Actions[] = { "New", "Open", "Save" };
    for (const CHAR* Action : Actions)
    {
        ToolBar->AddButton(
            FToolBarItemDesc().SetLabel(Action).SetToolTipText(String::Printf("%s the scene", Action)),
            FOnClicked::CreateLambda([Readout, Action]()
            {
                Readout->SetText(String::Printf("pressed %s", Action));
            }));
    }

    ToolBar->AddSeparator();

    const CHAR* Toggles[] = { "Grid", "Wireframe", "Stats" };
    for (const CHAR* Toggle : Toggles)
    {
        ToolBar->AddToggle(
            FToolBarItemDesc().SetLabel(Toggle).SetToolTipText(String::Printf("Show %s in the viewport", Toggle)),
            ECheckBoxState::Unchecked,
            FOnCheckStateChanged::CreateLambda([Readout, Toggle](ECheckBoxState NewState)
            {
                Readout->SetText(String::Printf("%s is now %s", Toggle, NewState == ECheckBoxState::Checked ? "on" : "off"));
            }));
    }

    ToolBar->AddSeparator();

    TSharedPtr<FMenu> ShadingMenu = FMenu::Create();
    ShadingMenu->AddItem(MakeItem(Fonts, "Lit", Readout));
    ShadingMenu->AddItem(MakeItem(Fonts, "Unlit", Readout));
    ShadingMenu->AddSeparator();
    ShadingMenu->AddItem(MakeItem(Fonts, "Base Color", Readout));
    ShadingMenu->AddItem(MakeItem(Fonts, "Normals", Readout));

    TSharedPtr<FMenu> SnapMenu = FMenu::Create();
    SnapMenu->AddItem(MakeItem(Fonts, "Off", Readout));
    SnapMenu->AddItem(MakeItem(Fonts, "10 cm", Readout));
    SnapMenu->AddItem(MakeItem(Fonts, "1 m", Readout));

    ToolBar->AddDropDown(FToolBarItemDesc().SetLabel("Shading"), ShadingMenu);
    ToolBar->AddDropDown(FToolBarItemDesc().SetLabel("Snap"), SnapMenu);

    ToolBar->AddSeparator();

    FComboBox::FDesc ComboDesc;
    ComboDesc.SetOptions({ "Perspective", "Top", "Front", "Side" }).SetFont(Fonts.Body);
    ComboDesc.SelectedIndex      = 0;
    ComboDesc.OnSelectionChanged = FOnComboSelectionChanged::CreateLambda([Readout](int32 SelectedIndex)
    {
        const CHAR* Views[] = { "Perspective", "Top", "Front", "Side" };
        Readout->SetText(String::Printf("camera is now %s", Views[Math::Clamp(SelectedIndex, 0, 3)]));
    });

    ToolBar->AddWidget(FComboBox::Create(ComboDesc));

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FrameToolBar(ToolBar)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeVerticalToolBarRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, "select is the tool in force");

    FToolBar::FDesc Desc;
    Desc.Font           = Fonts.Body;
    Desc.Orientation    = EOrientation::Vertical;
    Desc.bHasBackground = false;

    TSharedPtr<FToolBar> ToolBar = FToolBar::Create(Desc);
    TSharedPtr<TArray<TSharedPtr<FToolBarButton>>> Tools = MakeSharedPtr<TArray<TSharedPtr<FToolBarButton>>>();

    const CHAR* ToolNames[] = { "Select", "Move", "Rotate", "Scale" };
    for (const CHAR* ToolName : ToolNames)
    {
        TSharedPtr<FToolBarButton> Tool = ToolBar->AddToggle(
            FToolBarItemDesc().SetLabel(ToolName).SetToolTipText(String::Printf("The %s tool", ToolName)),
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

            Readout->SetText(String::Printf("%s is the tool in force", LiveTool->GetLabel().Data()));
        }));
    }

    (*Tools)[0]->SetCheckState(ECheckBoxState::Checked);

    ToolBar->AddSeparator();

    TSharedPtr<FToolBarButton> Retired = ToolBar->AddButton(
        FToolBarItemDesc().SetLabel("Bake").SetToolTipText("Nothing to bake in the playground"),
        FOnClicked());

    Retired->SetEnabled(false);

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(FrameToolBar(ToolBar)).SetVerticalAlignment(EVerticalAlignment::Top);
    Row->AddSlot(Readout).SetPadding(FMargin(12, 4, 0, 0)).SetVerticalAlignment(EVerticalAlignment::Top);
    Row->AddSlot(FSpacer::Create(IntVector2(0, 0))).SetFillCoefficient(1.0f);
    return Row;
}

FPlaygroundScene CreateToolBarScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Toolbar";
        Desc.Description = "Buttons that fire, toggles that latch and dropdowns that open menus, in groups divided by rules. Rest on one for its tip, and once a dropdown is open, hovering its sibling switches.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeMainToolBarRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Vertical strip";
        Desc.Description = "The same strip stacked downwards, holding a one-of-many tool group and an entry that is disabled.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeVerticalToolBarRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Toolbar", MakeSceneColumn("Toolbar", Fonts, Panels));
}

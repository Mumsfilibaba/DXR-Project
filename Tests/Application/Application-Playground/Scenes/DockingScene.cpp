#include <Core/Containers/SharedPtr.h>
#include <Application/Docking/DockDragState.h>
#include <Application/Docking/DockNode.h>
#include <Application/Docking/DockingArea.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/Button.h>
#include <Application/Elements/Overlay.h>
#include <Application/Elements/Spacer.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Style/UIStyle.h>

#include "PlaygroundScene.h"
#include "ScenePanel.h"

// Tall enough that the panels inside it can be split a few times before anything hits its minimum
constexpr int32 DOCKING_AREA_HEIGHT = 420;

// Where the scene keeps the layout it saves, next to the playground's working directory
constexpr const CHAR* DOCKING_LAYOUT_FILE = "PlaygroundLayout.ini";

static TSharedPtr<FDockingArea> GDockingArea;
static TSharedPtr<FTextBlock>   GLayoutReadout;

static TSharedPtr<FVisualElement> MakePanelBody(const FPlaygroundFonts& Fonts, const String& Title, const String& Body)
{
    FTextBlock::FDesc HeadingDesc;
    HeadingDesc.Text            = Title;
    HeadingDesc.Font            = Fonts.Heading;
    HeadingDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.Text;

    FTextBlock::FDesc BodyDesc;
    BodyDesc.Text            = Body;
    BodyDesc.Font            = Fonts.Body;
    BodyDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FTextBlock::Create(HeadingDesc)).SetPadding(FMargin(0, 0, 0, 8));
    Column->AddSlot(FTextBlock::Create(BodyDesc));
    Column->AddSlot(FSpacer::Create(IntVector2(0, 0))).SetFillCoefficient(1.0f);

    FBorder::FDesc BorderDesc;
    BorderDesc.BackgroundColor = FUIStyle::GetDefault().Colors.PanelBackground;
    BorderDesc.Padding         = FMargin(12, 12);
    BorderDesc.Content         = Column;

    return FBorder::Create(BorderDesc);
}

static void DescribeNode(const FDockNode& Node, String& OutText)
{
    if (Node.Kind == EDockNodeKind::Tabs)
    {
        if (Node.TabIds.IsEmpty())
        {
            OutText.Append("(empty)");
            return;
        }

        for (int32 Index = 0; Index < Node.TabIds.Size(); ++Index)
        {
            if (Index > 0)
            {
                OutText.Append('|');
            }

            if (Index == Node.ActiveTabIndex)
            {
                OutText.Append('*');
            }

            OutText.Append(Node.TabIds[Index]);
        }

        return;
    }

    OutText.Append(Node.Orientation == EDockSplitOrientation::Horizontal ? "row[" : "col[");

    for (int32 Index = 0; Index < Node.Children.Size(); ++Index)
    {
        if (Index > 0)
        {
            OutText.Append(' ');
        }

        const float Share = Index < Node.ChildFractions.Size() ? Node.ChildFractions[Index] : 0.0f;

        OutText.Append(String::Printf("%.0f%%:", Share * 100.0f));
        DescribeNode(Node.Children[Index], OutText);
    }

    OutText.Append(']');
}

static void RefreshReadout()
{
    if (!GDockingArea || !GLayoutReadout)
    {
        return;
    }

    String Description;
    DescribeNode(GDockingArea->SaveLayout(), Description);

    GLayoutReadout->SetText(Description);
}

static void ApplyDefaultLayout()
{
    FDockNode Details = FDockNode::MakeTabs({ "Details", "Materials" });
    FDockNode Right   = FDockNode::MakeSplit(EDockSplitOrientation::Vertical, Details, FDockNode::MakeTabs({ "Output" }));

    GDockingArea->RestoreLayout(FDockNode::MakeSplit(EDockSplitOrientation::Horizontal, FDockNode::MakeTabs({ "Outliner" }), Right));
    RefreshReadout();
}

static TSharedPtr<FVisualElement> MakeDockingArea(const FPlaygroundFonts& Fonts)
{
    FDockingArea::FDesc Desc;
    Desc.Font = Fonts.Body;

    Desc.OnPanelClosed = FOnPanelClosed::CreateLambda([](const String&)
    {
        RefreshReadout();
    });

    Desc.OnPanelTornOut = FOnPanelTornOut::CreateLambda([](const String&, const IntVector2&)
    {
        RefreshReadout();
    });

    GDockingArea = FDockingArea::Create(Desc);

    GDockingArea->RegisterPanel("Outliner", "Outliner", MakePanelBody(Fonts, "Outliner", "Drag a tab sideways to reorder it. Drag it clear of the strip and an indicator shows where it would land."));
    GDockingArea->RegisterPanel("Details", "Details", MakePanelBody(Fonts, "Details", "The edges of a panel are drop zones. The middle stacks the dragged panel as another tab."));
    GDockingArea->RegisterPanel("Materials", "Materials", MakePanelBody(Fonts, "Materials", "This one starts stacked behind Details, so the strip has two tabs to reorder."));
    GDockingArea->RegisterPanel("Output", "Output", MakePanelBody(Fonts, "Output", "Drag the bar between the panels to resize them. Neither side goes below its minimum."));

    ApplyDefaultLayout();

    FBorder::FDesc BorderDesc;
    BorderDesc.BorderColor     = FUIStyle::GetDefault().Colors.Border;
    BorderDesc.BorderThickness = FUIStyle::GetDefault().Metrics.BorderThickness;
    BorderDesc.Content         = GDockingArea;

    TSharedPtr<FOverlay> Sized = FOverlay::Create();
    Sized->AddSlot(FSpacer::CreateVertical(DOCKING_AREA_HEIGHT));
    Sized->AddSlot(FBorder::Create(BorderDesc));

    return Sized;
}

static TSharedPtr<FVisualElement> MakeLayoutControls(const FPlaygroundFonts& Fonts)
{
    FTextBlock::FDesc ReadoutDesc;
    ReadoutDesc.Text            = "";
    ReadoutDesc.Font            = Fonts.Monospace;
    ReadoutDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    GLayoutReadout = FTextBlock::Create(ReadoutDesc);
    RefreshReadout();

    FButton::FDesc SaveDesc;
    SaveDesc.SetText("Save the layout").SetFont(Fonts.Body);
    SaveDesc.SetOnClicked(FOnClicked::CreateLambda([]()
    {
        GDockingArea->SaveLayoutToFile(DOCKING_LAYOUT_FILE);
        RefreshReadout();
    }));

    FButton::FDesc RestoreDesc;
    RestoreDesc.SetText("Restore it").SetFont(Fonts.Body);
    RestoreDesc.SetOnClicked(FOnClicked::CreateLambda([]()
    {
        GDockingArea->RestoreLayoutFromFile(DOCKING_LAYOUT_FILE);
        RefreshReadout();
    }));

    FButton::FDesc ResetDesc;
    ResetDesc.SetText("Back to the default").SetFont(Fonts.Body);
    ResetDesc.SetOnClicked(FOnClicked::CreateLambda([]()
    {
        ApplyDefaultLayout();
    }));

    FButton::FDesc ReadDesc;
    ReadDesc.SetText("Re-read the tree").SetFont(Fonts.Body);
    ReadDesc.SetOnClicked(FOnClicked::CreateLambda([]()
    {
        RefreshReadout();
    }));

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(FButton::Create(SaveDesc)).SetPadding(FMargin(0, 0, 12, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FButton::Create(RestoreDesc)).SetPadding(FMargin(0, 0, 12, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FButton::Create(ResetDesc)).SetPadding(FMargin(0, 0, 12, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FButton::Create(ReadDesc)).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FSpacer::Create(IntVector2(0, 0))).SetFillCoefficient(1.0f);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(Row);
    Column->AddSlot(GLayoutReadout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Fill);
    return Column;
}

FPlaygroundScene CreateDockingScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Docked panels";
        Desc.Description = "Splitters, tab strips and drop zones over one tree. Every gesture edits the tree and the elements are rebuilt from it.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeDockingArea(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "The tree underneath";
        Desc.Description = "The same layout written out, with each split's shares. Saving writes it to an ini file that survives the run.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeLayoutControls(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Docking", MakeSceneColumn("Docking", Fonts, Panels));
}

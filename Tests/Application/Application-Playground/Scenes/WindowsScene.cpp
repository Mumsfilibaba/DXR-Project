#include <Core/Containers/SharedPtr.h>
#include <Application/Application.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/Button.h>
#include <Application/Elements/FloatingWindow.h>
#include <Application/Elements/Spacer.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Elements/TitleBar.h>
#include <Application/Menus/Menu.h>
#include <Application/Menus/MenuBar.h>
#include <Application/Menus/MenuItem.h>
#include <Application/Style/UIStyle.h>

#include "PlaygroundScene.h"
#include "ScenePanel.h"

// Where the first floating window lands, and how far each one after it is pushed so they do not stack up
constexpr int32 FLOATING_WINDOW_ORIGIN_X = 320;
constexpr int32 FLOATING_WINDOW_ORIGIN_Y = 240;
constexpr int32 FLOATING_WINDOW_CASCADE  = 32;

static TArray<TSharedPtr<FFloatingWindow>> GOpenWindows;

static TSharedPtr<FTextBlock> MakeReadout(const FPlaygroundFonts& Fonts, const String& InText)
{
    FTextBlock::FDesc Desc;
    Desc.Text            = InText;
    Desc.Font            = Fonts.Monospace;
    Desc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    return FTextBlock::Create(Desc);
}

static void PruneClosedWindows()
{
    for (int32 Index = GOpenWindows.Size() - 1; Index >= 0; --Index)
    {
        const TSharedPtr<FFloatingWindow>& Floating = GOpenWindows[Index];
        if (!Floating->IsOpen() || !FApplication::Get().GetWindows().Contains(Floating->GetWindow()))
        {
            GOpenWindows.RemoveAt(Index);
        }
    }
}

static void RefreshReadout(const FPlaygroundFonts& Fonts, const TSharedPtr<FTextBlock>& Readout)
{
    UNREFERENCED_VARIABLE(Fonts);

    PruneClosedWindows();

    if (GOpenWindows.IsEmpty())
    {
        Readout->SetText("no floating windows open");
        return;
    }

    const TSharedPtr<FTitleBar>& TitleBar = GOpenWindows.Last()->GetTitleBar();
    const FWindowTitleBarRegions& Regions = TitleBar->GetRegions();

    Readout->SetText(String::Printf(
        "%d open, last caption %d x %d with %d clickable region(s), platform inset %.0f leading %.0f trailing",
        GOpenWindows.Size(),
        Regions.CaptionRect.Right - Regions.CaptionRect.Left,
        Regions.CaptionRect.Bottom - Regions.CaptionRect.Top,
        Regions.InteractiveRects.Size(),
        TitleBar->GetMetrics().LeadingInset,
        TitleBar->GetMetrics().TrailingInset));
}

static TSharedPtr<FMenuBar> MakeCaptionMenuBar(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FMenu> ViewMenu = FMenu::Create();
    for (const CHAR* Label : { "Wireframe", "Normals", "Overdraw" })
    {
        FMenuItem::FDesc ItemDesc;
        ItemDesc.SetLabel(Label).SetFont(Fonts.Body);
        ItemDesc.bIsCheckable = true;

        ViewMenu->AddItem(FMenuItem::Create(ItemDesc));
    }

    TSharedPtr<FMenu> PanelMenu = FMenu::Create();
    for (const CHAR* Label : { "Details", "Materials", "Statistics" })
    {
        FMenuItem::FDesc ItemDesc;
        ItemDesc.SetLabel(Label).SetFont(Fonts.Body);

        PanelMenu->AddItem(FMenuItem::Create(ItemDesc));
    }

    TSharedPtr<FMenuBar> MenuBar = FMenuBar::Create();
    MenuBar->AddMenu("View", Fonts.Body, ViewMenu);
    MenuBar->AddMenu("Panel", Fonts.Body, PanelMenu);
    return MenuBar;
}

static TSharedPtr<FVisualElement> MakeFloatingWindowBody(const FPlaygroundFonts& Fonts, int32 Ordinal)
{
    FTextBlock::FDesc HeadingDesc;
    HeadingDesc.Text            = String::Printf("Floating window %d", Ordinal);
    HeadingDesc.Font            = Fonts.Heading;
    HeadingDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.Text;

    FTextBlock::FDesc BodyDesc;
    BodyDesc.Text            = "Drag the caption to move it. The gaps drag, the buttons and menus do not.";
    BodyDesc.Font            = Fonts.Body;
    BodyDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FTextBlock::Create(HeadingDesc)).SetPadding(FMargin(0, 0, 0, 8));
    Column->AddSlot(FTextBlock::Create(BodyDesc));
    Column->AddSlot(FSpacer::Create(IntVector2(0, 0))).SetFillCoefficient(1.0f);

    FBorder::FDesc BorderDesc;
    BorderDesc.BackgroundColor = FUIStyle::GetDefault().Colors.WindowBackground;
    BorderDesc.Padding         = FMargin(16, 16);
    BorderDesc.Content         = Column;

    return FBorder::Create(BorderDesc);
}

static void OpenFloatingWindow(const FPlaygroundFonts& Fonts, const TSharedPtr<FTextBlock>& Readout)
{
    PruneClosedWindows();

    const int32 Ordinal = GOpenWindows.Size() + 1;
    const int32 Offset  = GOpenWindows.Size() * FLOATING_WINDOW_CASCADE;

    FFloatingWindow::FDesc Desc;
    Desc.SetTitle(String::Printf("Inspector %d", Ordinal));
    Desc.SetBounds(IntVector2(FLOATING_WINDOW_ORIGIN_X + Offset, FLOATING_WINDOW_ORIGIN_Y + Offset), IntVector2(520, 300));
    Desc.SetContent(MakeFloatingWindowBody(Fonts, Ordinal));

    Desc.Font            = Fonts.Body;
    Desc.TitleBarContent = MakeCaptionMenuBar(Fonts);
    Desc.ParentWindow    = FApplication::Get().GetWindows().IsEmpty() ? nullptr : FApplication::Get().GetWindows()[0];

    if (TSharedPtr<FFloatingWindow> Floating = FFloatingWindow::Create(Desc))
    {
        GOpenWindows.Add(Floating);
    }

    RefreshReadout(Fonts, Readout);
}

static TSharedPtr<FVisualElement> MakeFloatingWindowRow(const FPlaygroundFonts& Fonts)
{
    TSharedPtr<FTextBlock> Readout = MakeReadout(Fonts, "no floating windows open");

    FButton::FDesc OpenDesc;
    OpenDesc.SetText("Open a floating window").SetFont(Fonts.Body);
    OpenDesc.SetOnClicked(FOnClicked::CreateLambda([Fonts, Readout]()
    {
        OpenFloatingWindow(Fonts, Readout);
    }));

    FButton::FDesc CloseDesc;
    CloseDesc.SetText("Close the last one").SetFont(Fonts.Body);
    CloseDesc.SetOnClicked(FOnClicked::CreateLambda([Fonts, Readout]()
    {
        PruneClosedWindows();

        if (!GOpenWindows.IsEmpty())
        {
            GOpenWindows.Last()->Close();
            GOpenWindows.Pop();
        }

        RefreshReadout(Fonts, Readout);
    }));

    FButton::FDesc RefreshDesc;
    RefreshDesc.SetText("Re-read the regions").SetFont(Fonts.Body);
    RefreshDesc.SetOnClicked(FOnClicked::CreateLambda([Fonts, Readout]()
    {
        RefreshReadout(Fonts, Readout);
    }));

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(FButton::Create(OpenDesc)).SetPadding(FMargin(0, 0, 12, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FButton::Create(CloseDesc)).SetPadding(FMargin(0, 0, 12, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FButton::Create(RefreshDesc)).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FSpacer::Create(IntVector2(0, 0))).SetFillCoefficient(1.0f);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(Row);
    Column->AddSlot(Readout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeTitleBarPreview(const FPlaygroundFonts& Fonts)
{
    FTitleBar::FDesc Desc;
    Desc.SetTitle("Sandbox").SetFont(Fonts.Body).SetContent(MakeCaptionMenuBar(Fonts));
    Desc.bShowCaptionButtons = true;

    FBorder::FDesc BorderDesc;
    BorderDesc.BorderColor     = FUIStyle::GetDefault().Colors.Border;
    BorderDesc.BorderThickness = FUIStyle::GetDefault().Metrics.BorderThickness;
    BorderDesc.Content         = FTitleBar::Create(Desc);

    return FBorder::Create(BorderDesc);
}

FPlaygroundScene CreateWindowsScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Floating windows";
        Desc.Description = "Real top-level windows with the caption drawn by us. Each one carries a menu bar where the title text usually sits.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeFloatingWindowRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Caption up close";
        Desc.Description = "The same bar laid out in the page. On macOS the window commands collapse, because the OS draws its own at the leading edge.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeTitleBarPreview(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Windows", MakeSceneColumn("Windows", Fonts, Panels));
}

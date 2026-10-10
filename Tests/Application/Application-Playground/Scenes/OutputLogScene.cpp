#include <Core/Containers/SharedPtr.h>
#include <Core/Misc/OutputDeviceManager.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/Button.h>
#include <Application/Elements/CheckBox.h>
#include <Application/Elements/EditableText.h>
#include <Application/Elements/LogView.h>
#include <Application/Elements/Overlay.h>
#include <Application/Elements/RichTextBlock.h>
#include <Application/Elements/Spacer.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Menus/ComboBox.h>
#include <Application/Style/UIStyle.h>

#include "PlaygroundScene.h"
#include "ScenePanel.h"

// Tall enough to show a dozen lines, which is what makes the autoscroll worth watching
constexpr int32 LOG_VIEW_HEIGHT = 260;

static TSharedPtr<FLogView>        GLogView;
static TSharedPtr<FRichTextBlock>  GRichText;
static TSharedPtr<FTextBlock>      GLogReadout;
static TSharedPtr<FTextBlock>      GSelectionReadout;
static bool                        GFilterToMatches = false;
static String                      GSearchText;
static int32                       GNextSample = 0;

static void RefreshLogReadout()
{
    if (!GLogView || !GLogReadout)
    {
        return;
    }

    GLogReadout->SetText(String::Printf(
        "%d line(s) held, %d showing, %s, %s",
        GLogView->GetNumLines(),
        GLogView->GetNumVisibleLines(),
        GLogView->IsAutoScrollEnabled() ? "following the tail" : "not following",
        GLogView->GetSearchText().IsEmpty() ? "no search" : (GLogView->IsFilteringToMatches() ? "hiding non-matches" : "highlighting matches")));
}

static void LogNextSample()
{
    struct FSample
    {
        ELogSeverity Severity;
        const CHAR*  Message;
    };

    static const FSample Samples[] =
    {
        { ELogSeverity::Info,    "Loaded 12 materials from the package cache" },
        { ELogSeverity::Info,    "Compiled GBuffer.hlsl in 42ms" },
        { ELogSeverity::Warning, "Texture 'Rock_N' has no mipmaps, sampling will alias" },
        { ELogSeverity::Info,    "Streamed 3 clusters into the visibility buffer" },
        { ELogSeverity::Error,   "Failed to create pipeline state, root signature mismatch" },
        { ELogSeverity::Info,    "Frame 1204 took 8.3ms, 6.1ms of it on the GPU" },
        { ELogSeverity::Warning, "Shader cache miss for VS_Skinned, compiling on demand" },
    };

    const FSample& Sample = Samples[GNextSample % static_cast<int32>(ARRAY_COUNT(Samples))];
    GNextSample++;

    GLogView->Log(Sample.Severity, String::Printf("%s (#%d)", Sample.Message, GNextSample));
    GLogView->Flush();

    RefreshLogReadout();
}

static TSharedPtr<FVisualElement> MakeSizedBorder(const TSharedPtr<FVisualElement>& Content, int32 Height)
{
    FBorder::FDesc BorderDesc;
    BorderDesc.BorderColor     = FUIStyle::GetDefault().Colors.Border;
    BorderDesc.BorderThickness = FUIStyle::GetDefault().Metrics.BorderThickness;
    BorderDesc.BackgroundColor = FUIStyle::GetDefault().Colors.PanelBackground;
    BorderDesc.Content         = Content;

    TSharedPtr<FOverlay> Sized = FOverlay::Create();
    Sized->AddSlot(FSpacer::CreateVertical(Height));
    Sized->AddSlot(FBorder::Create(BorderDesc));

    return Sized;
}

static TSharedPtr<FVisualElement> MakeLogView(const FPlaygroundFonts& Fonts)
{
    FLogView::FDesc Desc;
    Desc.Font         = Fonts.Monospace;
    Desc.MaxLineCount = 500;
    Desc.bAutoScroll  = true;

    GLogView = FLogView::Create(Desc);

    for (int32 Index = 0; Index < 6; ++Index)
    {
        LogNextSample();
    }

    return MakeSizedBorder(GLogView, LOG_VIEW_HEIGHT);
}

static TSharedPtr<FVisualElement> MakeLogControls(const FPlaygroundFonts& Fonts)
{
    FTextBlock::FDesc ReadoutDesc;
    ReadoutDesc.Font            = Fonts.Monospace;
    ReadoutDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    GLogReadout = FTextBlock::Create(ReadoutDesc);
    RefreshLogReadout();

    FButton::FDesc LogDesc;
    LogDesc.SetText("Log a line").SetFont(Fonts.Body);
    LogDesc.SetOnClicked(FOnClicked::CreateLambda([]() { LogNextSample(); }));

    FButton::FDesc BurstDesc;
    BurstDesc.SetText("Log twenty").SetFont(Fonts.Body);
    BurstDesc.SetOnClicked(FOnClicked::CreateLambda([]()
    {
        for (int32 Index = 0; Index < 20; ++Index)
        {
            LogNextSample();
        }
    }));

    FButton::FDesc ClearDesc;
    ClearDesc.SetText("Clear").SetFont(Fonts.Body);
    ClearDesc.SetOnClicked(FOnClicked::CreateLambda([]()
    {
        GLogView->Clear();
        RefreshLogReadout();
    }));

    FButton::FDesc CopyDesc;
    CopyDesc.SetText("Select all and copy").SetFont(Fonts.Body);
    CopyDesc.SetOnClicked(FOnClicked::CreateLambda([]()
    {
        GLogView->SelectAll();
        GLogView->CopyToClipboard();
    }));

    FComboBox::FDesc SeverityDesc;
    SeverityDesc.SetOptions({ "Info and up", "Warnings and up", "Errors only" }).SetFont(Fonts.Body);
    SeverityDesc.SelectedIndex      = 0;
    SeverityDesc.OnSelectionChanged = FOnComboSelectionChanged::CreateLambda([](int32 SelectedIndex)
    {
        static const ELogSeverity Severities[] = { ELogSeverity::Info, ELogSeverity::Warning, ELogSeverity::Error };

        GLogView->SetMinimumSeverity(Severities[Math::Clamp(SelectedIndex, 0, 2)]);
        RefreshLogReadout();
    });

    FCheckBox::FDesc AutoScrollDesc;
    AutoScrollDesc.SetText("Follow the tail").SetFont(Fonts.Body);
    AutoScrollDesc.InitialState  = ECheckBoxState::Checked;
    AutoScrollDesc.OnStateChanged = FOnCheckStateChanged::CreateLambda([](ECheckBoxState NewState)
    {
        GLogView->SetAutoScroll(NewState == ECheckBoxState::Checked);
        RefreshLogReadout();
    });

    FCheckBox::FDesc FilterDesc;
    FilterDesc.SetText("Hide non-matches").SetFont(Fonts.Body);
    FilterDesc.OnStateChanged = FOnCheckStateChanged::CreateLambda([](ECheckBoxState NewState)
    {
        GFilterToMatches = NewState == ECheckBoxState::Checked;

        GLogView->SetSearchText(GSearchText, GFilterToMatches);
        RefreshLogReadout();
    });

    FEditableText::FDesc SearchDesc;
    SearchDesc.HintText = "Search the log";
    SearchDesc.Font     = Fonts.Body;

    TSharedPtr<FEditableText> SearchField = FEditableText::Create(SearchDesc);
    SearchField->GetOnTextChanged() = FOnTextChangedDelegate::CreateLambda([](const String& NewText)
    {
        GSearchText = NewText;

        GLogView->SetSearchText(GSearchText, GFilterToMatches);
        RefreshLogReadout();
    });

    TSharedPtr<FHorizontalBox> ButtonRow = FHorizontalBox::Create();
    ButtonRow->AddSlot(FButton::Create(LogDesc)).SetPadding(FMargin(0, 0, 12, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
    ButtonRow->AddSlot(FButton::Create(BurstDesc)).SetPadding(FMargin(0, 0, 12, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
    ButtonRow->AddSlot(FButton::Create(ClearDesc)).SetPadding(FMargin(0, 0, 12, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
    ButtonRow->AddSlot(FButton::Create(CopyDesc)).SetVerticalAlignment(EVerticalAlignment::Center);
    ButtonRow->AddSlot(FSpacer::Create(IntVector2(0, 0))).SetFillCoefficient(1.0f);

    TSharedPtr<FHorizontalBox> FilterRow = FHorizontalBox::Create();
    FilterRow->AddSlot(FComboBox::Create(SeverityDesc)).SetPadding(FMargin(0, 0, 16, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
    FilterRow->AddSlot(SearchField).SetPadding(FMargin(0, 0, 16, 0)).SetVerticalAlignment(EVerticalAlignment::Center).SetFillCoefficient(1.0f);
    FilterRow->AddSlot(FCheckBox::Create(FilterDesc)).SetPadding(FMargin(0, 0, 16, 0)).SetVerticalAlignment(EVerticalAlignment::Center);
    FilterRow->AddSlot(FCheckBox::Create(AutoScrollDesc)).SetVerticalAlignment(EVerticalAlignment::Center);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(ButtonRow);
    Column->AddSlot(FilterRow).SetPadding(FMargin(0, 12, 0, 0));
    Column->AddSlot(GLogReadout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

static TSharedPtr<FVisualElement> MakeRichTextRow(const FPlaygroundFonts& Fonts)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FTextBlock::FDesc ReadoutDesc;
    ReadoutDesc.Text            = "nothing selected";
    ReadoutDesc.Font            = Fonts.Monospace;
    ReadoutDesc.ColorAndOpacity = Style.Colors.TextDisabled;

    GSelectionReadout = FTextBlock::Create(ReadoutDesc);

    FRichTextBlock::FDesc Desc;
    Desc.Margin        = FMargin(12, 10);
    Desc.bAutoWrapText = true;

    Desc.Runs.Add(FTextRun("A paragraph made of runs. ", Fonts.Body.Get(), Style.Colors.Text));
    Desc.Runs.Add(FTextRun("This part is accented", Fonts.Body.Get(), Style.Colors.Accent));
    Desc.Runs.Add(FTextRun(" and this part is dimmed, ", Fonts.Body.Get(), Style.Colors.TextDisabled));
    Desc.Runs.Add(FTextRun("but a selection crosses all three without noticing where one ends. ", Fonts.Body.Get(), Style.Colors.Text));
    Desc.Runs.Add(FTextRun("Drag across it, then press Command-C to copy what you took.", Fonts.Body.Get(), Style.Colors.Text));

    Desc.OnSelectionChanged = FOnSelectionChanged::CreateLambda([](const String& SelectedText)
    {
        GSelectionReadout->SetText(SelectedText.IsEmpty()
            ? String("nothing selected")
            : String::Printf("%d character(s): \"%s\"", SelectedText.Length(), SelectedText.Data()));
    });

    GRichText = FRichTextBlock::Create(Desc);

    FEditableText::FDesc SearchDesc;
    SearchDesc.HintText = "Highlight in the paragraph";
    SearchDesc.Font     = Fonts.Body;

    TSharedPtr<FEditableText> SearchField = FEditableText::Create(SearchDesc);
    SearchField->GetOnTextChanged() = FOnTextChangedDelegate::CreateLambda([](const String& NewText)
    {
        GRichText->SetSearchText(NewText);
    });

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(MakeSizedBorder(GRichText, 100));
    Column->AddSlot(SearchField).SetPadding(FMargin(0, 12, 0, 0));
    Column->AddSlot(GSelectionReadout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    return Column;
}

FPlaygroundScene CreateOutputLogScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Output log";
        Desc.Description = "An IOutputDevice with a ring buffer behind it. Severity colours the line, the filters decide which ones are built into the text at all.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeLogView(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Filters and the tail";
        Desc.Description = "Raising the severity floor or filtering to matches hides lines without dropping them. Scroll away from the bottom and the view stops following.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeLogControls(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Rich text";
        Desc.Description = "Selection and search over runs of different colours, which is what the log is built out of.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeRichTextRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Output Log", MakeSceneColumn("Output Log", Fonts, Panels));
}

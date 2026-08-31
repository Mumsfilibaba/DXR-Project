#include "Engine/EngineUI/EditorUI/EditorFooterPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Math/Math.h"
#include "Application/Console/ConsoleCommandLine.h"
#include "Application/Elements/Border.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/EditableText.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Input/Keys.h"

/** @brief The space between the bottom of the candidate list and the top of the footer, in pixels. */
constexpr int32 CANDIDATE_LIST_OFFSET = 4;

/** @brief How many rows the candidate list shows before the rest of them have to be scrolled to. */
constexpr int32 CANDIDATE_LIST_MAX_VISIBLE_ROWS = 12;

/** @brief How wide the candidate list is allowed to grow, in pixels, past which a row is clipped. */
constexpr int32 CANDIDATE_LIST_MAX_WIDTH = 640;

/** @brief The space between the rows and the edge of the list, in pixels. */
constexpr int32 CANDIDATE_LIST_PADDING = 2;

/** @brief The space above and below the glyphs of a candidate row, in pixels. */
constexpr int32 CANDIDATE_ROW_PADDING = 2;

/** @brief The inset of a candidate row from either edge of the list, in pixels. */
constexpr int32 CANDIDATE_ROW_INSET = 6;

/** @brief The space between the name column and the help text of a candidate row, in pixels. */
constexpr int32 CANDIDATE_COLUMN_SPACING = 10;

/** @brief What a row falls back to without a face, so the list does not collapse onto one line. */
constexpr int32 CANDIDATE_FALLBACK_ROW_HEIGHT = 20;

class FEditorFooterCandidateAnchor final : public FCompoundElement
{
public:

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override
    {
        return IntVector2(0, 0);
    }

    virtual void OnArrange(const FRectangle& AllottedBounds) override
    {
        if (!Content || !Content->IsVisible())
        {
            return;
        }

        const IntVector2 ContentSize = Content->GetCachedDesiredSize();
        const int32      RoomAbove   = Math::Max(0, AllottedBounds.Position.Y - CANDIDATE_LIST_OFFSET);

        FRectangle ContentBounds;
        ContentBounds.Width      = Math::Min(ContentSize.X, CANDIDATE_LIST_MAX_WIDTH);
        ContentBounds.Height     = Math::Min(ContentSize.Y, RoomAbove);
        ContentBounds.Position.X = AllottedBounds.Position.X;
        ContentBounds.Position.Y = AllottedBounds.Position.Y - CANDIDATE_LIST_OFFSET - ContentBounds.Height;

        Content->Tick(ContentBounds);
    }

    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override
    {
        if (!Content || !Content->IsVisible())
        {
            return LayerId;
        }

        return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
    }

    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override
    {
        if (Content && Content->IsVisible() && Content->GetContentRectangle().EncapsulatesPoint(ClientPosition))
        {
            Content->FindChildrenContainingPoint(ClientPosition, OutChildElements);
        }
    }
};

static int32 MeasureTextWidth(const TSharedPtr<IFontFace>& Font, const String& Text)
{
    return Font ? Font->MeasureWidth(StringView(Text.Data(), Text.Length())) : 0;
}

FEditorFooterPanel::FEditorFooterPanel()
    : CommandLine(MakeUniquePtr<FConsoleCommandLine>())
    , Field(nullptr)
    , StatusLabel(nullptr)
    , CandidateBackground(nullptr)
    , CandidateScrollBox(nullptr)
    , CandidateRows(nullptr)
    , Element(nullptr)
    , bIsSyncingField(false)
{
}

FEditorFooterPanel::~FEditorFooterPanel()
{
}

bool FEditorFooterPanel::Initialize()
{
    const FUIStyle& Style = FEditorStyle::GetStyle();

    FEditableText::FDesc FieldDesc;
    FieldDesc.HintText        = "Enter a console command";
    FieldDesc.Font            = FEditorStyle::GetFonts().Monospace;
    FieldDesc.ForegroundColor = Style.Colors.Text;
    FieldDesc.HintColor       = Style.Colors.TextDisabled;
    FieldDesc.TextCursorColor = Style.Colors.Text;
    FieldDesc.SelectionColor  = Style.Colors.TextSelectionBackground;
    FieldDesc.Padding         = FMargin(6, 3, 6, 3);

    Field = FEditableText::Create(FieldDesc);
    if (!Field)
    {
        return false;
    }

    Field->GetOnTextChanged().BindRaw(this, &FEditorFooterPanel::OnFieldTextChanged);
    Field->GetOnKeyDownInterceptor().BindRaw(this, &FEditorFooterPanel::OnFieldKeyDown);

    FTextBlock::FDesc StatusDesc;
    StatusDesc.Font            = FEditorStyle::GetFonts().Body;
    StatusDesc.ColorAndOpacity = Style.Colors.TextDisabled;
    StatusDesc.Margin          = FMargin(8, 0, 8, 0);

    StatusLabel = FTextBlock::Create(StatusDesc);
    if (!StatusLabel)
    {
        return false;
    }

    CandidateRows = FVerticalBox::Create();
    if (!CandidateRows)
    {
        return false;
    }

    CandidateScrollBox = FScrollBox::Create();
    if (!CandidateScrollBox)
    {
        return false;
    }

    CandidateScrollBox->SetContent(CandidateRows);

    FBorder::FDesc CandidateBackgroundDesc;
    CandidateBackgroundDesc.BackgroundColor = Style.Colors.WindowBackground;
    CandidateBackgroundDesc.BorderColor     = Style.Colors.Border;
    CandidateBackgroundDesc.BorderThickness = Style.Metrics.BorderThickness;
    CandidateBackgroundDesc.Padding         = FMargin(CANDIDATE_LIST_PADDING);
    CandidateBackgroundDesc.Content         = CandidateScrollBox;

    CandidateBackground = FBorder::Create(CandidateBackgroundDesc);
    if (!CandidateBackground)
    {
        return false;
    }

    CandidateBackground->SetVisibility(EVisibility::Hidden);

    TSharedPtr<FEditorFooterCandidateAnchor> CandidateAnchor = MakeSharedPtr<FEditorFooterCandidateAnchor>();
    if (!CandidateAnchor)
    {
        return false;
    }

    CandidateAnchor->SetContent(CandidateBackground);

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(CandidateAnchor);
    Row->AddSlot(Field).SetFillCoefficient(1.0f).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(StatusLabel).SetVerticalAlignment(EVerticalAlignment::Center);

    FBorder::FDesc BorderDesc;
    BorderDesc.BackgroundColor = Style.Colors.PanelBackground;
    BorderDesc.Padding         = FMargin(4, 2, 4, 2);
    BorderDesc.MinHeight       = FEditorStyle::FooterHeight;
    BorderDesc.Content         = Row;

    Element = FBorder::Create(BorderDesc);
    return Element != nullptr;
}

EKeyInterceptResult FEditorFooterPanel::OnFieldKeyDown(const FKeyEvent& KeyEvent)
{
    const FKey Key = KeyEvent.GetKey();

    if (Key == Keys::Up)
    {
        CommandLine->MoveSelectionUp();
        SyncFieldFromCommandLine();
        RebuildCandidateList();

        return EKeyInterceptResult::Handled;
    }

    if (Key == Keys::Down)
    {
        CommandLine->MoveSelectionDown();
        SyncFieldFromCommandLine();
        RebuildCandidateList();

        return EKeyInterceptResult::Handled;
    }

    if (Key == Keys::Tab)
    {
        if (CommandLine->AcceptCompletion())
        {
            SyncFieldFromCommandLine();
            RebuildCandidateList();
        }

        return EKeyInterceptResult::Handled;
    }

    if (Key == Keys::Enter)
    {
        CommandLine->Submit(*FOutputDeviceLogger::Get());
        SyncFieldFromCommandLine();
        RebuildCandidateList();

        return EKeyInterceptResult::Handled;
    }

    if (Key == Keys::Escape)
    {
        if (!CommandLine->HasCandidates())
        {
            return EKeyInterceptResult::NotHandled;
        }

        CommandLine->InvalidateCandidates();
        RebuildCandidateList();

        return EKeyInterceptResult::Handled;
    }

    return EKeyInterceptResult::NotHandled;
}

void FEditorFooterPanel::OnFieldTextChanged(const String& NewText)
{
    if (bIsSyncingField)
    {
        return;
    }

    CommandLine->SetText(NewText);
    CommandLine->SetTextCursorPosition(Field->GetTextCursorPosition());
    CommandLine->RefreshCandidates();

    RebuildCandidateList();
}

void FEditorFooterPanel::SyncFieldFromCommandLine()
{
    bIsSyncingField = true;

    Field->SetTextSilently(CommandLine->GetText());
    Field->SetTextCursorPosition(CommandLine->GetTextCursorPosition());

    bIsSyncingField = false;
}

void FEditorFooterPanel::RebuildCandidateList()
{
    CandidateRows->ClearSlots();

    if (!CommandLine->HasCandidates())
    {
        CandidateBackground->SetVisibility(EVisibility::Hidden);
        return;
    }

    const int32 NumCandidates   = CommandLine->GetCandidates().Size();
    const int32 NameColumnWidth = GetCandidateNameColumnWidth();

    for (int32 Index = 0; Index < NumCandidates; ++Index)
    {
        AddCandidateRow(Index, NameColumnWidth);
    }

    const int32 RowHeight   = GetCandidateRowHeight();
    const int32 VisibleRows = Math::Min(NumCandidates, CANDIDATE_LIST_MAX_VISIBLE_ROWS);

    CandidateBackground->SetMinHeight((VisibleRows * RowHeight) + CandidateBackground->GetPadding().GetTotalVertical());
    CandidateBackground->SetVisibility(EVisibility::Visible);

    const int32 SelectedIndex = CommandLine->GetSelectedCandidateIndex();
    if (CommandLine->ConsumeSelectionChanged() && SelectedIndex >= 0)
    {
        FRectangle RowBounds;
        RowBounds.Position.Y = SelectedIndex * RowHeight;
        RowBounds.Height     = RowHeight;
        CandidateScrollBox->ScrollIntoView(RowBounds);
    }
}

void FEditorFooterPanel::AddCandidateRow(int32 CandidateIndex, int32 NameColumnWidth)
{
    const TPair<IConsoleObject*, String>& Candidate = CommandLine->GetCandidates()[CandidateIndex];
    const TSharedPtr<IFontFace>&          Font      = FEditorStyle::GetFonts().Monospace;
    const FUIStyle&                       Style     = FEditorStyle::GetStyle();

    const bool bIsSelected = CandidateIndex == CommandLine->GetSelectedCandidateIndex();

    FTextBlock::FDesc NameDesc;
    NameDesc.Text            = Candidate.Second;
    NameDesc.Font            = Font;
    NameDesc.ColorAndOpacity = Style.Colors.Text;
    NameDesc.Margin          = FMargin(0, 0, Math::Max(0, NameColumnWidth - MeasureTextWidth(Font, Candidate.Second)), 0);

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(FTextBlock::Create(NameDesc)).SetVerticalAlignment(EVerticalAlignment::Center);

    const CHAR* HelpString = Candidate.First->GetHelpString();
    if (HelpString && HelpString[0] != 0)
    {
        FTextBlock::FDesc HelpDesc;
        HelpDesc.Text            = HelpString;
        HelpDesc.Font            = Font;
        HelpDesc.ColorAndOpacity = Style.Colors.TextDisabled;

        Row->AddSlot(FTextBlock::Create(HelpDesc)).SetVerticalAlignment(EVerticalAlignment::Center);
    }

    FBorder::FDesc RowDesc;
    RowDesc.BackgroundColor = bIsSelected ? FEditorStyle::GetSelectionColor() : FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);
    RowDesc.Padding         = FMargin(CANDIDATE_ROW_INSET, 0);
    RowDesc.MinHeight       = GetCandidateRowHeight();
    RowDesc.Content         = Row;

    CandidateRows->AddSlot(FBorder::Create(RowDesc));
}

int32 FEditorFooterPanel::GetCandidateRowHeight() const
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Monospace;
    return Font ? Font->GetLineHeight() + (CANDIDATE_ROW_PADDING * 2) : CANDIDATE_FALLBACK_ROW_HEIGHT;
}

int32 FEditorFooterPanel::GetCandidateNameColumnWidth() const
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Monospace;

    int32 Width = 0;
    for (const TPair<IConsoleObject*, String>& Candidate : CommandLine->GetCandidates())
    {
        Width = Math::Max(Width, MeasureTextWidth(Font, Candidate.Second));
    }

    return Width + CANDIDATE_COLUMN_SPACING;
}

void FEditorFooterPanel::Refresh()
{
    if (CommandLine->HasCandidates() && !Field->HasKeyboardFocus())
    {
        CommandLine->InvalidateCandidates();
        RebuildCandidateList();
    }

    const FFrameProfiler& Profiler = FFrameProfiler::Get();

    const float FrameTimeMs = Profiler.GetCPUFrameTime().GetAverage();
    const float FramesPerSecond = FrameTimeMs > 0.0f ? (1000.0f / FrameTimeMs) : 0.0f;

    StatusLabel->SetText(String::Printf("%.1f FPS  %.2f ms", FramesPerSecond, FrameTimeMs));
}

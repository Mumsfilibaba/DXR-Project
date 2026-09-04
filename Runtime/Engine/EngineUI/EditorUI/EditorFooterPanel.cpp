#include "Engine/EngineUI/EditorUI/EditorFooterPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Math/Math.h"
#include "Application/Console/ConsoleCommandLine.h"
#include "Application/Elements/Border.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/EditableText.h"
#include "Application/Elements/RichTextBlock.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/Spacer.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Input/Keys.h"
#include "Application/Menus/MenuStack.h"
#include "Application/Menus/ToolTipService.h"

/** @brief The space between either end of the footer strip and what it holds, in pixels. */
constexpr int32 FOOTER_INSET = 16;

/** @brief How wide the command field is, in pixels, which is fixed rather than a share of the strip. */
constexpr int32 FOOTER_FIELD_WIDTH = 512;

/** @brief The space between the command field's left or right edge and the line of text, in pixels. */
constexpr int32 FOOTER_FIELD_PADDING_X = 12;

/** @brief The space between the command field's top or bottom edge and the line of text, in pixels. */
constexpr int32 FOOTER_FIELD_PADDING_Y = 6;

/** @brief How many rows the candidate list shows before the rest of them have to be scrolled to. */
constexpr int32 CANDIDATE_LIST_MAX_VISIBLE_ROWS = 20;

/** @brief The space between the scrolled rows and either side of the list, in pixels. */
constexpr int32 CANDIDATE_LIST_PADDING_X = 2;

/** @brief The space between the rows and the top or bottom of the list, in pixels. */
constexpr int32 CANDIDATE_LIST_PADDING_Y = 4;

/** @brief The width of the stroke around the candidate list, in pixels. */
constexpr float CANDIDATE_LIST_BORDER_THICKNESS = 2.0f;

/** @brief The height of one candidate row, in pixels, which is fixed rather than drawn from the face. */
constexpr int32 CANDIDATE_ROW_HEIGHT = 20;

/** @brief The inset of a candidate row from either edge of the list, in pixels, carried by the rows rather than the list so the scroll bar keeps the list's edge. */
constexpr int32 CANDIDATE_ROW_INSET = 14;

/** @brief How far the match highlight runs past the glyphs on either side, in pixels. */
constexpr int32 CANDIDATE_HIGHLIGHT_BLEED = 1;

/** @brief The space between a candidate's name and the rest of its tip, in pixels. */
constexpr int32 CANDIDATE_TOOLTIP_SPACING = 4;

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
        const int32      RoomAbove   = Math::Max(0, AllottedBounds.Position.Y);
        const int32      FieldWidth  = WidthSource ? WidthSource->GetContentRectangle().Width : 0;

        FRectangle ContentBounds;
        ContentBounds.Width      = FieldWidth > 0 ? FieldWidth : ContentSize.X;
        ContentBounds.Height     = Math::Min(ContentSize.Y, RoomAbove);
        ContentBounds.Position.X = AllottedBounds.Position.X;
        ContentBounds.Position.Y = AllottedBounds.Position.Y - ContentBounds.Height;

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

    void SetWidthSource(const TSharedPtr<FVisualElement>& InWidthSource)
    {
        WidthSource = InWidthSource;
    }

private:
    TSharedPtr<FVisualElement> WidthSource;
};

DECLARE_DELEGATE(FOnCandidateRowHovered, int32 /*RowIndex*/);
DECLARE_DELEGATE(FOnCandidateRowClicked, int32 /*RowIndex*/);

class FEditorFooterCandidateList final : public FCompoundElement
{
public:
    static constexpr int32 InvalidRowIndex = -1;

    // FVisualElement Interface
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override
    {
        OnRowHovered.ExecuteIfBound(FindRowAtPoint(CursorEvent.GetClientPosition()));
        return FCompoundElement::OnMouseMove(CursorEvent);
    }

    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override
    {
        OnRowHovered.ExecuteIfBound(InvalidRowIndex);
        return FCompoundElement::OnMouseLeft(CursorEvent);
    }

    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override
    {
        if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
        {
            return FCompoundElement::OnMouseButtonDown(CursorEvent);
        }

        const int32 RowIndex = FindRowAtPoint(CursorEvent.GetClientPosition());
        if (RowIndex == InvalidRowIndex)
        {
            return FCompoundElement::OnMouseButtonDown(CursorEvent);
        }

        OnRowClicked.ExecuteIfBound(RowIndex);
        return FEventResponse::Handled();
    }

    void SetRows(const TSharedPtr<FVerticalBox>& InRows)
    {
        Rows = InRows;
    }

    FOnCandidateRowHovered OnRowHovered;
    FOnCandidateRowClicked OnRowClicked;

private:
    int32 FindRowAtPoint(const IntVector2& ClientPosition) const
    {
        if (!Rows || !GetContentRectangle().EncapsulatesPoint(ClientPosition))
        {
            return InvalidRowIndex;
        }

        for (int32 Index = 0; Index < Rows->GetNumSlots(); ++Index)
        {
            const TSharedPtr<FVisualElement>& Row = Rows->GetSlot(Index).Element;
            if (Row && Row->GetContentRectangle().EncapsulatesPoint(ClientPosition))
            {
                return Index;
            }
        }

        return InvalidRowIndex;
    }

    TSharedPtr<FVerticalBox> Rows;
};

static const CHAR* GetConsoleVariableTypeName(IConsoleVariable* Variable)
{
    if (Variable->IsVariableBool())
    {
        return "Bool";
    }

    if (Variable->IsVariableInt())
    {
        return "Int";
    }

    if (Variable->IsVariableFloat())
    {
        return "Float";
    }

    if (Variable->IsVariableString())
    {
        return "String";
    }

    return "Variable";
}

FEditorFooterPanel::FEditorFooterPanel()
    : CommandLine(MakeUniquePtr<FConsoleCommandLine>())
    , Field(nullptr)
    , FieldFrame(nullptr)
    , StatusLabel(nullptr)
    , CandidateBackground(nullptr)
    , CandidateList(nullptr)
    , CandidateScrollBox(nullptr)
    , CandidateRows(nullptr)
    , HoveredCandidateRow(nullptr)
    , Element(nullptr)
    , bIsSyncingField(false)
{
}

FEditorFooterPanel::~FEditorFooterPanel()
{
    ClearHoveredCandidate();
}

bool FEditorFooterPanel::Initialize()
{
    const FUIStyle&        Style      = FEditorStyle::GetStyle();
    const FInputFrameStyle FrameStyle = FEditorStyle::GetConsoleInputFrameStyle();

    FEditableText::FDesc FieldDesc;
    FieldDesc.HintText        = "Console Input";
    FieldDesc.Font            = FEditorStyle::GetFonts().Monospace;
    FieldDesc.ForegroundColor = FrameStyle.Text;
    FieldDesc.HintColor       = FrameStyle.HintNormal;
    FieldDesc.TextCursorColor = FrameStyle.Text;
    FieldDesc.SelectionColor  = FrameStyle.Selection;
    FieldDesc.Padding         = FMargin(FOOTER_FIELD_PADDING_X, FOOTER_FIELD_PADDING_Y);

    Field = FEditableText::Create(FieldDesc);
    if (!Field)
    {
        return false;
    }

    Field->GetOnTextChanged().BindRaw(this, &FEditorFooterPanel::OnFieldTextChanged);
    Field->GetOnKeyDownInterceptor().BindRaw(this, &FEditorFooterPanel::OnFieldKeyDown);

    FBorder::FDesc FieldFrameDesc;
    FieldFrameDesc.BackgroundColor = FrameStyle.Fill;
    FieldFrameDesc.BorderColor     = FrameStyle.BorderNormal;
    FieldFrameDesc.BorderThickness = FrameStyle.BorderThickness;
    FieldFrameDesc.CornerRadius    = FCornerRadii(FrameStyle.CornerRadius);
    FieldFrameDesc.MinWidth        = FOOTER_FIELD_WIDTH;
    FieldFrameDesc.Content         = Field;

    FieldFrame = FBorder::Create(FieldFrameDesc);
    if (!FieldFrame)
    {
        return false;
    }

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

    CandidateList = MakeSharedPtr<FEditorFooterCandidateList>();
    if (!CandidateList)
    {
        return false;
    }

    CandidateList->SetContent(CandidateScrollBox);
    CandidateList->SetRows(CandidateRows);
    CandidateList->OnRowHovered.BindRaw(this, &FEditorFooterPanel::OnCandidateRowHovered);
    CandidateList->OnRowClicked.BindRaw(this, &FEditorFooterPanel::OnCandidateRowClicked);

    FBorder::FDesc CandidateBackgroundDesc;
    CandidateBackgroundDesc.BackgroundColor = FEditorStyle::GetCandidateListColor();
    CandidateBackgroundDesc.BorderColor     = FEditorStyle::GetCandidateListBorderColor();
    CandidateBackgroundDesc.BorderThickness = CANDIDATE_LIST_BORDER_THICKNESS;
    CandidateBackgroundDesc.Padding         = FMargin(CANDIDATE_LIST_PADDING_X, CANDIDATE_LIST_PADDING_Y);
    CandidateBackgroundDesc.Content         = CandidateList;

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
    CandidateAnchor->SetWidthSource(FieldFrame);

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(CandidateAnchor);
    Row->AddSlot(FieldFrame)
        .SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(StatusLabel)
        .SetFillCoefficient(1.0f)
        .SetHorizontalAlignment(EHorizontalAlignment::Right)
        .SetVerticalAlignment(EVerticalAlignment::Center);

    FBorder::FDesc BorderDesc;
    BorderDesc.BackgroundColor = FEditorStyle::GetFooterColor();
    BorderDesc.Padding         = FMargin(FOOTER_INSET, 0, FOOTER_INSET, 0);
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
    ClearHoveredCandidate();

    CandidateRows->ClearSlots();
    CandidateRowVisuals.Clear();

    if (!CommandLine->HasCandidates())
    {
        CandidateBackground->SetVisibility(EVisibility::Hidden);
        return;
    }

    const int32 NumCandidates = CommandLine->GetCandidates().Size();

    for (int32 Index = 0; Index < NumCandidates; ++Index)
    {
        AddCandidateRow(Index);
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

void FEditorFooterPanel::AddCandidateRow(int32 CandidateIndex)
{
    const TPair<IConsoleObject*, String>& Candidate = CommandLine->GetCandidates()[CandidateIndex];

    const bool bIsSelected = CandidateIndex == CommandLine->GetSelectedCandidateIndex();

    const FFloatColor NameColor = bIsSelected ? FFloatColor::White : FEditorStyle::GetCandidateTextColor();

    FCandidateRow RowVisuals;

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    AddCandidateNameRuns(Row, Candidate.Second, NameColor, RowVisuals.NameRuns);

    FBorder::FDesc RowDesc;
    RowDesc.BackgroundColor = bIsSelected ? FEditorStyle::GetCandidateSelectionColor() : FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);
    RowDesc.Padding         = FMargin(CANDIDATE_ROW_INSET, 0);
    RowDesc.MinHeight       = GetCandidateRowHeight();
    RowDesc.Content         = Row;

    RowVisuals.Frame = FBorder::Create(RowDesc);

    CandidateRows->AddSlot(RowVisuals.Frame);
    CandidateRowVisuals.Add(RowVisuals);
}

void FEditorFooterPanel::AddCandidateNameRuns(const TSharedPtr<FHorizontalBox>& Row, const String& Name, const FFloatColor& TextColor, TArray<TSharedPtr<FTextBlock>>& OutNameRuns)
{
    const TSharedPtr<IFontFace>& Font   = FEditorStyle::GetFonts().Monospace;
    const String                 Filter = GetCandidateFilterText();

    const int32 MatchLength = Filter.Length();
    const int32 MatchStart  = MatchLength > 0 ? StringView(Name.Data(), Name.Length()).Find(Filter.Data(), EStringCaseType::NoCase) : -1;
    const bool  bHasMatch   = MatchStart >= 0 && (MatchStart + MatchLength) <= Name.Length();

    const auto MakeRun = [&](const String& RunText, const FFloatColor& RunColor)
    {
        FTextBlock::FDesc RunDesc;
        RunDesc.Text            = RunText;
        RunDesc.Font            = Font;
        RunDesc.ColorAndOpacity = RunColor;

        return FTextBlock::Create(RunDesc);
    };

    const auto AddPlainRun = [&](const String& RunText)
    {
        TSharedPtr<FTextBlock> RunBlock = MakeRun(RunText, TextColor);

        Row->AddSlot(RunBlock).SetVerticalAlignment(EVerticalAlignment::Center);
        OutNameRuns.Add(RunBlock);
    };

    if (!bHasMatch)
    {
        AddPlainRun(Name);
    }
    else
    {
        const int32 SuffixStart = MatchStart + MatchLength;

        if (MatchStart > 0)
        {
            AddPlainRun(Name.SubString(0, MatchStart));
        }

        FBorder::FDesc HighlightDesc;
        HighlightDesc.BackgroundColor = FEditorStyle::GetCandidateHighlightColor();
        HighlightDesc.Padding         = FMargin(CANDIDATE_HIGHLIGHT_BLEED, 0);
        HighlightDesc.Content         = MakeRun(Name.SubString(MatchStart, MatchLength), FFloatColor::Black);

        Row->AddSlot(FBorder::Create(HighlightDesc)).SetVerticalAlignment(EVerticalAlignment::Center);

        if (SuffixStart < Name.Length())
        {
            AddPlainRun(Name.SubString(SuffixStart, Name.Length() - SuffixStart));
        }
    }
}

void FEditorFooterPanel::ApplyCandidateSelection()
{
    const int32 SelectedIndex = CommandLine->GetSelectedCandidateIndex();

    for (int32 Index = 0; Index < CandidateRowVisuals.Size(); ++Index)
    {
        const FCandidateRow& RowVisuals  = CandidateRowVisuals[Index];
        const bool           bIsSelected = Index == SelectedIndex;

        RowVisuals.Frame->SetBackgroundColor(bIsSelected ? FEditorStyle::GetCandidateSelectionColor() : FFloatColor(0.0f, 0.0f, 0.0f, 0.0f));

        const FFloatColor NameColor = bIsSelected ? FFloatColor::White : FEditorStyle::GetCandidateTextColor();
        for (const TSharedPtr<FTextBlock>& NameRun : RowVisuals.NameRuns)
        {
            NameRun->SetColorAndOpacity(NameColor);
        }
    }
}

void FEditorFooterPanel::OnCandidateRowHovered(int32 RowIndex)
{
    const bool bIsValidRow = RowIndex >= 0 && RowIndex < CandidateRows->GetNumSlots() && RowIndex < CommandLine->GetCandidates().Size();

    TSharedPtr<FVisualElement> Row = bIsValidRow ? CandidateRows->GetSlot(RowIndex).Element : nullptr;
    if (Row == HoveredCandidateRow)
    {
        return;
    }

    ClearHoveredCandidate();

    if (!Row)
    {
        return;
    }

    HoveredCandidateRow = Row;

    CommandLine->SetSelectedCandidateIndex(RowIndex);
    CommandLine->ConsumeSelectionChanged();

    ApplyCandidateSelection();

    FRectangle       AnchorBounds = FMenuStack::GetScreenBounds(Row);
    const FRectangle ListBounds   = FMenuStack::GetScreenBounds(CandidateBackground);

    AnchorBounds.Position.X = ListBounds.Position.X;
    AnchorBounds.Width      = ListBounds.Width;

    FToolTipService::Get().RequestToolTip(Row, MakeCandidateToolTip(RowIndex), EToolTipPlacement::RightOfAnchor, 0.0f, AnchorBounds);
}

void FEditorFooterPanel::OnCandidateRowClicked(int32 RowIndex)
{
    CommandLine->SetSelectedCandidateIndex(RowIndex);

    if (CommandLine->AcceptCompletion())
    {
        SyncFieldFromCommandLine();
        RebuildCandidateList();
    }
}

void FEditorFooterPanel::ClearHoveredCandidate()
{
    if (!HoveredCandidateRow)
    {
        return;
    }

    FToolTipService::Get().CancelToolTip(HoveredCandidateRow);
    HoveredCandidateRow = nullptr;
}

TSharedPtr<FVisualElement> FEditorFooterPanel::MakeCandidateToolTip(int32 CandidateIndex) const
{
    const TPair<IConsoleObject*, String>& Candidate = CommandLine->GetCandidates()[CandidateIndex];
    const TSharedPtr<IFontFace>&          Font      = FEditorStyle::GetFonts().Monospace;

    FRichTextBlock::FDesc NameDesc;
    NameDesc.Runs.Add(FTextRun(Candidate.Second, Font.Get(), FFloatColor::White));
    NameDesc.WrapWidth     = FEditorStyle::ToolTipTextWidth;
    NameDesc.bIsSelectable = false;

    TSharedPtr<FVerticalBox> Content = FVerticalBox::Create();
    Content->AddSlot(FRichTextBlock::Create(NameDesc));

    const FFloatColor DetailColor = FEditorStyle::GetCandidateTextColor();

    TArray<FTextRun> DetailRuns;
    if (IConsoleVariable* Variable = Candidate.First->AsVariable())
    {
        const EConsoleVariableFlags SetByFlags = static_cast<EConsoleVariableFlags>(Variable->GetFlags() & EConsoleVariableFlags::SetByMask);

        DetailRuns.Add(FTextRun(String("Type: ") + GetConsoleVariableTypeName(Variable) + "\n", Font.Get(), DetailColor));
        DetailRuns.Add(FTextRun(String("Value: ") + Variable->GetString() + "\n", Font.Get(), DetailColor));
        DetailRuns.Add(FTextRun(String("Set By: ") + SetByFlagToString(SetByFlags), Font.Get(), DetailColor));
    }
    else if (Candidate.First->AsCommand())
    {
        DetailRuns.Add(FTextRun("Type: Command", Font.Get(), DetailColor));
    }

    const CHAR* HelpString = Candidate.First->GetHelpString();
    if (HelpString && HelpString[0] != 0)
    {
        const String HelpText = DetailRuns.IsEmpty() ? String(HelpString) : String("\n") + HelpString;
        DetailRuns.Add(FTextRun(HelpText, Font.Get(), FFloatColor::White));
    }

    if (!DetailRuns.IsEmpty())
    {
        FRichTextBlock::FDesc DetailDesc;
        DetailDesc.Runs          = ::Move(DetailRuns);
        DetailDesc.WrapWidth     = FEditorStyle::ToolTipTextWidth;
        DetailDesc.bIsSelectable = false;

        Content->AddSlot(FSpacer::CreateVertical(CANDIDATE_TOOLTIP_SPACING));
        Content->AddSlot(FRichTextBlock::Create(DetailDesc));
    }

    return FEditorStyle::MakeToolTipFrame(Content);
}

int32 FEditorFooterPanel::GetCandidateRowHeight() const
{
    return CANDIDATE_ROW_HEIGHT;
}

String FEditorFooterPanel::GetCandidateFilterText() const
{
    const FConsoleWordRange Word = CommandLine->FindWordRangeAtTextCursor();
    if (Word.IsEmpty())
    {
        return String();
    }

    return CommandLine->GetText().SubString(Word.Position, Word.Length);
}

void FEditorFooterPanel::Refresh()
{
    const bool bHasFocus = Field->HasKeyboardFocus();

    if (CommandLine->HasCandidates() && !bHasFocus)
    {
        CommandLine->InvalidateCandidates();
        RebuildCandidateList();
    }

    const FInputFrameStyle FrameStyle = FEditorStyle::GetConsoleInputFrameStyle();

    FieldFrame->SetBorderColor(bHasFocus ? FrameStyle.BorderFocused : FrameStyle.BorderNormal);
    Field->SetHintColor(bHasFocus ? FrameStyle.HintFocused : FrameStyle.HintNormal);

    const FFrameProfiler& Profiler = FFrameProfiler::Get();

    const float FrameTimeMs = Profiler.GetCPUFrameTime().GetAverage();
    const float FramesPerSecond = FrameTimeMs > 0.0f ? (1000.0f / FrameTimeMs) : 0.0f;

    StatusLabel->SetText(String::Printf("%.1f FPS  %.2f ms", FramesPerSecond, FrameTimeMs));
}

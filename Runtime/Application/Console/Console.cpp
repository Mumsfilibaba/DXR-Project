#include "Application/Console/Console.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Input/Keys.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

constexpr int32 CANDIDATE_COLUMN_SPACING   = 10;
constexpr int32 CANDIDATE_NAME_MIN_WIDTH   = 30;
constexpr int32 CANDIDATE_VALUE_MIN_WIDTH  = 20;
constexpr int32 CONSOLE_HORIZONTAL_PADDING = 10;

constexpr float INPUT_CORNER_RADIUS = 8.0f;

constexpr int32 INPUT_VERTICAL_PADDING = 6;
constexpr int32 INPUT_FRAME_PADDING    = 4;
constexpr int32 CANDIDATE_ROW_PADDING  = 2;
constexpr int32 FALLBACK_BOX_HEIGHT    = 20;

struct FConsoleCandidateText
{
    String Name;
    String Value;
    String Type;
    String SetBy;
    String Help;
};

static int32 MeasureTextWidth(const TSharedPtr<IFontFace>& Font, const String& Text)
{
    return Font ? Font->MeasureWidth(StringView(Text.Data(), Text.Length())) : 0;
}

static FConsoleCandidateText GetCandidateText(const TPair<IConsoleObject*, String>& Candidate)
{
    FConsoleCandidateText RowText;
    RowText.Name = Candidate.Second;

    const CHAR* TypeText = "";

    if (IConsoleVariable* ConsoleVariable = Candidate.First->AsVariable())
    {
        RowText.Value = ConsoleVariable->GetString();

        if (ConsoleVariable->IsVariableBool())
        {
            TypeText = "Bool";
        }
        else if (ConsoleVariable->IsVariableInt())
        {
            TypeText = "Int";
        }
        else if (ConsoleVariable->IsVariableFloat())
        {
            TypeText = "Float";
        }
        else if (ConsoleVariable->IsVariableString())
        {
            TypeText = "String";
        }

        const EConsoleVariableFlags VariableFlags = ConsoleVariable->GetFlags() & EConsoleVariableFlags::SetByMask;
        RowText.SetBy = String::Printf("[%s]", SetByFlagToString(VariableFlags));
    }
    else if (Candidate.First->AsCommand())
    {
        TypeText = "Command";
    }

    RowText.Type = String::Printf("[%s]", TypeText);
    RowText.Help = String::Printf("[Help: %s]", Candidate.First->GetHelpString());
    return RowText;
}

static int32 GetTypeColumnWidth(const TSharedPtr<IFontFace>& Font)
{
    static const CHAR* TypeTexts[] = { "[Bool]", "[Int]", "[Float]", "[String]", "[Command]" };

    int32 Width = 0;
    for (const CHAR* TypeText : TypeTexts)
    {
        Width = Math::Max(Width, MeasureTextWidth(Font, String(TypeText)));
    }

    return Width;
}

static int32 GetSetByColumnWidth(const TSharedPtr<IFontFace>& Font)
{
    static const EConsoleVariableFlags SetByFlags[] =
    {
        EConsoleVariableFlags::SetByConstructor,
        EConsoleVariableFlags::SetByConfigFile,
        EConsoleVariableFlags::SetByCommandLine,
        EConsoleVariableFlags::SetByConsole,
        EConsoleVariableFlags::SetByCode,
    };

    int32 Width = 0;
    for (EConsoleVariableFlags SetByFlag : SetByFlags)
    {
        Width = Math::Max(Width, MeasureTextWidth(Font, String::Printf("[%s]", SetByFlagToString(SetByFlag))));
    }

    return Width;
}

TSharedPtr<FConsole> FConsole::Create(const FDesc& Desc)
{
    TSharedPtr<FConsole> NewInstance = MakeSharedPtr<FConsole>();
    if (NewInstance)
    {
        NewInstance->Initialize(Desc);
    }

    return NewInstance;
}

bool FConsole::IsToggleKey(FKey Key)
{
    return Key == Keys::GraveAccent || Key == Keys::World1;
}

FConsole::FConsole()
    : FCompoundElement()
    , LogBuffer()
    , CommandLine()
    , Font(nullptr)
    , Background(nullptr)
    , RootBox(nullptr)
    , ScrollBox(nullptr)
    , ScrollContent(nullptr)
    , InputBackground(nullptr)
    , Input(nullptr)
    , SelectedCandidateColor(0.6f, 0.6f, 0.6f, 1.0f)
    , CandidateDetailColor(0.85f, 0.85f, 0.85f, 1.0f)
    , LastLogRevision(0)
    , TextAreaHeight(384)
    , bIsOpen(false)
    , bIsScrollContentDirty(true)
    , bIsScrollToEndPending(false)
    , bIsSyncingInput(false)
{
}

FConsole::~FConsole() = default;

void FConsole::Initialize(const FDesc& Desc)
{
    Font                   = Desc.Font;
    TextAreaHeight         = Desc.TextAreaHeight;
    SelectedCandidateColor = Desc.SelectedCandidateColor;
    CandidateDetailColor   = Desc.CandidateDetailColor;

    LogBuffer.SetMaxLines(Desc.MaxLogLines);

    ScrollContent = FVerticalBox::Create();

    ScrollBox = FScrollBox::Create();
    ScrollBox->SetContent(ScrollContent);

    FEditableText::FDesc InputDesc;
    InputDesc.Font     = Font;
    InputDesc.HintText = "Console Input";
    InputDesc.Padding  = FMargin(4, 0);

    Input = FEditableText::Create(InputDesc);
    Input->GetOnTextChanged().BindRaw(this, &FConsole::HandleTextChanged);
    Input->GetOnKeyDownInterceptor().BindRaw(this, &FConsole::HandleInputKeyDown);

    FBorder::FDesc InputBackgroundDesc;
    InputBackgroundDesc.BackgroundColor = Desc.InputBackgroundColor;
    InputBackgroundDesc.Padding         = FMargin(10, 0);
    InputBackgroundDesc.MinHeight       = GetInputFieldHeight();
    InputBackgroundDesc.CornerRadius    = INPUT_CORNER_RADIUS;
    InputBackgroundDesc.Content         = Input;

    InputBackgroundDesc.SetCursor(ECursor::TextInput);

    InputBackground = FBorder::Create(InputBackgroundDesc);

    RootBox = FVerticalBox::Create();
    RootBox->AddSlot(ScrollBox).SetFillCoefficient(1.0f);
    RootBox->AddSlot(InputBackground)
        .SetVerticalAlignment(EVerticalAlignment::Bottom)
        .SetPadding(FMargin(CONSOLE_HORIZONTAL_PADDING, INPUT_VERTICAL_PADDING));

    const FUIInnerFrameStyle& Frame = FUIStyle::GetDefault().InnerFrame;

    FBorder::FDesc BackgroundDesc;
    BackgroundDesc.BackgroundColor = Desc.BackgroundColor;
    BackgroundDesc.BorderColor     = Frame.Border;
    BackgroundDesc.BorderThickness = Frame.BorderThickness;
    BackgroundDesc.CornerRadius    = FCornerRadii(Frame.CornerRadius);
    BackgroundDesc.Content         = RootBox;

    Background = FBorder::Create(BackgroundDesc);

    SetContent(Background);
    SetVisibility(EVisibility::Hidden);

    if (Desc.bRegisterWithLogger)
    {
        LogBuffer.RegisterWithLogger();
    }

    bIsScrollContentDirty = true;
}

void FConsole::OnArrange(const FRectangle& AllottedBounds)
{
    if (LastLogRevision != LogBuffer.GetRevision())
    {
        LastLogRevision       = LogBuffer.GetRevision();
        bIsScrollContentDirty = true;
        bIsScrollToEndPending = true;
    }

    if (bIsScrollContentDirty)
    {
        RebuildScrollContent();
        bIsScrollContentDirty = false;

        // The rebuilt subtree has no cached sizes yet, so it has to be measured before arranging
        ScrollContent->PrepareDesiredSize();
    }

    if (bIsScrollToEndPending)
    {
        ScrollBox->ScrollToEnd();
        bIsScrollToEndPending = false;
    }

    // The console drops down from the top edge, so it only occupies the text-area height
    FRectangle ConsoleBounds = AllottedBounds;
    ConsoleBounds.Height     = Math::Min(AllottedBounds.Height, TextAreaHeight);

    SetContentRectangle(ConsoleBounds);
    FCompoundElement::OnArrange(ConsoleBounds);
}

int32 FConsole::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (!bIsOpen)
    {
        return LayerId;
    }

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
}

bool FConsole::CapturesAllInput() const
{
    return bIsOpen;
}

TSharedPtr<FVisualElement> FConsole::GetFocusTarget()
{
    return bIsOpen && Input ? Input : FCompoundElement::GetFocusTarget();
}

FEventResponse FConsole::OnKeyDown(const FKeyEvent& KeyEvent)
{

    if (KeyEvent.IsDown() && IsToggleKey(KeyEvent.GetKey()))
    {
        if (!KeyEvent.IsRepeat())
        {
            Toggle();
        }

        return FEventResponse::Handled();
    }

    return FCompoundElement::OnKeyDown(KeyEvent);
}

void FConsole::Toggle()
{
    SetIsOpen(!bIsOpen);
}

void FConsole::SetIsOpen(bool bInIsOpen)
{
    if (bIsOpen == bInIsOpen)
    {
        return;
    }

    bIsOpen = bInIsOpen;

    CommandLine.Reset();
    SyncInputFromCommandLine();

    SetVisibility(bIsOpen ? EVisibility::Visible : EVisibility::Hidden);

    bIsScrollContentDirty = true;
    bIsScrollToEndPending = true;
}

void FConsole::RebuildScrollContent()
{
    ScrollContent->ClearSlots();

    if (CommandLine.HasCandidates())
    {
        const TArray<TPair<IConsoleObject*, String>>& Candidates    = CommandLine.GetCandidates();
        const int32                                   SelectedIndex = CommandLine.GetSelectedCandidateIndex();

        // Measured once for the whole list, so every row starts its columns in the same place
        const FConsoleCandidateColumns Columns = ComputeCandidateColumns();

        for (int32 Index = 0; Index < Candidates.Size(); ++Index)
        {
            AddCandidateRow(Candidates[Index], Index == SelectedIndex, Columns);
        }

        // Only on a change, so scrolling the list by hand is not undone on the next rebuild
        if (CommandLine.ConsumeSelectionChanged() && SelectedIndex >= 0)
        {
            const int32 RowHeight = GetCandidateRowHeight();

            FRectangle RowBounds;
            RowBounds.Position.Y = SelectedIndex * RowHeight;
            RowBounds.Height     = RowHeight;
            ScrollBox->ScrollIntoView(RowBounds);
        }

        return;
    }

    TArray<FConsoleLogLine> Lines;
    LogBuffer.GetSnapshot(Lines);

    for (const FConsoleLogLine& Line : Lines)
    {
        AddLogLine(Line);
    }
}

void FConsole::SyncInputFromCommandLine()
{
    bIsSyncingInput = true;

    Input->SetTextSilently(CommandLine.GetText());
    Input->SetTextCursorPosition(CommandLine.GetTextCursorPosition());

    bIsSyncingInput = false;
}

EKeyInterceptResult FConsole::HandleInputKeyDown(const FKeyEvent& KeyEvent)
{
    const FKey Key = KeyEvent.GetKey();

    if (IsToggleKey(Key))
    {
        if (!KeyEvent.IsRepeat())
        {
            Toggle();
        }

        return EKeyInterceptResult::Handled;
    }

    if (Key == Keys::Up)
    {
        CommandLine.MoveSelectionUp();
        SyncInputFromCommandLine();

        bIsScrollContentDirty = true;
        return EKeyInterceptResult::Handled;
    }

    if (Key == Keys::Down)
    {
        CommandLine.MoveSelectionDown();
        SyncInputFromCommandLine();

        bIsScrollContentDirty = true;
        return EKeyInterceptResult::Handled;
    }

    if (Key == Keys::Tab)
    {
        if (CommandLine.AcceptCompletion())
        {
            SyncInputFromCommandLine();

            bIsScrollContentDirty = true;
            bIsScrollToEndPending = true;
        }

        return EKeyInterceptResult::Handled;
    }

    if (Key == Keys::Enter)
    {
        const EConsoleSubmitResult Result = CommandLine.Submit(LogBuffer);
        SyncInputFromCommandLine();

        bIsScrollContentDirty = true;

        if (Result == EConsoleSubmitResult::ExecutedCommand)
        {
            bIsScrollToEndPending = true;
        }

        return EKeyInterceptResult::Handled;
    }

    return EKeyInterceptResult::NotHandled;
}

void FConsole::HandleTextChanged(const String& NewText)
{
    if (bIsSyncingInput)
    {
        return;
    }

    CommandLine.SetText(NewText);
    CommandLine.SetTextCursorPosition(Input->GetTextCursorPosition());
    CommandLine.RefreshCandidates();

    bIsScrollContentDirty = true;

    // Erasing the whole line brings the log back, so it should show the newest line again
    if (NewText.IsEmpty())
    {
        bIsScrollToEndPending = true;
    }
}

void FConsole::AddLogLine(const FConsoleLogLine& Line)
{
    FTextBlock::FDesc TextDesc;
    TextDesc.Text            = Line.Message;
    TextDesc.Font            = Font;
    TextDesc.ColorAndOpacity = FConsoleLogBuffer::GetSeverityColor(Line.Severity);

    ScrollContent->AddSlot(FTextBlock::Create(TextDesc))
        .SetHorizontalAlignment(EHorizontalAlignment::Left)
        .SetPadding(FMargin(CONSOLE_HORIZONTAL_PADDING, 0));
}

int32 FConsole::GetCandidateRowHeight() const
{
    return Font ? Font->GetTextBandHeight() + (CANDIDATE_ROW_PADDING * 2) : FALLBACK_BOX_HEIGHT;
}

int32 FConsole::GetInputFieldHeight() const
{
    return Font ? Font->GetTextBandHeight() + (INPUT_FRAME_PADDING * 2) : FALLBACK_BOX_HEIGHT;
}

FConsoleCandidateColumns FConsole::ComputeCandidateColumns() const
{
    FConsoleCandidateColumns Columns;
    Columns.NameWidth  = CANDIDATE_NAME_MIN_WIDTH;
    Columns.ValueWidth = CANDIDATE_VALUE_MIN_WIDTH;

    for (const TPair<IConsoleObject*, String>& Candidate : CommandLine.GetCandidates())
    {
        const FConsoleCandidateText RowText = GetCandidateText(Candidate);
        Columns.NameWidth  = Math::Max(Columns.NameWidth, MeasureTextWidth(Font, RowText.Name));
        Columns.ValueWidth = Math::Max(Columns.ValueWidth, MeasureTextWidth(Font, RowText.Value));
    }

    Columns.TypeWidth  = GetTypeColumnWidth(Font);
    Columns.SetByWidth = GetSetByColumnWidth(Font);

    Columns.NameWidth  += CANDIDATE_COLUMN_SPACING;
    Columns.ValueWidth += CANDIDATE_COLUMN_SPACING;
    Columns.TypeWidth  += CANDIDATE_COLUMN_SPACING;
    Columns.SetByWidth += CANDIDATE_COLUMN_SPACING;

    return Columns;
}

void FConsole::AddCandidateRow(const TPair<IConsoleObject*, String>& Candidate, bool bIsSelected, const FConsoleCandidateColumns& Columns)
{
    const FConsoleCandidateText RowText  = GetCandidateText(Candidate);
    const FFloatColor           RowColor = bIsSelected ? FFloatColor::White : CandidateDetailColor;

    const auto CreateCell = [this](const String& CellText, const FFloatColor& CellColor, int32 ColumnWidth)
    {
        FTextBlock::FDesc CellDesc;
        CellDesc.Text            = CellText;
        CellDesc.Font            = Font;
        CellDesc.ColorAndOpacity = CellColor;
        CellDesc.Margin          = FMargin(0, 0, Math::Max(0, ColumnWidth - MeasureTextWidth(Font, CellText)), 0);
        return FTextBlock::Create(CellDesc);
    };

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(CreateCell(RowText.Name, RowColor, Columns.NameWidth));
    Row->AddSlot(CreateCell(RowText.Value, CandidateDetailColor, Columns.ValueWidth));
    Row->AddSlot(CreateCell(RowText.Type, CandidateDetailColor, Columns.TypeWidth));
    Row->AddSlot(CreateCell(RowText.SetBy, CandidateDetailColor, Columns.SetByWidth));
    Row->AddSlot(CreateCell(RowText.Help, CandidateDetailColor, 0));

    FBorder::FDesc SelectionDesc;
    SelectionDesc.BackgroundColor = bIsSelected ? SelectedCandidateColor : FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);
    SelectionDesc.Padding         = FMargin(CONSOLE_HORIZONTAL_PADDING, 0);
    SelectionDesc.MinHeight       = GetCandidateRowHeight();
    SelectionDesc.Content         = Row;

    ScrollContent->AddSlot(FBorder::Create(SelectionDesc));
}

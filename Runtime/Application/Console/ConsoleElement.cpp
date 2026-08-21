#include "Application/Console/ConsoleElement.h"
#include "Application/Elements/TextBlockElement.h"
#include "Application/Input/Keys.h"
#include "Core/Math/Math.h"

/** @brief The space between the columns of a candidate row, in pixels. */
static constexpr int32 GCandidateColumnSpacing = 10;

TSharedPtr<FConsoleElement> FConsoleElement::Create(const FInitializer& Initializer)
{
    TSharedPtr<FConsoleElement> NewElement = MakeSharedPtr<FConsoleElement>();
    if (NewElement)
    {
        NewElement->Initialize(Initializer);
    }

    return NewElement;
}

bool FConsoleElement::IsToggleKey(FKey Key)
{
    return Key == Keys::GraveAccent || Key == Keys::World1;
}

FConsoleElement::FConsoleElement()
    : FCompoundElement()
    , LogBuffer()
    , CommandLine()
    , Font(nullptr)
    , Background(nullptr)
    , RootBox(nullptr)
    , ScrollBox(nullptr)
    , ScrollContent(nullptr)
    , InputBackground(nullptr)
    , InputElement(nullptr)
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

FConsoleElement::~FConsoleElement() = default;

void FConsoleElement::Initialize(const FInitializer& Initializer)
{
    Font                   = Initializer.Font;
    TextAreaHeight         = Initializer.TextAreaHeight;
    SelectedCandidateColor = Initializer.SelectedCandidateColor;
    CandidateDetailColor   = Initializer.CandidateDetailColor;

    LogBuffer.SetMaxLines(Initializer.MaxLogLines);

    ScrollContent = FVerticalBoxElement::Create();

    ScrollBox = FScrollBoxElement::Create();
    ScrollBox->SetContent(ScrollContent);

    FEditableTextElement::FInitializer InputInitializer;
    InputInitializer.Font     = Font;
    InputInitializer.HintText = "Console Input";

    InputElement = FEditableTextElement::Create(InputInitializer);
    InputElement->GetOnTextChanged().BindRaw(this, &FConsoleElement::HandleTextChanged);
    InputElement->GetOnKeyDownInterceptor().BindRaw(this, &FConsoleElement::HandleInputKeyDown);

    FBorderElement::FInitializer InputBackgroundInitializer;
    InputBackgroundInitializer.BackgroundColor = Initializer.InputBackgroundColor;
    InputBackgroundInitializer.Padding         = FMargin(10, 6);
    InputBackgroundInitializer.Content         = InputElement;

    InputBackground = FBorderElement::Create(InputBackgroundInitializer);

    RootBox = FVerticalBoxElement::Create();
    RootBox->AddSlot(ScrollBox).SetFillCoefficient(1.0f);
    RootBox->AddSlot(InputBackground).SetVerticalAlignment(EVerticalAlignment::Bottom);

    FBorderElement::FInitializer BackgroundInitializer;
    BackgroundInitializer.BackgroundColor = Initializer.BackgroundColor;
    BackgroundInitializer.Padding         = FMargin(10, 0);
    BackgroundInitializer.Content         = RootBox;

    Background = FBorderElement::Create(BackgroundInitializer);

    SetContent(Background);
    SetVisibility(EVisibility::Hidden);

    if (Initializer.bRegisterWithLogger)
    {
        LogBuffer.RegisterWithLogger();
    }

    bIsScrollContentDirty = true;
}

void FConsoleElement::OnArrange(const FRectangle& AllottedBounds)
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

int32 FConsoleElement::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (!bIsOpen)
    {
        return LayerId;
    }

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
}

FEventResponse FConsoleElement::OnKeyDown(const FKeyEvent& KeyEvent)
{
    // The input line intercepts the toggle key while it has focus, so this only catches the key that
    // opens a closed console
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

void FConsoleElement::Toggle()
{
    SetIsOpen(!bIsOpen);
}

void FConsoleElement::SetIsOpen(bool bInIsOpen)
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

void FConsoleElement::RebuildScrollContent()
{
    ScrollContent->ClearSlots();

    if (CommandLine.HasCandidates())
    {
        const TArray<TPair<IConsoleObject*, String>>& Candidates    = CommandLine.GetCandidates();
        const int32                                   SelectedIndex = CommandLine.GetSelectedCandidateIndex();

        for (int32 Index = 0; Index < Candidates.Size(); ++Index)
        {
            AddCandidateRow(Candidates[Index], Index == SelectedIndex);
        }

        // Replaces the old SetScrollHereY call that was guarded by bCandidateSelectionChanged
        if (CommandLine.ConsumeSelectionChanged() && SelectedIndex >= 0 && Font)
        {
            const int32 RowHeight = Font->GetLineHeight();

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
        AddLogLineElement(Line);
    }
}

void FConsoleElement::SyncInputFromCommandLine()
{
    bIsSyncingInput = true;
    InputElement->SetTextSilently(CommandLine.GetText());
    InputElement->SetTextCursorPosition(CommandLine.GetTextCursorPosition());
    bIsSyncingInput = false;
}

EKeyInterceptResult FConsoleElement::HandleInputKeyDown(const FKeyEvent& KeyEvent)
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

void FConsoleElement::HandleTextChanged(const String& NewText)
{
    if (bIsSyncingInput)
    {
        return;
    }

    CommandLine.SetText(NewText);
    CommandLine.SetTextCursorPosition(InputElement->GetTextCursorPosition());
    CommandLine.RefreshCandidates();

    bIsScrollContentDirty = true;

    // Erasing the whole line brings the log back, so it should show the newest line again
    if (NewText.IsEmpty())
    {
        bIsScrollToEndPending = true;
    }
}

void FConsoleElement::AddLogLineElement(const FConsoleLogLine& Line)
{
    FTextBlockElement::FInitializer TextInitializer;
    TextInitializer.Text            = Line.Message;
    TextInitializer.Font            = Font;
    TextInitializer.ColorAndOpacity = FConsoleLogBuffer::GetSeverityColor(Line.Severity);

    ScrollContent->AddSlot(FTextBlockElement::Create(TextInitializer)).SetHorizontalAlignment(EHorizontalAlignment::Left);
}

void FConsoleElement::AddCandidateRow(const TPair<IConsoleObject*, String>& Candidate, bool bIsSelected)
{
    String      ValueText;
    String      SetByText;
    const CHAR* TypeText = "";

    if (IConsoleVariable* ConsoleVariable = Candidate.First->AsVariable())
    {
        ValueText = ConsoleVariable->GetString();

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
        SetByText = String::Printf("[%s]", SetByFlagToString(VariableFlags));
    }
    else if (Candidate.First->AsCommand())
    {
        TypeText = "Command";
    }

    const FFloatColor RowColor = bIsSelected ? FFloatColor::White : CandidateDetailColor;

    FTextBlockElement::FInitializer NameInitializer;
    NameInitializer.Text            = Candidate.Second;
    NameInitializer.Font            = Font;
    NameInitializer.ColorAndOpacity = RowColor;

    FTextBlockElement::FInitializer ValueInitializer;
    ValueInitializer.Text            = ValueText;
    ValueInitializer.Font            = Font;
    ValueInitializer.ColorAndOpacity = CandidateDetailColor;
    ValueInitializer.Margin          = FMargin(GCandidateColumnSpacing, 0, 0, 0);

    FTextBlockElement::FInitializer TypeInitializer;
    TypeInitializer.Text            = String::Printf("[%s]", TypeText);
    TypeInitializer.Font            = Font;
    TypeInitializer.ColorAndOpacity = CandidateDetailColor;
    TypeInitializer.Margin          = FMargin(GCandidateColumnSpacing, 0, 0, 0);

    FTextBlockElement::FInitializer HelpInitializer;
    HelpInitializer.Text            = String::Printf("%s[Help: %s]", *SetByText, Candidate.First->GetHelpString());
    HelpInitializer.Font            = Font;
    HelpInitializer.ColorAndOpacity = CandidateDetailColor;
    HelpInitializer.Margin          = FMargin(GCandidateColumnSpacing, 0, 0, 0);

    TSharedPtr<FHorizontalBoxElement> Row = FHorizontalBoxElement::Create();
    Row->AddSlot(FTextBlockElement::Create(NameInitializer));
    Row->AddSlot(FTextBlockElement::Create(ValueInitializer));
    Row->AddSlot(FTextBlockElement::Create(TypeInitializer));
    Row->AddSlot(FTextBlockElement::Create(HelpInitializer));

    if (bIsSelected)
    {
        // The fill behind the selected row stands in for the ImGui Selectable header color
        FBorderElement::FInitializer SelectionInitializer;
        SelectionInitializer.BackgroundColor = SelectedCandidateColor;
        SelectionInitializer.Content         = Row;

        ScrollContent->AddSlot(FBorderElement::Create(SelectionInitializer));
        return;
    }

    ScrollContent->AddSlot(Row);
}

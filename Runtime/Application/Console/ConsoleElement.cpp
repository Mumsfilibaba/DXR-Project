#include "Application/Console/ConsoleElement.h"
#include "Application/Elements/TextBlockElement.h"
#include "Application/Input/Keys.h"
#include "Core/Math/Math.h"

/** @brief The space between the columns of a candidate row, in pixels. */
static constexpr int32 GCandidateColumnSpacing = 10;

/** @brief The least the name column is given, before the spacing, so short names still separate. */
static constexpr int32 GCandidateNameMinWidth = 30;

/** @brief The least the value column is given, before the spacing. */
static constexpr int32 GCandidateValueMinWidth = 20;

/** @brief The inset of the console content from either edge, in pixels, carried by the rows themselves. */
static constexpr int32 GConsoleHorizontalPadding = 10;

/** @brief How far the input field is rounded at its corners. */
static constexpr float GInputCornerRadius = 8.0f;

/** @brief The space above and below the input field, matching the dummies around the ImGui one. */
static constexpr int32 GInputVerticalPadding = 6;

/** @brief The space between the glyphs and the edge of the input field, the way ImGui frames a field. */
static constexpr int32 GInputFramePadding = 4;

/** @brief The same for a candidate row, which the ImGui console gave a 20 pixel selectable. */
static constexpr int32 GCandidateRowPadding = 2;

/** @brief What a row and the field fall back to without a face, so scrolling does not put every row at the top. */
static constexpr int32 GFallbackBoxHeight = 20;

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
    InputInitializer.Padding  = FMargin(4, 0);

    InputElement = FEditableTextElement::Create(InputInitializer);
    InputElement->GetOnTextChanged().BindRaw(this, &FConsoleElement::HandleTextChanged);
    InputElement->GetOnKeyDownInterceptor().BindRaw(this, &FConsoleElement::HandleInputKeyDown);

    FBorderElement::FInitializer InputBackgroundInitializer;
    InputBackgroundInitializer.BackgroundColor = Initializer.InputBackgroundColor;
    InputBackgroundInitializer.Padding         = FMargin(10, 0);
    InputBackgroundInitializer.MinHeight       = GetInputFieldHeight();
    InputBackgroundInitializer.CornerRadius    = GInputCornerRadius;
    InputBackgroundInitializer.Content         = InputElement;

    InputBackgroundInitializer.SetCursor(ECursor::TextInput);

    InputBackground = FBorderElement::Create(InputBackgroundInitializer);

    RootBox = FVerticalBoxElement::Create();
    RootBox->AddSlot(ScrollBox).SetFillCoefficient(1.0f);
    RootBox->AddSlot(InputBackground)
        .SetVerticalAlignment(EVerticalAlignment::Bottom)
        .SetPadding(FMargin(GConsoleHorizontalPadding, GInputVerticalPadding));

    FBorderElement::FInitializer BackgroundInitializer;
    BackgroundInitializer.BackgroundColor = Initializer.BackgroundColor;
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

bool FConsoleElement::CapturesAllInput() const
{
    return bIsOpen;
}

TSharedPtr<FVisualElement> FConsoleElement::GetFocusTarget()
{
    return bIsOpen && InputElement ? InputElement : FCompoundElement::GetFocusTarget();
}

FEventResponse FConsoleElement::OnKeyDown(const FKeyEvent& KeyEvent)
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

    ScrollContent->AddSlot(FTextBlockElement::Create(TextInitializer))
        .SetHorizontalAlignment(EHorizontalAlignment::Left)
        .SetPadding(FMargin(GConsoleHorizontalPadding, 0));
}

int32 FConsoleElement::GetCandidateRowHeight() const
{
    return Font ? Font->GetTextBandHeight() + (GCandidateRowPadding * 2) : GFallbackBoxHeight;
}

int32 FConsoleElement::GetInputFieldHeight() const
{
    return Font ? Font->GetTextBandHeight() + (GInputFramePadding * 2) : GFallbackBoxHeight;
}

FConsoleCandidateColumns FConsoleElement::ComputeCandidateColumns() const
{
    FConsoleCandidateColumns Columns;
    Columns.NameWidth  = GCandidateNameMinWidth;
    Columns.ValueWidth = GCandidateValueMinWidth;

    for (const TPair<IConsoleObject*, String>& Candidate : CommandLine.GetCandidates())
    {
        const FConsoleCandidateText RowText = GetCandidateText(Candidate);
        Columns.NameWidth  = Math::Max(Columns.NameWidth, MeasureTextWidth(Font, RowText.Name));
        Columns.ValueWidth = Math::Max(Columns.ValueWidth, MeasureTextWidth(Font, RowText.Value));
    }

    Columns.TypeWidth  = GetTypeColumnWidth(Font);
    Columns.SetByWidth = GetSetByColumnWidth(Font);

    Columns.NameWidth  += GCandidateColumnSpacing;
    Columns.ValueWidth += GCandidateColumnSpacing;
    Columns.TypeWidth  += GCandidateColumnSpacing;
    Columns.SetByWidth += GCandidateColumnSpacing;

    return Columns;
}

void FConsoleElement::AddCandidateRow(const TPair<IConsoleObject*, String>& Candidate, bool bIsSelected, const FConsoleCandidateColumns& Columns)
{
    const FConsoleCandidateText RowText  = GetCandidateText(Candidate);
    const FFloatColor           RowColor = bIsSelected ? FFloatColor::White : CandidateDetailColor;

    const auto MakeCell = [this](const String& CellText, const FFloatColor& CellColor, int32 ColumnWidth)
    {
        FTextBlockElement::FInitializer CellInitializer;
        CellInitializer.Text            = CellText;
        CellInitializer.Font            = Font;
        CellInitializer.ColorAndOpacity = CellColor;
        CellInitializer.Margin          = FMargin(0, 0, Math::Max(0, ColumnWidth - MeasureTextWidth(Font, CellText)), 0);
        return FTextBlockElement::Create(CellInitializer);
    };

    TSharedPtr<FHorizontalBoxElement> Row = FHorizontalBoxElement::Create();
    Row->AddSlot(MakeCell(RowText.Name, RowColor, Columns.NameWidth));
    Row->AddSlot(MakeCell(RowText.Value, CandidateDetailColor, Columns.ValueWidth));
    Row->AddSlot(MakeCell(RowText.Type, CandidateDetailColor, Columns.TypeWidth));
    Row->AddSlot(MakeCell(RowText.SetBy, CandidateDetailColor, Columns.SetByWidth));
    Row->AddSlot(MakeCell(RowText.Help, CandidateDetailColor, 0));

    FBorderElement::FInitializer SelectionInitializer;
    SelectionInitializer.BackgroundColor = bIsSelected ? SelectedCandidateColor : FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);
    SelectionInitializer.Padding         = FMargin(GConsoleHorizontalPadding, 0);
    SelectionInitializer.MinHeight       = GetCandidateRowHeight();
    SelectionInitializer.Content         = Row;

    ScrollContent->AddSlot(FBorderElement::Create(SelectionInitializer));
}

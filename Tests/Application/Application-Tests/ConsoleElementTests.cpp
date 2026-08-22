#include "ConsoleElementTests.h"
#include "ConsoleTestVariables.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Misc/Paths.h>
#include <Application/Application.h>
#include <Application/Console/ConsoleElement.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Draw/UIDrawData.h>
#include <Application/Elements/BorderElement.h>
#include <Application/Elements/WindowElement.h>
#include <Application/Input/Keys.h>
#include <Application/Text/FixedWidthFontFace.h>
#include <Application/Text/TrueTypeFontFace.h>

/** @brief The inset the console rows carry, matching GConsoleHorizontalPadding in the console itself. */
static constexpr int32 GExpectedRowInset = 10;

/** @brief A candidate row for the test face: its 16 pixel band and GCandidateRowPadding on each side. */
static constexpr int32 GExpectedRowHeight = 20;

/** @brief The space above and below the input field, matching GInputVerticalPadding. */
static constexpr int32 GExpectedInputPadding = 6;

/** @brief The input field for the test face: the same band with GInputFramePadding on each side. */
static constexpr int32 GExpectedInputHeight = 24;

static FKeyEvent CreateKeyDownEvent(FKey Key)
{
    return FKeyEvent(EInputEventType::KeyDown, Key, FModifierKeyState(), false, true);
}

static FKeyEvent CreateCharEvent(CHAR Character)
{
    return FKeyEvent(EInputEventType::KeyChar, Keys::Unknown, FModifierKeyState(), static_cast<uint32>(Character), false, true);
}

static TSharedPtr<FConsoleElement> CreateConsole()
{
    FConsoleElement::FInitializer Initializer;
    Initializer.Font                = MakeSharedPtr<FFixedWidthFontFace>(8, 16);
    Initializer.bRegisterWithLogger = false;
    return FConsoleElement::Create(Initializer);
}

static void TypeText(const TSharedPtr<FConsoleElement>& Console, const CHAR* Text)
{
    TSharedPtr<FEditableTextElement> Input = Console->GetInputElement();
    for (int32 Index = 0; Text[Index] != 0; ++Index)
    {
        Input->OnKeyChar(CreateCharEvent(Text[Index]));
    }
}

static void LayOutAndDraw(const TSharedPtr<FConsoleElement>& Console, FDrawCommandList& OutCommandList)
{
    Console->PrepareDesiredSize();
    Console->Tick(FRectangle(IntVector2(0, 0), 1280, 720));

    OutCommandList.Reset();
    Console->OnDraw(FDrawGeometry(Console->GetContentRectangle(), 1.0f), OutCommandList, 0);
}

static int32 FindBoxCommand(const FDrawCommandList& CommandList, const FFloatColor& Tint)
{
    for (int32 Index = 0; Index < CommandList.Size(); ++Index)
    {
        if (CommandList[Index].Type == EDrawCommandType::Box && CommandList[Index].Tint == Tint)
        {
            return Index;
        }
    }

    return FDrawCommandList::InvalidIndex;
}

static int32 FindHighestClipPopLayer(const FDrawCommandList& CommandList)
{
    int32 HighestLayer = -1;
    for (int32 Index = 0; Index < CommandList.Size(); ++Index)
    {
        if (CommandList[Index].Type == EDrawCommandType::ClipPop)
        {
            HighestLayer = Math::Max(HighestLayer, CommandList[Index].LayerId);
        }
    }

    return HighestLayer;
}

static TArray<int32> GetRowCellPositions(const FDrawCommandList& CommandList, int32 RowTop)
{
    TArray<int32> Positions;
    for (int32 Index = 0; Index < CommandList.Size(); ++Index)
    {
        if (CommandList[Index].Type == EDrawCommandType::Text && CommandList[Index].Bounds.Position.Y == RowTop)
        {
            Positions.Add(CommandList[Index].Bounds.Position.X);
        }
    }

    return Positions;
}

static int32 FindVertex(const FUIDrawData& DrawData, const Vector2& Position)
{
    const TArray<FUIVertex>& Vertices = DrawData.GetVertices();
    for (int32 Index = 0; Index < Vertices.Size(); ++Index)
    {
        if (Vertices[Index].Position == Position)
        {
            return Index;
        }
    }

    return -1;
}

static int32 FindBatchDrawingVertex(const FUIDrawData& DrawData, int32 VertexIndex)
{
    const TArray<uint16>& Indices = DrawData.GetIndices();

    for (int32 BatchIndex = 0; BatchIndex < DrawData.GetBatches().Size(); ++BatchIndex)
    {
        const FUIDrawBatch& Batch = DrawData.GetBatches()[BatchIndex];
        for (int32 Index = Batch.IndexOffset; Index < Batch.IndexOffset + Batch.IndexCount; ++Index)
        {
            if (static_cast<int32>(Indices[Index]) == VertexIndex)
            {
                return BatchIndex;
            }
        }
    }

    return -1;
}

bool ConsoleElementToggle_Test()
{
    TEST_BEGIN();

    TSharedPtr<FConsoleElement> Console = CreateConsole();

    TEST_SECTION("A new console starts closed");
    TEST_EXPECT(!Console->IsOpen());

    TEST_SECTION("The grave accent and World1 both toggle");
    TEST_EXPECT(FConsoleElement::IsToggleKey(Keys::GraveAccent));
    TEST_EXPECT(FConsoleElement::IsToggleKey(Keys::World1));
    TEST_EXPECT(!FConsoleElement::IsToggleKey(Keys::A));

    TEST_SECTION("The toggle key opens a closed console");
    TEST_EXPECT(Console->OnKeyDown(CreateKeyDownEvent(Keys::GraveAccent)).IsEventHandled());
    TEST_EXPECT(Console->IsOpen());
    TEST_EXPECT(Console->GetVisibility() == EVisibility::Visible);

    TEST_SECTION("The input line intercepts the toggle key and closes it again");
    TEST_EXPECT(Console->GetInputElement()->OnKeyDown(CreateKeyDownEvent(Keys::GraveAccent)).IsEventHandled());
    TEST_EXPECT(!Console->IsOpen());
    TEST_EXPECT(Console->GetVisibility() == EVisibility::Hidden);

    TEST_SECTION("A repeat does not toggle");
    Console->SetIsOpen(true);
    Console->OnKeyDown(FKeyEvent(EInputEventType::KeyDown, Keys::GraveAccent, FModifierKeyState(), true, true));
    TEST_EXPECT(Console->IsOpen());

    TEST_SECTION("Toggling drops the candidate list and the history walk");
    RegisterConsoleTestVariables();
    TypeText(Console, "Test.Console.");
    Console->GetCommandLine().MoveSelectionUp();

    TEST_EXPECT(Console->GetCommandLine().HasCandidates());
    TEST_EXPECT(Console->GetCommandLine().GetSelectedCandidateIndex() != FConsoleCommandLine::InvalidIndex);

    Console->Toggle();
    TEST_EXPECT(!Console->IsOpen());
    TEST_EXPECT(!Console->GetCommandLine().HasCandidates());
    TEST_EXPECT_EQ(Console->GetCommandLine().GetHistoryIndex(), FConsoleCommandLine::InvalidIndex);

    TEST_END();
}

bool ConsoleElementLayout_Test()
{
    TEST_BEGIN();

    TSharedPtr<FConsoleElement> Console = CreateConsole();
    Console->SetIsOpen(true);

    Console->GetLogBuffer().Log(ELogSeverity::Info, "A line of log");

    Console->PrepareDesiredSize();
    Console->Tick(FRectangle(IntVector2(0, 0), 1280, 720));

    TEST_SECTION("The console takes the full width but only the text-area height");
    TEST_EXPECT_EQ(Console->GetContentRectangle().Width, 1280);
    TEST_EXPECT_EQ(Console->GetContentRectangle().Height, 384);

    TEST_SECTION("The input line sits at the bottom of the console");
    const FRectangle InputBounds = Console->GetInputElement()->GetContentRectangle();
    TEST_EXPECT(InputBounds.GetBottom() <= Console->GetContentRectangle().GetBottom());
    TEST_EXPECT(InputBounds.Position.Y > Console->GetScrollBox()->GetContentRectangle().Position.Y);

    TEST_SECTION("The scrolled area is above the input line and takes the rest");
    const FRectangle ScrollBounds = Console->GetScrollBox()->GetContentRectangle();
    TEST_EXPECT(ScrollBounds.GetBottom() <= InputBounds.Position.Y);
    TEST_EXPECT(ScrollBounds.Height > 0);

    TEST_SECTION("A console shorter than the text area is clamped to what it was given");
    TSharedPtr<FConsoleElement> SmallConsole = CreateConsole();
    SmallConsole->SetIsOpen(true);
    SmallConsole->PrepareDesiredSize();
    SmallConsole->Tick(FRectangle(IntVector2(0, 0), 640, 200));

    TEST_EXPECT_EQ(SmallConsole->GetContentRectangle().Height, 200);

    TEST_END();
}

bool ConsoleElementLogDraw_Test()
{
    TEST_BEGIN();

    TSharedPtr<FConsoleElement> Console = CreateConsole();
    Console->SetIsOpen(true);

    Console->GetLogBuffer().Log(ELogSeverity::Info, "Info line");
    Console->GetLogBuffer().Log(ELogSeverity::Warning, "Warning line");
    Console->GetLogBuffer().Log(ELogSeverity::Error, "Error line");

    FDrawCommandList CommandList;
    LayOutAndDraw(Console, CommandList);

    TEST_SECTION("Every log line turns up as text, inside a balanced clip stack");
    TEST_EXPECT(!CommandList.IsEmpty());
    TEST_EXPECT(CommandList.IsClipStackBalanced());

    const int32 InfoIndex    = CommandList.FindTextCommand("Info line");
    const int32 WarningIndex = CommandList.FindTextCommand("Warning line");
    const int32 ErrorIndex   = CommandList.FindTextCommand("Error line");

    TEST_EXPECT(InfoIndex != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(WarningIndex != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(ErrorIndex != FDrawCommandList::InvalidIndex);

    TEST_SECTION("Each line is drawn in the color of its severity");
    if (InfoIndex != FDrawCommandList::InvalidIndex && WarningIndex != FDrawCommandList::InvalidIndex && ErrorIndex != FDrawCommandList::InvalidIndex)
    {
        TEST_EXPECT(CommandList[InfoIndex].Tint == FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Info));
        TEST_EXPECT(CommandList[WarningIndex].Tint == FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Warning));
        TEST_EXPECT(CommandList[ErrorIndex].Tint == FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Error));
    }

    TEST_SECTION("The lines are stacked in the order they were logged");
    if (InfoIndex != FDrawCommandList::InvalidIndex && ErrorIndex != FDrawCommandList::InvalidIndex)
    {
        TEST_EXPECT(CommandList[InfoIndex].Bounds.Position.Y < CommandList[ErrorIndex].Bounds.Position.Y);
    }

    TEST_SECTION("A closed console draws nothing at all");
    Console->SetIsOpen(false);
    LayOutAndDraw(Console, CommandList);
    TEST_EXPECT(CommandList.IsEmpty());

    TEST_SECTION("The newest line is in view once the log overflows");
    TSharedPtr<FConsoleElement> LongConsole = CreateConsole();
    LongConsole->SetIsOpen(true);

    for (int32 Index = 0; Index < 200; ++Index)
    {
        LongConsole->GetLogBuffer().Log(ELogSeverity::Info, String::Printf("Line %d", Index));
    }

    TEST_EXPECT_EQ(LongConsole->GetLogBuffer().GetNumLines(), FConsoleLogBuffer::DefaultMaxLines);

    LayOutAndDraw(LongConsole, CommandList);

    TEST_EXPECT(LongConsole->GetScrollBox()->IsScrolledToEnd());
    TEST_EXPECT(CommandList.FindTextCommand("Line 199") != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(CommandList.FindTextCommand("Line 0") == FDrawCommandList::InvalidIndex);

    TEST_END();
}

bool ConsoleElementCandidateDraw_Test()
{
    TEST_BEGIN();

    RegisterConsoleTestVariables();

    TSharedPtr<FConsoleElement> Console = CreateConsole();
    Console->SetIsOpen(true);

    Console->GetLogBuffer().Log(ELogSeverity::Info, "A line of log");

    FDrawCommandList CommandList;
    LayOutAndDraw(Console, CommandList);

    TEST_SECTION("The log is showing before anything is typed");
    TEST_EXPECT(CommandList.FindTextCommand("A line of log") != FDrawCommandList::InvalidIndex);

    TEST_SECTION("Candidates replace the log once the line matches something");
    TypeText(Console, "Test.Console.");
    TEST_EXPECT(Console->GetCommandLine().HasCandidates());

    LayOutAndDraw(Console, CommandList);

    TEST_EXPECT(CommandList.FindTextCommand("A line of log") == FDrawCommandList::InvalidIndex);
    TEST_EXPECT(CommandList.FindTextCommand("Test.Console.Int") != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(CommandList.FindTextCommand("Test.Console.Bool") != FDrawCommandList::InvalidIndex);

    TEST_SECTION("A candidate row carries the value and the type");
    TEST_EXPECT(CommandList.FindTextCommand("[Int]") != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(CommandList.FindTextCommand("[Bool]") != FDrawCommandList::InvalidIndex);

    TEST_SECTION("No row is filled in until a selection is made");
    const int32 NumBoxesWithoutSelection = CommandList.CountCommandsOfType(EDrawCommandType::Box);

    TSharedPtr<FEditableTextElement> Input = Console->GetInputElement();
    Input->OnKeyDown(CreateKeyDownEvent(Keys::Up));

    TEST_EXPECT(Console->GetCommandLine().GetSelectedCandidateIndex() != FConsoleCommandLine::InvalidIndex);

    LayOutAndDraw(Console, CommandList);
    TEST_EXPECT(CommandList.CountCommandsOfType(EDrawCommandType::Box) > NumBoxesWithoutSelection);

    TEST_SECTION("Tab completes the line and the log comes back");
    Input->OnKeyDown(CreateKeyDownEvent(Keys::Tab));
    TEST_EXPECT(!Console->GetCommandLine().HasCandidates());

    LayOutAndDraw(Console, CommandList);
    TEST_EXPECT(CommandList.FindTextCommand("A line of log") != FDrawCommandList::InvalidIndex);

    TEST_SECTION("Clearing the line brings the log back too");
    Input->ClearText();

    TEST_EXPECT(Console->GetCommandLine().GetText().IsEmpty());
    TEST_EXPECT(!Console->GetCommandLine().HasCandidates());

    LayOutAndDraw(Console, CommandList);
    TEST_EXPECT(CommandList.FindTextCommand("A line of log") != FDrawCommandList::InvalidIndex);

    TEST_END();
}

bool ConsoleElementCandidateHighlight_Test()
{
    TEST_BEGIN();

    RegisterConsoleTestVariables();

    TSharedPtr<FConsoleElement> Console = CreateConsole();
    Console->SetIsOpen(true);

    TypeText(Console, "Test.Console.");
    Console->GetInputElement()->OnKeyDown(CreateKeyDownEvent(Keys::Up));

    TEST_EXPECT(Console->GetCommandLine().GetSelectedCandidateIndex() != FConsoleCommandLine::InvalidIndex);

    FDrawCommandList CommandList;
    LayOutAndDraw(Console, CommandList);

    const FFloatColor HighlightColor = FConsoleElement::FInitializer().SelectedCandidateColor;

    int32 HighlightIndex = FDrawCommandList::InvalidIndex;
    int32 NumHighlights  = 0;

    for (int32 Index = 0; Index < CommandList.Size(); ++Index)
    {
        if (CommandList[Index].Type == EDrawCommandType::Box && CommandList[Index].Tint == HighlightColor)
        {
            HighlightIndex = Index;
            NumHighlights++;
        }
    }

    TEST_SECTION("Exactly one row is filled in, the selected one");
    TEST_EXPECT_EQ(NumHighlights, 1);

    if (HighlightIndex == FDrawCommandList::InvalidIndex)
    {
        TEST_END();
    }

    TEST_SECTION("The fill spans the console from edge to edge");
    const FRectangle ConsoleBounds   = Console->GetContentRectangle();
    const FRectangle HighlightBounds = CommandList[HighlightIndex].Bounds;

    TEST_EXPECT_EQ(HighlightBounds.Position.X, ConsoleBounds.Position.X);
    TEST_EXPECT_EQ(HighlightBounds.Width, ConsoleBounds.Width);

    TEST_SECTION("The row text still carries the inset the fill does not");
    const int32 NameIndex = CommandList.FindTextCommand("Test.Console.Int");
    TEST_EXPECT(NameIndex != FDrawCommandList::InvalidIndex);

    if (NameIndex != FDrawCommandList::InvalidIndex)
    {
        TEST_EXPECT_EQ(CommandList[NameIndex].Bounds.Position.X, ConsoleBounds.Position.X + GExpectedRowInset);
    }

    TEST_SECTION("A log line lines up with a candidate row");
    Console->GetInputElement()->ClearText();
    Console->GetLogBuffer().Log(ELogSeverity::Info, "A line of log");

    LayOutAndDraw(Console, CommandList);

    const int32 LogIndex = CommandList.FindTextCommand("A line of log");
    TEST_EXPECT(LogIndex != FDrawCommandList::InvalidIndex);

    if (LogIndex != FDrawCommandList::InvalidIndex)
    {
        TEST_EXPECT_EQ(CommandList[LogIndex].Bounds.Position.X, ConsoleBounds.Position.X + GExpectedRowInset);
    }

    TEST_SECTION("The input field keeps the inset too, and is rounded");
    const FRectangle InputBounds = Console->GetInputElement()->GetContentRectangle();
    TEST_EXPECT(InputBounds.Position.X >= ConsoleBounds.Position.X + GExpectedRowInset);

    const FFloatColor InputColor = FConsoleElement::FInitializer().InputBackgroundColor;

    int32 InputFillIndex = FDrawCommandList::InvalidIndex;
    for (int32 Index = 0; Index < CommandList.Size(); ++Index)
    {
        if (CommandList[Index].Type == EDrawCommandType::Box && CommandList[Index].Tint == InputColor)
        {
            InputFillIndex = Index;
            break;
        }
    }

    TEST_EXPECT(InputFillIndex != FDrawCommandList::InvalidIndex);

    if (InputFillIndex != FDrawCommandList::InvalidIndex)
    {
        TEST_EXPECT(CommandList[InputFillIndex].CornerRadius > 0.0f);
        TEST_EXPECT_EQ(CommandList[InputFillIndex].Bounds.Position.X, ConsoleBounds.Position.X + GExpectedRowInset);
    }

    TEST_SECTION("The console background is lighter than the field sunk into it");
    const FConsoleElement::FInitializer DefaultInitializer;
    TEST_EXPECT(DefaultInitializer.BackgroundColor.R > DefaultInitializer.InputBackgroundColor.R);

    TEST_END();
}

bool ConsoleElementCandidateColumns_Test()
{
    TEST_BEGIN();

    RegisterConsoleTestVariables();

    TSharedPtr<FConsoleElement> Console = CreateConsole();
    Console->SetIsOpen(true);

    TypeText(Console, "Test.Console.");
    Console->GetInputElement()->OnKeyDown(CreateKeyDownEvent(Keys::Up));

    FDrawCommandList CommandList;
    LayOutAndDraw(Console, CommandList);

    static const CHAR* RowNames[] =
    {
        "Test.Console.Int",
        "Test.Console.Float",
        "Test.Console.Bool",
        "Test.Console.String",
    };

    TArray<TArray<int32>> Rows;
    for (const CHAR* RowName : RowNames)
    {
        const int32 NameIndex = CommandList.FindTextCommand(RowName);
        if (NameIndex != FDrawCommandList::InvalidIndex)
        {
            Rows.Add(GetRowCellPositions(CommandList, CommandList[NameIndex].Bounds.Position.Y));
        }
    }

    TEST_SECTION("Every candidate is drawn as a row of five cells");
    TEST_EXPECT_EQ(Rows.Size(), static_cast<int32>(ARRAY_COUNT(RowNames)));

    for (const TArray<int32>& Row : Rows)
    {
        TEST_EXPECT_EQ(Row.Size(), 5);
    }

    TEST_SECTION("And every column starts at the same offset, whatever the name in front of it is");
    for (int32 RowIndex = 1; RowIndex < Rows.Size(); ++RowIndex)
    {
        if (Rows[RowIndex].Size() != Rows[0].Size())
        {
            continue;
        }

        for (int32 CellIndex = 0; CellIndex < Rows[0].Size(); ++CellIndex)
        {
            TEST_EXPECT_EQ(Rows[RowIndex][CellIndex], Rows[0][CellIndex]);
        }
    }

    TEST_SECTION("The columns are in the order the ImGui rows had them");
    if (!Rows.IsEmpty() && Rows[0].Size() == 5)
    {
        for (int32 CellIndex = 1; CellIndex < Rows[0].Size(); ++CellIndex)
        {
            TEST_EXPECT(Rows[0][CellIndex] > Rows[0][CellIndex - 1]);
        }
    }

    TEST_SECTION("The set-by flag has a column of its own rather than riding along with the help");
    TEST_EXPECT(CommandList.FindTextCommand("[Int]") != FDrawCommandList::InvalidIndex);
    const String SetByText = String::Printf("[%s]", SetByFlagToString(EConsoleVariableFlags::SetByConstructor));
    TEST_EXPECT(CommandList.FindTextCommand(StringView(SetByText.Data(), SetByText.Length())) != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(CommandList.FindTextCommand("[Help: An int variable for the console tests]") != FDrawCommandList::InvalidIndex);

    TEST_SECTION("A row is as tall as the selectable the ImGui console used, so its text can centre");
    const int32 HighlightIndex = FindBoxCommand(CommandList, FConsoleElement::FInitializer().SelectedCandidateColor);
    TEST_EXPECT(HighlightIndex != FDrawCommandList::InvalidIndex);

    if (HighlightIndex == FDrawCommandList::InvalidIndex)
    {
        TEST_END();
    }

    TEST_EXPECT_EQ(CommandList[HighlightIndex].Bounds.Height, GExpectedRowHeight);

    TEST_SECTION("The text of the selected row fills that height rather than sitting on top of it");
    const int32   SelectedIndex = Console->GetCommandLine().GetSelectedCandidateIndex();
    const String& SelectedName  = Console->GetCommandLine().GetCandidates()[SelectedIndex].Second;

    const int32 SelectedNameIndex = CommandList.FindTextCommand(StringView(SelectedName.Data(), SelectedName.Length()));
    TEST_EXPECT(SelectedNameIndex != FDrawCommandList::InvalidIndex);

    if (SelectedNameIndex != FDrawCommandList::InvalidIndex)
    {
        TEST_EXPECT_EQ(CommandList[SelectedNameIndex].Bounds.Position.Y, CommandList[HighlightIndex].Bounds.Position.Y);
        TEST_EXPECT_EQ(CommandList[SelectedNameIndex].Bounds.Height, GExpectedRowHeight);
    }

    TEST_SECTION("Every row is that tall, so the list steps by one row height");
    if (Rows.Size() >= 2)
    {
        const int32 FirstRowTop  = CommandList[CommandList.FindTextCommand(RowNames[0])].Bounds.Position.Y;
        const int32 SecondRowTop = CommandList[CommandList.FindTextCommand(RowNames[1])].Bounds.Position.Y;
        TEST_EXPECT_EQ(Math::Abs(SecondRowTop - FirstRowTop) % GExpectedRowHeight, 0);
    }

    TEST_END();
}

bool ConsoleElementInputChrome_Test()
{
    TEST_BEGIN();

    TSharedPtr<FConsoleElement> Console = CreateConsole();
    Console->SetIsOpen(true);
    Console->GetLogBuffer().Log(ELogSeverity::Info, "A line of log");
    Console->GetInputElement()->OnFocusGained();

    FDrawCommandList CommandList;
    LayOutAndDraw(Console, CommandList);

    const int32 InputBoxIndex = FindBoxCommand(CommandList, FConsoleElement::FInitializer().InputBackgroundColor);
    TEST_EXPECT(InputBoxIndex != FDrawCommandList::InvalidIndex);

    if (InputBoxIndex == FDrawCommandList::InvalidIndex)
    {
        TEST_END();
    }

    const FRectangle InputBounds   = CommandList[InputBoxIndex].Bounds;
    const FRectangle ConsoleBounds = Console->GetContentRectangle();

    TEST_SECTION("The field is the height an ImGui InputText framed its text at");
    TEST_EXPECT_EQ(InputBounds.Height, GExpectedInputHeight);

    TEST_SECTION("The text cursor sits in the middle of it rather than against either edge");
    int32 TextCursorIndex = FDrawCommandList::InvalidIndex;
    for (int32 Index = 0; Index < CommandList.Size(); ++Index)
    {
        if (CommandList[Index].Type == EDrawCommandType::Line)
        {
            TextCursorIndex = Index;
            break;
        }
    }

    TEST_EXPECT(TextCursorIndex != FDrawCommandList::InvalidIndex);

    if (TextCursorIndex != FDrawCommandList::InvalidIndex)
    {
        const FFixedWidthFontFace TestFace(8, 16);
        const FRectangle          TextCursorBounds = CommandList[TextCursorIndex].Bounds;

        TEST_EXPECT_EQ(TextCursorBounds.Height, TestFace.GetCapHeight() + (TestFace.GetDescent() * 2));
        TEST_EXPECT(TextCursorBounds.Height < InputBounds.Height);
        TEST_EXPECT_EQ(TextCursorBounds.Position.Y - InputBounds.Position.Y, InputBounds.GetBottom() - TextCursorBounds.GetBottom());
    }

    TEST_SECTION("The field clears the bottom of the console rather than sitting flush against it");
    TEST_EXPECT_EQ(ConsoleBounds.GetBottom() - InputBounds.GetBottom(), GExpectedInputPadding);

    TEST_SECTION("It keeps the inset the rows above it carry, on both edges");
    TEST_EXPECT_EQ(InputBounds.Position.X, ConsoleBounds.Position.X + GExpectedRowInset);
    TEST_EXPECT_EQ(ConsoleBounds.GetRight() - InputBounds.GetRight(), GExpectedRowInset);

    TEST_SECTION("The corners are rounded far enough to be seen, and not so far it becomes a capsule");
    TEST_EXPECT(CommandList[InputBoxIndex].CornerRadius >= 8.0f);
    TEST_EXPECT(CommandList[InputBoxIndex].CornerRadius <= static_cast<float>(InputBounds.Height) * 0.5f);

    TEST_SECTION("The scrolled area stops above the padding, so a log line never runs into the field");
    TEST_EXPECT(Console->GetScrollBox()->GetContentRectangle().GetBottom() <= InputBounds.Position.Y - GExpectedInputPadding);

    TEST_END();
}

bool ConsoleElementCursorShape_Test()
{
    TEST_BEGIN();

    TSharedPtr<FConsoleElement> Console = CreateConsole();
    Console->SetIsOpen(true);
    Console->PrepareDesiredSize();
    Console->Tick(FRectangle(IntVector2(0, 0), 1280, 720));

    TEST_SECTION("An element has no opinion on the shape unless it says so");
    ECursor ElementCursor = ECursor::None;
    TEST_EXPECT(!Console->GetCursor(ElementCursor));
    TEST_EXPECT(Console->GetInputElement()->GetCursor(ElementCursor));
    TEST_EXPECT(ElementCursor == ECursor::TextInput);

    TEST_SECTION("A path with nothing to say leaves the arrow alone");
    FElementPath EmptyPath;
    TEST_EXPECT(FApplication::ResolveCursor(EmptyPath) == ECursor::Arrow);

    FElementPath ConsolePath;
    ConsolePath.Add(EVisibility::Visible, Console);
    TEST_EXPECT(FApplication::ResolveCursor(ConsolePath) == ECursor::Arrow);

    TEST_SECTION("A path ending in the input field asks for the I-beam");
    FElementPath FieldPath;
    FieldPath.Add(EVisibility::Visible, Console);
    FieldPath.Add(EVisibility::Visible, Console->GetInputElement());
    TEST_EXPECT(FApplication::ResolveCursor(FieldPath) == ECursor::TextInput);

    TEST_SECTION("So does the padding ring around it, which has no element of its own");
    const FRectangle TextBounds = Console->GetInputElement()->GetContentRectangle();
    const IntVector2 RingPoint(TextBounds.Position.X - 5, TextBounds.Position.Y + 2);

    FElementPath RingPath;
    Console->FindChildrenContainingPoint(RingPoint, RingPath);

    TEST_EXPECT(!RingPath.IsEmpty());
    TEST_EXPECT(!RingPath.Contains(Console->GetInputElement()));
    TEST_EXPECT(FApplication::ResolveCursor(RingPath) == ECursor::TextInput);

    TEST_SECTION("The area above it leaves the arrow, since nothing on that path edits text");
    FElementPath LogPath;
    Console->FindChildrenContainingPoint(Console->GetScrollBox()->GetContentRectangle().Position, LogPath);

    TEST_EXPECT(!LogPath.IsEmpty());
    TEST_EXPECT(FApplication::ResolveCursor(LogPath) == ECursor::Arrow);

    TEST_SECTION("The leaf has the last word, so a field inside a panel still wins");
    FBorderElement::FInitializer PanelInitializer;
    PanelInitializer.SetCursor(ECursor::Hand);

    TSharedPtr<FBorderElement> Panel = FBorderElement::Create(PanelInitializer);

    FElementPath LeafPath;
    LeafPath.Add(EVisibility::Visible, Panel);
    LeafPath.Add(EVisibility::Visible, Console->GetInputElement());
    TEST_EXPECT(FApplication::ResolveCursor(LeafPath) == ECursor::TextInput);

    FElementPath RootPath;
    RootPath.Add(EVisibility::Visible, Console->GetInputElement());
    RootPath.Add(EVisibility::Visible, Panel);
    TEST_EXPECT(FApplication::ResolveCursor(RootPath) == ECursor::Hand);

    TEST_END();
}

bool ConsoleElementInputFieldSurvives_Test()
{
    TEST_BEGIN();

    RegisterConsoleCrowdVariables();

    TSharedPtr<FTrueTypeFontFace> Font = FTrueTypeFontFace::CreateFromFile(Paths::GetAssetDir() + "/Editor/Fonts/consola.ttf", 16);
    TEST_EXPECT(Font != nullptr);

    if (!Font)
    {
        TEST_END();
    }

    FConsoleElement::FInitializer Initializer;
    Initializer.Font                = Font;
    Initializer.bRegisterWithLogger = false;

    TSharedPtr<FConsoleElement> Console = FConsoleElement::Create(Initializer);
    Console->SetIsOpen(true);
    Console->GetInputElement()->OnFocusGained();

    for (int32 Index = 0; Index < FConsoleLogBuffer::DefaultMaxLines; ++Index)
    {
        Console->GetLogBuffer().Log(ELogSeverity::Info, String::Printf("[FRenderer]: Frame %d finished in 16.7ms with 428 draw calls", Index));
    }

    TEST_SECTION("One typed character brings up a candidate list longer than the console");
    const CHAR FirstCharacter[2] = { GConsoleCrowdPrefix[0], 0 };
    TypeText(Console, FirstCharacter);
    TEST_EXPECT(Console->GetCommandLine().GetCandidates().Size() >= GNumConsoleCrowdVariables);

    FDrawCommandList CommandList;
    LayOutAndDraw(Console, CommandList);

    const FFloatColor InputColor    = Initializer.InputBackgroundColor;
    const int32       InputBoxIndex = FindBoxCommand(CommandList, InputColor);

    TEST_SECTION("The field is still drawn, at the rectangle it was arranged into");
    TEST_EXPECT(InputBoxIndex != FDrawCommandList::InvalidIndex);

    if (InputBoxIndex == FDrawCommandList::InvalidIndex)
    {
        TEST_END();
    }

    const FRectangle InputBoxBounds = CommandList[InputBoxIndex].Bounds;
    const FRectangle TextBounds     = Console->GetInputElement()->GetContentRectangle();

    TEST_EXPECT(!InputBoxBounds.IsEmpty());
    TEST_EXPECT(InputBoxBounds.Intersect(TextBounds) == TextBounds);
    TEST_EXPECT(InputBoxBounds.GetBottom() <= Console->GetContentRectangle().GetBottom());

    TEST_SECTION("It is drawn above every clip region the rows are drawn inside");
    TEST_EXPECT(CommandList[InputBoxIndex].LayerId > FindHighestClipPopLayer(CommandList));

    FUIDrawData DrawData;
    DrawData.BuildFromCommandList(CommandList);

    TEST_SECTION("The whole frame fits the vertex budget, so nothing was truncated away");
    TEST_EXPECT(DrawData.GetVertices().Size() < FUIDrawData::MaxVertexCount);

    const Vector2 FanCenter(
        static_cast<float>(InputBoxBounds.Position.X + InputBoxBounds.GetRight()) * 0.5f,
        static_cast<float>(InputBoxBounds.Position.Y + InputBoxBounds.GetBottom()) * 0.5f);

    const int32 CenterVertex = FindVertex(DrawData, FanCenter);

    TEST_SECTION("Its geometry survives the translation into triangles");
    TEST_EXPECT(CommandList[InputBoxIndex].CornerRadius > 0.0f);
    TEST_EXPECT(CenterVertex >= 0);

    if (CenterVertex < 0)
    {
        TEST_END();
    }

    const int32 BatchIndex = FindBatchDrawingVertex(DrawData, CenterVertex);

    TEST_SECTION("And the batch carrying it draws, unclipped");
    TEST_EXPECT(BatchIndex >= 0);

    if (BatchIndex >= 0)
    {
        TEST_EXPECT(DrawData.GetBatches()[BatchIndex].IndexCount > 0);
        TEST_EXPECT(!DrawData.GetBatches()[BatchIndex].bIsClipped);
    }

    TEST_SECTION("The text cursor is drawn over the field rather than under the rows");
    int32 TextCursorLayer = -1;
    for (int32 Index = 0; Index < CommandList.Size(); ++Index)
    {
        if (CommandList[Index].Type == EDrawCommandType::Line)
        {
            TextCursorLayer = Math::Max(TextCursorLayer, CommandList[Index].LayerId);
        }
    }

    TEST_EXPECT(TextCursorLayer >= CommandList[InputBoxIndex].LayerId);

    TEST_SECTION("Every further character keeps the field on screen");
    for (int32 Index = 1; GConsoleCrowdPrefix[Index] != 0; ++Index)
    {
        const CHAR NextCharacter[2] = { GConsoleCrowdPrefix[Index], 0 };
        TypeText(Console, NextCharacter);

        LayOutAndDraw(Console, CommandList);

        TEST_EXPECT(Console->GetCommandLine().GetCandidates().Size() >= GNumConsoleCrowdVariables);
        TEST_EXPECT(FindBoxCommand(CommandList, InputColor) != FDrawCommandList::InvalidIndex);
    }

    TEST_END();
}

bool ConsoleElementModalInput_Test()
{
    TEST_BEGIN();

    FWindowElement::FInitializer WindowInitializer;
    WindowInitializer.Title = "Console Host";
    WindowInitializer.Size  = IntVector2(1280, 720);

    TSharedPtr<FWindowElement> Window = FWindowElement::Create(WindowInitializer);

    FBorderElement::FInitializer ContentInitializer;
    ContentInitializer.BackgroundColor = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);

    TSharedPtr<FBorderElement>  Content = FBorderElement::Create(ContentInitializer);
    TSharedPtr<FConsoleElement> Console = CreateConsole();

    Window->SetContent(Content);
    Window->SetOverlay(Console);

    FApplication::LayoutWindow(Window);

    TEST_SECTION("A closed console leaves the input alone");
    TEST_EXPECT(!Console->CapturesAllInput());

    FElementPath ClosedPath;
    Window->FindChildrenContainingPoint(IntVector2(640, 600), ClosedPath);
    TEST_EXPECT(ClosedPath.Contains(Content));

    TEST_SECTION("An open console reports that it takes everything");
    Console->SetIsOpen(true);
    FApplication::LayoutWindow(Window);
    TEST_EXPECT(Console->CapturesAllInput());

    TEST_SECTION("A click below the console never reaches the content underneath it");
    FElementPath BelowConsolePath;
    Window->FindChildrenContainingPoint(IntVector2(640, 600), BelowConsolePath);

    TEST_EXPECT(!BelowConsolePath.Contains(Content));
    TEST_EXPECT(!BelowConsolePath.IsEmpty());

    TEST_SECTION("A click on the console still reaches the console");
    FElementPath OnConsolePath;
    Window->FindChildrenContainingPoint(IntVector2(640, 40), OnConsolePath);

    TEST_EXPECT(OnConsolePath.Contains(Console));
    TEST_EXPECT(!OnConsolePath.Contains(Content));

    TEST_SECTION("Closing the console hands the window back");
    Console->SetIsOpen(false);
    FApplication::LayoutWindow(Window);

    FElementPath ReopenedPath;
    Window->FindChildrenContainingPoint(IntVector2(640, 600), ReopenedPath);
    TEST_EXPECT(ReopenedPath.Contains(Content));

    TEST_END();
}

bool ConsoleElementClickFocus_Test()
{
    TEST_BEGIN();

    TSharedPtr<FApplication> Application = CreateStubApplication();

    FWindowElement::FInitializer WindowInitializer;
    WindowInitializer.Title = "Console Host";
    WindowInitializer.Size  = IntVector2(1280, 720);

    TSharedPtr<FWindowElement> Window = FWindowElement::Create(WindowInitializer);

    FBorderElement::FInitializer ContentInitializer;
    ContentInitializer.BackgroundColor = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);

    TSharedPtr<FBorderElement>  Content = FBorderElement::Create(ContentInitializer);
    TSharedPtr<FConsoleElement> Console = CreateConsole();

    Window->SetContent(Content);
    Window->SetOverlay(Console);

    Console->SetIsOpen(true);
    FApplication::LayoutWindow(Window);

    TSharedPtr<FEditableTextElement> Input = Console->GetInputElement();

    TEST_SECTION("Opening the console hands the keyboard to the input line rather than to the shell around it");
    Application->SetFocusElement(Console->GetFocusTarget());
    TEST_EXPECT(Input->HasKeyboardFocus());

    TEST_SECTION("Clicking the log leaves it there, since nothing along that path reads a keyboard");
    FElementPath LogPath;
    Window->FindChildrenContainingPoint(Console->GetScrollBox()->GetContentRectangle().Position, LogPath);

    TEST_EXPECT(!LogPath.IsEmpty());
    TEST_EXPECT(!LogPath.Contains(Input));

    Application->SetFocusFromCursorPath(LogPath);
    TEST_EXPECT(Input->HasKeyboardFocus());

    TEST_SECTION("So does clicking below the console, where the window is all there is under the cursor");
    FElementPath BelowPath;
    Window->FindChildrenContainingPoint(IntVector2(640, 600), BelowPath);

    TEST_EXPECT(!BelowPath.Contains(Content));

    Application->SetFocusFromCursorPath(BelowPath);
    TEST_EXPECT(Input->HasKeyboardFocus());

    TEST_SECTION("A path with nothing focusable on it never takes the keyboard from something that has it");
    FElementPath BorderPath;
    BorderPath.Add(EVisibility::Visible, FBorderElement::Create(ContentInitializer));

    Application->SetFocusFromCursorPath(BorderPath);
    TEST_EXPECT(Input->HasKeyboardFocus());

    TEST_SECTION("Clicking the field itself keeps it focused");
    FElementPath FieldPath;
    Window->FindChildrenContainingPoint(Input->GetContentRectangle().Position, FieldPath);

    TEST_EXPECT(FieldPath.Contains(Input));

    Application->SetFocusFromCursorPath(FieldPath);
    TEST_EXPECT(Input->HasKeyboardFocus());

    TEST_SECTION("Another field does take it, so the rule is not simply refusing to move focus");
    FEditableTextElement::FInitializer FieldInitializer;
    FieldInitializer.Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    TSharedPtr<FEditableTextElement> OtherField = FEditableTextElement::Create(FieldInitializer);

    FElementPath OtherFieldPath;
    OtherFieldPath.Add(EVisibility::Visible, Window);
    OtherFieldPath.Add(EVisibility::Visible, OtherField);

    Application->SetFocusFromCursorPath(OtherFieldPath);

    TEST_EXPECT(OtherField->HasKeyboardFocus());
    TEST_EXPECT(!Input->HasKeyboardFocus());

    TEST_END();
}

bool ConsoleElementInWindow_Test()
{
    TEST_BEGIN();

    RegisterConsoleTestVariables();

    FWindowElement::FInitializer WindowInitializer;
    WindowInitializer.Title    = "Console Host";
    WindowInitializer.Size     = IntVector2(1280, 720);
    WindowInitializer.Position = IntVector2(220, 140);

    TSharedPtr<FWindowElement>  Window  = FWindowElement::Create(WindowInitializer);
    TSharedPtr<FConsoleElement> Console = CreateConsole();

    Window->SetOverlay(Console);
    Console->SetIsOpen(true);
    Console->GetLogBuffer().Log(ELogSeverity::Info, "A line of log");

    TSharedPtr<FEditableTextElement> Input = Console->GetInputElement();
    Input->OnFocusGained();

    FApplication::LayoutWindow(Window);

    TEST_SECTION("The console hangs from the top of the client area, wherever the window is on the desktop");
    TEST_EXPECT_EQ(Console->GetContentRectangle().Position.X, 0);
    TEST_EXPECT_EQ(Console->GetContentRectangle().Position.Y, 0);
    TEST_EXPECT_EQ(Console->GetContentRectangle().Width, 1280);

    TEST_SECTION("Moving the window does not drag the console along");
    Window->OnWindowMoved(IntVector2(700, 320));
    FApplication::LayoutWindow(Window);
    TEST_EXPECT_EQ(Console->GetContentRectangle().Position.X, 0);
    TEST_EXPECT_EQ(Console->GetContentRectangle().Position.Y, 0);

    TEST_SECTION("The input line is measured through the overlay, so it has room to draw into");
    const FRectangle InputBounds = Input->GetContentRectangle();
    TEST_EXPECT(InputBounds.Height > 0);
    TEST_EXPECT(InputBounds.Width > 0);
    TEST_EXPECT(Console->GetContentRectangle().EncapsulatesPoint(InputBounds.Position));

    FDrawCommandList CommandList;
    Window->OnDraw(FDrawGeometry(Window->GetContentRectangle(), 1.0f), CommandList, 0);

    TEST_SECTION("The focused input line draws its caret");
    TEST_EXPECT(CommandList.CountCommandsOfType(EDrawCommandType::Line) > 0);

    TEST_SECTION("Typed text and its candidates both come out with a height to be seen at");
    TypeText(Console, "Test.Console.");
    FApplication::LayoutWindow(Window);

    CommandList.Reset();
    Window->OnDraw(FDrawGeometry(Window->GetContentRectangle(), 1.0f), CommandList, 0);

    const int32 TypedIndex     = CommandList.FindTextCommand("Test.Console.");
    const int32 CandidateIndex = CommandList.FindTextCommand("Test.Console.Int");

    TEST_EXPECT(TypedIndex != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(CandidateIndex != FDrawCommandList::InvalidIndex);

    if (TypedIndex != FDrawCommandList::InvalidIndex && CandidateIndex != FDrawCommandList::InvalidIndex)
    {
        TEST_EXPECT(CommandList[TypedIndex].Bounds.Height > 0);
        TEST_EXPECT(CommandList[CandidateIndex].Bounds.Height > 0);
    }

    TEST_SECTION("Backspace reaches the input line and shortens the command");
    Input->OnKeyDown(CreateKeyDownEvent(Keys::Backspace));
    TEST_EXPECT(Console->GetCommandLine().GetText().Equals("Test.Console"));
    TEST_EXPECT(Input->GetText().Equals("Test.Console"));

    TEST_END();
}

bool ConsoleElementTypeAndExecute_Test()
{
    TEST_BEGIN();

    RegisterConsoleTestVariables();

    TSharedPtr<FConsoleElement> Console = CreateConsole();
    Console->SetIsOpen(true);

    TSharedPtr<FEditableTextElement> Input = Console->GetInputElement();
    Input->OnFocusGained();

    TEST_SECTION("Typing a prefix finds candidates");
    TypeText(Console, "Test.Console.Int");
    TEST_EXPECT(Console->GetCommandLine().HasCandidates());
    TEST_EXPECT(Console->GetCommandLine().GetText().Equals("Test.Console.Int"));

    TEST_SECTION("Enter with no selection executes and echoes to the log");
    Input->OnKeyDown(CreateKeyDownEvent(Keys::Enter));

    TArray<FConsoleLogLine> Lines;
    Console->GetLogBuffer().GetSnapshot(Lines);

    TEST_EXPECT(!Lines.IsEmpty());
    TEST_EXPECT(Lines[0].Message.Equals("Test.Console.Int"));
    TEST_EXPECT(Console->GetCommandLine().GetText().IsEmpty());
    TEST_EXPECT(Console->GetInputElement()->GetText().IsEmpty());

    TEST_SECTION("The tree lays out and draws without a device");
    Console->PrepareDesiredSize();
    Console->Tick(FRectangle(IntVector2(0, 0), 1280, 720));

    FDrawCommandList CommandList;
    Console->OnDraw(FDrawGeometry(Console->GetContentRectangle(), 1.0f), CommandList, 0);

    TEST_EXPECT(!CommandList.IsEmpty());
    TEST_EXPECT(CommandList.IsClipStackBalanced());
    TEST_EXPECT(CommandList.FindTextCommand("Test.Console.Int") != FDrawCommandList::InvalidIndex);

    TEST_SECTION("The executed line is reachable again through the history");
    Input->OnKeyDown(CreateKeyDownEvent(Keys::Up));

    const TArray<String>& History = FConsoleManager::Get().GetHistory();
    TEST_EXPECT(!History.IsEmpty());
    TEST_EXPECT(Console->GetCommandLine().GetText().Equals(History.Last()));
    TEST_EXPECT(Console->GetInputElement()->GetText().Equals(History.Last()));

    TEST_END();
}

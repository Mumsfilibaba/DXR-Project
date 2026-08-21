#include "ConsoleElementTests.h"
#include "ConsoleTestVariables.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Application/Console/ConsoleElement.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Input/Keys.h>
#include <Application/Text/FixedWidthFontFace.h>

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

    // The registry is process-global, so the newest entry is compared rather than a literal
    const TArray<String>& History = FConsoleManager::Get().GetHistory();
    TEST_EXPECT(!History.IsEmpty());
    TEST_EXPECT(Console->GetCommandLine().GetText().Equals(History.Last()));
    TEST_EXPECT(Console->GetInputElement()->GetText().Equals(History.Last()));

    TEST_END();
}

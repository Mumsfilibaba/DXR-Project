#include "ConsoleCommandLineTests.h"
#include "ConsoleTestVariables.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Application/Console/ConsoleCommandLine.h>
#include <Application/Console/ConsoleLogBuffer.h>

static bool AreAllCandidatesPrefixed(const FConsoleCommandLine& CommandLine, const CHAR* Prefix)
{
    const int32 PrefixLength = StringView(Prefix).Length();
    for (const TPair<IConsoleObject*, String>& Candidate : CommandLine.GetCandidates())
    {
        if (Candidate.Second.Length() < PrefixLength)
        {
            return false;
        }

        if (!StringView(Candidate.Second.Data(), PrefixLength).Equals(Prefix))
        {
            return false;
        }
    }

    return true;
}

bool ConsoleWordRange_Test()
{
    TEST_BEGIN();

    FConsoleCommandLine CommandLine;

    TEST_SECTION("An empty line has an empty word");
    TEST_EXPECT(CommandLine.FindWordRangeAtTextCursor().IsEmpty());

    TEST_SECTION("With no separator the word is the whole line");
    CommandLine.SetText("Command");
    CommandLine.SetTextCursorPosition(7);

    FConsoleWordRange Range = CommandLine.FindWordRangeAtTextCursor();
    TEST_EXPECT_EQ(Range.Position, 0);
    TEST_EXPECT_EQ(Range.Length, 7);

    TEST_SECTION("Each separator ends the word");
    const CHAR Separators[] = { ' ', '\t', ',', ';' };
    for (const CHAR Separator : Separators)
    {
        String Text("First");
        Text.Insert(Separator, Text.Length());
        Text.Insert("Second", Text.Length());

        CommandLine.SetText(Text);
        CommandLine.SetTextCursorPosition(Text.Length());

        Range = CommandLine.FindWordRangeAtTextCursor();
        TEST_EXPECT_EQ(Range.Position, 6);
        TEST_EXPECT_EQ(Range.Length, 6);
    }

    TEST_SECTION("The word ends at the text cursor, not at the end of the line");
    CommandLine.SetText("Command Argument");
    CommandLine.SetTextCursorPosition(4);

    Range = CommandLine.FindWordRangeAtTextCursor();
    TEST_EXPECT_EQ(Range.Position, 0);
    TEST_EXPECT_EQ(Range.Length, 4);

    TEST_SECTION("A text cursor sitting on a separator has an empty word");
    CommandLine.SetText("Command ");
    CommandLine.SetTextCursorPosition(8);
    TEST_EXPECT(CommandLine.FindWordRangeAtTextCursor().IsEmpty());

    TEST_SECTION("The text cursor clamps into the line");
    CommandLine.SetText("Ab");
    CommandLine.SetTextCursorPosition(500);
    TEST_EXPECT_EQ(CommandLine.GetTextCursorPosition(), 2);

    CommandLine.SetTextCursorPosition(-5);
    TEST_EXPECT_EQ(CommandLine.GetTextCursorPosition(), 0);

    TEST_END();
}

bool ConsoleCandidates_Test()
{
    TEST_BEGIN();

    RegisterConsoleTestVariables();

    FConsoleCommandLine CommandLine;

    TEST_SECTION("An empty line finds nothing");
    CommandLine.RefreshCandidates();
    TEST_EXPECT(!CommandLine.HasCandidates());
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), FConsoleCommandLine::InvalidIndex);

    TEST_SECTION("A prefix finds every object under it");
    CommandLine.SetText("Test.Console.");
    CommandLine.SetTextCursorPosition(13);
    CommandLine.RefreshCandidates();

    TEST_EXPECT(CommandLine.HasCandidates());
    TEST_EXPECT(CommandLine.GetCandidates().Size() >= 4);
    TEST_EXPECT(AreAllCandidatesPrefixed(CommandLine, "Test.Console."));

    TEST_SECTION("Matching is case insensitive, as the old console was");
    CommandLine.SetText("test.console.");
    CommandLine.SetTextCursorPosition(13);
    CommandLine.RefreshCandidates();
    TEST_EXPECT(CommandLine.HasCandidates());

    TEST_SECTION("A longer prefix narrows the list");
    CommandLine.SetText("Test.Console.Int");
    CommandLine.SetTextCursorPosition(16);
    CommandLine.RefreshCandidates();

    TEST_EXPECT_EQ(CommandLine.GetCandidates().Size(), 1);
    TEST_EXPECT(CommandLine.GetCandidates()[0].Second.Equals("Test.Console.Int"));

    TEST_SECTION("A prefix matching nothing drops the previous list");
    CommandLine.SetText("Test.Console.NoSuchThing");
    CommandLine.SetTextCursorPosition(24);
    CommandLine.RefreshCandidates();

    TEST_EXPECT(!CommandLine.HasCandidates());
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), FConsoleCommandLine::InvalidIndex);

    TEST_SECTION("Only the word at the text cursor is matched");
    CommandLine.SetText("Something Test.Console.Int");
    CommandLine.SetTextCursorPosition(26);
    CommandLine.RefreshCandidates();

    TEST_EXPECT_EQ(CommandLine.GetCandidates().Size(), 1);

    TEST_SECTION("Invalidating drops the list and the selection");
    CommandLine.MoveSelectionUp();
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), 0);

    CommandLine.InvalidateCandidates();
    TEST_EXPECT(!CommandLine.HasCandidates());
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), FConsoleCommandLine::InvalidIndex);

    TEST_END();
}

bool ConsoleCandidateSelection_Test()
{
    TEST_BEGIN();

    RegisterConsoleTestVariables();

    FConsoleCommandLine CommandLine;
    CommandLine.SetText("Test.Console.");
    CommandLine.SetTextCursorPosition(13);
    CommandLine.RefreshCandidates();

    const int32 NumCandidates = CommandLine.GetCandidates().Size();
    TEST_EXPECT(NumCandidates >= 4);

    TEST_SECTION("Up from no selection wraps to the last candidate");
    CommandLine.MoveSelectionUp();
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), NumCandidates - 1);
    TEST_EXPECT(CommandLine.ConsumeSelectionChanged());

    TEST_SECTION("The change flag is cleared once it is read");
    TEST_EXPECT(!CommandLine.ConsumeSelectionChanged());

    TEST_SECTION("Up walks back through the list");
    CommandLine.MoveSelectionUp();
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), NumCandidates - 2);

    TEST_SECTION("Down wraps around from the last candidate");
    CommandLine.InvalidateCandidates();
    CommandLine.RefreshCandidates();

    CommandLine.MoveSelectionDown();
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), 0);

    for (int32 Index = 0; Index < NumCandidates - 1; ++Index)
    {
        CommandLine.MoveSelectionDown();
    }

    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), NumCandidates - 1);

    CommandLine.MoveSelectionDown();
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), 0);

    TEST_SECTION("A single candidate does not move, so the flag stays clear");
    FConsoleCommandLine SingleCandidateLine;
    SingleCandidateLine.SetText("Test.Console.Int");
    SingleCandidateLine.SetTextCursorPosition(16);
    SingleCandidateLine.RefreshCandidates();

    TEST_EXPECT_EQ(SingleCandidateLine.GetCandidates().Size(), 1);

    SingleCandidateLine.MoveSelectionUp();
    TEST_EXPECT_EQ(SingleCandidateLine.GetSelectedCandidateIndex(), 0);
    TEST_EXPECT(SingleCandidateLine.ConsumeSelectionChanged());

    SingleCandidateLine.MoveSelectionUp();
    TEST_EXPECT_EQ(SingleCandidateLine.GetSelectedCandidateIndex(), 0);
    TEST_EXPECT(!SingleCandidateLine.ConsumeSelectionChanged());

    TEST_END();
}

bool ConsoleCompletion_Test()
{
    TEST_BEGIN();

    RegisterConsoleTestVariables();

    TEST_SECTION("Tab does nothing without a selection");
    FConsoleCommandLine CommandLine;
    CommandLine.SetText("Test.Console.I");
    CommandLine.SetTextCursorPosition(14);
    CommandLine.RefreshCandidates();

    TEST_EXPECT(CommandLine.HasCandidates());
    TEST_EXPECT(!CommandLine.AcceptCompletion());
    TEST_EXPECT(CommandLine.GetText().Equals("Test.Console.I"));

    TEST_SECTION("Tab with a selection completes the word and drops the list");
    CommandLine.MoveSelectionUp();
    TEST_EXPECT(CommandLine.AcceptCompletion());
    TEST_EXPECT(CommandLine.GetText().Equals("Test.Console.Int"));
    TEST_EXPECT_EQ(CommandLine.GetTextCursorPosition(), 16);
    TEST_EXPECT(!CommandLine.HasCandidates());

    TEST_SECTION("Tab replaces only the word at the text cursor");
    FConsoleCommandLine ArgumentLine;
    ArgumentLine.SetText("Prefix Test.Console.I");
    ArgumentLine.SetTextCursorPosition(21);
    ArgumentLine.RefreshCandidates();
    ArgumentLine.MoveSelectionUp();

    TEST_EXPECT(ArgumentLine.AcceptCompletion());
    TEST_EXPECT(ArgumentLine.GetText().Equals("Prefix Test.Console.Int"));
    TEST_EXPECT_EQ(ArgumentLine.GetTextCursorPosition(), 23);

    TEST_SECTION("Tab on an empty word does nothing");
    FConsoleCommandLine EmptyLine;
    TEST_EXPECT(!EmptyLine.AcceptCompletion());
    TEST_EXPECT(EmptyLine.GetText().IsEmpty());

    TEST_END();
}

bool ConsoleHistory_Test()
{
    TEST_BEGIN();

    FConsoleManager::Get().ClearHistory();

    FConsoleLogBuffer   LogBuffer;
    FConsoleCommandLine CommandLine;

    CommandLine.SetText("First.Command");
    CommandLine.Submit(LogBuffer);

    CommandLine.SetText("Second.Command");
    CommandLine.Submit(LogBuffer);

    TEST_SECTION("Submitting leaves the walk unstarted");
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), FConsoleCommandLine::InvalidIndex);
    TEST_EXPECT(CommandLine.GetText().IsEmpty());

    TEST_SECTION("Up starts at the newest entry");
    CommandLine.MoveSelectionUp();
    TEST_EXPECT(CommandLine.GetText().Equals("Second.Command"));
    TEST_EXPECT_EQ(CommandLine.GetTextCursorPosition(), CommandLine.GetText().Length());

    TEST_SECTION("Up walks back to the oldest entry and clamps there");
    CommandLine.MoveSelectionUp();
    TEST_EXPECT(CommandLine.GetText().Equals("First.Command"));

    CommandLine.MoveSelectionUp();
    TEST_EXPECT(CommandLine.GetText().Equals("First.Command"));
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), 0);

    TEST_SECTION("Down walks forward again");
    CommandLine.MoveSelectionDown();
    TEST_EXPECT(CommandLine.GetText().Equals("Second.Command"));

    TEST_SECTION("Down past the newest entry leaves the walk and clears the line");
    CommandLine.MoveSelectionDown();
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), FConsoleCommandLine::InvalidIndex);
    TEST_EXPECT(CommandLine.GetText().IsEmpty());

    TEST_SECTION("Down outside the walk does nothing");
    CommandLine.MoveSelectionDown();
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), FConsoleCommandLine::InvalidIndex);
    TEST_EXPECT(CommandLine.GetText().IsEmpty());

    TEST_SECTION("Reset drops the walk");
    CommandLine.MoveSelectionUp();
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), 1);

    CommandLine.Reset();
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), FConsoleCommandLine::InvalidIndex);

    TEST_SECTION("Candidates take the arrow keys over the history");
    RegisterConsoleTestVariables();

    CommandLine.SetText("Test.Console.");
    CommandLine.SetTextCursorPosition(13);
    CommandLine.RefreshCandidates();

    CommandLine.MoveSelectionUp();
    TEST_EXPECT(CommandLine.GetText().Equals("Test.Console."));
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), FConsoleCommandLine::InvalidIndex);
    TEST_EXPECT(CommandLine.GetSelectedCandidateIndex() != FConsoleCommandLine::InvalidIndex);

    TEST_END();
}

bool ConsoleSubmit_Test()
{
    TEST_BEGIN();

    RegisterConsoleTestVariables();
    FConsoleManager::Get().ClearHistory();

    FConsoleLogBuffer LogBuffer;

    TEST_SECTION("An empty line submits to nothing");
    FConsoleCommandLine EmptyLine;
    TEST_EXPECT(EmptyLine.Submit(LogBuffer) == EConsoleSubmitResult::Nothing);
    TEST_EXPECT_EQ(LogBuffer.GetNumLines(), 0);

    TEST_SECTION("A selected candidate is filled in rather than executed");
    FConsoleCommandLine SelectedLine;
    SelectedLine.SetText("Test.Console.I");
    SelectedLine.SetTextCursorPosition(14);
    SelectedLine.RefreshCandidates();
    SelectedLine.MoveSelectionUp();

    TEST_EXPECT(SelectedLine.Submit(LogBuffer) == EConsoleSubmitResult::AppliedCandidate);
    TEST_EXPECT(SelectedLine.GetText().Equals("Test.Console.Int"));
    TEST_EXPECT_EQ(SelectedLine.GetTextCursorPosition(), 16);
    TEST_EXPECT(!SelectedLine.HasCandidates());
    TEST_EXPECT_EQ(LogBuffer.GetNumLines(), 0);

    TEST_SECTION("A line with no selection executes, echoes and clears");
    FConsoleCommandLine ExecutedLine;
    ExecutedLine.SetText("Test.Console.Int 7");

    TEST_EXPECT(ExecutedLine.Submit(LogBuffer) == EConsoleSubmitResult::ExecutedCommand);
    TEST_EXPECT(ExecutedLine.GetText().IsEmpty());
    TEST_EXPECT_EQ(ExecutedLine.GetTextCursorPosition(), 0);

    TArray<FConsoleLogLine> Lines;
    LogBuffer.GetSnapshot(Lines);

    TEST_EXPECT(!Lines.IsEmpty());
    TEST_EXPECT(Lines[0].Message.Equals("Test.Console.Int 7"));

    TEST_SECTION("The command actually ran");
    if (IConsoleVariable* Variable = FConsoleManager::Get().FindConsoleVariable("Test.Console.Int"))
    {
        TEST_EXPECT_EQ(Variable->GetInt(), 7);
    }
    else
    {
        TEST_EXPECT(false);
    }

    TEST_SECTION("The executed line went into the history");
    const TArray<String>& History = FConsoleManager::Get().GetHistory();
    TEST_EXPECT(!History.IsEmpty());
    TEST_EXPECT(History.Last().Equals("Test.Console.Int 7"));

    TEST_END();
}

bool ConsoleLogBufferRing_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A new buffer is empty and holds the default cap");
    FConsoleLogBuffer LogBuffer;
    TEST_EXPECT_EQ(LogBuffer.GetNumLines(), 0);
    TEST_EXPECT_EQ(LogBuffer.GetMaxLines(), FConsoleLogBuffer::DefaultMaxLines);

    TEST_SECTION("Lines keep their severity");
    LogBuffer.Log(ELogSeverity::Info, "Info line");
    LogBuffer.Log(ELogSeverity::Warning, "Warning line");
    LogBuffer.Log(ELogSeverity::Error, "Error line");

    TArray<FConsoleLogLine> Lines;
    LogBuffer.GetSnapshot(Lines);

    TEST_EXPECT_EQ(Lines.Size(), 3);
    TEST_EXPECT(Lines[0].Severity == ELogSeverity::Info);
    TEST_EXPECT(Lines[1].Severity == ELogSeverity::Warning);
    TEST_EXPECT(Lines[2].Severity == ELogSeverity::Error);
    TEST_EXPECT(Lines[1].Message.Equals("Warning line"));

    TEST_SECTION("A line with no severity is treated as info");
    LogBuffer.Log(String("Plain line"));
    LogBuffer.GetSnapshot(Lines);
    TEST_EXPECT(Lines.Last().Severity == ELogSeverity::Info);

    TEST_SECTION("The revision moves on every change");
    const uint64 RevisionBefore = LogBuffer.GetRevision();
    LogBuffer.Log(ELogSeverity::Info, "Another line");
    TEST_EXPECT(LogBuffer.GetRevision() != RevisionBefore);

    TEST_SECTION("The buffer never grows past the cap and drops the oldest first");
    FConsoleLogBuffer RingBuffer;
    for (int32 Index = 0; Index < FConsoleLogBuffer::DefaultMaxLines + 10; ++Index)
    {
        RingBuffer.Log(ELogSeverity::Info, String::Printf("Line %d", Index));
    }

    TEST_EXPECT_EQ(RingBuffer.GetNumLines(), FConsoleLogBuffer::DefaultMaxLines);

    RingBuffer.GetSnapshot(Lines);
    TEST_EXPECT(Lines[0].Message.Equals("Line 10"));
    TEST_EXPECT(Lines.Last().Message.Equals("Line 109"));

    TEST_SECTION("Shrinking the cap trims immediately");
    RingBuffer.SetMaxLines(5);
    TEST_EXPECT_EQ(RingBuffer.GetMaxLines(), 5);
    TEST_EXPECT_EQ(RingBuffer.GetNumLines(), 5);

    RingBuffer.GetSnapshot(Lines);
    TEST_EXPECT(Lines[0].Message.Equals("Line 105"));

    TEST_SECTION("The cap is at least one line");
    RingBuffer.SetMaxLines(0);
    TEST_EXPECT_EQ(RingBuffer.GetMaxLines(), 1);
    TEST_EXPECT_EQ(RingBuffer.GetNumLines(), 1);

    TEST_SECTION("Clearing empties the buffer");
    RingBuffer.Clear();
    TEST_EXPECT_EQ(RingBuffer.GetNumLines(), 0);

    TEST_END();
}

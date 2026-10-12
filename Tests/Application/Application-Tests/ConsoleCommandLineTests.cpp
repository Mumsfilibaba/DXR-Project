#include "ConsoleCommandLineTests.h"
#include "ConsoleTestVariables.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Application/Console/ConsoleCommandLine.h>
#include <Application/Console/ConsoleLogBuffer.h>

static bool AreAllCandidatesPrefixed(const FConsoleCommandLine& CommandLine, const CHAR_T* Prefix)
{
    const int32 PrefixLength = FStringView(Prefix).Length();
    for (const TPair<IConsoleObject*, FString>& Candidate : CommandLine.GetCandidates())
    {
        if (Candidate.Second.Length() < PrefixLength)
        {
            return false;
        }

        if (!FStringView(Candidate.Second.Data(), PrefixLength).Equals(Prefix))
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

    TEST_SECTION(TEXT("An empty line has an empty word"));
    TEST_EXPECT(CommandLine.FindWordRangeAtTextCursor().IsEmpty());

    TEST_SECTION(TEXT("With no separator the word is the whole line"));
    CommandLine.SetText(TEXT("Command"));
    CommandLine.SetTextCursorPosition(7);

    FConsoleWordRange Range = CommandLine.FindWordRangeAtTextCursor();
    TEST_EXPECT_EQ(Range.Position, 0);
    TEST_EXPECT_EQ(Range.Length, 7);

    TEST_SECTION(TEXT("Each separator ends the word"));
    const CHAR_T Separators[] = { ' ', '\t', ',', ';' };
    for (const CHAR_T Separator : Separators)
    {
        FString Text(TEXT("First"));
        Text.Insert(Separator, Text.Length());
        Text.Insert(TEXT("Second"), Text.Length());

        CommandLine.SetText(Text);
        CommandLine.SetTextCursorPosition(Text.Length());

        Range = CommandLine.FindWordRangeAtTextCursor();
        TEST_EXPECT_EQ(Range.Position, 6);
        TEST_EXPECT_EQ(Range.Length, 6);
    }

    TEST_SECTION(TEXT("The word ends at the text cursor, not at the end of the line"));
    CommandLine.SetText(TEXT("Command Argument"));
    CommandLine.SetTextCursorPosition(4);

    Range = CommandLine.FindWordRangeAtTextCursor();
    TEST_EXPECT_EQ(Range.Position, 0);
    TEST_EXPECT_EQ(Range.Length, 4);

    TEST_SECTION(TEXT("A text cursor sitting on a separator has an empty word"));
    CommandLine.SetText(TEXT("Command "));
    CommandLine.SetTextCursorPosition(8);
    TEST_EXPECT(CommandLine.FindWordRangeAtTextCursor().IsEmpty());

    TEST_SECTION(TEXT("The text cursor clamps into the line"));
    CommandLine.SetText(TEXT("Ab"));
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

    TEST_SECTION(TEXT("An empty line finds nothing"));
    CommandLine.RefreshCandidates();
    TEST_EXPECT(!CommandLine.HasCandidates());
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), FConsoleCommandLine::InvalidIndex);

    TEST_SECTION(TEXT("A prefix finds every object under it"));
    CommandLine.SetText(TEXT("Test.Console."));
    CommandLine.SetTextCursorPosition(13);
    CommandLine.RefreshCandidates();

    TEST_EXPECT(CommandLine.HasCandidates());
    TEST_EXPECT(CommandLine.GetCandidates().Size() >= 4);
    TEST_EXPECT(AreAllCandidatesPrefixed(CommandLine, TEXT("Test.Console.")));

    TEST_SECTION(TEXT("Matching is case insensitive, as the old console was"));
    CommandLine.SetText(TEXT("test.console."));
    CommandLine.SetTextCursorPosition(13);
    CommandLine.RefreshCandidates();
    TEST_EXPECT(CommandLine.HasCandidates());

    TEST_SECTION(TEXT("A longer prefix narrows the list"));
    CommandLine.SetText(TEXT("Test.Console.Int"));
    CommandLine.SetTextCursorPosition(16);
    CommandLine.RefreshCandidates();

    TEST_EXPECT_EQ(CommandLine.GetCandidates().Size(), 1);
    TEST_EXPECT(CommandLine.GetCandidates()[0].Second.Equals(TEXT("Test.Console.Int")));

    TEST_SECTION(TEXT("A prefix matching nothing drops the previous list"));
    CommandLine.SetText(TEXT("Test.Console.NoSuchThing"));
    CommandLine.SetTextCursorPosition(24);
    CommandLine.RefreshCandidates();

    TEST_EXPECT(!CommandLine.HasCandidates());
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), FConsoleCommandLine::InvalidIndex);

    TEST_SECTION(TEXT("Only the word at the text cursor is matched"));
    CommandLine.SetText(TEXT("Something Test.Console.Int"));
    CommandLine.SetTextCursorPosition(26);
    CommandLine.RefreshCandidates();

    TEST_EXPECT_EQ(CommandLine.GetCandidates().Size(), 1);

    TEST_SECTION(TEXT("Invalidating drops the list and the selection"));
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
    CommandLine.SetText(TEXT("Test.Console."));
    CommandLine.SetTextCursorPosition(13);
    CommandLine.RefreshCandidates();

    const int32 NumCandidates = CommandLine.GetCandidates().Size();
    TEST_EXPECT(NumCandidates >= 4);

    TEST_SECTION(TEXT("Up from no selection wraps to the last candidate"));
    CommandLine.MoveSelectionUp();
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), NumCandidates - 1);
    TEST_EXPECT(CommandLine.ConsumeSelectionChanged());

    TEST_SECTION(TEXT("The change flag is cleared once it is read"));
    TEST_EXPECT(!CommandLine.ConsumeSelectionChanged());

    TEST_SECTION(TEXT("Up walks back through the list"));
    CommandLine.MoveSelectionUp();
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), NumCandidates - 2);

    TEST_SECTION(TEXT("Down wraps around from the last candidate"));
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

    TEST_SECTION(TEXT("A single candidate does not move, so the flag stays clear"));
    FConsoleCommandLine SingleCandidateLine;
    SingleCandidateLine.SetText(TEXT("Test.Console.Int"));
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

bool ConsoleCandidateDirectSelection_Test()
{
    TEST_BEGIN();

    RegisterConsoleTestVariables();

    FConsoleCommandLine CommandLine;
    CommandLine.SetText(TEXT("Test.Console."));
    CommandLine.SetTextCursorPosition(13);
    CommandLine.RefreshCandidates();

    const int32 NumCandidates = CommandLine.GetCandidates().Size();
    TEST_EXPECT(NumCandidates >= 4);

    TEST_SECTION(TEXT("Picking a candidate selects it and raises the flag"));
    CommandLine.SetSelectedCandidateIndex(2);
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), 2);
    TEST_EXPECT(CommandLine.ConsumeSelectionChanged());

    TEST_SECTION(TEXT("Picking the same candidate again leaves the flag clear"));
    CommandLine.SetSelectedCandidateIndex(2);
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), 2);
    TEST_EXPECT(!CommandLine.ConsumeSelectionChanged());

    TEST_SECTION(TEXT("An index past the end selects nothing rather than the last candidate"));
    CommandLine.SetSelectedCandidateIndex(NumCandidates);
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), FConsoleCommandLine::InvalidIndex);

    TEST_SECTION(TEXT("A negative index selects nothing"));
    CommandLine.SetSelectedCandidateIndex(1);
    CommandLine.SetSelectedCandidateIndex(-5);
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), FConsoleCommandLine::InvalidIndex);

    TEST_SECTION(TEXT("A pick the arrow keys can then walk on from"));
    CommandLine.SetSelectedCandidateIndex(0);
    CommandLine.MoveSelectionDown();
    TEST_EXPECT_EQ(CommandLine.GetSelectedCandidateIndex(), 1);

    TEST_SECTION(TEXT("Tab completes whatever the mouse picked"));
    CommandLine.SetSelectedCandidateIndex(0);

    const FString FirstCandidateName = CommandLine.GetCandidates()[0].Second;

    TEST_EXPECT(CommandLine.AcceptCompletion());
    TEST_EXPECT_EQ(CommandLine.GetText(), FirstCandidateName);

    TEST_SECTION(TEXT("Picking into an empty list selects nothing"));
    FConsoleCommandLine EmptyLine;
    EmptyLine.SetSelectedCandidateIndex(0);
    TEST_EXPECT_EQ(EmptyLine.GetSelectedCandidateIndex(), FConsoleCommandLine::InvalidIndex);
    TEST_EXPECT(!EmptyLine.ConsumeSelectionChanged());

    TEST_END();
}

bool ConsoleCompletion_Test()
{
    TEST_BEGIN();

    RegisterConsoleTestVariables();

    TEST_SECTION(TEXT("Tab does nothing without a selection"));
    FConsoleCommandLine CommandLine;
    CommandLine.SetText(TEXT("Test.Console.I"));
    CommandLine.SetTextCursorPosition(14);
    CommandLine.RefreshCandidates();

    TEST_EXPECT(CommandLine.HasCandidates());
    TEST_EXPECT(!CommandLine.AcceptCompletion());
    TEST_EXPECT(CommandLine.GetText().Equals(TEXT("Test.Console.I")));

    TEST_SECTION(TEXT("Tab with a selection completes the word and drops the list"));
    CommandLine.MoveSelectionUp();
    TEST_EXPECT(CommandLine.AcceptCompletion());
    TEST_EXPECT(CommandLine.GetText().Equals(TEXT("Test.Console.Int")));
    TEST_EXPECT_EQ(CommandLine.GetTextCursorPosition(), 16);
    TEST_EXPECT(!CommandLine.HasCandidates());

    TEST_SECTION(TEXT("Tab replaces only the word at the text cursor"));
    FConsoleCommandLine ArgumentLine;
    ArgumentLine.SetText(TEXT("Prefix Test.Console.I"));
    ArgumentLine.SetTextCursorPosition(21);
    ArgumentLine.RefreshCandidates();
    ArgumentLine.MoveSelectionUp();

    TEST_EXPECT(ArgumentLine.AcceptCompletion());
    TEST_EXPECT(ArgumentLine.GetText().Equals(TEXT("Prefix Test.Console.Int")));
    TEST_EXPECT_EQ(ArgumentLine.GetTextCursorPosition(), 23);

    TEST_SECTION(TEXT("Tab on an empty word does nothing"));
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

    CommandLine.SetText(TEXT("First.Command"));
    CommandLine.Submit(LogBuffer);

    CommandLine.SetText(TEXT("Second.Command"));
    CommandLine.Submit(LogBuffer);

    TEST_SECTION(TEXT("Submitting leaves the walk unstarted"));
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), FConsoleCommandLine::InvalidIndex);
    TEST_EXPECT(CommandLine.GetText().IsEmpty());

    TEST_SECTION(TEXT("Up starts at the newest entry"));
    CommandLine.MoveSelectionUp();
    TEST_EXPECT(CommandLine.GetText().Equals(TEXT("Second.Command")));
    TEST_EXPECT_EQ(CommandLine.GetTextCursorPosition(), CommandLine.GetText().Length());

    TEST_SECTION(TEXT("Up walks back to the oldest entry and clamps there"));
    CommandLine.MoveSelectionUp();
    TEST_EXPECT(CommandLine.GetText().Equals(TEXT("First.Command")));

    CommandLine.MoveSelectionUp();
    TEST_EXPECT(CommandLine.GetText().Equals(TEXT("First.Command")));
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), 0);

    TEST_SECTION(TEXT("Down walks forward again"));
    CommandLine.MoveSelectionDown();
    TEST_EXPECT(CommandLine.GetText().Equals(TEXT("Second.Command")));

    TEST_SECTION(TEXT("Down past the newest entry leaves the walk and clears the line"));
    CommandLine.MoveSelectionDown();
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), FConsoleCommandLine::InvalidIndex);
    TEST_EXPECT(CommandLine.GetText().IsEmpty());

    TEST_SECTION(TEXT("Down outside the walk does nothing"));
    CommandLine.MoveSelectionDown();
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), FConsoleCommandLine::InvalidIndex);
    TEST_EXPECT(CommandLine.GetText().IsEmpty());

    TEST_SECTION(TEXT("Reset drops the walk"));
    CommandLine.MoveSelectionUp();
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), 1);

    CommandLine.Reset();
    TEST_EXPECT_EQ(CommandLine.GetHistoryIndex(), FConsoleCommandLine::InvalidIndex);

    TEST_SECTION(TEXT("Candidates take the arrow keys over the history"));
    RegisterConsoleTestVariables();

    CommandLine.SetText(TEXT("Test.Console."));
    CommandLine.SetTextCursorPosition(13);
    CommandLine.RefreshCandidates();

    CommandLine.MoveSelectionUp();
    TEST_EXPECT(CommandLine.GetText().Equals(TEXT("Test.Console.")));
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

    TEST_SECTION(TEXT("An empty line submits to nothing"));
    FConsoleCommandLine EmptyLine;
    TEST_EXPECT(EmptyLine.Submit(LogBuffer) == EConsoleSubmitResult::Nothing);
    TEST_EXPECT_EQ(LogBuffer.GetNumLines(), 0);

    TEST_SECTION(TEXT("A selected candidate is filled in rather than executed"));
    FConsoleCommandLine SelectedLine;
    SelectedLine.SetText(TEXT("Test.Console.I"));
    SelectedLine.SetTextCursorPosition(14);
    SelectedLine.RefreshCandidates();
    SelectedLine.MoveSelectionUp();

    TEST_EXPECT(SelectedLine.Submit(LogBuffer) == EConsoleSubmitResult::AppliedCandidate);
    TEST_EXPECT(SelectedLine.GetText().Equals(TEXT("Test.Console.Int")));
    TEST_EXPECT_EQ(SelectedLine.GetTextCursorPosition(), 16);
    TEST_EXPECT(!SelectedLine.HasCandidates());
    TEST_EXPECT_EQ(LogBuffer.GetNumLines(), 0);

    TEST_SECTION(TEXT("A line with no selection executes, echoes and clears"));
    FConsoleCommandLine ExecutedLine;
    ExecutedLine.SetText(TEXT("Test.Console.Int 7"));

    TEST_EXPECT(ExecutedLine.Submit(LogBuffer) == EConsoleSubmitResult::ExecutedCommand);
    TEST_EXPECT(ExecutedLine.GetText().IsEmpty());
    TEST_EXPECT_EQ(ExecutedLine.GetTextCursorPosition(), 0);

    TArray<FConsoleLogLine> Lines;
    LogBuffer.GetSnapshot(Lines);

    TEST_EXPECT(!Lines.IsEmpty());
    TEST_EXPECT(Lines[0].Message.Equals(TEXT("Test.Console.Int 7")));

    TEST_SECTION(TEXT("The command actually ran"));
    if (IConsoleVariable* Variable = FConsoleManager::Get().FindConsoleVariable(TEXT("Test.Console.Int")))
    {
        TEST_EXPECT_EQ(Variable->GetInt(), 7);
    }
    else
    {
        TEST_EXPECT(false);
    }

    TEST_SECTION(TEXT("The executed line went into the history"));
    const TArray<FString>& History = FConsoleManager::Get().GetHistory();
    TEST_EXPECT(!History.IsEmpty());
    TEST_EXPECT(History.Last().Equals(TEXT("Test.Console.Int 7")));

    TEST_END();
}

bool ConsoleLogBufferRing_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("A new buffer is empty and holds the default cap"));
    FConsoleLogBuffer LogBuffer;
    TEST_EXPECT_EQ(LogBuffer.GetNumLines(), 0);
    TEST_EXPECT_EQ(LogBuffer.GetMaxLines(), FConsoleLogBuffer::DefaultMaxLines);

    TEST_SECTION(TEXT("Lines keep their severity"));
    LogBuffer.Log(ELogSeverity::Info, TEXT("Info line"));
    LogBuffer.Log(ELogSeverity::Warning, TEXT("Warning line"));
    LogBuffer.Log(ELogSeverity::Error, TEXT("Error line"));

    TArray<FConsoleLogLine> Lines;
    LogBuffer.GetSnapshot(Lines);

    TEST_EXPECT_EQ(Lines.Size(), 3);
    TEST_EXPECT(Lines[0].Severity == ELogSeverity::Info);
    TEST_EXPECT(Lines[1].Severity == ELogSeverity::Warning);
    TEST_EXPECT(Lines[2].Severity == ELogSeverity::Error);
    TEST_EXPECT(Lines[1].Message.Equals(TEXT("Warning line")));

    TEST_SECTION(TEXT("A line with no severity is treated as info"));
    LogBuffer.Log(FString(TEXT("Plain line")));
    LogBuffer.GetSnapshot(Lines);
    TEST_EXPECT(Lines.Last().Severity == ELogSeverity::Info);

    TEST_SECTION(TEXT("The revision moves on every change"));
    const uint64 RevisionBefore = LogBuffer.GetRevision();
    LogBuffer.Log(ELogSeverity::Info, TEXT("Another line"));
    TEST_EXPECT(LogBuffer.GetRevision() != RevisionBefore);

    TEST_SECTION(TEXT("The buffer never grows past the cap and drops the oldest first"));
    FConsoleLogBuffer RingBuffer;
    for (int32 Index = 0; Index < FConsoleLogBuffer::DefaultMaxLines + 10; ++Index)
    {
        RingBuffer.Log(ELogSeverity::Info, FString::Printf(TEXT("Line %d"), Index));
    }

    TEST_EXPECT_EQ(RingBuffer.GetNumLines(), FConsoleLogBuffer::DefaultMaxLines);

    RingBuffer.GetSnapshot(Lines);
    TEST_EXPECT(Lines[0].Message.Equals(TEXT("Line 10")));
    TEST_EXPECT(Lines.Last().Message.Equals(TEXT("Line 109")));

    TEST_SECTION(TEXT("Shrinking the cap trims immediately"));
    RingBuffer.SetMaxLines(5);
    TEST_EXPECT_EQ(RingBuffer.GetMaxLines(), 5);
    TEST_EXPECT_EQ(RingBuffer.GetNumLines(), 5);

    RingBuffer.GetSnapshot(Lines);
    TEST_EXPECT(Lines[0].Message.Equals(TEXT("Line 105")));

    TEST_SECTION(TEXT("The cap is at least one line"));
    RingBuffer.SetMaxLines(0);
    TEST_EXPECT_EQ(RingBuffer.GetMaxLines(), 1);
    TEST_EXPECT_EQ(RingBuffer.GetNumLines(), 1);

    TEST_SECTION(TEXT("Clearing empties the buffer"));
    RingBuffer.Clear();
    TEST_EXPECT_EQ(RingBuffer.GetNumLines(), 0);

    TEST_SECTION(TEXT("A view that caught up only takes the lines added since"));
    RingBuffer.SetMaxLines(4);
    RingBuffer.Log(ELogSeverity::Info, TEXT("A"));
    RingBuffer.Log(ELogSeverity::Info, TEXT("B"));

    const uint64 CaughtUp = RingBuffer.GetSnapshot(Lines);
    RingBuffer.Log(ELogSeverity::Info, TEXT("C"));

    bool         bIsContinuous = false;
    const uint64 NextRevision  = RingBuffer.GetLinesSince(CaughtUp, Lines, bIsContinuous);
    TEST_EXPECT(bIsContinuous);
    TEST_EXPECT_EQ(Lines.Size(), 1);
    TEST_EXPECT(Lines[0].Message.Equals(TEXT("C")));
    TEST_EXPECT_EQ(NextRevision, RingBuffer.GetRevision());

    TEST_SECTION(TEXT("Nothing new is an empty, continuous answer"));
    RingBuffer.GetLinesSince(NextRevision, Lines, bIsContinuous);
    TEST_EXPECT(bIsContinuous);
    TEST_EXPECT(Lines.IsEmpty());

    TEST_SECTION(TEXT("More new lines than the buffer holds means the view has to start over"));
    for (int32 Index = 0; Index < 6; ++Index)
    {
        RingBuffer.Log(ELogSeverity::Info, FString::Printf(TEXT("Flood %d"), Index));
    }

    RingBuffer.GetLinesSince(NextRevision, Lines, bIsContinuous);
    TEST_EXPECT(!bIsContinuous);
    TEST_EXPECT_EQ(Lines.Size(), 4);
    TEST_EXPECT(Lines.Last().Message.Equals(TEXT("Flood 5")));

    TEST_SECTION(TEXT("So does a clear, however few lines followed it"));
    const uint64 BeforeClear = RingBuffer.GetRevision();
    RingBuffer.Clear();
    RingBuffer.Log(ELogSeverity::Info, TEXT("After"));

    RingBuffer.GetLinesSince(BeforeClear, Lines, bIsContinuous);
    TEST_EXPECT(!bIsContinuous);
    TEST_EXPECT_EQ(Lines.Size(), 1);

    TEST_END();
}

#include "Application/Console/ConsoleCommandLine.h"
#include "Core/Math/Math.h"

bool FConsoleCommandLine::IsWordSeparator(CHAR Character)
{
    return Character == ' ' || Character == '\t' || Character == ',' || Character == ';';
}

FConsoleCommandLine::FConsoleCommandLine()
    : Text()
    , Candidates()
    , TextCursorPosition(0)
    , SelectedCandidateIndex(InvalidIndex)
    , HistoryIndex(InvalidIndex)
    , bSelectionChanged(false)
{
}

FConsoleCommandLine::~FConsoleCommandLine() = default;

void FConsoleCommandLine::SetText(const String& InText)
{
    Text               = InText;
    TextCursorPosition = Math::Clamp(TextCursorPosition, 0, Text.Length());
}

void FConsoleCommandLine::SetTextCursorPosition(int32 InTextCursorPosition)
{
    TextCursorPosition = Math::Clamp(InTextCursorPosition, 0, Text.Length());
}

FConsoleWordRange FConsoleCommandLine::FindWordRangeAtTextCursor() const
{
    const int32 WordEnd = Math::Clamp(TextCursorPosition, 0, Text.Length());

    int32 WordStart = WordEnd;
    while (WordStart > 0 && !IsWordSeparator(Text.Data()[WordStart - 1]))
    {
        WordStart--;
    }

    FConsoleWordRange Range;
    Range.Position = WordStart;
    Range.Length   = WordEnd - WordStart;
    return Range;
}

void FConsoleCommandLine::RefreshCandidates()
{
    InvalidateCandidates();

    const FConsoleWordRange Range = FindWordRangeAtTextCursor();
    if (Range.IsEmpty())
    {
        return;
    }

    const StringView CandidateName(Text.Data() + Range.Position, Range.Length);
    FConsoleManager::Get().FindCandidates(CandidateName, Candidates);

    if (!Candidates.IsEmpty())
    {
        HistoryIndex = InvalidIndex;
    }
}

void FConsoleCommandLine::InvalidateCandidates()
{
    Candidates.Clear();
    SelectedCandidateIndex = InvalidIndex;
}

bool FConsoleCommandLine::ConsumeSelectionChanged()
{
    const bool bResult = bSelectionChanged;
    bSelectionChanged  = false;
    return bResult;
}

void FConsoleCommandLine::MoveSelectionUp()
{
    if (Candidates.IsEmpty())
    {
        const int32           PreviousHistoryIndex = HistoryIndex;
        const TArray<String>& History              = FConsoleManager::Get().GetHistory();

        if (!History.IsEmpty())
        {
            if (HistoryIndex != InvalidIndex)
            {
                // Already walking, so step back and clamp at the oldest entry
                HistoryIndex = Math::Max(HistoryIndex - 1, 0);
            }
            else
            {
                HistoryIndex = History.LastIndex();
            }
        }
        else
        {
            HistoryIndex = InvalidIndex;
        }

        if (PreviousHistoryIndex != HistoryIndex)
        {
            ApplyHistoryText();
        }

        return;
    }

    const int32 PreviousSelectedCandidateIndex = SelectedCandidateIndex;
    if (SelectedCandidateIndex <= 0)
    {
        SelectedCandidateIndex = Candidates.LastIndex();
    }
    else
    {
        SelectedCandidateIndex--;
    }

    if (PreviousSelectedCandidateIndex != SelectedCandidateIndex)
    {
        bSelectionChanged = true;
    }
}

void FConsoleCommandLine::SetSelectedCandidateIndex(int32 Index)
{
    const int32 NewIndex = (Index >= 0 && Index < Candidates.Size()) ? Index : InvalidIndex;
    if (NewIndex == SelectedCandidateIndex)
    {
        return;
    }

    SelectedCandidateIndex = NewIndex;
    bSelectionChanged      = true;
}

void FConsoleCommandLine::MoveSelectionDown()
{
    if (Candidates.IsEmpty())
    {
        const int32           PreviousHistoryIndex = HistoryIndex;
        const TArray<String>& History              = FConsoleManager::Get().GetHistory();

        if (History.IsEmpty())
        {
            HistoryIndex = InvalidIndex;
        }
        else if (HistoryIndex != InvalidIndex)
        {
            // Walking past the newest entry leaves the history and clears the line
            HistoryIndex++;
            if (HistoryIndex >= History.Size())
            {
                HistoryIndex = InvalidIndex;
            }
        }

        if (PreviousHistoryIndex != HistoryIndex)
        {
            ApplyHistoryText();
        }

        return;
    }

    const int32 PreviousSelectedCandidateIndex = SelectedCandidateIndex;
    if (SelectedCandidateIndex >= Candidates.LastIndex())
    {
        SelectedCandidateIndex = 0;
    }
    else
    {
        SelectedCandidateIndex++;
    }

    if (PreviousSelectedCandidateIndex != SelectedCandidateIndex)
    {
        bSelectionChanged = true;
    }
}

bool FConsoleCommandLine::AcceptCompletion()
{
    const FConsoleWordRange Range = FindWordRangeAtTextCursor();
    if (Range.IsEmpty())
    {
        return false;
    }

    // Tab needs a selection, so it does nothing until an arrow key has picked a candidate
    if (Candidates.IsEmpty() || SelectedCandidateIndex <= InvalidIndex)
    {
        return false;
    }

    const String& CandidateName = Candidates[SelectedCandidateIndex].Second;
    Text.Remove(Range.Position, Range.Length);
    Text.Insert(*CandidateName, Range.Position);
    TextCursorPosition = Range.Position + CandidateName.Length();

    InvalidateCandidates();
    return true;
}

EConsoleSubmitResult FConsoleCommandLine::Submit(IOutputDevice& OutputDevice)
{
    EConsoleSubmitResult Result = EConsoleSubmitResult::Nothing;

    if (!Text.IsEmpty())
    {
        if (SelectedCandidateIndex >= 0 && !Candidates.IsEmpty())
        {
            // Enter on a selected candidate fills the line in rather than running it
            Text               = Candidates[SelectedCandidateIndex].Second;
            TextCursorPosition = Text.Length();
            Result             = EConsoleSubmitResult::AppliedCandidate;
        }
        else
        {
            FConsoleManager::Get().ExecuteCommand(OutputDevice, Text);
            Text.Clear();
            TextCursorPosition = 0;
            Result             = EConsoleSubmitResult::ExecutedCommand;
        }
    }

    InvalidateCandidates();
    HistoryIndex = InvalidIndex;
    return Result;
}

void FConsoleCommandLine::Reset()
{
    InvalidateCandidates();
    HistoryIndex      = InvalidIndex;
    bSelectionChanged = false;
}

void FConsoleCommandLine::ApplyHistoryText()
{
    const TArray<String>& History = FConsoleManager::Get().GetHistory();

    Text               = (HistoryIndex >= 0 && HistoryIndex < History.Size()) ? History[HistoryIndex] : String();
    TextCursorPosition = Text.Length();
}

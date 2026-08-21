#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/String.h"
#include "Core/Misc/ConsoleManager.h"

struct FConsoleWordRange
{
    FConsoleWordRange()
        : Position(0)
        , Length(0)
    {
    }

    /** @brief True when the range covers no characters. */
    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return Length <= 0;
    }

    int32 Position;
    int32 Length;
};

enum class EConsoleSubmitResult : uint8
{
    /** The line was empty, so nothing happened. */
    Nothing,

    /** A candidate was selected, so its name replaced the line instead of executing. */
    AppliedCandidate,

    /** The line went to FConsoleManager::ExecuteCommand and was cleared. */
    ExecutedCommand,
};

class APPLICATION_API FConsoleCommandLine
{
public:
    static constexpr int32 InvalidIndex = -1;

    /**
     * @brief True when the character ends a word for the purposes of candidate lookup.
     *
     * @param Character The character to test.
     * @return True when the character separates words.
     */
    NODISCARD static bool IsWordSeparator(CHAR Character);

public:
    FConsoleCommandLine();
    ~FConsoleCommandLine();

    /**
     * @brief Replaces the line, clamping the text cursor into it.
     *
     * @param InText The new line.
     */
    void SetText(const String& InText);

    /** @brief The line as it currently reads. */
    NODISCARD FORCEINLINE const String& GetText() const
    {
        return Text;
    }

    /**
     * @brief Moves the text cursor, clamped into the line.
     *
     * @param InTextCursorPosition The new text cursor index.
     */
    void SetTextCursorPosition(int32 InTextCursorPosition);

    /** @brief The text cursor index, in [0, Length]. */
    NODISCARD FORCEINLINE int32 GetTextCursorPosition() const
    {
        return TextCursorPosition;
    }

    /** @brief Finds the word the text cursor sits at the end of, scanning back to a separator. */
    NODISCARD FConsoleWordRange FindWordRangeAtTextCursor() const;

    /**
     * @brief Recomputes the candidate list from the word at the text cursor.
     *
     * The candidates are always dropped first and then refilled from FConsoleManager, and finding
     * any of them resets the history walk, so erasing the line and pressing up starts from the end
     * of the history again.
     */
    void RefreshCandidates();

    /** @brief Drops the candidate list and the selection. */
    void InvalidateCandidates();

    /** @brief The console objects whose names start with the word at the text cursor. */
    NODISCARD FORCEINLINE const TArray<TPair<IConsoleObject*, String>>& GetCandidates() const
    {
        return Candidates;
    }

    /** @brief The index of the selected candidate, or InvalidIndex when none is selected. */
    NODISCARD FORCEINLINE int32 GetSelectedCandidateIndex() const
    {
        return SelectedCandidateIndex;
    }

    /** @brief True when the candidate list is not empty. */
    NODISCARD FORCEINLINE bool HasCandidates() const
    {
        return !Candidates.IsEmpty();
    }

    /**
     * @brief Reports whether the selection moved since the last call, and clears the flag.
     *
     * @return True when the selection moved.
     */
    bool ConsumeSelectionChanged();

    /**
     * @brief Up arrow. Moves the candidate selection when a list is showing, and walks back through
     * the command history when it is not.
     */
    void MoveSelectionUp();

    /** @brief Down arrow, the mirror of MoveSelectionUp. */
    void MoveSelectionDown();

    /**
     * @brief Tab. Replaces the word at the text cursor with the selected candidate.
     *
     * A selection is required, so tab does nothing until an arrow key has picked a candidate. This
     * matches the old behavior.
     * @return True when the text changed.
     */
    bool AcceptCompletion();

    /**
     * @brief Enter. Applies the selected candidate, or executes the line and clears it.
     *
     * @param OutputDevice The device the command echoes and reports to.
     * @return What the submission did.
     */
    EConsoleSubmitResult Submit(IOutputDevice& OutputDevice);

    /** @brief Drops the candidate list and the history walk, as toggling the console does. */
    void Reset();

    /** @brief The index into the command history being shown, or InvalidIndex when not walking. */
    NODISCARD FORCEINLINE int32 GetHistoryIndex() const
    {
        return HistoryIndex;
    }

private:
    void ApplyHistoryText();

    String                                 Text;
    TArray<TPair<IConsoleObject*, String>> Candidates;
    int32                                  TextCursorPosition;
    int32                                  SelectedCandidateIndex;
    int32                                  HistoryIndex;
    bool                                   bSelectionChanged : 1;
};

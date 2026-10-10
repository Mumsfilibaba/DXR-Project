#pragma once
#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Misc/IOutputDevice.h>
#include <Core/Platform/CriticalSection.h>
#include <Core/Threading/Atomic/AtomicBool.h>
#include <Core/Threading/Runnable.h>

struct FCompletionCandidate
{
    FString Name;
    FString Kind; // "command", "variable", or "remote console command" for the ':' commands
};

class FConsoleLineEditor final : public FRunnable
{
public:
    FConsoleLineEditor();
    ~FConsoleLineEditor();

    /** @brief Switches a console into character input and starts the input thread. */
    bool Initialize();

    /** @brief Puts the console back the way it was. The input thread stays parked in its read until the process ends. */
    void Release();

    /** @return True when the user types on a console, false when the input is piped in. */
    NODISCARD bool IsInteractive() const
    {
        return bIsInteractive;
    }

    /** @return True once the input has ended, which is EOF for piped input and Ctrl+D or Ctrl+Z on a console. */
    NODISCARD bool IsFinished() const
    {
        return bFinished.Load();
    }

    /** @brief Takes the oldest line the user submitted. */
    bool PopLine(FString& OutLine);

    /** @return True while submitted lines are waiting for PopLine. */
    NODISCARD bool HasPendingLines();

    /** @brief Prints above the line being edited, any thread. */
    void Print(ELogSeverity Severity, const FString& Message);

    /** @brief Prints above the line being edited in the default color, any thread. */
    void Print(const FString& Message);

    void SetPrompt(const FString& InPrompt);

    /** @brief Replaces the console object names that Tab completes. */
    void SetCompletions(TArray<FCompletionCandidate>&& InCandidates);

    /** @return True once, after a Tab found nothing, so the caller can fetch the names again in case new ones were registered. */
    NODISCARD bool ConsumeCompletionRefreshRequest()
    {
        return bCompletionRefreshRequested.Exchange(false);
    }

    virtual int32 Run() override final;

private:
    enum class EKey : uint8
    {
        None,
        Character,
        Enter,
        Backspace,
        Delete,
        Left,
        Right,
        Up,
        Down,
        Home,
        End,
        Tab,
        Escape,
        EndOfInput,
    };

    struct FKeyPress
    {
        EKey   Key         = EKey::None;
        CHAR_T Character   = 0;
        int32  RepeatCount = 1;
    };

    void RunInteractive();
    void RunLineMode();
    void HandleKey(const FKeyPress& KeyPress);
    void SubmitLine();
    void Complete();
    void MoveThroughHistory(int32 Direction);
    void EraseInputLine();
    void RedrawInputLine();

    FCriticalSection             EditorCS;
    FString                      Prompt;
    FString                      Buffer;
    int32                        CursorPosition;
    TArray<FString>              History;
    int32                        HistoryIndex;
    FString                      HistoryDraft;
    TArray<FCompletionCandidate> Candidates;
    FString                      LastListedBuffer;
    TArray<FString>              SubmittedLines;
    AtomicBool                   bFinished;
    AtomicBool                   bCompletionRefreshRequested;
    bool                         bIsInteractive;
    bool                         bIsRawModeActive;
};

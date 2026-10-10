#include "ConsoleLineEditor.h"
#include <Core/Math/Math.h>
#include <Core/Misc/OutputDeviceManager.h>
#include <Core/Misc/RemoteConsoleProtocol.h>
#include <Core/Platform/PlatformThread.h>
#include <Core/Templates/CString.h>
#include <Core/Threading/ScopedLock.h>

#if PLATFORM_WINDOWS
    #include <Core/Windows/Windows.h>
#else
    #include <poll.h>
    #include <signal.h>
    #include <stdlib.h>
    #include <sys/ioctl.h>
    #include <termios.h>
    #include <unistd.h>
#endif

static constexpr int32 GMaxListedCandidates      = 64;
static constexpr int32 GCandidateNameColumnWidth = 56;
static constexpr int32 GDefaultTerminalWidth     = 80;
static constexpr int32 GMinTerminalWidth         = 20;
static constexpr int32 GMinVisibleInputLength    = 8;
static constexpr int32 GInputChunkSize           = 1024;
static constexpr CHAR GControlD                  = 0x04;
static constexpr CHAR GControlZ                  = 0x1A;
static constexpr CHAR GBackspaceCharacter        = 0x08;
static constexpr CHAR GEscapeCharacter           = 0x1B;
static constexpr CHAR GDeleteCharacter           = 0x7F;
static constexpr CHAR GFirstPrintable            = 0x20;

#if !PLATFORM_WINDOWS
static constexpr int32 GSignalExitCodeBase                = 128;
static constexpr int32 GEscapeSequenceTimeoutMilliseconds = 30;
static constexpr int32 GWaitForever                       = -1;
static constexpr CHAR  GControlSequenceIntroducer         = '[';
static constexpr CHAR  GSingleShiftIntroducer             = 'O';
static constexpr CHAR  GCursorUpCode                      = 'A';
static constexpr CHAR  GCursorDownCode                    = 'B';
static constexpr CHAR  GCursorRightCode                   = 'C';
static constexpr CHAR  GCursorLeftCode                    = 'D';
static constexpr CHAR  GCursorHomeCode                    = 'H';
static constexpr CHAR  GCursorEndCode                     = 'F';
static constexpr CHAR  GEditSequenceTerminator            = '~';
static constexpr CHAR  GEditDeleteCode                    = '3';
static constexpr CHAR  GEditHomeCode                      = '1';
static constexpr CHAR  GEditHomeCodeAlt                   = '7';
static constexpr CHAR  GEditEndCode                       = '4';
static constexpr CHAR  GEditEndCodeAlt                    = '8';
static constexpr CHAR  GEraseLineSequence[]               = "\r\x1b[K";

#endif

static const FCompletionCandidate GMetaCommands[] =
{
    { ":help", "remote console command" },
    { ":quit", "remote console command" },
    { ":list", "remote console command" },
    { ":get",  "remote console command" },
    { ":log",  "remote console command" },
};

#if PLATFORM_WINDOWS

static HANDLE GInputHandle       = nullptr;
static HANDLE GOutputHandle      = nullptr;
static DWORD  GOriginalInputMode = 0;
static bool   GHasOriginalMode   = false;

static void RestoreConsoleMode()
{
    if (GHasOriginalMode)
    {
        ::SetConsoleMode(GInputHandle, GOriginalInputMode);
    }
}

static BOOL WINAPI RestoreOnConsoleControl(DWORD)
{
    RestoreConsoleMode();
    return FALSE;
}

static bool IsConsoleAttached()
{
    GInputHandle  = ::GetStdHandle(STD_INPUT_HANDLE);
    GOutputHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD Mode = 0;
    return ::GetConsoleMode(GInputHandle, &Mode) && ::GetConsoleMode(GOutputHandle, &Mode);
}

static bool EnterCharacterMode()
{
    if (!::GetConsoleMode(GInputHandle, &GOriginalInputMode))
    {
        return false;
    }

    GHasOriginalMode = true;
    ::SetConsoleCtrlHandler(&RestoreOnConsoleControl, TRUE);

    DWORD Mode = GOriginalInputMode;
    Mode &= ~static_cast<DWORD>(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT);
    Mode |= ENABLE_PROCESSED_INPUT;
    return ::SetConsoleMode(GInputHandle, Mode) != 0;
}

static void WriteRaw(const CHAR* Data, int32 Length)
{
    if (Length > 0)
    {
        ::WriteConsoleA(GOutputHandle, Data, static_cast<DWORD>(Length), nullptr, nullptr);
    }
}

static int32 GetTerminalWidth()
{
    CONSOLE_SCREEN_BUFFER_INFO Info;
    if (::GetConsoleScreenBufferInfo(GOutputHandle, &Info))
    {
        return static_cast<int32>(Info.srWindow.Right - Info.srWindow.Left + 1);
    }

    return GDefaultTerminalWidth;
}

static int32 ReadInputBytes(CHAR* Buffer, int32 Size)
{
    DWORD BytesRead = 0;
    if (!::ReadFile(::GetStdHandle(STD_INPUT_HANDLE), Buffer, static_cast<DWORD>(Size), &BytesRead, nullptr))
    {
        return -1;
    }

    return static_cast<int32>(BytesRead);
}

#else

static termios GOriginalTermios;
static bool    GHasOriginalMode = false;

static void RestoreConsoleMode()
{
    if (GHasOriginalMode)
    {
        ::tcsetattr(STDIN_FILENO, TCSANOW, &GOriginalTermios);
    }
}

static void RestoreOnSignal(int Signal)
{
    RestoreConsoleMode();
    ::_exit(GSignalExitCodeBase + Signal);
}

static bool IsConsoleAttached()
{
    return ::isatty(STDIN_FILENO) && ::isatty(STDOUT_FILENO);
}

static bool EnterCharacterMode()
{
    if (::tcgetattr(STDIN_FILENO, &GOriginalTermios) != 0)
    {
        return false;
    }

    GHasOriginalMode = true;
    ::atexit(&RestoreConsoleMode);
    ::signal(SIGINT, &RestoreOnSignal);
    ::signal(SIGTERM, &RestoreOnSignal);
    ::signal(SIGHUP, &RestoreOnSignal);

    termios Raw = GOriginalTermios;
    Raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO);
    Raw.c_cc[VMIN]  = 1;
    Raw.c_cc[VTIME] = 0;
    return ::tcsetattr(STDIN_FILENO, TCSANOW, &Raw) == 0;
}

static void WriteRaw(const CHAR* Data, int32 Length)
{
    while (Length > 0)
    {
        const ssize_t Written = ::write(STDOUT_FILENO, Data, static_cast<size_t>(Length));
        if (Written <= 0)
        {
            return;
        }

        Data   += Written;
        Length -= static_cast<int32>(Written);
    }
}

static int32 GetTerminalWidth()
{
    winsize Size = {};
    if ((::ioctl(STDOUT_FILENO, TIOCGWINSZ, &Size) == 0) && (Size.ws_col > 0))
    {
        return static_cast<int32>(Size.ws_col);
    }

    return GDefaultTerminalWidth;
}

static int32 ReadInputBytes(CHAR* Buffer, int32 Size)
{
    return static_cast<int32>(::read(STDIN_FILENO, Buffer, static_cast<size_t>(Size)));
}

static bool ReadByte(CHAR& OutByte, int32 TimeoutMilliseconds)
{
    if (TimeoutMilliseconds >= 0)
    {
        pollfd PollFd = {};
        PollFd.fd     = STDIN_FILENO;
        PollFd.events = POLLIN;
        if (::poll(&PollFd, 1, TimeoutMilliseconds) <= 0)
        {
            return false;
        }
    }

    return ::read(STDIN_FILENO, &OutByte, 1) == 1;
}

#endif

static void WriteRaw(const String& Text)
{
    WriteRaw(Text.Data(), Text.Length());
}

FConsoleLineEditor::FConsoleLineEditor()
    : EditorCS()
    , Prompt("> ")
    , Buffer()
    , CursorPosition(0)
    , History()
    , HistoryIndex(0)
    , HistoryDraft()
    , Candidates()
    , LastListedBuffer()
    , SubmittedLines()
    , bFinished(false)
    , bCompletionRefreshRequested(false)
    , bIsInteractive(false)
    , bIsRawModeActive(false)
{
}

FConsoleLineEditor::~FConsoleLineEditor() = default;

bool FConsoleLineEditor::Initialize()
{
    bIsInteractive = IsConsoleAttached() && EnterCharacterMode();
    bIsRawModeActive = bIsInteractive;

    IPlatformThread* InputThread = FPlatformThread::Create(this, "RemoteConsoleInput");
    if (!InputThread || !InputThread->Start())
    {
        Release();
        return false;
    }

    if (bIsInteractive)
    {
        TScopedLock Lock(EditorCS);
        RedrawInputLine();
    }

    return true;
}

void FConsoleLineEditor::Release()
{
    TScopedLock Lock(EditorCS);
    if (bIsRawModeActive)
    {
        EraseInputLine();
        RestoreConsoleMode();
        bIsRawModeActive = false;
    }
}

bool FConsoleLineEditor::PopLine(String& OutLine)
{
    TScopedLock Lock(EditorCS);
    if (SubmittedLines.IsEmpty())
    {
        return false;
    }

    OutLine = ::Move(SubmittedLines[0]);
    SubmittedLines.RemoveAt(0);
    return true;
}

bool FConsoleLineEditor::HasPendingLines()
{
    TScopedLock Lock(EditorCS);
    return !SubmittedLines.IsEmpty();
}

void FConsoleLineEditor::Print(ELogSeverity Severity, const String& Message)
{
    TScopedLock Lock(EditorCS);
    EraseInputLine();
    FOutputDeviceManager::Get()->Log(Severity, Message);
    RedrawInputLine();
}

void FConsoleLineEditor::Print(const String& Message)
{
    TScopedLock Lock(EditorCS);
    EraseInputLine();
    FOutputDeviceManager::Get()->Log(Message);
    RedrawInputLine();
}

void FConsoleLineEditor::SetPrompt(const String& InPrompt)
{
    TScopedLock Lock(EditorCS);
    EraseInputLine();
    Prompt = InPrompt;
    RedrawInputLine();
}

void FConsoleLineEditor::SetCompletions(TArray<FCompletionCandidate>&& InCandidates)
{
    TScopedLock Lock(EditorCS);
    Candidates = ::Move(InCandidates);
}

int32 FConsoleLineEditor::Run()
{
    if (bIsInteractive)
    {
        RunInteractive();
    }
    else
    {
        RunLineMode();
    }

    bFinished.Store(true);
    return 0;
}

void FConsoleLineEditor::RunInteractive()
{
    for (;;)
    {
        FKeyPress KeyPress;

#if PLATFORM_WINDOWS
        INPUT_RECORD Record;
        DWORD        NumRead = 0;

        if (!::ReadConsoleInputA(GInputHandle, &Record, 1, &NumRead))
        {
            return;
        }

        if ((NumRead == 0) || (Record.EventType != KEY_EVENT) || !Record.Event.KeyEvent.bKeyDown)
        {
            continue;
        }

        const KEY_EVENT_RECORD& KeyEvent = Record.Event.KeyEvent;
        KeyPress.RepeatCount = Math::Max<int32>(1, KeyEvent.wRepeatCount);

        switch (KeyEvent.wVirtualKeyCode)
        {
            case VK_RETURN: KeyPress.Key = EKey::Enter;     break;
            case VK_BACK:   KeyPress.Key = EKey::Backspace; break;
            case VK_DELETE: KeyPress.Key = EKey::Delete;    break;
            case VK_LEFT:   KeyPress.Key = EKey::Left;      break;
            case VK_RIGHT:  KeyPress.Key = EKey::Right;     break;
            case VK_UP:     KeyPress.Key = EKey::Up;        break;
            case VK_DOWN:   KeyPress.Key = EKey::Down;      break;
            case VK_HOME:   KeyPress.Key = EKey::Home;      break;
            case VK_END:    KeyPress.Key = EKey::End;       break;
            case VK_TAB:    KeyPress.Key = EKey::Tab;       break;
            case VK_ESCAPE: KeyPress.Key = EKey::Escape;    break;
            default:
            {
                const CHAR Character = KeyEvent.uChar.AsciiChar;
                if ((Character == GControlD) || (Character == GControlZ))
                {
                    KeyPress.Key = EKey::EndOfInput;
                }
                else if (static_cast<uint8>(Character) >= GFirstPrintable)
                {
                    KeyPress.Key       = EKey::Character;
                    KeyPress.Character = Character;
                }

                break;
            }
        }
#else
        CHAR Byte = 0;
        if (!ReadByte(Byte, GWaitForever))
        {
            return;
        }

        switch (Byte)
        {
            case '\r':
            case '\n': KeyPress.Key = EKey::Enter;      break;
            case GDeleteCharacter:
            case GBackspaceCharacter: KeyPress.Key = EKey::Backspace;  break;
            case '\t': KeyPress.Key = EKey::Tab;        break;
            case GControlD: KeyPress.Key = EKey::EndOfInput; break;
            case GEscapeCharacter:
            {
                CHAR Introducer = 0;
                CHAR Code       = 0;

                if (!ReadByte(Introducer, GEscapeSequenceTimeoutMilliseconds) || ((Introducer != GControlSequenceIntroducer) && (Introducer != GSingleShiftIntroducer)) || !ReadByte(Code, GEscapeSequenceTimeoutMilliseconds))
                {
                    KeyPress.Key = EKey::Escape;
                    break;
                }

                switch (Code)
                {
                    case GCursorUpCode:    KeyPress.Key = EKey::Up;    break;
                    case GCursorDownCode:  KeyPress.Key = EKey::Down;  break;
                    case GCursorRightCode: KeyPress.Key = EKey::Right; break;
                    case GCursorLeftCode:  KeyPress.Key = EKey::Left;  break;
                    case GCursorHomeCode:  KeyPress.Key = EKey::Home;  break;
                    case GCursorEndCode:   KeyPress.Key = EKey::End;   break;
                    default:
                    {
                        CHAR Terminator = 0;
                        while (ReadByte(Terminator, GEscapeSequenceTimeoutMilliseconds) && (Terminator != GEditSequenceTerminator))
                        {
                        }

                        if (Code == GEditDeleteCode)
                        {
                            KeyPress.Key = EKey::Delete;
                        }
                        else if ((Code == GEditHomeCode) || (Code == GEditHomeCodeAlt))
                        {
                            KeyPress.Key = EKey::Home;
                        }
                        else if ((Code == GEditEndCode) || (Code == GEditEndCodeAlt))
                        {
                            KeyPress.Key = EKey::End;
                        }

                        break;
                    }
                }

                break;
            }
            default:
            {
                if (static_cast<uint8>(Byte) >= GFirstPrintable)
                {
                    KeyPress.Key       = EKey::Character;
                    KeyPress.Character = Byte;
                }

                break;
            }
        }
#endif

        if (KeyPress.Key == EKey::None)
        {
            continue;
        }

        TScopedLock Lock(EditorCS);
        if ((KeyPress.Key == EKey::EndOfInput) && Buffer.IsEmpty())
        {
            EraseInputLine();
            return;
        }

        for (int32 Repeat = 0; Repeat < KeyPress.RepeatCount; ++Repeat)
        {
            HandleKey(KeyPress);
        }
    }
}

void FConsoleLineEditor::RunLineMode()
{
    CHAR   Chunk[GInputChunkSize];
    String Pending;

    for (;;)
    {
        const int32 BytesRead = ReadInputBytes(Chunk, sizeof(Chunk));
        if (BytesRead <= 0)
        {
            break;
        }

        Pending.Append(Chunk, BytesRead);

        TArray<String> NewLines;
        RemoteConsoleProtocol::ExtractLines(Pending, NewLines);

        TScopedLock Lock(EditorCS);
        for (String& Line : NewLines)
        {
            SubmittedLines.Add(::Move(Line));
        }
    }

    if (!Pending.IsEmpty())
    {
        TScopedLock Lock(EditorCS);
        SubmittedLines.Add(::Move(Pending));
    }
}

void FConsoleLineEditor::HandleKey(const FKeyPress& KeyPress)
{
    switch (KeyPress.Key)
    {
        case EKey::Character:
        {
            Buffer.Insert(KeyPress.Character, CursorPosition);
            ++CursorPosition;
            break;
        }
        case EKey::Backspace:
        {
            if (CursorPosition > 0)
            {
                Buffer.Remove(CursorPosition - 1, 1);
                --CursorPosition;
            }

            break;
        }
        case EKey::Delete:
        {
            if (CursorPosition < Buffer.Length())
            {
                Buffer.Remove(CursorPosition, 1);
            }

            break;
        }
        case EKey::Left:  CursorPosition = Math::Max(0, CursorPosition - 1);               break;
        case EKey::Right: CursorPosition = Math::Min(Buffer.Length(), CursorPosition + 1); break;
        case EKey::Home:  CursorPosition = 0;                                              break;
        case EKey::End:   CursorPosition = Buffer.Length();                                break;
        case EKey::Up:    MoveThroughHistory(-1);                                          break;
        case EKey::Down:  MoveThroughHistory(1);                                           break;
        case EKey::Tab:   Complete();                                                      break;
        case EKey::Escape:
        {
            Buffer.Clear();
            CursorPosition = 0;
            break;
        }
        case EKey::Enter:
        {
            SubmitLine();
            return;
        }
        default:
        {
            return;
        }
    }

    EraseInputLine();
    RedrawInputLine();
}

void FConsoleLineEditor::SubmitLine()
{
    const String Line = Buffer;
    EraseInputLine();
    WriteRaw(Prompt);
    WriteRaw(Line);
    WriteRaw("\n", 1);

    String Trimmed = Line;
    Trimmed.TrimInline();
    if (!Trimmed.IsEmpty() && (History.IsEmpty() || (History.Last() != Line)))
    {
        History.Add(Line);
    }

    HistoryIndex = History.Size();
    HistoryDraft.Clear();

    Buffer.Clear();
    CursorPosition = 0;

    SubmittedLines.Add(Line);
    RedrawInputLine();
}

void FConsoleLineEditor::Complete()
{
    int32 WordStart = CursorPosition;
    while ((WordStart > 0) && (Buffer[WordStart - 1] != ' '))
    {
        --WordStart;
    }

    int32 WordEnd = CursorPosition;
    while ((WordEnd < Buffer.Length()) && (Buffer[WordEnd] != ' '))
    {
        ++WordEnd;
    }

    const String Prefix(Buffer.Data() + WordStart, CursorPosition - WordStart);

    bool bMetaCommands  = false;
    bool bVariablesOnly = false;

    if (WordStart == 0)
    {
        bMetaCommands = Prefix.StartsWith(":");
    }
    else
    {
        int32 FirstWordEnd = 0;
        while ((FirstWordEnd < Buffer.Length()) && (Buffer[FirstWordEnd] != ' '))
        {
            ++FirstWordEnd;
        }

        const String FirstWord(Buffer.Data(), FirstWordEnd);

        int32 SecondWordStart = FirstWordEnd;
        while ((SecondWordStart < Buffer.Length()) && (Buffer[SecondWordStart] == ' '))
        {
            ++SecondWordStart;
        }

        const bool bIsSecondWord = (WordStart == SecondWordStart);
        if (!bIsSecondWord)
        {
            return;
        }

        if (FirstWord.Equals(":get", EStringCaseType::NoCase))
        {
            bVariablesOnly = true;
        }
        else if (!FirstWord.Equals(":list", EStringCaseType::NoCase))
        {
            return;
        }
    }

    TArray<const FCompletionCandidate*> Matches;
    if (bMetaCommands)
    {
        for (const FCompletionCandidate& Candidate : GMetaCommands)
        {
            if (Candidate.Name.StartsWith(*Prefix, Prefix.Length(), EStringCaseType::NoCase))
            {
                Matches.Add(&Candidate);
            }
        }
    }
    else
    {
        for (const FCompletionCandidate& Candidate : Candidates)
        {
            if ((!bVariablesOnly || (Candidate.Kind == "variable")) && Candidate.Name.StartsWith(*Prefix, Prefix.Length(), EStringCaseType::NoCase))
            {
                Matches.Add(&Candidate);
            }
        }
    }

    if (Matches.IsEmpty())
    {
        bCompletionRefreshRequested.Store(true);
        return;
    }

    const String& First = Matches[0]->Name;
    int32 CommonLength = First.Length();
    for (const FCompletionCandidate* Match : Matches)
    {
        int32 Length = 0;
        while ((Length < CommonLength) && (Length < Match->Name.Length()) && (TCharTraits<CHAR>::ToLower(Match->Name[Length]) == TCharTraits<CHAR>::ToLower(First[Length])))
        {
            ++Length;
        }

        CommonLength = Length;
    }

    if ((Matches.Size() == 1) || (CommonLength > Prefix.Length()))
    {
        String Replacement(First.Data(), CommonLength);
        if ((Matches.Size() == 1) && (WordEnd == Buffer.Length()))
        {
            Replacement.Append(' ');
        }

        const String Before(Buffer.Data(), WordStart);
        const String After(Buffer.Data() + WordEnd, Buffer.Length() - WordEnd);

        Buffer = Before;
        Buffer.Append(Replacement);
        CursorPosition = Buffer.Length();
        Buffer.Append(After);
        return;
    }

    if (Buffer == LastListedBuffer)
    {
        return;
    }

    LastListedBuffer = Buffer;
    EraseInputLine();

    const int32 NumListed = Math::Min(Matches.Size(), GMaxListedCandidates);
    for (int32 Index = 0; Index < NumListed; ++Index)
    {
        FOutputDeviceManager::Get()->Log(String::Printf("  %-*s %s", GCandidateNameColumnWidth, *Matches[Index]->Name, *Matches[Index]->Kind));
    }

    if (Matches.Size() > NumListed)
    {
        FOutputDeviceManager::Get()->Log(String::Printf("  ... and %d more, keep typing to narrow it down", Matches.Size() - NumListed));
    }
}

void FConsoleLineEditor::MoveThroughHistory(int32 Direction)
{
    if (History.IsEmpty())
    {
        return;
    }

    if (HistoryIndex == History.Size())
    {
        HistoryDraft = Buffer;
    }

    const int32 NewIndex = Math::Clamp(HistoryIndex + Direction, 0, History.Size());
    if (NewIndex == HistoryIndex)
    {
        return;
    }

    HistoryIndex   = NewIndex;
    Buffer         = (HistoryIndex == History.Size()) ? HistoryDraft : History[HistoryIndex];
    CursorPosition = Buffer.Length();
}

void FConsoleLineEditor::EraseInputLine()
{
    if (!bIsRawModeActive)
    {
        return;
    }

#if PLATFORM_WINDOWS
    String Blank;
    Blank.Append('\r');
    for (int32 Index = 1; Index < GetTerminalWidth(); ++Index)
    {
        Blank.Append(' ');
    }

    Blank.Append('\r');
    WriteRaw(Blank);
#else
    WriteRaw(GEraseLineSequence, ARRAY_COUNT(GEraseLineSequence) - 1);
#endif
}

void FConsoleLineEditor::RedrawInputLine()
{
    if (!bIsRawModeActive || bFinished.Load())
    {
        return;
    }

    const int32 Width     = Math::Max(GetTerminalWidth(), GMinTerminalWidth);
    const int32 Available = Math::Max(Width - Prompt.Length() - 1, GMinVisibleInputLength);
    const int32 Start     = (CursorPosition > Available) ? (CursorPosition - Available) : 0;
    const int32 Visible   = Math::Min(Buffer.Length() - Start, Available);

    String Line = Prompt;
    Line.Append(Buffer.Data() + Start, Visible);
    WriteRaw(Line);

    String Back;
    for (int32 Index = Start + Visible; Index > CursorPosition; --Index)
    {
        Back.Append('\b');
    }

    WriteRaw(Back);
}

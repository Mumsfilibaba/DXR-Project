#include "ConsoleLineEditor.h"
#include <LaunchProgram/ProgramEntry.h>
#include <Core/Containers/Function.h>
#include <Core/Containers/Optional.h>
#include <Core/Containers/UniquePtr.h>
#include <Core/Json/JsonReader.h>
#include <Core/Misc/OutputDeviceLogger.h>
#include <Core/Misc/RemoteConsoleProtocol.h>
#include <Core/Network/NetworkSocket.h>
#include <Core/Platform/CriticalSection.h>
#include <Core/Platform/PlatformThread.h>
#include <Core/Platform/PlatformThreadMisc.h>
#include <Core/Platform/PlatformTime.h>
#include <Core/Threading/Atomic/AtomicBool.h>
#include <Core/Templates/CString.h>
#include <Core/Templates/NumericLimits.h>
#include <Core/Threading/ScopedLock.h>

#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();

static constexpr uint32      GConnectAttemptMilliseconds     = 2000;
static constexpr uint32      GRetryDelayMilliseconds         = 250;
static constexpr uint32      GInteractivePollMilliseconds    = 30;
static constexpr uint32      GInteractiveAttemptMilliseconds = 1000;
static constexpr uint32      GReconnectDelayMilliseconds     = 500;
static constexpr uint32      GRejectedRetryMilliseconds      = 3000;
static constexpr int32       GReceiveChunkSize               = 64 * 1024;
static constexpr uint32      GDefaultTimeoutMilliseconds     = 30000;
static constexpr uint64      GMillisecondsPerSecond          = 1000;
static constexpr uint32      GMaxTimeoutMilliseconds         = static_cast<uint32>(TNumericLimits<int32>::Max());
static constexpr uint32      GMaxWaitSeconds                 = 24 * 60 * 60;
static constexpr const CHAR* GListCommand                    = ":list";
static constexpr const CHAR* GGetCommand                     = ":get";
static FConsoleLineEditor*   GLineEditor                     = nullptr;

enum class ERemoteConsoleExitCode : int32
{
    Success          = 0,
    CommandFailed    = 1,
    ConnectionFailed = 2,
    InvalidUsage     = 3,
};

static int32 ToExitCode(ERemoteConsoleExitCode Code)
{
    return static_cast<int32>(Code);
}

static void Print(ELogSeverity Severity, const String& Message)
{
    if (GLineEditor)
    {
        GLineEditor->Print(Severity, Message);
    }
    else
    {
        FOutputDeviceLogger::Get()->Log(Severity, Message);
    }
}

static void Print(const String& Message)
{
    if (GLineEditor)
    {
        GLineEditor->Print(Message);
    }
    else
    {
        FOutputDeviceLogger::Get()->Log(Message);
    }
}

static uint64 GetMilliseconds()
{
    return (FPlatformTime::QueryPerformanceCounter() * GMillisecondsPerSecond) / FPlatformTime::QueryPerformanceFrequency();
}

struct FRemoteConsoleOptions
{
    String            Host                = RemoteConsoleProtocol::LoopbackAddress;
    uint16            Port                = RemoteConsoleProtocol::DefaultPort;
    uint32            TimeoutMilliseconds = GDefaultTimeoutMilliseconds;
    uint32            WaitSeconds         = 0;     // Keeps retrying the connection this long while the engine boots
    bool              bJsonOutput         = false; // Prints the raw response lines instead of formatted text
    bool              bShowHelp           = false;
    TArray<String>    ExecCommands;                // From -exec="CmdA;CmdB", which may be given more than once
    TOptional<String> ListFilter;                  // From -list or -list=Filter
    String            GetName;                     // From -get=Name

    NODISCARD bool IsOneShot() const
    {
        return !ExecCommands.IsEmpty() || ListFilter.HasValue() || !GetName.IsEmpty();
    }
};

enum class EConnectResult
{
    Connected,
    UnknownHost, // The host is neither an address nor a name that resolves
    Unreachable, // Nothing accepted the connection, so the engine is not running or not listening
    Rejected,    // The engine refused this client; GetRejectReason says why
    NoGreeting,  // Something accepted the connection but did not speak the protocol
};

class FRemoteConsoleConnection
{
public:
    FRemoteConsoleConnection();
    ~FRemoteConsoleConnection();

    /** @brief Connects and reads the greeting, retrying for WaitSeconds while nothing accepts. Prints nothing. */
    EConnectResult Connect(const String& Host, uint16 Port, uint32 WaitSeconds, uint32 AttemptMilliseconds, uint32 TimeoutMilliseconds);

    /** @return The id the response will carry, or -1 when the send failed */
    int64 SendRequest(const CHAR* Type, FJsonValue&& Payload);

    /** @brief Pumps the socket until the result with RequestId arrives. Log events received meanwhile are printed. */
    bool WaitForResult(int64 RequestId, uint32 TimeoutMilliseconds, FJsonValue& OutResult, String& OutLine);

    /** @brief Interactive mode. Reads whatever has arrived within the timeout and prints log events. */
    bool Poll(uint32 TimeoutMilliseconds);

    NODISCARD const FJsonValue& GetHello() const { return Hello; }
    NODISCARD const String& GetRejectReason() const { return RejectReason; }
    NODISCARD bool IsClosed() const { return bIsClosed; }

    /** @brief Receives results that arrive while nothing is waiting for their id, such as a background 'list'. */
    void SetOnUnsolicitedResult(const TFunction<void(const FJsonValue&)>& InCallback) { OnUnsolicitedResult = InCallback; }

    NODISCARD bool IsLogSubscribed() const { return bIsLogSubscribed; }
    void SetLogSubscribed(bool bInLogSubscribed) { bIsLogSubscribed = bInLogSubscribed; }

    void SetJsonOutput(bool bInJsonOutput) { bJsonOutput = bInJsonOutput; }

private:
    bool ReadNextMessage(uint64 DeadlineMilliseconds, FJsonValue& OutMessage, String& OutLine);
    void Receive(uint32 TimeoutMilliseconds);
    void PrintEvent(const FJsonValue& Event, const String& Line);
    bool SendAll(const String& Line);

    void HandleMessage(const FJsonValue& Message, const String& Line);

    FNetworkSocket                      Socket;
    String                              ReceiveBuffer;
    TArray<String>                      PendingLines;
    int64                               NextRequestId;
    FJsonValue                          Hello;
    String                              RejectReason;
    TFunction<void(const FJsonValue&)>  OnUnsolicitedResult;
    bool                                bJsonOutput;
    bool                                bIsClosed;
    bool                                bIsLogSubscribed;
};

FRemoteConsoleConnection::FRemoteConsoleConnection()
    : Socket(ESocketType::TCP)
    , ReceiveBuffer()
    , PendingLines()
    , NextRequestId(1)
    , Hello()
    , RejectReason()
    , OnUnsolicitedResult()
    , bJsonOutput(false)
    , bIsClosed(false)
    , bIsLogSubscribed(false)
{
}

FRemoteConsoleConnection::~FRemoteConsoleConnection() = default;

EConnectResult FRemoteConsoleConnection::Connect(const String& Host, uint16 Port, uint32 WaitSeconds, uint32 AttemptMilliseconds, uint32 TimeoutMilliseconds)
{
    const uint64 RetryDeadline = GetMilliseconds() + (static_cast<uint64>(WaitSeconds) * GMillisecondsPerSecond);

    TArray<FSocketAddress> Addresses;
    if (!FNetworkSocket::Resolve(Host, Port, Addresses))
    {
        return EConnectResult::UnknownHost;
    }

    for (;;)
    {
        if (Socket.ConnectToAny(Addresses, AttemptMilliseconds))
        {
            break;
        }

        if (GetMilliseconds() >= RetryDeadline)
        {
            Socket.Close();
            return EConnectResult::Unreachable;
        }

        FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(GRetryDelayMilliseconds));
    }

    FJsonValue Greeting;
    String     Line;
    if (!ReadNextMessage(GetMilliseconds() + TimeoutMilliseconds, Greeting, Line))
    {
        Socket.Close();
        return EConnectResult::NoGreeting;
    }

    const String Type = Greeting.Find("type") ? Greeting.Find("type")->GetStringOr("") : String();
    if (Type == RemoteConsoleProtocol::TypeRejected)
    {
        const FJsonValue* Reason = Greeting.Find("reason");
        RejectReason = Reason ? Reason->GetStringOr("") : String("no reason given");
        Socket.Close();
        return EConnectResult::Rejected;
    }

    if (Type != RemoteConsoleProtocol::TypeHello)
    {
        Socket.Close();
        return EConnectResult::NoGreeting;
    }

    Hello = ::Move(Greeting);
    return EConnectResult::Connected;
}

int64 FRemoteConsoleConnection::SendRequest(const CHAR* Type, FJsonValue&& Payload)
{
    const int64 RequestId = NextRequestId++;

    FJsonValue Request = ::Move(Payload);
    Request.AddMember("id", FJsonValue(RequestId));
    Request.AddMember("type", FJsonValue(Type));

    return SendAll(RemoteConsoleProtocol::ToLine(Request)) ? RequestId : -1;
}

bool FRemoteConsoleConnection::WaitForResult(int64 RequestId, uint32 TimeoutMilliseconds, FJsonValue& OutResult, String& OutLine)
{
    const uint64 Deadline = GetMilliseconds() + TimeoutMilliseconds;

    FJsonValue Message;
    String     Line;
    while (ReadNextMessage(Deadline, Message, Line))
    {
        const FJsonValue* Type = Message.Find("type");
        const FJsonValue* Id   = Message.Find("id");
        if (Type && (Type->GetStringOr("") == RemoteConsoleProtocol::TypeResult) && Id && (Id->GetInt64Or(-1) == RequestId))
        {
            OutResult = ::Move(Message);
            OutLine   = ::Move(Line);
            return true;
        }

        HandleMessage(Message, Line);
    }

    return false;
}

bool FRemoteConsoleConnection::Poll(uint32 TimeoutMilliseconds)
{
    const uint64 Deadline = GetMilliseconds() + TimeoutMilliseconds;

    FJsonValue Message;
    String     Line;
    while (ReadNextMessage(Deadline, Message, Line))
    {
        HandleMessage(Message, Line);
    }

    return !bIsClosed;
}

void FRemoteConsoleConnection::HandleMessage(const FJsonValue& Message, const String& Line)
{
    const FJsonValue* Type = Message.Find("type");
    if (Type && (Type->GetStringOr("") == RemoteConsoleProtocol::TypeResult))
    {
        if (OnUnsolicitedResult)
        {
            OnUnsolicitedResult(Message);
        }

        return;
    }

    PrintEvent(Message, Line);
}

bool FRemoteConsoleConnection::ReadNextMessage(uint64 DeadlineMilliseconds, FJsonValue& OutMessage, String& OutLine)
{
    for (;;)
    {
        while (!PendingLines.IsEmpty())
        {
            String Line = ::Move(PendingLines[0]);
            PendingLines.RemoveAt(0);

            FJsonReader Reader;
            if (Reader.Parse(StringView(Line.Data(), Line.Length()), OutMessage) && OutMessage.IsObject())
            {
                OutLine = ::Move(Line);
                return true;
            }
        }

        const uint64 Now = GetMilliseconds();
        if (bIsClosed || (Now >= DeadlineMilliseconds))
        {
            return false;
        }

        Receive(static_cast<uint32>(DeadlineMilliseconds - Now));
    }
}

void FRemoteConsoleConnection::Receive(uint32 TimeoutMilliseconds)
{
    if (!Socket.IsOpen())
    {
        bIsClosed = true;
        return;
    }

    FNetworkSocket* Sockets[]  = { &Socket };
    bool            bReadable  = false;
    const int32     NumReady   = FNetworkSocket::WaitForRead(Sockets, 1, &bReadable, TimeoutMilliseconds);
    if (NumReady < 0)
    {
        bIsClosed = true;
        return;
    }

    if (!bReadable)
    {
        return;
    }

    TArray<CHAR> Chunk;
    Chunk.Resize(GReceiveChunkSize);

    int32 BytesRead = 0;
    if (Socket.Recv(Chunk.Data(), Chunk.Size(), BytesRead) != ESocketResult::Success)
    {
        bIsClosed = true;
        return;
    }

    ReceiveBuffer.Append(Chunk.Data(), BytesRead);
    RemoteConsoleProtocol::ExtractLines(ReceiveBuffer, PendingLines);
}

void FRemoteConsoleConnection::PrintEvent(const FJsonValue& Event, const String& Line)
{
    const FJsonValue* Type = Event.Find("type");
    if (!Type || (Type->GetStringOr("") != RemoteConsoleProtocol::TypeLog))
    {
        return;
    }

    if (bJsonOutput)
    {
        String TrimmedLine = Line;
        TrimmedLine.TrimInline();
        Print(ELogSeverity::Info, TrimmedLine);
        return;
    }

    ELogSeverity Severity = ELogSeverity::Info;
    if (const FJsonValue* SeverityValue = Event.Find("severity"))
    {
        RemoteConsoleProtocol::SeverityFromString(SeverityValue->GetStringOr("info"), Severity);
    }

    const FJsonValue* Message = Event.Find("message");
    Print(Severity, Message ? Message->GetStringOr("") : String());
}

bool FRemoteConsoleConnection::SendAll(const String& Line)
{
    int32 Offset = 0;
    while (Socket.IsOpen() && (Offset < Line.Length()))
    {
        int32 BytesSent = 0;
        const ESocketResult Result = Socket.Send(Line.Data() + Offset, Line.Length() - Offset, BytesSent);
        if (Result == ESocketResult::WouldBlock)
        {
            FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(1));
            continue;
        }
        else if (Result != ESocketResult::Success)
        {
            bIsClosed = true;
            return false;
        }

        Offset += BytesSent;
    }

    return Socket.IsOpen();
}

static bool ParseUnsigned(const String& Text, uint32 MaxValue, uint32& OutValue)
{
    int64 Value = 0;
    if (Text.IsEmpty() || !TTypeFromString<int64>::FromString(Text, Value) || (Value < 0) || (Value > static_cast<int64>(MaxValue)))
    {
        return false;
    }

    OutValue = static_cast<uint32>(Value);
    return true;
}

static void SplitCommands(const String& Text, TArray<String>& OutCommands)
{
    int32 Start = 0;
    while (Start <= Text.Length())
    {
        int32 End = Start;
        while ((End < Text.Length()) && (Text[End] != ';'))
        {
            ++End;
        }

        String Command(Text.Data() + Start, End - Start);
        Command.TrimInline();
        if (!Command.IsEmpty())
        {
            OutCommands.Add(::Move(Command));
        }

        Start = End + 1;
    }
}

static bool IsOptionArgument(const String& Argument)
{
    return !Argument.IsEmpty() && (Argument[0] == '-');
}

static bool OptionTakesValue(const String& Name)
{
    static const CHAR* ValueOptions[] = { "host", "port", "timeout", "wait", "exec", "list", "get" };
    for (const CHAR* ValueOption : ValueOptions)
    {
        if (Name.Equals(ValueOption, EStringCaseType::NoCase))
        {
            return true;
        }
    }

    return false;
}

static bool ParseOptions(FRemoteConsoleOptions& OutOptions)
{
    const TArray<String>& Arguments = GProgramArgs;
    for (int32 ArgumentIndex = 0; ArgumentIndex < Arguments.Size(); ++ArgumentIndex)
    {
        const String& Argument = Arguments[ArgumentIndex];

        int32 NameStart = 0;
        while ((NameStart < Argument.Length()) && (Argument[NameStart] == '-'))
        {
            ++NameStart;
        }

        if (NameStart == 0)
        {
            Print(ELogSeverity::Error, String::Printf("[RemoteConsole] Unexpected argument '%s'", *Argument));
            return false;
        }

        int32 NameEnd = NameStart;
        while ((NameEnd < Argument.Length()) && (Argument[NameEnd] != '='))
        {
            ++NameEnd;
        }

        const String Name(Argument.Data() + NameStart, NameEnd - NameStart);
        bool         bHasValue = NameEnd < Argument.Length();
        String       Value     = bHasValue ? String(Argument.Data() + NameEnd + 1, Argument.Length() - NameEnd - 1) : String();

        if (bHasValue)
        {
            while ((ArgumentIndex + 1 < Arguments.Size()) && Arguments[ArgumentIndex + 1].StartsWith("."))
            {
                Value.Append(Arguments[++ArgumentIndex]);
            }
        }
        else if (OptionTakesValue(Name) && (ArgumentIndex + 1 < Arguments.Size()) && !IsOptionArgument(Arguments[ArgumentIndex + 1]))
        {
            Value     = Arguments[++ArgumentIndex];
            bHasValue = true;
        }

        uint32 Number = 0;
        if (Name.Equals("help", EStringCaseType::NoCase) || Name.Equals("?"))
        {
            OutOptions.bShowHelp = true;
        }
        else if (Name.Equals("host", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            OutOptions.Host = Value;
        }
        else if (Name.Equals("port", EStringCaseType::NoCase) && ParseUnsigned(Value, RemoteConsoleProtocol::MaxPort, Number) && (Number >= RemoteConsoleProtocol::MinPort))
        {
            OutOptions.Port = static_cast<uint16>(Number);
        }
        else if (Name.Equals("timeout", EStringCaseType::NoCase) && ParseUnsigned(Value, GMaxTimeoutMilliseconds, Number))
        {
            OutOptions.TimeoutMilliseconds = Number;
        }
        else if (Name.Equals("wait", EStringCaseType::NoCase) && ParseUnsigned(Value, GMaxWaitSeconds, Number))
        {
            OutOptions.WaitSeconds = Number;
        }
        else if (Name.Equals("json", EStringCaseType::NoCase) && !bHasValue)
        {
            OutOptions.bJsonOutput = true;
        }
        else if (Name.Equals("exec", EStringCaseType::NoCase) && bHasValue)
        {
            SplitCommands(Value, OutOptions.ExecCommands);
        }
        else if (Name.Equals("list", EStringCaseType::NoCase))
        {
            OutOptions.ListFilter = Value;
        }
        else if (Name.Equals("get", EStringCaseType::NoCase) && !Value.IsEmpty())
        {
            OutOptions.GetName = Value;
        }
        else
        {
            Print(ELogSeverity::Error, String::Printf("[RemoteConsole] Unknown or malformed option '%s'", *Argument));
            return false;
        }
    }

    return true;
}

static void PrintUsage()
{
    Print(String::Printf(
        "Usage: RemoteConsole [options]\n"
        "\n"
        "Runs console commands in a running engine as if they were typed into its console.\n"
        "With -exec, -list or -get it runs them and exits. Without them it opens an interactive console that waits for\n"
        "the engine to start, reconnects whenever the engine restarts, and keeps running until you leave it.\n"
        "\n"
        "Options:\n"
        "  -host=<address>     Engine to connect to: a name, an IPv4 or an IPv6 address (default %s)\n"
        "  -port=<port>        Port the engine listens on (default %u, RemoteConsole.Port)\n"
        "  -timeout=<ms>       How long to wait for each response (default %u)\n"
        "  -wait=<seconds>     With -exec, -list or -get, keep retrying while the engine starts (default 0)\n"
        "  -exec=\"A;B\"         Run commands in order; may be given more than once\n"
        "  -list[=filter]      List console commands and variables whose name contains the filter\n"
        "  -get=<name>         Print the value of a console variable\n"
        "  -json               Print the raw JSON responses, one per line\n"
        "\n"
        "Values may also follow as the next argument, as in -get Engine.ViewportWidth.\n"
        "\n"
        "Exit codes: 0 success, 1 a command failed, 2 could not connect, 3 invalid usage\n"
        "\n"
        "Interactive console:\n"
        "  Tab                 Complete a command or variable name; press it again to list the choices\n"
        "  Up / Down           Walk through the lines entered before\n"
        "  Escape              Clear the line\n"
        "  :list [filter]      List console commands and variables\n"
        "  :get <name>         Print the value of a console variable\n"
        "  :log on|off         Stream the engine's whole log\n"
        "  :help               Show this text\n"
        "  :quit               Leave; Ctrl+C, or Ctrl+D / Ctrl+Z on an empty line, also leave",
        RemoteConsoleProtocol::LoopbackAddress,
        static_cast<uint32>(RemoteConsoleProtocol::DefaultPort),
        GDefaultTimeoutMilliseconds));
}

/** @return True when Line is Command, alone or followed by a space, with whatever follows trimmed into OutArgument. */
static bool ParseCommandArgument(const String& Line, const CHAR* Command, String& OutArgument)
{
    const int32 CommandLength = CString::Strlen(Command);
    if (!Line.StartsWith(Command, CommandLength, EStringCaseType::NoCase))
    {
        return false;
    }

    if ((Line.Length() > CommandLength) && (Line[CommandLength] != ' '))
    {
        return false;
    }

    OutArgument = String(Line.Data() + CommandLength, Line.Length() - CommandLength);
    OutArgument.TrimInline();
    return true;
}

static bool CheckResult(const FJsonValue& Result)
{
    const FJsonValue* Ok = Result.Find("ok");
    return Ok && Ok->GetBoolOr(false);
}

static void PrintError(const FJsonValue& Result)
{
    if (const FJsonValue* Error = Result.Find("error"))
    {
        Print(ELogSeverity::Error, Error->GetStringOr(""));
    }
}

static void PrintExecOutput(const FJsonValue& Result, const String& EchoToSkip)
{
    if (const FJsonValue* Output = Result.Find("output"))
    {
        for (int32 Index = 0; Index < Output->Num(); ++Index)
        {
            const FJsonValue& Line = (*Output)[Index];

            ELogSeverity Severity = ELogSeverity::Info;
            if (const FJsonValue* SeverityValue = Line.Find("severity"))
            {
                RemoteConsoleProtocol::SeverityFromString(SeverityValue->GetStringOr("info"), Severity);
            }

            const FJsonValue* Message = Line.Find("message");
            const String      Text    = Message ? Message->GetStringOr("") : String();

            if ((Index == 0) && !EchoToSkip.IsEmpty() && (Text == EchoToSkip))
            {
                continue;
            }

            Print(Severity, Text);
        }
    }

    PrintError(Result);
}

static void PrintListResult(const FJsonValue& Result)
{
    const FJsonValue* Objects = Result.Find("objects");
    if (!Objects)
    {
        PrintError(Result);
        return;
    }

    for (int32 Index = 0; Index < Objects->Num(); ++Index)
    {
        const FJsonValue& Object = (*Objects)[Index];

        const FJsonValue* Name  = Object.Find("name");
        const FJsonValue* Help  = Object.Find("help");
        const FJsonValue* Value = Object.Find("value");

        const String NameText = Name ? Name->GetStringOr("") : String();
        const String HelpText = Help ? Help->GetStringOr("") : String();
        if (Value)
        {
            Print(ELogSeverity::Info, String::Printf("%s = %s    %s", *NameText, *Value->GetStringOr(""), *HelpText));
        }
        else
        {
            Print(ELogSeverity::Info, String::Printf("%s    %s", *NameText, *HelpText));
        }
    }

    Print(ELogSeverity::Info, String::Printf("%d console objects", Objects->Num()));
}

static void PrintGetResult(const FJsonValue& Result)
{
    const FJsonValue* Name      = Result.Find("name");
    const FJsonValue* Value     = Result.Find("value");
    const FJsonValue* ValueType = Result.Find("valueType");
    const FJsonValue* SetBy     = Result.Find("setBy");
    if (!Name || !Value)
    {
        PrintError(Result);
        return;
    }

    Print(ELogSeverity::Info, String::Printf("%s = %s (%s, set by %s)",
        *Name->GetStringOr(""),
        *Value->GetStringOr(""),
        ValueType ? *ValueType->GetStringOr("") : "unknown",
        SetBy ? *SetBy->GetStringOr("") : "unknown"));
}

enum class ERequestOutcome
{
    Succeeded,
    Failed,
    ConnectionLost,
};

typedef TFunction<void(const FJsonValue&)> FPrintResult;

static ERequestOutcome RunRequest(FRemoteConsoleConnection& Connection, const FRemoteConsoleOptions& Options, const CHAR* Type, FJsonValue&& Payload, FPrintResult PrintResult)
{
    const int64 RequestId = Connection.SendRequest(Type, ::Move(Payload));
    if (RequestId < 0)
    {
        Print(ELogSeverity::Error, "[RemoteConsole] Lost the connection to the engine");
        return ERequestOutcome::ConnectionLost;
    }

    FJsonValue Result;
    String     ResultLine;
    if (!Connection.WaitForResult(RequestId, Options.TimeoutMilliseconds, Result, ResultLine))
    {
        if (Connection.IsClosed())
        {
            Print(ELogSeverity::Error, "[RemoteConsole] Lost the connection to the engine");
            return ERequestOutcome::ConnectionLost;
        }

        Print(ELogSeverity::Error, String::Printf("[RemoteConsole] No response within %u ms (raise it with -timeout)", Options.TimeoutMilliseconds));
        return ERequestOutcome::Failed;
    }

    if (Options.bJsonOutput)
    {
        Print(ELogSeverity::Info, ResultLine.Trim());
    }
    else
    {
        PrintResult(Result);
    }

    return CheckResult(Result) ? ERequestOutcome::Succeeded : ERequestOutcome::Failed;
}

static ERequestOutcome RunExec(FRemoteConsoleConnection& Connection, const FRemoteConsoleOptions& Options, const String& Command, bool bSkipEcho)
{
    FJsonValue Payload = FJsonValue::MakeObject();
    Payload.AddMember("command", FJsonValue(Command));

    const String EchoToSkip = bSkipEcho ? Command : String();
    const FPrintResult PrintResult = Connection.IsLogSubscribed()
        ? FPrintResult(&PrintError)
        : FPrintResult([EchoToSkip](const FJsonValue& Result) { PrintExecOutput(Result, EchoToSkip); });

    return RunRequest(Connection, Options, RemoteConsoleProtocol::TypeExec, ::Move(Payload), PrintResult);
}

static ERequestOutcome RunList(FRemoteConsoleConnection& Connection, const FRemoteConsoleOptions& Options, const String& Filter)
{
    FJsonValue Payload = FJsonValue::MakeObject();
    Payload.AddMember("filter", FJsonValue(Filter));
    return RunRequest(Connection, Options, RemoteConsoleProtocol::TypeList, ::Move(Payload), FPrintResult(&PrintListResult));
}

static ERequestOutcome RunGet(FRemoteConsoleConnection& Connection, const FRemoteConsoleOptions& Options, const String& Name)
{
    FJsonValue Payload = FJsonValue::MakeObject();
    Payload.AddMember("name", FJsonValue(Name));
    return RunRequest(Connection, Options, RemoteConsoleProtocol::TypeGet, ::Move(Payload), FPrintResult(&PrintGetResult));
}

static ERequestOutcome RunSubscribeLog(FRemoteConsoleConnection& Connection, const FRemoteConsoleOptions& Options, bool bEnabled)
{
    FJsonValue Payload = FJsonValue::MakeObject();
    Payload.AddMember("enabled", FJsonValue(bEnabled));

    const ERequestOutcome Outcome = RunRequest(Connection, Options, RemoteConsoleProtocol::TypeSubscribeLog, ::Move(Payload), FPrintResult(&PrintError));
    if (Outcome == ERequestOutcome::Succeeded)
    {
        Connection.SetLogSubscribed(bEnabled);
    }

    return Outcome;
}

static int32 RunOneShot(FRemoteConsoleConnection& Connection, const FRemoteConsoleOptions& Options)
{
    bool bAnyFailed = false;

    auto Record = [&bAnyFailed](ERequestOutcome Outcome) -> bool
    {
        bAnyFailed |= (Outcome != ERequestOutcome::Succeeded);
        return Outcome != ERequestOutcome::ConnectionLost;
    };

    for (const String& Command : Options.ExecCommands)
    {
        if (!Record(RunExec(Connection, Options, Command, false)))
        {
            return ToExitCode(ERemoteConsoleExitCode::ConnectionFailed);
        }
    }

    if (Options.ListFilter.HasValue() && !Record(RunList(Connection, Options, Options.ListFilter.GetValue())))
    {
        return ToExitCode(ERemoteConsoleExitCode::ConnectionFailed);
    }

    if (!Options.GetName.IsEmpty() && !Record(RunGet(Connection, Options, Options.GetName)))
    {
        return ToExitCode(ERemoteConsoleExitCode::ConnectionFailed);
    }

    return ToExitCode(bAnyFailed ? ERemoteConsoleExitCode::CommandFailed : ERemoteConsoleExitCode::Success);
}

static String DescribeEngine(const FJsonValue& Hello)
{
    auto Field = [&Hello](const CHAR* Name) -> String
    {
        const FJsonValue* Value = Hello.Find(Name);
        return Value ? Value->GetStringOr("") : String();
    };

    return String::Printf("%s %s (%s, %s)", *Field("engine"), *Field("version"), *Field("project"), *Field("config"));
}

static int64 RequestCompletions(FRemoteConsoleConnection& Connection)
{
    return Connection.SendRequest(RemoteConsoleProtocol::TypeList, FJsonValue::MakeObject());
}

static void ApplyCompletions(FConsoleLineEditor& Editor, const FJsonValue& Result)
{
    const FJsonValue* Objects = Result.Find("objects");
    if (!Objects)
    {
        return;
    }

    TArray<FCompletionCandidate> Candidates;
    Candidates.Reserve(Objects->Num());

    for (int32 Index = 0; Index < Objects->Num(); ++Index)
    {
        const FJsonValue& Object = (*Objects)[Index];
        const FJsonValue* Name   = Object.Find("name");
        const FJsonValue* Kind   = Object.Find("kind");
        if (Name)
        {
            Candidates.Add(FCompletionCandidate{ Name->GetStringOr(""), Kind ? Kind->GetStringOr("") : String() });
        }
    }

    Editor.SetCompletions(::Move(Candidates));
}

static int32 RunInteractive(const FRemoteConsoleOptions& Options)
{
    FConsoleLineEditor* Editor = new FConsoleLineEditor();
    if (!Editor->Initialize())
    {
        Print(ELogSeverity::Error, "[RemoteConsole] Failed to start reading the input");
        return ToExitCode(ERemoteConsoleExitCode::InvalidUsage);
    }

    GLineEditor = Editor;

    const String Address       = FSocketAddress::FormatHostAndPort(Options.Host, Options.Port);
    const String OnlinePrompt  = "> ";
    const String OfflinePrompt = "(offline) > ";

    Editor->SetPrompt(OfflinePrompt);
    Print(String::Printf("RemoteConsole: waiting for the engine on %s. Tab completes names, Up and Down walk the history, :help lists the commands", *Address));

    TUniquePtr<FRemoteConsoleConnection> Connection;
    String LastRejectReason;
    uint64 NextAttemptMilliseconds = 0;
    int64  CompletionRequestId     = -1;
    bool   bWantsLogStream         = false;
    bool   bAnyFailed              = false;
    bool   bReportedUnknownHost    = false;

    const auto Finish = [&]() -> int32
    {
        GLineEditor = nullptr;
        Editor->Release();
        return ToExitCode(bAnyFailed ? ERemoteConsoleExitCode::CommandFailed : ERemoteConsoleExitCode::Success);
    };

    const auto OnConnectionLost = [&]()
    {
        Connection.Reset();
        CompletionRequestId     = -1;
        NextAttemptMilliseconds = GetMilliseconds() + GReconnectDelayMilliseconds;

        Editor->SetPrompt(OfflinePrompt);
        Print(ELogSeverity::Warning, String::Printf("Lost the connection to the engine on %s, waiting for it to come back", *Address));
    };

    for (;;)
    {
        if (!Connection && (GetMilliseconds() >= NextAttemptMilliseconds))
        {
            TUniquePtr<FRemoteConsoleConnection> Attempt = MakeUniquePtr<FRemoteConsoleConnection>();
            Attempt->SetJsonOutput(Options.bJsonOutput);

            const EConnectResult Result = Attempt->Connect(Options.Host, Options.Port, 0, GInteractiveAttemptMilliseconds, Options.TimeoutMilliseconds);
            if (Result == EConnectResult::Connected)
            {
                Connection = ::Move(Attempt);
                Connection->SetOnUnsolicitedResult([Editor, &CompletionRequestId](const FJsonValue& Unsolicited)
                {
                    const FJsonValue* Id = Unsolicited.Find("id");
                    if (Id && (CompletionRequestId >= 0) && (Id->GetInt64Or(-1) == CompletionRequestId))
                    {
                        CompletionRequestId = -1;
                        ApplyCompletions(*Editor, Unsolicited);
                    }
                });

                LastRejectReason.Clear();
                Editor->SetPrompt(OnlinePrompt);
                Print(ELogSeverity::Info, String::Printf("Connected to %s on %s", *DescribeEngine(Connection->GetHello()), *Address));

                CompletionRequestId = RequestCompletions(*Connection);
                if (bWantsLogStream)
                {
                    FJsonValue Payload = FJsonValue::MakeObject();
                    Payload.AddMember("enabled", FJsonValue(true));
                    Connection->SendRequest(RemoteConsoleProtocol::TypeSubscribeLog, ::Move(Payload));
                    Connection->SetLogSubscribed(true);
                }
            }
            else if (Result == EConnectResult::Rejected)
            {
                if (Attempt->GetRejectReason() != LastRejectReason)
                {
                    LastRejectReason = Attempt->GetRejectReason();
                    Print(ELogSeverity::Warning, String::Printf("The engine on %s refused the connection: %s. Retrying every few seconds", *Address, *LastRejectReason));
                }

                NextAttemptMilliseconds = GetMilliseconds() + GRejectedRetryMilliseconds;
            }
            else if (Result == EConnectResult::UnknownHost)
            {
                if (!bReportedUnknownHost)
                {
                    bReportedUnknownHost = true;
                    Print(ELogSeverity::Warning, String::Printf("Could not resolve '%s'. Retrying every few seconds", *Options.Host));
                }

                NextAttemptMilliseconds = GetMilliseconds() + GRejectedRetryMilliseconds;
            }
            else
            {
                NextAttemptMilliseconds = GetMilliseconds() + GReconnectDelayMilliseconds;
            }
        }

        if (Connection)
        {
            if (!Connection->Poll(GInteractivePollMilliseconds))
            {
                OnConnectionLost();
            }
            else if (Editor->ConsumeCompletionRefreshRequest() && (CompletionRequestId < 0))
            {
                CompletionRequestId = RequestCompletions(*Connection);
            }
        }
        else
        {
            FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(GInteractivePollMilliseconds));
        }

        const bool bInputFinished = Editor->IsFinished();
        if (!Connection && !Editor->IsInteractive())
        {
            if (bInputFinished && !Editor->HasPendingLines())
            {
                return Finish();
            }

            continue;
        }

        String Line;
        while (Editor->PopLine(Line))
        {
            Line.TrimInline();
            if (Line.IsEmpty())
            {
                continue;
            }

            if (Line.Equals(":quit", EStringCaseType::NoCase) || Line.Equals(":exit", EStringCaseType::NoCase))
            {
                return Finish();
            }

            if (Line.Equals(":help", EStringCaseType::NoCase))
            {
                PrintUsage();
                continue;
            }

            if (!Connection)
            {
                Print(ELogSeverity::Warning, String::Printf("Not connected to the engine on %s yet, '%s' was not sent", *Address, *Line));
                continue;
            }

            ERequestOutcome Outcome = ERequestOutcome::Succeeded;
            String Argument;
            if (ParseCommandArgument(Line, GListCommand, Argument))
            {
                Outcome = RunList(*Connection, Options, Argument);
            }
            else if (ParseCommandArgument(Line, GGetCommand, Argument))
            {
                Outcome = RunGet(*Connection, Options, Argument);
            }
            else if (Line.Equals(":log on", EStringCaseType::NoCase) || Line.Equals(":log off", EStringCaseType::NoCase))
            {
                bWantsLogStream = Line.Equals(":log on", EStringCaseType::NoCase);
                Outcome = RunSubscribeLog(*Connection, Options, bWantsLogStream);
            }
            else if (Line[0] == ':')
            {
                Print(ELogSeverity::Error, String::Printf("Unknown command '%s', type :help for the list", *Line));
            }
            else
            {
                Outcome = RunExec(*Connection, Options, Line, Editor->IsInteractive());
            }

            if (Outcome == ERequestOutcome::ConnectionLost)
            {
                OnConnectionLost();
                continue;
            }

            bAnyFailed |= (Outcome == ERequestOutcome::Failed);
        }

        if (bInputFinished && !Editor->HasPendingLines())
        {
            return Finish();
        }
    }
}

static int32 RemoteConsoleMain()
{
    FRemoteConsoleOptions Options;
    if (!ParseOptions(Options))
    {
        PrintUsage();
        return ToExitCode(ERemoteConsoleExitCode::InvalidUsage);
    }

    if (Options.bShowHelp)
    {
        PrintUsage();
        return ToExitCode(ERemoteConsoleExitCode::Success);
    }

    if (!Options.IsOneShot())
    {
        return RunInteractive(Options);
    }

    FRemoteConsoleConnection Connection;
    Connection.SetJsonOutput(Options.bJsonOutput);

    const EConnectResult Result = Connection.Connect(Options.Host, Options.Port, Options.WaitSeconds, GConnectAttemptMilliseconds, Options.TimeoutMilliseconds);
    const String Address = FSocketAddress::FormatHostAndPort(Options.Host, Options.Port);
    if (Result == EConnectResult::Rejected)
    {
        Print(ELogSeverity::Error, String::Printf("[RemoteConsole] The engine rejected the connection: %s", *Connection.GetRejectReason()));
        return ToExitCode(ERemoteConsoleExitCode::ConnectionFailed);
    }
    else if (Result == EConnectResult::UnknownHost)
    {
        Print(ELogSeverity::Error, String::Printf("[RemoteConsole] Could not resolve '%s'", *Options.Host));
        return ToExitCode(ERemoteConsoleExitCode::ConnectionFailed);
    }
    else if (Result == EConnectResult::NoGreeting)
    {
        Print(ELogSeverity::Error, String::Printf("[RemoteConsole] Something on %s accepted the connection but is not a remote console", *Address));
        return ToExitCode(ERemoteConsoleExitCode::ConnectionFailed);
    }
    else if (Result == EConnectResult::Unreachable)
    {
        Print(ELogSeverity::Error, String::Printf(
            "[RemoteConsole] Could not connect to %s. The engine may not be running, RemoteConsole.Enable may be false, "
            "or the engine is on another machine with RemoteConsole.AllowRemoteConnections set to false",
            *Address));
        return ToExitCode(ERemoteConsoleExitCode::ConnectionFailed);
    }

    return RunOneShot(Connection, Options);
}

IMPLEMENT_PROGRAM_MAIN("RemoteConsole", &RemoteConsoleMain);


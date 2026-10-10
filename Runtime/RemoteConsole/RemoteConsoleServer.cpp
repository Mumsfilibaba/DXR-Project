#include "RemoteConsole/RemoteConsoleServer.h"
#include "Core/Algorithms/Algorithm.h"
#include "Core/Containers/Pair.h"
#include "Core/Json/JsonReader.h"
#include "Core/Misc/BuildInfo.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/Paths.h"
#include "Core/Misc/RemoteConsoleProtocol.h"
#include "Core/Platform/PlatformThread.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/ThreadManager.h"

#if RELEASE_BUILD
    #define REMOTE_CONSOLE_ENABLED_BY_DEFAULT (false)
#else
    #define REMOTE_CONSOLE_ENABLED_BY_DEFAULT (true)
#endif

static constexpr uint32 GPollIntervalMilliseconds = 10;
static constexpr uint32 GFinalFlushMilliseconds   = 250;
static constexpr int32  GReceiveChunkSize         = 4096;
static constexpr int32  GListenBacklog            = 8;

static constexpr const CHAR* GDefaultBindAddress = "::";

static constexpr int32 GDefaultMaxClients = 8;
static constexpr int32 GMinMaxClients     = 1;
static constexpr int32 GMaxMaxClients     = 64;

static FRemoteConsoleLogDevice GRemoteConsoleLogDevice;

static void OnRemoteConsoleSettingChanged(IConsoleVariable*)
{
    FRemoteConsoleServer::RequestSettingsRefresh();
}

static TAutoConsoleVariable<bool> CVarRemoteConsoleEnable(
    "RemoteConsole.Enable",
    "Opens the remote console socket. When false the engine does not listen on any port",
    REMOTE_CONSOLE_ENABLED_BY_DEFAULT,
    FConsoleVariableDelegate::CreateStatic(&OnRemoteConsoleSettingChanged),
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarRemoteConsoleAllowRemoteConnections(
    "RemoteConsole.AllowRemoteConnections",
    "When false the sockets are bound to 127.0.0.1 and ::1 and any peer that is not on this machine is rejected. "
    "When true the socket is bound to RemoteConsole.BindAddress and connections from other machines are accepted without authentication",
    false,
    FConsoleVariableDelegate::CreateStatic(&OnRemoteConsoleSettingChanged),
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<String> CVarRemoteConsoleBindAddress(
    "RemoteConsole.BindAddress",
    "IPv4 or IPv6 address to bind to when RemoteConsole.AllowRemoteConnections is true. "
    ":: listens on every interface for both IPv4 and IPv6, 0.0.0.0 on every interface for IPv4 only",
    GDefaultBindAddress,
    FConsoleVariableDelegate::CreateStatic(&OnRemoteConsoleSettingChanged),
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarRemoteConsolePort(
    "RemoteConsole.Port",
    "TCP port the remote console listens on",
    RemoteConsoleProtocol::DefaultPort,
    RemoteConsoleProtocol::MinPort, RemoteConsoleProtocol::MaxPort,
    FConsoleVariableDelegate::CreateStatic(&OnRemoteConsoleSettingChanged),
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarRemoteConsoleMaxClients(
    "RemoteConsole.MaxClients",
    "Connections beyond this many are rejected straight after being accepted",
    GDefaultMaxClients,
    GMinMaxClients, GMaxMaxClients,
    EConsoleVariableFlags::Default);

static const CHAR* GetVariableTypeName(const IConsoleVariable& Variable)
{
    if (Variable.IsVariableInt())
    {
        return "int";
    }
    else if (Variable.IsVariableFloat())
    {
        return "float";
    }
    else if (Variable.IsVariableBool())
    {
        return "bool";
    }

    return "string";
}

FRemoteConsoleLogDevice::FRemoteConsoleLogDevice()
    : CapturedLines()
    , bIsCapturing(false)
{
}

FRemoteConsoleLogDevice::~FRemoteConsoleLogDevice() = default;

void FRemoteConsoleLogDevice::Log(const String& Message)
{
    Log(ELogSeverity::Info, Message);
}

void FRemoteConsoleLogDevice::Log(ELogSeverity Severity, const String& Message)
{
    if (bIsCapturing && FThreadManager::IsMainThread())
    {
        FJsonValue Line = FJsonValue::MakeObject();
        Line.AddMember("severity", FJsonValue(RemoteConsoleProtocol::SeverityToString(Severity)));
        Line.AddMember("message", FJsonValue(Message));
        CapturedLines.Add(::Move(Line));
    }

    FRemoteConsoleServer::BroadcastLog(Severity, Message);
}

void FRemoteConsoleLogDevice::BeginCapture()
{
    CapturedLines = FJsonValue::MakeArray();
    bIsCapturing  = true;
}

FJsonValue FRemoteConsoleLogDevice::EndCapture()
{
    bIsCapturing = false;

    FJsonValue Result = ::Move(CapturedLines);
    CapturedLines.Reset();
    return Result;
}

FRemoteConsoleServer* FRemoteConsoleServer::Instance         = nullptr;
FCriticalSection      FRemoteConsoleServer::InstanceCS;
bool                  FRemoteConsoleServer::bIsInitialized   = false;
bool                  FRemoteConsoleServer::bSettingsChanged = false;

bool FRemoteConsoleServer::Initialize()
{
    FOutputDeviceManager::Get()->RegisterOutputDevice(&GRemoteConsoleLogDevice);

    bIsInitialized   = true;
    bSettingsChanged = false;
    return StartFromSettings();
}

void FRemoteConsoleServer::Release()
{
    StopInstance();
    FOutputDeviceManager::Get()->UnregisterOutputDevice(&GRemoteConsoleLogDevice);
    bIsInitialized = false;
}

void FRemoteConsoleServer::Tick()
{
    if (bIsInitialized && bSettingsChanged)
    {
        bSettingsChanged = false;
        StartFromSettings();
    }

    if (Instance)
    {
        Instance->ProcessRequests();
    }
}

void FRemoteConsoleServer::RequestSettingsRefresh()
{
    bSettingsChanged = true;
}

FRemoteConsoleServer* FRemoteConsoleServer::Get()
{
    return Instance;
}

int32 FRemoteConsoleServer::GetNumConnectedClients()
{
    return Instance ? Instance->NumConnectedClients.Load() : 0;
}

void FRemoteConsoleServer::BroadcastLog(ELogSeverity Severity, const String& Message)
{
    TScopedLock Lock(InstanceCS);
    if (!Instance)
    {
        return;
    }

    String Line;

    TScopedLock ClientsLock(Instance->ClientsCS);
    for (const TUniquePtr<FRemoteConsoleClient>& Client : Instance->Clients)
    {
        if (!Client->bSubscribedToLog || Client->bPendingClose)
        {
            continue;
        }

        if (Line.IsEmpty())
        {
            FJsonValue Event = FJsonValue::MakeObject();
            Event.AddMember("type", FJsonValue(RemoteConsoleProtocol::TypeLog));
            Event.AddMember("severity", FJsonValue(RemoteConsoleProtocol::SeverityToString(Severity)));
            Event.AddMember("message", FJsonValue(Message));
            Line = RemoteConsoleProtocol::ToLine(Event);
        }

        if (Client->SendBuffer.Length() + Line.Length() > RemoteConsoleProtocol::MaxSendBufferSize)
        {
            Client->bPendingClose = true;
            Client->CloseReason   = "the client stopped reading its log stream";
            continue;
        }

        Client->SendBuffer.Append(Line);
    }
}

bool FRemoteConsoleServer::StartFromSettings()
{
    StopInstance();

    if (!CVarRemoteConsoleEnable.GetValue())
    {
        LOG_INFO("[RemoteConsole] Disabled, no socket is open");
        return true;
    }

    const bool   bAllowRemote = CVarRemoteConsoleAllowRemoteConnections.GetValue();
    const uint16 Port         = static_cast<uint16>(CVarRemoteConsolePort.GetValue());

    FRemoteConsoleServer* NewServer = new FRemoteConsoleServer(bAllowRemote);

    bool bListening = false;
    if (bAllowRemote)
    {
        const String   BindAddressText = CVarRemoteConsoleBindAddress.GetValue();
        FSocketAddress BindAddress;
        if (!FNetworkSocket::ParseAddress(BindAddressText, Port, BindAddress))
        {
            LOG_ERROR("[RemoteConsole] RemoteConsole.BindAddress '%s' is not an IPv4 or IPv6 address", *BindAddressText);
        }
        else
        {
            bListening = NewServer->ListenOn(BindAddress);

            if (!bListening && (BindAddress == FSocketAddress::AnyIPv6(Port)))
            {
                bListening = NewServer->ListenOn(FSocketAddress::AnyIPv4(Port));
            }
        }
    }
    else
    {
        bListening = NewServer->ListenOn(FSocketAddress::LoopbackIPv4(Port));
        if (bListening)
        {
            NewServer->ListenOn(FSocketAddress::LoopbackIPv6(Port));
        }
    }

    if (!bListening)
    {
        LOG_ERROR("[RemoteConsole] Failed to listen on port %u", static_cast<uint32>(Port));
        delete NewServer;
        return false;
    }

    NewServer->Thread = FPlatformThread::Create(NewServer, "RemoteConsole");
    if (!NewServer->Thread || !NewServer->Thread->Start())
    {
        LOG_ERROR("[RemoteConsole] Failed to start the server thread");
        delete NewServer;
        return false;
    }

    if (bAllowRemote)
    {
        LOG_WARNING("[RemoteConsole] Listening on %s and accepting other machines without authentication", *NewServer->DescribeListeners());
    }
    else
    {
        LOG_INFO("[RemoteConsole] Listening on %s, this machine only", *NewServer->DescribeListeners());
    }

    TScopedLock Lock(InstanceCS);
    Instance = NewServer;
    return true;
}

void FRemoteConsoleServer::StopInstance()
{
    FRemoteConsoleServer* OldServer = nullptr;
    {
        TScopedLock Lock(InstanceCS);
        OldServer = Instance;
        Instance  = nullptr;
    }

    delete OldServer;
}

FRemoteConsoleServer::FRemoteConsoleServer(bool bInAllowRemoteConnections)
    : ListenSockets()
    , ListenAddresses()
    , Thread(nullptr)
    , bStopRequested(false)
    , HelloLine()
    , Clients()
    , ClientsCS()
    , NextClientId(1)
    , NumConnectedClients(0)
    , PendingRequests()
    , PendingRequestsCS()
    , bAllowRemoteConnections(bInAllowRemoteConnections)
{
    FJsonValue Hello = FJsonValue::MakeObject();
    Hello.AddMember("type", FJsonValue(RemoteConsoleProtocol::TypeHello));
    Hello.AddMember("protocol", FJsonValue(RemoteConsoleProtocol::Version));
    Hello.AddMember("engine", FJsonValue(BuildInfo::GetEngineName()));
    Hello.AddMember("version", FJsonValue(BuildInfo::GetVersionString()));
    Hello.AddMember("project", FJsonValue(Paths::GetProjectName()));
    Hello.AddMember("config", FJsonValue(BuildInfo::GetConfigurationName()));
    HelloLine = RemoteConsoleProtocol::ToLine(Hello);
}

FRemoteConsoleServer::~FRemoteConsoleServer()
{
    if (Thread)
    {
        Stop();
        Thread->WaitForCompletion();
        delete Thread;
        Thread = nullptr;
    }

    Clients.Clear();
    ListenSockets.Clear();
}

bool FRemoteConsoleServer::ListenOn(const FSocketAddress& Address)
{
    FNetworkSocket ListenSocket(ESocketType::TCP);
    if (!ListenSocket.SetNonBlocking(true) || !ListenSocket.Bind(Address) || !ListenSocket.Listen(GListenBacklog))
    {
        return false;
    }

    ListenAddresses.Add(Address);
    ListenSockets.Add(::Move(ListenSocket));
    return true;
}

String FRemoteConsoleServer::DescribeListeners() const
{
    String Description;
    for (const FSocketAddress& Address : ListenAddresses)
    {
        if (!Description.IsEmpty())
        {
            Description.Append(" and ");
        }

        Description.Append(Address.ToString());
    }

    return Description;
}

bool FRemoteConsoleServer::Start()
{
    return true;
}

int32 FRemoteConsoleServer::Run()
{
    TArray<FNetworkSocket*>       WaitSockets;
    TArray<bool>                  Readable;
    TArray<FRemoteConsoleRequest> NewRequests;
    TArray<String>                LogMessages;

    while (!bStopRequested.Load())
    {
        WaitSockets.Clear();
        for (FNetworkSocket& ListenSocket : ListenSockets)
        {
            WaitSockets.Add(&ListenSocket);
        }

        const int32 NumListenSockets = ListenSockets.Size();
        {
            TScopedLock Lock(ClientsCS);
            for (const TUniquePtr<FRemoteConsoleClient>& Client : Clients)
            {
                WaitSockets.Add(&Client->Socket);
            }
        }

        Readable.Resize(WaitSockets.Size());
        const int32 NumReadable = FNetworkSocket::WaitForRead(WaitSockets.Data(), WaitSockets.Size(), Readable.Data(), GPollIntervalMilliseconds);
        if (bStopRequested.Load())
        {
            break;
        }

        if (NumReadable < 0)
        {
            FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(GPollIntervalMilliseconds));
        }

        for (int32 ListenIndex = 0; (NumReadable > 0) && (ListenIndex < NumListenSockets); ++ListenIndex)
        {
            if (Readable[ListenIndex])
            {
                AcceptClients(&ListenSockets[ListenIndex], LogMessages);
            }
        }

        {
            TScopedLock Lock(ClientsCS);

            for (int32 Index = 0; Index < Clients.Size(); ++Index)
            {
                FRemoteConsoleClient& Client = *Clients[Index];

                const int32 ReadableIndex = NumListenSockets + Index;
                if ((NumReadable > 0) && (ReadableIndex < Readable.Size()) && Readable[ReadableIndex])
                {
                    ReceiveFromClient(Client, NewRequests);
                }

                FlushClient(Client);
            }

            for (int32 Index = Clients.Size() - 1; Index >= 0; --Index)
            {
                if (Clients[Index]->bPendingClose)
                {
                    const FRemoteConsoleClient& Client = *Clients[Index];
                    const int32 NumRemaining = Clients.Size() - 1;
                    if (Client.CloseReason.IsEmpty())
                    {
                        LogMessages.Add(String::Printf("[RemoteConsole] Remote console at %s disconnected (%d still connected)", *Client.PeerAddress, NumRemaining));
                    }
                    else
                    {
                        LogMessages.Add(String::Printf("[RemoteConsole] Dropped the remote console at %s because %s (%d still connected)", *Client.PeerAddress, *Client.CloseReason, NumRemaining));
                    }

                    Clients.RemoveAt(Index);
                }
            }

            NumConnectedClients.Store(Clients.Size());
        }

        if (!NewRequests.IsEmpty())
        {
            TScopedLock Lock(PendingRequestsCS);
            for (FRemoteConsoleRequest& Request : NewRequests)
            {
                PendingRequests.Add(::Move(Request));
            }

            NewRequests.Clear();
        }

        for (const String& Message : LogMessages)
        {
            LOG_INFO("%s", *Message);
        }

        LogMessages.Clear();
    }

    FlushAllClients(GFinalFlushMilliseconds);
    return 0;
}

void FRemoteConsoleServer::Stop()
{
    bStopRequested.Store(true);
}

void FRemoteConsoleServer::AcceptClients(FNetworkSocket* ListenSocket, TArray<String>& OutLogMessages)
{
    CHECK(ListenSocket != nullptr);

    FNetworkSocket Socket(ESocketType::TCP);
    while (ListenSocket->Accept(Socket))
    {
        FSocketAddress PeerAddress;
        Socket.GetPeerAddress(PeerAddress);
        const String PeerText = PeerAddress.ToString();

        if (!bAllowRemoteConnections && !PeerAddress.IsLoopback())
        {
            const CHAR* Reason = "Remote connections are disabled (RemoteConsole.AllowRemoteConnections is false)";
            RejectClient(&Socket, Reason);
            OutLogMessages.Add(String::Printf("[RemoteConsole] Rejected %s, %s", *PeerText, Reason));
            continue;
        }

        TScopedLock Lock(ClientsCS);
        if (Clients.Size() >= CVarRemoteConsoleMaxClients.GetValue())
        {
            const CHAR* Reason = "Too many clients are connected (RemoteConsole.MaxClients)";
            RejectClient(&Socket, Reason);
            OutLogMessages.Add(String::Printf("[RemoteConsole] Rejected %s, %s", *PeerText, Reason));
            continue;
        }

        TUniquePtr<FRemoteConsoleClient> Client = MakeUniquePtr<FRemoteConsoleClient>();
        Client->ClientId    = NextClientId++;
        Client->Socket      = ::Move(Socket);
        Client->PeerAddress = PeerText;
        Client->SendBuffer  = HelloLine;
        Clients.Add(::Move(Client));
        NumConnectedClients.Store(Clients.Size());

        OutLogMessages.Add(String::Printf("[RemoteConsole] Remote console connected from %s (%d connected)", *PeerText, Clients.Size()));
    }
}

void FRemoteConsoleServer::RejectClient(FNetworkSocket* Socket, const CHAR* Reason)
{
    CHECK(Socket != nullptr);

    FJsonValue Message = FJsonValue::MakeObject();
    Message.AddMember("type", FJsonValue(RemoteConsoleProtocol::TypeRejected));
    Message.AddMember("reason", FJsonValue(Reason));

    const String Line = RemoteConsoleProtocol::ToLine(Message);
    int32 BytesSent = 0;
    Socket->Send(Line.Data(), Line.Length(), BytesSent);
    Socket->Close();
}

void FRemoteConsoleServer::ReceiveFromClient(FRemoteConsoleClient& Client, TArray<FRemoteConsoleRequest>& OutRequests)
{
    CHAR Chunk[GReceiveChunkSize];

    while (!Client.bPendingClose)
    {
        int32 BytesRead = 0;
        const ESocketResult Result = Client.Socket.Recv(Chunk, GReceiveChunkSize, BytesRead);
        if (Result == ESocketResult::WouldBlock)
        {
            break;
        }
        else if (Result != ESocketResult::Success)
        {
            Client.bPendingClose = true;
            break;
        }

        Client.ReceiveBuffer.Append(Chunk, BytesRead);
    }

    TArray<String> Lines;
    if (!RemoteConsoleProtocol::ExtractLines(Client.ReceiveBuffer, Lines))
    {
        Client.bPendingClose = true;
        Client.CloseReason   = "it sent a line longer than the protocol allows";
        return;
    }

    for (const String& Line : Lines)
    {
        FRemoteConsoleRequest Request;
        if (ParseLine(Client, Line, Request))
        {
            OutRequests.Add(::Move(Request));
        }
    }
}

void FRemoteConsoleServer::FlushClient(FRemoteConsoleClient& Client)
{
    while (!Client.bPendingClose && !Client.SendBuffer.IsEmpty())
    {
        int32 BytesSent = 0;
        const ESocketResult Result = Client.Socket.Send(Client.SendBuffer.Data(), Client.SendBuffer.Length(), BytesSent);
        if (Result == ESocketResult::WouldBlock)
        {
            break;
        }
        else if (Result != ESocketResult::Success)
        {
            Client.bPendingClose = true;
            break;
        }

        if (BytesSent >= Client.SendBuffer.Length())
        {
            Client.SendBuffer.Clear();
        }
        else if (BytesSent > 0)
        {
            Client.SendBuffer.Remove(0, BytesSent);
        }
    }
}

void FRemoteConsoleServer::FlushAllClients(uint32 TimeoutMilliseconds)
{
    for (uint32 Elapsed = 0; Elapsed < TimeoutMilliseconds; Elapsed += GPollIntervalMilliseconds)
    {
        bool bHasPendingData = false;
        {
            TScopedLock Lock(ClientsCS);
            for (const TUniquePtr<FRemoteConsoleClient>& Client : Clients)
            {
                FlushClient(*Client);
                bHasPendingData |= (!Client->bPendingClose && !Client->SendBuffer.IsEmpty());
            }
        }

        if (!bHasPendingData)
        {
            break;
        }

        FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(GPollIntervalMilliseconds));
    }
}

bool FRemoteConsoleServer::ParseLine(FRemoteConsoleClient& Client, const String& Line, FRemoteConsoleRequest& OutRequest)
{
    String Trimmed = Line;
    Trimmed.TrimInline();
    if (Trimmed.IsEmpty())
    {
        return false;
    }

    OutRequest.ClientId = Client.ClientId;

    if (Trimmed[0] != '{')
    {
        OutRequest.Type       = RemoteConsoleProtocol::TypeExec;
        OutRequest.Payload    = FJsonValue::MakeObject();
        OutRequest.Payload.AddMember("command", FJsonValue(Trimmed));
        OutRequest.bIsRawText = true;
        return true;
    }

    // Errors found here are answered directly; the caller holds ClientsCS, so append to the buffer rather than SendToClient
    FJsonValue Error = FJsonValue::MakeObject();
    Error.AddMember("type", FJsonValue(RemoteConsoleProtocol::TypeResult));
    Error.AddMember("ok", FJsonValue(false));

    FJsonReader Reader;
    FJsonError  ParseError;
    FJsonValue  Request;
    if (!Reader.Parse(StringView(Trimmed.Data(), Trimmed.Length()), Request, &ParseError))
    {
        Error.AddMember("error", FJsonValue(String::Printf("Malformed JSON, %s", *ParseError.ToString())));
        Client.SendBuffer.Append(RemoteConsoleProtocol::ToLine(Error));
        return false;
    }

    if (const FJsonValue* Id = Request.Find("id"))
    {
        OutRequest.Id = *Id;
        Error.AddMember("id", FJsonValue(*Id));
    }

    String Type;
    const FJsonValue* TypeValue = Request.IsObject() ? Request.Find("type") : nullptr;
    if (!TypeValue || !TypeValue->TryGetString(Type) || Type.IsEmpty())
    {
        Error.AddMember("error", FJsonValue("A request must be a JSON object with a 'type' string"));
        Client.SendBuffer.Append(RemoteConsoleProtocol::ToLine(Error));
        return false;
    }

    OutRequest.Type    = ::Move(Type);
    OutRequest.Payload = ::Move(Request);
    return true;
}

void FRemoteConsoleServer::ProcessRequests()
{
    TArray<FRemoteConsoleRequest> Requests;
    {
        TScopedLock Lock(PendingRequestsCS);
        Requests = ::Move(PendingRequests);
        PendingRequests.Clear();
    }

    for (const FRemoteConsoleRequest& Request : Requests)
    {
        HandleRequest(Request);
    }
}

void FRemoteConsoleServer::HandleRequest(const FRemoteConsoleRequest& Request)
{
    if (Request.Type == RemoteConsoleProtocol::TypeExec)
    {
        HandleExec(Request);
    }
    else if (Request.Type == RemoteConsoleProtocol::TypeList)
    {
        HandleList(Request);
    }
    else if (Request.Type == RemoteConsoleProtocol::TypeGet)
    {
        HandleGet(Request);
    }
    else if (Request.Type == RemoteConsoleProtocol::TypeSubscribeLog)
    {
        HandleSubscribeLog(Request);
    }
    else if (Request.Type == RemoteConsoleProtocol::TypePing)
    {
        SendResult(Request, MakeResult(Request, true));
    }
    else
    {
        SendError(Request, String::Printf("Unknown request type '%s'", *Request.Type));
    }
}

void FRemoteConsoleServer::HandleExec(const FRemoteConsoleRequest& Request)
{
    String Command;
    const FJsonValue* CommandValue = Request.Payload.Find("command");
    if (!CommandValue || !CommandValue->TryGetString(Command))
    {
        SendError(Request, "'exec' needs a non-empty 'command' string");
        return;
    }

    Command.TrimInline();
    if (Command.IsEmpty())
    {
        SendError(Request, "'exec' needs a non-empty 'command' string");
        return;
    }

    if (Command.StartsWith(RemoteConsoleProtocol::SettingsPrefix, EStringCaseType::NoCase))
    {
        SendError(Request, "RemoteConsole.* settings can only be changed from the config files, the command line or the in-game console");
        return;
    }

    GRemoteConsoleLogDevice.BeginCapture();
    const bool bSucceeded = FConsoleManager::Get().ExecuteCommand(*FOutputDeviceManager::Get(), Command);
    FJsonValue Output = GRemoteConsoleLogDevice.EndCapture();

    FJsonValue Result = MakeResult(Request, bSucceeded);
    Result.AddMember("output", ::Move(Output));
    SendResult(Request, Result);
}

void FRemoteConsoleServer::HandleList(const FRemoteConsoleRequest& Request)
{
    String Filter;
    const FJsonValue* FilterValue = Request.Payload.Find("filter");
    if (FilterValue && !FilterValue->IsNull() && !FilterValue->TryGetString(Filter))
    {
        SendError(Request, "'filter' must be a string");
        return;
    }

    TArray<TPair<String, IConsoleObject*>> ConsoleObjects;
    FConsoleManager::Get().GetConsoleObjects(ConsoleObjects);

    Algorithm::Sort(ConsoleObjects, [](const TPair<String, IConsoleObject*>& First, const TPair<String, IConsoleObject*>& Second) -> bool
    {
        return First.First.Compare(Second.First, EStringCaseType::NoCase) < 0;
    });

    FJsonValue Objects = FJsonValue::MakeArray();
    for (const TPair<String, IConsoleObject*>& Pair : ConsoleObjects)
    {
        if (!Pair.Second || (!Filter.IsEmpty() && !Pair.First.Contains(*Filter, EStringCaseType::NoCase)))
        {
            continue;
        }

        FJsonValue Object = FJsonValue::MakeObject();
        Object.AddMember("name", FJsonValue(Pair.First));
        Object.AddMember("help", FJsonValue(Pair.Second->GetHelpString()));

        if (IConsoleVariable* Variable = Pair.Second->AsVariable())
        {
            Object.AddMember("kind", FJsonValue("variable"));
            Object.AddMember("valueType", FJsonValue(GetVariableTypeName(*Variable)));
            Object.AddMember("value", FJsonValue(Variable->GetString()));
        }
        else
        {
            Object.AddMember("kind", FJsonValue("command"));
        }

        Objects.Add(::Move(Object));
    }

    FJsonValue Result = MakeResult(Request, true);
    Result.AddMember("objects", ::Move(Objects));
    SendResult(Request, Result);
}

void FRemoteConsoleServer::HandleGet(const FRemoteConsoleRequest& Request)
{
    String Name;
    const FJsonValue* NameValue = Request.Payload.Find("name");
    if (!NameValue || !NameValue->TryGetString(Name) || Name.IsEmpty())
    {
        SendError(Request, "'get' needs a non-empty 'name' string");
        return;
    }

    IConsoleVariable* Variable = FConsoleManager::Get().FindConsoleVariable(*Name);
    if (!Variable)
    {
        if (FConsoleManager::Get().FindConsoleCommand(*Name))
        {
            SendError(Request, String::Printf("'%s' is a command, not a variable", *Name));
        }
        else
        {
            SendError(Request, String::Printf("'%s' is not a registered variable", *Name));
        }

        return;
    }

    FJsonValue Result = MakeResult(Request, true);
    Result.AddMember("name", FJsonValue(Name));
    Result.AddMember("valueType", FJsonValue(GetVariableTypeName(*Variable)));
    Result.AddMember("value", FJsonValue(Variable->GetString()));
    Result.AddMember("setBy", FJsonValue(SetByFlagToString(Variable->GetFlags() & EConsoleVariableFlags::SetByMask)));
    SendResult(Request, Result);
}

void FRemoteConsoleServer::HandleSubscribeLog(const FRemoteConsoleRequest& Request)
{
    bool bEnabled = true;
    if (const FJsonValue* EnabledValue = Request.Payload.Find("enabled"))
    {
        bEnabled = EnabledValue->GetBoolOr(true);
    }

    {
        TScopedLock Lock(ClientsCS);
        for (const TUniquePtr<FRemoteConsoleClient>& Client : Clients)
        {
            if (Client->ClientId == Request.ClientId)
            {
                Client->bSubscribedToLog = bEnabled;
                break;
            }
        }
    }

    SendResult(Request, MakeResult(Request, true));
}

FJsonValue FRemoteConsoleServer::MakeResult(const FRemoteConsoleRequest& Request, bool bSucceeded) const
{
    FJsonValue Result = FJsonValue::MakeObject();
    if (!Request.Id.IsNull())
    {
        Result.AddMember("id", FJsonValue(Request.Id));
    }

    Result.AddMember("type", FJsonValue(RemoteConsoleProtocol::TypeResult));
    Result.AddMember("ok", FJsonValue(bSucceeded));
    return Result;
}

void FRemoteConsoleServer::SendError(const FRemoteConsoleRequest& Request, const String& Error)
{
    FJsonValue Result = MakeResult(Request, false);
    Result.AddMember("error", FJsonValue(Error));
    SendResult(Request, Result);
}

void FRemoteConsoleServer::SendResult(const FRemoteConsoleRequest& Request, const FJsonValue& Result)
{
    if (!Request.bIsRawText)
    {
        SendToClient(Request.ClientId, RemoteConsoleProtocol::ToLine(Result));
        return;
    }

    String Text;
    if (const FJsonValue* Output = Result.Find("output"))
    {
        for (int32 Index = 0; Index < Output->Num(); ++Index)
        {
            if (const FJsonValue* Message = (*Output)[Index].Find("message"))
            {
                Text.Append(Message->GetStringOr(""));
                Text.Append('\n');
            }
        }
    }

    const FJsonValue* Ok = Result.Find("ok");
    if (Ok && Ok->GetBoolOr(false))
    {
        Text.Append("OK\n");
    }
    else
    {
        const FJsonValue* Error = Result.Find("error");
        Text.Append(Error ? String::Printf("ERROR %s\n", *Error->GetStringOr("")) : String("ERROR\n"));
    }

    SendToClient(Request.ClientId, Text);
}

void FRemoteConsoleServer::SendToClient(uint32 ClientId, const String& Line)
{
    TScopedLock Lock(ClientsCS);
    for (const TUniquePtr<FRemoteConsoleClient>& Client : Clients)
    {
        if (Client->ClientId != ClientId)
        {
            continue;
        }

        if (Client->SendBuffer.Length() + Line.Length() > RemoteConsoleProtocol::MaxSendBufferSize)
        {
            Client->bPendingClose = true;
            Client->CloseReason   = "it stopped reading responses";
        }
        else
        {
            Client->SendBuffer.Append(Line);
        }

        break;
    }
}

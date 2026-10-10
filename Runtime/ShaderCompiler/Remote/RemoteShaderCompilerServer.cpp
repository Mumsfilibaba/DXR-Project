#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformThread.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Tasks/Tasks.h"
#include "Core/Threading/ScopedLock.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "ShaderCompiler/ShaderSourceHash.h"
#include "ShaderCompiler/Remote/RemoteShaderCompilerServer.h"

static constexpr uint32 GServerPollIntervalMilliseconds = 10;
static constexpr uint32 GFinalFlushMilliseconds         = 250;
static constexpr int32  GServerReceiveChunkSize         = 64 * 1024;
static constexpr int32  GListenBacklog                  = 16;

static constexpr uint32 GForwardTimeoutMilliseconds = 120000;
static constexpr uint32 GForwardConnectMilliseconds = 2000;
static constexpr uint64 GForwardRetryMilliseconds   = 5000;

static constexpr EShaderOutputLanguage GServedLanguages[] =
{
    EShaderOutputLanguage::DXIL,
    EShaderOutputLanguage::DXBC,
    EShaderOutputLanguage::SPIRV,
    EShaderOutputLanguage::MSL,
};

static uint64 GetServerMilliseconds()
{
    return static_cast<uint64>(static_cast<double>(FPlatformTime::QueryPerformanceCounter()) * 1000.0 / static_cast<double>(FPlatformTime::QueryPerformanceFrequency()));
}

static String MakeFileKey(const String& Path, uint32 Hash)
{
    return String::Printf("%s:%u", *Path, Hash);
}

bool FRemoteRequestSources::FileExists(const String& Path) const
{
    return Files.Contains(RemoteShaderCompilerProtocol::CollapsePath(Path));
}

bool FRemoteRequestSources::ReadFile(const String& Path, TArray<CHAR>& OutText) const
{
    const TSharedPtr<TArray<uint8>>* Contents = Files.Find(RemoteShaderCompilerProtocol::CollapsePath(Path));
    if (!Contents || !*Contents)
    {
        return false;
    }

    const TArray<uint8>& Bytes = **Contents;
    OutText.Resize(Bytes.Size() + 1);

    if (!Bytes.IsEmpty())
    {
        Memory::Memcpy(OutText.Data(), Bytes.Data(), Bytes.Size());
    }

    OutText[Bytes.Size()] = '\0';
    return true;
}

FRemoteShaderCompilerServer::FRemoteShaderCompilerServer(const FRemoteShaderCompilerServerSettings& InSettings)
    : Settings(InSettings)
    , ListenSockets()
    , ListenAddresses()
    , Thread(nullptr)
    , bStopRequested(false)
    , Connections()
    , ConnectionsCS()
    , NextConnectionId(1)
    , Forwarders()
    , NumTasksInFlight(0)
{
}

FRemoteShaderCompilerServer::~FRemoteShaderCompilerServer()
{
    Shutdown();
}

bool FRemoteShaderCompilerServer::ListenOn(const FSocketAddress& Address)
{
    FNetworkSocket ListenSocket(ESocketType::TCP);
    if (!ListenSocket.SetNonBlocking(true) || !ListenSocket.Bind(Address) || !ListenSocket.Listen(GListenBacklog))
    {
        return false;
    }

    FSocketAddress BoundAddress;
    ListenAddresses.Add(ListenSocket.GetLocalAddress(BoundAddress) ? BoundAddress : Address);
    ListenSockets.Add(::Move(ListenSocket));
    return true;
}

bool FRemoteShaderCompilerServer::Launch()
{
    uint16 Port = Settings.Port;

    bool bListening = false;
    if (Settings.bAllowRemoteConnections)
    {
        FSocketAddress BindAddress;
        if (!FNetworkSocket::ParseAddress(Settings.BindAddress, Port, BindAddress))
        {
            LOG_ERROR("[RemoteShaderCompiler] -bind '%s' is not an IPv4 or IPv6 address", *Settings.BindAddress);
            return false;
        }

        bListening = ListenOn(BindAddress);
        if (!bListening && (BindAddress == FSocketAddress::AnyIPv6(Port)))
        {
            bListening = ListenOn(FSocketAddress::AnyIPv4(Port));
        }
    }
    else
    {
        // ::1 is optional, a machine without IPv6 still serves 127.0.0.1
        bListening = ListenOn(FSocketAddress::LoopbackIPv4(Port));
        if (bListening)
        {
            ListenOn(FSocketAddress::LoopbackIPv6(ListenAddresses[0].GetPort()));
        }
    }

    if (!bListening)
    {
        LOG_ERROR("[RemoteShaderCompiler] Failed to listen on port %u", Port);
        ListenSockets.Clear();
        ListenAddresses.Clear();
        return false;
    }

    for (const FRemoteForwardTarget& Target : Settings.ForwardTargets)
    {
        TUniquePtr<FForwarder> Forwarder = MakeUniquePtr<FForwarder>();
        Forwarder->Target = Target;
        Forwarder->Client = MakeUniquePtr<FRemoteShaderCompilerClient>();

        if (Forwarder->Client->Connect(Target.Host, Target.Port, GForwardConnectMilliseconds))
        {
            LOG_INFO("[RemoteShaderCompiler] Forwarding %s to %s", ToString(Target.OutputLanguage), *Forwarder->Client->GetPeerName());
        }
        else
        {
            LOG_WARNING("[RemoteShaderCompiler] %s is not reachable for %s yet (%s), retrying every %llu seconds",
                *FSocketAddress::FormatHostAndPort(Target.Host, Target.Port), ToString(Target.OutputLanguage), *Forwarder->Client->GetLastError(), GForwardRetryMilliseconds / 1000);
            Forwarder->NextRetryMilliseconds = GetServerMilliseconds() + GForwardRetryMilliseconds;
        }

        Forwarders.Add(::Move(Forwarder));
    }

    bStopRequested.Store(false);

    Thread = FPlatformThread::Create(this, "RemoteShaderCompilerServer");
    if (!Thread || !Thread->Start())
    {
        LOG_ERROR("[RemoteShaderCompiler] Failed to start the server thread");
        delete Thread;
        Thread = nullptr;
        ListenSockets.Clear();
        ListenAddresses.Clear();
        return false;
    }

    if (Settings.bAllowRemoteConnections)
    {
        LOG_WARNING("[RemoteShaderCompiler] Listening on %s and accepting other machines without authentication", *DescribeListeners());
    }
    else
    {
        LOG_INFO("[RemoteShaderCompiler] Listening on %s, this machine only (pass -allowremote to accept other machines)", *DescribeListeners());
    }

    return true;
}

void FRemoteShaderCompilerServer::Shutdown()
{
    if (Thread)
    {
        Stop();
        Thread->WaitForCompletion();
        delete Thread;
        Thread = nullptr;
    }

    while (NumTasksInFlight.Load() > 0)
    {
        FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(GServerPollIntervalMilliseconds));
    }

    for (TUniquePtr<FForwarder>& Forwarder : Forwarders)
    {
        Forwarder->Client->Disconnect();
    }

    Forwarders.Clear();

    {
        TScopedLock Lock(ConnectionsCS);
        for (TSharedPtr<FConnection>& Connection : Connections)
        {
            TScopedLock SendLock(Connection->SendCS);
            Connection->Socket.Close();
        }

        Connections.Clear();
    }

    ListenSockets.Clear();
    ListenAddresses.Clear();
}

uint16 FRemoteShaderCompilerServer::GetPort() const
{
    return ListenAddresses.IsEmpty() ? 0 : ListenAddresses[0].GetPort();
}

int32 FRemoteShaderCompilerServer::GetNumConnections() const
{
    TScopedLock Lock(ConnectionsCS);
    return Connections.Size();
}

String FRemoteShaderCompilerServer::DescribeListeners() const
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

bool FRemoteShaderCompilerServer::Start()
{
    return true;
}

int32 FRemoteShaderCompilerServer::Run()
{
    TArray<FNetworkSocket*>         WaitSockets;
    TArray<TSharedPtr<FConnection>> WaitConnections;
    TArray<bool>                    Readable;
    TArray<String>                  LogMessages;

    while (!bStopRequested.Load())
    {
        RetryForwarders();

        WaitSockets.Clear();
        WaitConnections.Clear();
        for (FNetworkSocket& ListenSocket : ListenSockets)
        {
            WaitSockets.Add(&ListenSocket);
        }

        {
            TScopedLock Lock(ConnectionsCS);
            for (const TSharedPtr<FConnection>& Connection : Connections)
            {
                WaitSockets.Add(&Connection->Socket);
                WaitConnections.Add(Connection);
            }
        }

        Readable.Resize(WaitSockets.Size());

        const int32 NumReadable = FNetworkSocket::WaitForRead(WaitSockets.Data(), WaitSockets.Size(), Readable.Data(), GServerPollIntervalMilliseconds);
        if (bStopRequested.Load())
        {
            break;
        }

        if (NumReadable < 0)
        {
            FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(GServerPollIntervalMilliseconds));
        }

        const int32 NumListenSockets = ListenSockets.Size();
        for (int32 ListenIndex = 0; (NumReadable > 0) && (ListenIndex < NumListenSockets); ++ListenIndex)
        {
            if (Readable[ListenIndex])
            {
                AcceptConnections(ListenSockets[ListenIndex], LogMessages);
            }
        }

        for (int32 Index = 0; Index < WaitConnections.Size(); ++Index)
        {
            const TSharedPtr<FConnection>& Connection = WaitConnections[Index];

            const int32 ReadableIndex = NumListenSockets + Index;
            if ((NumReadable > 0) && Readable[ReadableIndex])
            {
                ReceiveFrom(Connection);
            }

            FlushConnection(*Connection);
        }

        {
            TScopedLock Lock(ConnectionsCS);
            for (int32 Index = Connections.Size() - 1; Index >= 0; --Index)
            {
                FConnection& Connection = *Connections[Index];
                if (!Connection.bPendingClose)
                {
                    continue;
                }

                if (Connection.CloseReason.IsEmpty())
                {
                    LogMessages.Add(String::Printf("[RemoteShaderCompiler] %s disconnected (%d still connected)", *Connection.PeerAddress, Connections.Size() - 1));
                }
                else
                {
                    LogMessages.Add(String::Printf("[RemoteShaderCompiler] Dropped %s because %s (%d still connected)", 
                        *Connection.PeerAddress, *Connection.CloseReason, Connections.Size() - 1));
                }

                {
                    TScopedLock SendLock(Connection.SendCS);
                    Connection.Socket.Close();
                }

                Connections.RemoveAt(Index);
            }
        }

        WaitConnections.Clear();

        for (const String& Message : LogMessages)
        {
            LOG_INFO("%s", *Message);
        }

        LogMessages.Clear();
    }

    for (uint32 Elapsed = 0; Elapsed < GFinalFlushMilliseconds; Elapsed += GServerPollIntervalMilliseconds)
    {
        bool bHasPendingData = false;

        TScopedLock Lock(ConnectionsCS);
        for (const TSharedPtr<FConnection>& Connection : Connections)
        {
            FlushConnection(*Connection);

            TScopedLock SendLock(Connection->SendCS);
            bHasPendingData |= !Connection->bPendingClose && !Connection->SendBuffer.IsEmpty();
        }

        if (!bHasPendingData)
        {
            break;
        }

        FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(GServerPollIntervalMilliseconds));
    }

    return 0;
}

void FRemoteShaderCompilerServer::Stop()
{
    bStopRequested.Store(true);
}

void FRemoteShaderCompilerServer::AcceptConnections(FNetworkSocket& ListenSocket, TArray<String>& OutLogMessages)
{
    FNetworkSocket Socket(ESocketType::TCP);
    while (ListenSocket.Accept(Socket))
    {
        FSocketAddress PeerAddress;
        Socket.GetPeerAddress(PeerAddress);
        const String PeerText = PeerAddress.ToString();

        if (!Settings.bAllowRemoteConnections && !PeerAddress.IsLoopback())
        {
            const CHAR* Reason = "Remote connections are disabled (start the server with -allowremote)";
            Reject(Socket, Reason);
            OutLogMessages.Add(String::Printf("[RemoteShaderCompiler] Rejected %s, %s", *PeerText, Reason));
            continue;
        }

        TScopedLock Lock(ConnectionsCS);
        if (Connections.Size() >= Settings.MaxClients)
        {
            const CHAR* Reason = "Too many clients are connected";
            Reject(Socket, Reason);
            OutLogMessages.Add(String::Printf("[RemoteShaderCompiler] Rejected %s, %s", *PeerText, Reason));
            continue;
        }

        TSharedPtr<FConnection> Connection = MakeSharedPtr<FConnection>(NextConnectionId++, ::Move(Socket), PeerText);

        RemoteShaderCompilerProtocol::EncodeFrame(BuildHello(), Connection->SendBuffer);
        Connections.Add(Connection);

        OutLogMessages.Add(String::Printf("[RemoteShaderCompiler] %s connected (%d connected)", *PeerText, Connections.Size()));
    }
}

void FRemoteShaderCompilerServer::Reject(FNetworkSocket& Socket, const CHAR* Reason)
{
    RemoteShaderCompilerProtocol::FFrame Frame;
    Frame.Type   = RemoteShaderCompilerProtocol::EMessageType::Rejected;
    Frame.Header = FJsonValue::MakeObject();
    Frame.Header.AddMember("reason", FJsonValue(Reason));

    TArray<uint8> Bytes;
    RemoteShaderCompilerProtocol::EncodeFrame(Frame, Bytes);

    int32 BytesSent = 0;
    Socket.Send(Bytes.Data(), Bytes.Size(), BytesSent);
    Socket.Close();
}

RemoteShaderCompilerProtocol::FFrame FRemoteShaderCompilerServer::BuildHello()
{
    FShaderCompiler& Compiler = FShaderCompiler::Get();

    FJsonValue LanguageArray = FJsonValue::MakeArray();
    for (EShaderOutputLanguage OutputLanguage : GServedLanguages)
    {
        FShaderCompilerIdentity Identity;
        const CHAR*             Route = nullptr;

        if (Compiler.CanCompileLocally(OutputLanguage))
        {
            Identity = Compiler.GetLocalIdentity(OutputLanguage);
            Route    = "local";
        }
        else if (FRemoteShaderCompilerClient* Forwarder = FindForwarder(OutputLanguage))
        {
            Identity = Forwarder->GetIdentity(OutputLanguage).GetValueOrDefault(FShaderCompilerIdentity());
            Route    = "forwarded";
        }

        if (!Route || !Identity.IsValid())
        {
            continue;
        }

        FJsonValue Language = FJsonValue::MakeObject();
        Language.AddMember("language", FJsonValue(ToString(OutputLanguage)));
        Language.AddMember("route", FJsonValue(Route));
        Language.AddMember("identity", RemoteShaderCompilerProtocol::IdentityToJson(Identity));
        LanguageArray.Add(::Move(Language));
    }

    RemoteShaderCompilerProtocol::FFrame Frame;
    Frame.Type   = RemoteShaderCompilerProtocol::EMessageType::Hello;
    Frame.Header = FJsonValue::MakeObject();
    Frame.Header.AddMember("protocol", FJsonValue(RemoteShaderCompilerProtocol::Version));
    Frame.Header.AddMember("platform", FJsonValue(RemoteShaderCompilerProtocol::GetPlatformName()));
    Frame.Header.AddMember("languages", ::Move(LanguageArray));
    return Frame;
}

void FRemoteShaderCompilerServer::ReceiveFrom(const TSharedPtr<FConnection>& Connection)
{
    TArray<uint8> Chunk;
    Chunk.Resize(GServerReceiveChunkSize);

    while (!Connection->bPendingClose)
    {
        int32 BytesRead = 0;

        const ESocketResult Result = Connection->Socket.Recv(Chunk.Data(), Chunk.Size(), BytesRead);
        if (Result == ESocketResult::WouldBlock)
        {
            break;
        }

        if (Result != ESocketResult::Success)
        {
            Connection->bPendingClose = true;
            break;
        }

        Connection->ReceiveBuffer.Append(Chunk.Data(), BytesRead);
    }

    while (!Connection->bPendingClose)
    {
        RemoteShaderCompilerProtocol::FFrame Frame;

        bool bHasFrame = false;
        if (!RemoteShaderCompilerProtocol::TryDecodeFrame(Connection->ReceiveBuffer, Frame, bHasFrame))
        {
            Connection->bPendingClose = true;
            Connection->CloseReason   = "it sent a malformed frame";
            break;
        }

        if (!bHasFrame)
        {
            break;
        }

        if (Frame.Type == RemoteShaderCompilerProtocol::EMessageType::CompileRequest)
        {
            HandleCompileRequest(Connection, ::Move(Frame));
        }
        else if (Frame.Type == RemoteShaderCompilerProtocol::EMessageType::Ping)
        {
            RemoteShaderCompilerProtocol::FFrame Pong;
            Pong.Type      = RemoteShaderCompilerProtocol::EMessageType::Pong;
            Pong.RequestId = Frame.RequestId;

            TScopedLock Lock(Connection->SendCS);
            RemoteShaderCompilerProtocol::EncodeFrame(Pong, Connection->SendBuffer);
        }
    }
}

bool FRemoteShaderCompilerServer::IngestFiles(FConnection& Connection, const RemoteShaderCompilerProtocol::FFrame& Frame, bool bKeepSourceFiles, FRemoteRequestSources& OutSources, String& OutError)
{
    if (const FJsonValue* NewFiles = Frame.Header.Find("newFiles"); NewFiles && NewFiles->IsArray())
    {
        for (int32 Index = 0; Index < NewFiles->Num(); ++Index)
        {
            const FJsonValue& NewFile     = (*NewFiles)[Index];
            const FJsonValue* PathValue   = NewFile.Find("path");
            const FJsonValue* HashValue   = NewFile.Find("hash");
            const FJsonValue* OffsetValue = NewFile.Find("offset");
            const FJsonValue* SizeValue   = NewFile.Find("size");

            const String Path   = PathValue ? PathValue->GetStringOr("") : String();
            const int64  Hash   = HashValue ? HashValue->GetInt64Or(-1) : -1;
            const int64  Offset = OffsetValue ? OffsetValue->GetInt64Or(-1) : -1;
            const int64  Size   = SizeValue ? SizeValue->GetInt64Or(-1) : -1;

            if (!RemoteShaderCompilerProtocol::IsValidSourcePath(Path))
            {
                OutError = String::Printf("'%s' is not a relative source path", *Path);
                return false;
            }

            if (Hash < 0 || Offset < 0 || Size < 0 || (Offset + Size) > static_cast<int64>(Frame.Payload.Size()))
            {
                OutError = String::Printf("The contents of '%s' are outside the payload", *Path);
                return false;
            }

            TSharedPtr<TArray<uint8>> Contents = MakeSharedPtr<TArray<uint8>>();
            if (Size > 0)
            {
                Contents->Append(Frame.Payload.Data() + Offset, static_cast<int32>(Size));
            }

            if (ShaderSourceHash::Compute(*Contents) != static_cast<uint32>(Hash))
            {
                OutError = String::Printf("The contents of '%s' do not match its hash", *Path);
                return false;
            }

            Connection.Files.Add(MakeFileKey(Path, static_cast<uint32>(Hash)), Contents);
        }
    }

    const FJsonValue* FileRefs = Frame.Header.Find("files");
    if (!FileRefs || !FileRefs->IsArray())
    {
        OutError = "The request lists no files";
        return false;
    }

    for (int32 Index = 0; Index < FileRefs->Num(); ++Index)
    {
        const FJsonValue& FileRef   = (*FileRefs)[Index];
        const FJsonValue* PathValue = FileRef.Find("path");
        const FJsonValue* HashValue = FileRef.Find("hash");

        const String Path = PathValue ? PathValue->GetStringOr("") : String();
        const int64  Hash = HashValue ? HashValue->GetInt64Or(-1) : -1;
        if (!RemoteShaderCompilerProtocol::IsValidSourcePath(Path) || Hash < 0)
        {
            OutError = String::Printf("'%s' is not a relative source path with a hash", *Path);
            return false;
        }

        const TSharedPtr<TArray<uint8>>* Contents = Connection.Files.Find(MakeFileKey(Path, static_cast<uint32>(Hash)));
        if (!Contents)
        {
            OutError = String::Printf("'%s' was referenced but never sent on this connection", *Path);
            return false;
        }

        OutSources.Files.Add(RemoteShaderCompilerProtocol::CollapsePath(String(SourceRoot) + '/' + Path), *Contents);

        if (bKeepSourceFiles)
        {
            FShaderSourceFile& SourceFile = OutSources.SourceFiles.Emplace();
            SourceFile.Path     = Path;
            SourceFile.Contents = **Contents;
            SourceFile.Hash     = static_cast<uint32>(Hash);
        }
    }

    return true;
}

void FRemoteShaderCompilerServer::HandleCompileRequest(const TSharedPtr<FConnection>& Connection, RemoteShaderCompilerProtocol::FFrame&& Frame)
{
    const TArray<String> NoDependencies;
    const TArray<uint8>  NoShaderCode;

    const FJsonValue* JobValue  = Frame.Header.Find("job");
    const FJsonValue* HopsValue = Frame.Header.Find("hops");
    const int32       Hops      = HopsValue ? static_cast<int32>(HopsValue->GetInt64Or(0)) : 0;

    FShaderCompileJob Job;
    String            Error;
    if (!JobValue || !FShaderCompileJob::FromJson(*JobValue, Job, Error))
    {
        QueueResult(*Connection, Frame.RequestId, RemoteShaderCompilerProtocol::StatusFailed, String::Printf("Invalid job: %s", *Error), NoDependencies, NoShaderCode);
        return;
    }

    if (!RemoteShaderCompilerProtocol::IsValidSourcePath(Job.SourceFile))
    {
        QueueResult(*Connection, Frame.RequestId, RemoteShaderCompilerProtocol::StatusFailed, String::Printf("'%s' is not a relative source path", *Job.SourceFile), NoDependencies, NoShaderCode);
        return;
    }

    for (const String& IncludeDir : Job.IncludeDirs)
    {
        if (!RemoteShaderCompilerProtocol::IsValidSourcePath(IncludeDir))
        {
            QueueResult(*Connection, Frame.RequestId, RemoteShaderCompilerProtocol::StatusFailed, String::Printf("The include directory '%s' is not a relative source path", *IncludeDir), NoDependencies, NoShaderCode);
            return;
        }
    }

    const bool bLocal   = FShaderCompiler::Get().CanCompileLocally(Job.OutputLanguage);
    const bool bForward = !bLocal && (Hops < RemoteShaderCompilerProtocol::MaxForwardHops) && (FindForwarder(Job.OutputLanguage) != nullptr);

    TSharedPtr<FRemoteRequestSources> Sources = MakeSharedPtr<FRemoteRequestSources>();
    if (!IngestFiles(*Connection, Frame, bForward, *Sources, Error))
    {
        QueueResult(*Connection, Frame.RequestId, RemoteShaderCompilerProtocol::StatusFailed, Error, NoDependencies, NoShaderCode);
        return;
    }

    if (!bLocal && !bForward)
    {
        const String Reason = String::Printf("%s cannot be compiled on this %s machine and no forward target serves it", ToString(Job.OutputLanguage), RemoteShaderCompilerProtocol::GetPlatformName());
        QueueResult(*Connection, Frame.RequestId, RemoteShaderCompilerProtocol::StatusUnavailable, Reason, NoDependencies, NoShaderCode);
        return;
    }

    NumTasksInFlight.Increment();

    const uint32 RequestId = Frame.RequestId;
    Tasks::Async([this, Connection, RequestId, Job = ::Move(Job), Sources, Hops, bForward]() mutable
    {
        RunCompile(Connection, RequestId, ::Move(Job), Sources, Hops, bForward);
        NumTasksInFlight.Decrement();
    });
}

void FRemoteShaderCompilerServer::RunCompile(TSharedPtr<FConnection> Connection, uint32 RequestId, FShaderCompileJob Job, TSharedPtr<FRemoteRequestSources> Sources, int32 Hops, bool bForward)
{
    const uint64 StartMilliseconds = GetServerMilliseconds();

    TArray<uint8>  ShaderCode;
    TArray<String> Dependencies;
    String         Messages;
    const CHAR*    Status = RemoteShaderCompilerProtocol::StatusFailed;
    String         Route;

    if (!bForward)
    {
        Route = "local";
        if (FShaderCompiler::Get().CompileJob(Job, SourceRoot, *Sources, ShaderCode, Dependencies, Messages))
        {
            Status = RemoteShaderCompilerProtocol::StatusSucceeded;
        }
    }
    else
    {
        FRemoteShaderCompilerClient* Forwarder = FindForwarder(Job.OutputLanguage);
        if (!Forwarder)
        {
            Route    = "forward";
            Status   = RemoteShaderCompilerProtocol::StatusUnavailable;
            Messages = String::Printf("The forward target for %s disconnected", ToString(Job.OutputLanguage));
        }
        else
        {
            Route = String::Printf("forwarded to %s", *Forwarder->GetPeerName());

            FRemoteCompileResult Result;
            const ERemoteCompileStatus ForwardStatus = Forwarder->Compile(Job, Sources->SourceFiles, Result, GForwardTimeoutMilliseconds, Hops + 1);
            switch (ForwardStatus)
            {
                case ERemoteCompileStatus::Succeeded:     Status = RemoteShaderCompilerProtocol::StatusSucceeded;   break;
                case ERemoteCompileStatus::CompileFailed: Status = RemoteShaderCompilerProtocol::StatusFailed;      break;
                default:                                  Status = RemoteShaderCompilerProtocol::StatusUnavailable; break;
            }

            ShaderCode   = ::Move(Result.ShaderCode);
            Dependencies = ::Move(Result.Dependencies);
            Messages     = ::Move(Result.Messages);
        }
    }

    QueueResult(*Connection, RequestId, Status, Messages, Dependencies, ShaderCode);

    const uint64 Elapsed = GetServerMilliseconds() - StartMilliseconds;
    const String JobName = Job.Name.IsEmpty() ? String::Printf("%s:%s", *Job.SourceFile, *Job.EntryPoint) : Job.Name;
    LOG_INFO("[RemoteShaderCompiler] %s %s %s %s, %s in %llu ms", *Connection->PeerAddress, *JobName, ToString(Job.OutputLanguage), Status, *Route, Elapsed);
}

void FRemoteShaderCompilerServer::QueueResult(FConnection& Connection, uint32 RequestId, const CHAR* Status, const String& Messages, const TArray<String>& Dependencies, const TArray<uint8>& ShaderCode)
{
    FJsonValue DependencyArray = FJsonValue::MakeArray();
    for (const String& Dependency : Dependencies)
    {
        DependencyArray.Add(FJsonValue(Dependency));
    }

    RemoteShaderCompilerProtocol::FFrame Frame;
    Frame.Type      = RemoteShaderCompilerProtocol::EMessageType::CompileResult;
    Frame.RequestId = RequestId;
    Frame.Header    = FJsonValue::MakeObject();
    Frame.Header.AddMember("status", FJsonValue(Status));
    Frame.Header.AddMember("messages", FJsonValue(Messages));
    Frame.Header.AddMember("dependencies", ::Move(DependencyArray));

    if (String(Status) == RemoteShaderCompilerProtocol::StatusSucceeded)
    {
        Frame.Payload = ShaderCode;
    }

    TScopedLock Lock(Connection.SendCS);
    RemoteShaderCompilerProtocol::EncodeFrame(Frame, Connection.SendBuffer);
}

void FRemoteShaderCompilerServer::FlushConnection(FConnection& Connection)
{
    TScopedLock Lock(Connection.SendCS);

    while (!Connection.bPendingClose && !Connection.SendBuffer.IsEmpty())
    {
        int32 BytesSent = 0;

        const ESocketResult Result = Connection.Socket.Send(Connection.SendBuffer.Data(), Connection.SendBuffer.Size(), BytesSent);
        if (Result == ESocketResult::WouldBlock)
        {
            break;
        }

        if (Result != ESocketResult::Success)
        {
            Connection.bPendingClose = true;
            break;
        }

        if (BytesSent >= Connection.SendBuffer.Size())
        {
            Connection.SendBuffer.Clear();
        }
        else if (BytesSent > 0)
        {
            Connection.SendBuffer.RemoveAt(0, BytesSent);
        }
    }
}

FRemoteShaderCompilerClient* FRemoteShaderCompilerServer::FindForwarder(EShaderOutputLanguage OutputLanguage)
{
    for (const TUniquePtr<FForwarder>& Forwarder : Forwarders)
    {
        if (Forwarder->Target.OutputLanguage != OutputLanguage || Forwarder->bConnecting.Load())
        {
            continue;
        }

        for (const FRemoteLanguageInfo& Language : Forwarder->Client->GetLanguages())
        {
            if (Language.OutputLanguage == OutputLanguage && !Language.bForwarded && Forwarder->Client->IsConnected())
            {
                return Forwarder->Client.Get();
            }
        }
    }

    return nullptr;
}

void FRemoteShaderCompilerServer::RetryForwarders()
{
    const uint64 Now = GetServerMilliseconds();

    for (TUniquePtr<FForwarder>& Forwarder : Forwarders)
    {
        if (Forwarder->Client->IsConnected() || Forwarder->bConnecting.Load() || Now < Forwarder->NextRetryMilliseconds)
        {
            continue;
        }

        Forwarder->NextRetryMilliseconds = Now + GForwardRetryMilliseconds;
        Forwarder->bConnecting.Store(true);
        NumTasksInFlight.Increment();

        FForwarder* Target = Forwarder.Get();
        Tasks::Async([this, Target]()
        {
            if (Target->Client->Connect(Target->Target.Host, Target->Target.Port, GForwardConnectMilliseconds))
            {
                LOG_INFO("[RemoteShaderCompiler] Forwarding %s to %s", ToString(Target->Target.OutputLanguage), *Target->Client->GetPeerName());
            }

            Target->bConnecting.Store(false);
            NumTasksInFlight.Decrement();
        });
    }
}

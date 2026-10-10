#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformThread.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/PlatformInterface/IPlatformEvent.h"
#include "Core/Threading/ScopedLock.h"
#include "ShaderCompiler/Remote/RemoteShaderCompilerClient.h"

static constexpr uint32 GClientPollIntervalMilliseconds = 10;
static constexpr int32  GClientReceiveChunkSize         = 64 * 1024;

static uint64 GetMillisecondsNow()
{
    return static_cast<uint64>(static_cast<double>(FPlatformTime::QueryPerformanceCounter()) * 1000.0 / static_cast<double>(FPlatformTime::QueryPerformanceFrequency()));
}

static String MakeSentFileKey(const String& Path, uint32 Hash)
{
    return String::Printf("%s:%u", *Path, Hash);
}

FRemoteShaderCompilerClient::FRemoteShaderCompilerClient()
    : Host()
    , Port(0)
    , Socket(ESocketType::TCP)
    , Thread(nullptr)
    , bStopRequested(false)
    , bConnected(false)
    , PeerPlatform()
    , LastError()
    , Languages()
    , LanguagesCS()
    , SendBuffer()
    , SentFiles()
    , SendCS()
    , ReceiveBuffer()
    , PendingRequests()
    , PendingCS()
    , NextRequestId(1)
{
}

FRemoteShaderCompilerClient::~FRemoteShaderCompilerClient()
{
    Disconnect();
}

bool FRemoteShaderCompilerClient::Connect(const String& InHost, uint16 InPort, uint32 TimeoutMilliseconds)
{
    Disconnect();

    Host = InHost;
    Port = InPort;

    FNetworkSocket NewSocket(ESocketType::TCP);
    if (!NewSocket.ConnectToHost(InHost, InPort, TimeoutMilliseconds))
    {
        LastError = String::Printf("Could not connect to %s", *FSocketAddress::FormatHostAndPort(InHost, InPort));
        return false;
    }

    {
        TScopedLock Lock(SendCS);
        Socket = ::Move(NewSocket);
    }

    ReceiveBuffer.Clear();

    if (!ReceiveHello(TimeoutMilliseconds))
    {
        Socket.Close();
        return false;
    }

    if (!Socket.SetNonBlocking(true))
    {
        LastError = "Could not make the socket non-blocking";
        Socket.Close();
        return false;
    }

    LastError.Clear();
    bStopRequested.Store(false);
    bConnected.Store(true);

    Thread = FPlatformThread::Create(this, "RemoteShaderCompilerClient");
    if (!Thread || !Thread->Start())
    {
        LastError = "Could not start the network thread";
        delete Thread;
        Thread = nullptr;

        bConnected.Store(false);
        Socket.Close();
        return false;
    }

    return true;
}

void FRemoteShaderCompilerClient::Disconnect()
{
    bConnected.Store(false);

    if (Thread)
    {
        bStopRequested.Store(true);
        Thread->WaitForCompletion();
        delete Thread;
        Thread = nullptr;
    }

    {
        // Compile sends under SendCS, so the socket only closes once no caller is inside Send
        TScopedLock Lock(SendCS);
        Socket.Close();
        SendBuffer.Clear();
        SentFiles.Clear();
    }

    FailAllPending();

    {
        TScopedLock Lock(LanguagesCS);
        Languages.Clear();
    }

    ReceiveBuffer.Clear();
    bStopRequested.Store(false);
}

bool FRemoteShaderCompilerClient::IsConnected() const
{
    return bConnected.Load();
}

bool FRemoteShaderCompilerClient::CanCompile(EShaderOutputLanguage OutputLanguage) const
{
    return IsConnected() && GetIdentity(OutputLanguage).HasValue();
}

TOptional<FShaderCompilerIdentity> FRemoteShaderCompilerClient::GetIdentity(EShaderOutputLanguage OutputLanguage) const
{
    TScopedLock Lock(LanguagesCS);
    for (const FRemoteLanguageInfo& Language : Languages)
    {
        if (Language.OutputLanguage == OutputLanguage)
        {
            return TOptional<FShaderCompilerIdentity>(EInPlace::InPlace, Language.Identity);
        }
    }

    return TOptional<FShaderCompilerIdentity>();
}

TArray<FRemoteLanguageInfo> FRemoteShaderCompilerClient::GetLanguages() const
{
    TScopedLock Lock(LanguagesCS);
    return Languages;
}

String FRemoteShaderCompilerClient::GetPeerName() const
{
    const String Address = FSocketAddress::FormatHostAndPort(Host, Port);
    return PeerPlatform.IsEmpty() ? Address : String::Printf("%s (%s)", *Address, *PeerPlatform);
}

String FRemoteShaderCompilerClient::GetLastError() const
{
    return LastError;
}

bool FRemoteShaderCompilerClient::ReceiveHello(uint32 TimeoutMilliseconds)
{
    const uint64 Deadline = GetMillisecondsNow() + TimeoutMilliseconds;

    TArray<uint8> Chunk;
    Chunk.Resize(GClientReceiveChunkSize);

    for (;;)
    {
        RemoteShaderCompilerProtocol::FFrame Frame;
        bool bHasFrame = false;
        if (!RemoteShaderCompilerProtocol::TryDecodeFrame(ReceiveBuffer, Frame, bHasFrame))
        {
            LastError = "The server sent a malformed frame";
            return false;
        }

        if (bHasFrame)
        {
            if (Frame.Type == RemoteShaderCompilerProtocol::EMessageType::Rejected)
            {
                const FJsonValue* Reason = Frame.Header.Find("reason");
                LastError = String::Printf("The server rejected the connection: %s", Reason ? *Reason->GetStringOr("no reason given") : "no reason given");
                return false;
            }

            if (Frame.Type != RemoteShaderCompilerProtocol::EMessageType::Hello)
            {
                LastError = "The server did not start with a Hello";
                return false;
            }

            const FJsonValue* ProtocolValue = Frame.Header.Find("protocol");
            if (!ProtocolValue || ProtocolValue->GetInt64Or(0) != static_cast<int64>(RemoteShaderCompilerProtocol::Version))
            {
                LastError = String::Printf("The server speaks protocol %lld, this client speaks %u", ProtocolValue ? ProtocolValue->GetInt64Or(0) : 0ll, RemoteShaderCompilerProtocol::Version);
                return false;
            }

            const FJsonValue* PlatformValue = Frame.Header.Find("platform");
            PeerPlatform = PlatformValue ? PlatformValue->GetStringOr("") : String();

            TArray<FRemoteLanguageInfo> NewLanguages;
            if (const FJsonValue* LanguagesValue = Frame.Header.Find("languages"); LanguagesValue && LanguagesValue->IsArray())
            {
                for (int32 Index = 0; Index < LanguagesValue->Num(); ++Index)
                {
                    const FJsonValue& LanguageValue = (*LanguagesValue)[Index];
                    const FJsonValue* NameValue     = LanguageValue.Find("language");
                    const FJsonValue* RouteValue    = LanguageValue.Find("route");
                    const FJsonValue* IdentityValue = LanguageValue.Find("identity");

                    FRemoteLanguageInfo Language;
                    if (!NameValue || !TryParseShaderOutputLanguage(NameValue->GetStringOr(""), Language.OutputLanguage))
                    {
                        continue;
                    }

                    if (!IdentityValue || !RemoteShaderCompilerProtocol::IdentityFromJson(*IdentityValue, Language.Identity))
                    {
                        continue;
                    }

                    Language.bForwarded = RouteValue && RouteValue->GetStringOr("") == "forwarded";
                    NewLanguages.Add(::Move(Language));
                }
            }

            TScopedLock Lock(LanguagesCS);
            Languages = ::Move(NewLanguages);
            return true;
        }

        const uint64 Now = GetMillisecondsNow();
        if (Now >= Deadline)
        {
            LastError = "The server did not send a Hello in time";
            return false;
        }

        FNetworkSocket* Sockets[] = { &Socket };
        bool            bReadable = false;
        const int32     NumReady  = FNetworkSocket::WaitForRead(Sockets, 1, &bReadable, static_cast<uint32>(Deadline - Now));
        if (NumReady < 0)
        {
            LastError = "Waiting for the Hello failed";
            return false;
        }

        if (!bReadable)
        {
            continue;
        }

        int32 BytesRead = 0;
        if (Socket.Recv(Chunk.Data(), Chunk.Size(), BytesRead) != ESocketResult::Success)
        {
            LastError = "The server closed the connection before its Hello";
            return false;
        }

        ReceiveBuffer.Append(Chunk.Data(), BytesRead);
    }
}

ERemoteCompileStatus FRemoteShaderCompilerClient::Compile(const FShaderCompileJob& Job, const TArray<FShaderSourceFile>& Files, FRemoteCompileResult& OutResult, uint32 TimeoutMilliseconds, int32 Hops)
{
    if (!IsConnected())
    {
        return ERemoteCompileStatus::Unreachable;
    }

    FPendingRequest Pending;
    Pending.Event = IPlatformEvent::Create(false);

    uint32 RequestId = 0;
    {
        TScopedLock SendLock(SendCS);
        if (!IsConnected())
        {
            IPlatformEvent::Recycle(Pending.Event);
            return ERemoteCompileStatus::Unreachable;
        }

        RequestId = NextRequestId++;

        {
            TScopedLock PendingLock(PendingCS);
            PendingRequests.Add(RequestId, &Pending);
        }

        FJsonValue FileRefs = FJsonValue::MakeArray();
        FJsonValue NewFiles = FJsonValue::MakeArray();

        RemoteShaderCompilerProtocol::FFrame Frame;
        Frame.Type      = RemoteShaderCompilerProtocol::EMessageType::CompileRequest;
        Frame.RequestId = RequestId;

        for (const FShaderSourceFile& File : Files)
        {
            FJsonValue FileRef = FJsonValue::MakeObject();
            FileRef.AddMember("path", FJsonValue(File.Path));
            FileRef.AddMember("hash", FJsonValue(File.Hash));
            FileRefs.Add(::Move(FileRef));

            bool bAlreadySent = false;
            SentFiles.Add(MakeSentFileKey(File.Path, File.Hash), &bAlreadySent);
            if (bAlreadySent)
            {
                continue;
            }

            FJsonValue NewFile = FJsonValue::MakeObject();
            NewFile.AddMember("path", FJsonValue(File.Path));
            NewFile.AddMember("hash", FJsonValue(File.Hash));
            NewFile.AddMember("offset", FJsonValue(Frame.Payload.Size()));
            NewFile.AddMember("size", FJsonValue(File.Contents.Size()));
            NewFiles.Add(::Move(NewFile));

            Frame.Payload.Append(File.Contents.Data(), File.Contents.Size());
        }

        Frame.Header = FJsonValue::MakeObject();
        Frame.Header.AddMember("hops", FJsonValue(Hops));
        Frame.Header.AddMember("job", Job.ToJson());
        Frame.Header.AddMember("files", ::Move(FileRefs));
        Frame.Header.AddMember("newFiles", ::Move(NewFiles));

        RemoteShaderCompilerProtocol::EncodeFrame(Frame, SendBuffer);

        if (!FlushSendBuffer())
        {
            bConnected.Store(false);
        }
    }

    Pending.Event->Wait(static_cast<uint64>(TimeoutMilliseconds));

    ERemoteCompileStatus Status = ERemoteCompileStatus::Unreachable;
    {
        TScopedLock PendingLock(PendingCS);
        if (Pending.bCompleted)
        {
            Status    = Pending.Status;
            OutResult = ::Move(Pending.Result);
        }
        else
        {
            PendingRequests.Remove(RequestId);
            OutResult.Messages = String::Printf("%s did not answer within %u ms", *GetPeerName(), TimeoutMilliseconds);
        }
    }

    IPlatformEvent::Recycle(Pending.Event);
    return Status;
}

bool FRemoteShaderCompilerClient::FlushSendBuffer()
{
    while (!SendBuffer.IsEmpty())
    {
        int32 BytesSent = 0;
        const ESocketResult Result = Socket.Send(SendBuffer.Data(), SendBuffer.Size(), BytesSent);
        if (Result == ESocketResult::WouldBlock)
        {
            return true;
        }

        if (Result != ESocketResult::Success)
        {
            return false;
        }

        if (BytesSent >= SendBuffer.Size())
        {
            SendBuffer.Clear();
        }
        else if (BytesSent > 0)
        {
            SendBuffer.RemoveAt(0, BytesSent);
        }
    }

    return true;
}

void FRemoteShaderCompilerClient::HandleFrame(RemoteShaderCompilerProtocol::FFrame&& Frame)
{
    if (Frame.Type == RemoteShaderCompilerProtocol::EMessageType::Rejected)
    {
        const FJsonValue* Reason = Frame.Header.Find("reason");
        LOG_WARNING("[RemoteShaderCompiler] %s closed the connection: %s", *GetPeerName(), Reason ? *Reason->GetStringOr("no reason given") : "no reason given");
        bConnected.Store(false);
        return;
    }

    if (Frame.Type != RemoteShaderCompilerProtocol::EMessageType::CompileResult)
    {
        return;
    }

    TScopedLock Lock(PendingCS);

    FPendingRequest** Found = PendingRequests.Find(Frame.RequestId);
    if (!Found)
    {
        return;
    }

    FPendingRequest* Pending = *Found;
    PendingRequests.Remove(Frame.RequestId);

    const FJsonValue* StatusValue   = Frame.Header.Find("status");
    const FJsonValue* MessagesValue = Frame.Header.Find("messages");
    const String      StatusText    = StatusValue ? StatusValue->GetStringOr("") : String();

    Pending->Result.Messages = MessagesValue ? MessagesValue->GetStringOr("") : String();

    if (const FJsonValue* DependenciesValue = Frame.Header.Find("dependencies"); DependenciesValue && DependenciesValue->IsArray())
    {
        for (int32 Index = 0; Index < DependenciesValue->Num(); ++Index)
        {
            String Dependency;
            if ((*DependenciesValue)[Index].TryGetString(Dependency) && RemoteShaderCompilerProtocol::IsValidSourcePath(Dependency))
            {
                Pending->Result.Dependencies.Add(Dependency);
            }
        }
    }

    if (StatusText == RemoteShaderCompilerProtocol::StatusSucceeded)
    {
        Pending->Status            = ERemoteCompileStatus::Succeeded;
        Pending->Result.ShaderCode = ::Move(Frame.Payload);
    }
    else if (StatusText == RemoteShaderCompilerProtocol::StatusFailed)
    {
        Pending->Status = ERemoteCompileStatus::CompileFailed;
    }
    else
    {
        Pending->Status = ERemoteCompileStatus::Unreachable;
    }

    Pending->bCompleted = true;
    Pending->Event->Trigger();
}

void FRemoteShaderCompilerClient::FailAllPending()
{
    TScopedLock Lock(PendingCS);

    PendingRequests.Foreach([](const uint32&, FPendingRequest* Pending)
    {
        Pending->Status          = ERemoteCompileStatus::Unreachable;
        Pending->Result.Messages = "The connection to the remote shader compiler was lost";
        Pending->bCompleted      = true;
        Pending->Event->Trigger();
    });

    PendingRequests.Clear();
}

bool FRemoteShaderCompilerClient::Start()
{
    return true;
}

int32 FRemoteShaderCompilerClient::Run()
{
    TArray<uint8> Chunk;
    Chunk.Resize(GClientReceiveChunkSize);

    while (!bStopRequested.Load() && bConnected.Load())
    {
        {
            TScopedLock Lock(SendCS);
            if (!FlushSendBuffer())
            {
                break;
            }
        }

        FNetworkSocket* Sockets[] = { &Socket };
        bool            bReadable = false;
        const int32     NumReady  = FNetworkSocket::WaitForRead(Sockets, 1, &bReadable, GClientPollIntervalMilliseconds);
        if (NumReady < 0)
        {
            break;
        }

        if (!bReadable)
        {
            continue;
        }

        bool bClosed = false;
        for (;;)
        {
            int32 BytesRead = 0;
            const ESocketResult Result = Socket.Recv(Chunk.Data(), Chunk.Size(), BytesRead);
            if (Result == ESocketResult::WouldBlock)
            {
                break;
            }

            if (Result != ESocketResult::Success)
            {
                bClosed = true;
                break;
            }

            ReceiveBuffer.Append(Chunk.Data(), BytesRead);
        }

        for (;;)
        {
            RemoteShaderCompilerProtocol::FFrame Frame;
            bool bHasFrame = false;
            if (!RemoteShaderCompilerProtocol::TryDecodeFrame(ReceiveBuffer, Frame, bHasFrame))
            {
                LOG_WARNING("[RemoteShaderCompiler] %s sent a malformed frame, disconnecting", *GetPeerName());
                bClosed = true;
                break;
            }

            if (!bHasFrame)
            {
                break;
            }

            HandleFrame(::Move(Frame));
        }

        if (bClosed)
        {
            break;
        }
    }

    if (!bStopRequested.Load())
    {
        LOG_WARNING("[RemoteShaderCompiler] Lost the connection to %s", *GetPeerName());
    }

    bConnected.Store(false);
    FailAllPending();
    return 0;
}

void FRemoteShaderCompilerClient::Stop()
{
    bStopRequested.Store(true);
}

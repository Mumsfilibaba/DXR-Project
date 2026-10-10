#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Network/NetworkSocket.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/PlatformInterface/IPlatformThread.h"
#include "Core/Threading/Runnable.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "ShaderCompiler/ShaderPreprocessor.h"
#include "ShaderCompiler/Remote/RemoteShaderCompilerClient.h"
#include "ShaderCompiler/Remote/RemoteShaderCompilerProtocol.h"

struct FRemoteForwardTarget
{
    EShaderOutputLanguage OutputLanguage = EShaderOutputLanguage::Unknown;
    String                Host;
    uint16                Port = RemoteShaderCompilerProtocol::DefaultPort;
};

struct FRemoteShaderCompilerServerSettings
{
    /**
     * Only used with bAllowRemoteConnections, any IPv4 or IPv6 literal FNetworkSocket::ParseAddress reads. "::" binds
     * dual-stack and falls back to 0.0.0.0 where IPv6 is unavailable. Without remote connections the server binds
     * 127.0.0.1 and ::1 instead.
     */
    String                       BindAddress             = "::";

    /** Zero lets the OS pick, which is what the tests do */
    uint16                       Port                    = RemoteShaderCompilerProtocol::DefaultPort;
    bool                         bAllowRemoteConnections = false;
    int32                        MaxClients              = 16;

    /** Where the languages this machine cannot produce are sent */
    TArray<FRemoteForwardTarget> ForwardTargets;
};

class FRemoteRequestSources final : public IShaderSourceProvider
{
public:
    NODISCARD virtual bool FileExists(const String& Path) const override final;
    virtual bool ReadFile(const String& Path, TArray<CHAR>& OutText) const override final;

    /** Keyed by the collapsed path below the server's source root */
    TMap<String, TSharedPtr<TArray<uint8>>> Files;

    /** The same files as they were sent, only filled when the request is forwarded */
    TArray<FShaderSourceFile>               SourceFiles;
};

class SHADERCOMPILER_API FRemoteShaderCompilerServer final : public FRunnable
{
public:

    /** Stands in for the asset directory on the server, nothing below it exists on disk */
    static constexpr const CHAR* SourceRoot = "/RemoteShaderCompiler/Assets";

public:
    explicit FRemoteShaderCompilerServer(const FRemoteShaderCompilerServerSettings& InSettings);
    ~FRemoteShaderCompilerServer();

    /** @brief Opens the listen sockets, connects the forward targets it can reach, and starts the network thread */
    bool Launch();

    /** @brief Stops accepting, waits for the compiles in flight and closes every connection */
    void Shutdown();

    /** @return The port of the first listen socket, from GetLocalAddress, which is how a test that bound port 0 finds it */
    NODISCARD uint16 GetPort() const;
    NODISCARD int32  GetNumConnections() const;

    /** @return Every bound address, "127.0.0.1:9371 and [::1]:9371", for the startup log */
    NODISCARD String DescribeListeners() const;

public:

    // FRunnable
    virtual bool  Start() override final;
    virtual int32 Run() override final;
    virtual void  Stop() override final;

private:
    struct FConnection
    {
        FConnection(uint32 InConnectionId, FNetworkSocket&& InSocket, const String& InPeerAddress)
            : ConnectionId(InConnectionId)
            , Socket(::Move(InSocket))
            , PeerAddress(InPeerAddress)
            , ReceiveBuffer()
            , Files()
            , SendBuffer()
            , SendCS()
            , CloseReason()
            , bPendingClose(false)
        {
        }

        uint32                                  ConnectionId;
        FNetworkSocket                          Socket;
        String                                  PeerAddress;
        TArray<uint8>                           ReceiveBuffer;
        TMap<String, TSharedPtr<TArray<uint8>>> Files;
        TArray<uint8>                           SendBuffer;
        FCriticalSection                        SendCS;
        String                                  CloseReason;
        bool                                    bPendingClose;
    };

    struct FForwarder
    {
        FRemoteForwardTarget                    Target;
        TUniquePtr<FRemoteShaderCompilerClient> Client;
        AtomicBool                              bConnecting;
        uint64                                  NextRetryMilliseconds = 0;
    };

    bool ListenOn(const FSocketAddress& Address);
    void AcceptConnections(FNetworkSocket& ListenSocket, TArray<String>& OutLogMessages);
    void Reject(FNetworkSocket& Socket, const CHAR* Reason);
    void ReceiveFrom(const TSharedPtr<FConnection>& Connection);
    void HandleCompileRequest(const TSharedPtr<FConnection>& Connection, RemoteShaderCompilerProtocol::FFrame&& Frame);
    bool IngestFiles(FConnection& Connection, const RemoteShaderCompilerProtocol::FFrame& Frame, bool bKeepSourceFiles, FRemoteRequestSources& OutSources, String& OutError);
    void FlushConnection(FConnection& Connection);
    void RetryForwarders();
    void RunCompile(TSharedPtr<FConnection> Connection, uint32 RequestId, FShaderCompileJob Job, TSharedPtr<FRemoteRequestSources> Sources, int32 Hops, bool bForward);
    void QueueResult(FConnection& Connection, uint32 RequestId, const CHAR* Status, const String& Messages, const TArray<String>& Dependencies, const TArray<uint8>& ShaderCode);

    NODISCARD FRemoteShaderCompilerClient* FindForwarder(EShaderOutputLanguage OutputLanguage);
    NODISCARD RemoteShaderCompilerProtocol::FFrame BuildHello();

    FRemoteShaderCompilerServerSettings Settings;
    TArray<FNetworkSocket>              ListenSockets;
    TArray<FSocketAddress>              ListenAddresses;
    IPlatformThread*                    Thread;
    AtomicBool                          bStopRequested;
    TArray<TSharedPtr<FConnection>>     Connections;
    mutable FCriticalSection            ConnectionsCS;
    uint32                              NextConnectionId;
    TArray<TUniquePtr<FForwarder>>      Forwarders;
    AtomicInt32                         NumTasksInFlight;
};

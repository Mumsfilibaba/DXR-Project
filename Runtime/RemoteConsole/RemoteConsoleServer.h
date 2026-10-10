#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Json/JsonValue.h"
#include "Core/Misc/IOutputDevice.h"
#include "Core/Network/NetworkSocket.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/PlatformInterface/IPlatformThread.h"
#include "Core/Threading/Runnable.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "Core/Threading/Atomic/AtomicInt.h"

struct FRemoteConsoleRequest
{
    uint32     ClientId   = 0;
    FJsonValue Id;                // Echoed back verbatim, null for raw text lines
    String     Type;
    FJsonValue Payload;           // The whole request object
    bool       bIsRawText = false;
};

struct FRemoteConsoleClient
{
    FRemoteConsoleClient(uint32 InClientId, FNetworkSocket&& InSocket, const String& InPeerAddress, const String& InSendBuffer)
        : ClientId(InClientId)
        , Socket(::Move(InSocket))
        , PeerAddress(InPeerAddress)
        , ReceiveBuffer()
        , SendBuffer(InSendBuffer)
        , CloseReason()
        , bSubscribedToLog(false)
        , bPendingClose(false)
    {
    }

    uint32         ClientId;
    FNetworkSocket Socket;
    String         PeerAddress;
    String         ReceiveBuffer;
    String         SendBuffer;
    String         CloseReason;      // Logged by the network thread once the client is removed
    bool           bSubscribedToLog;
    bool           bPendingClose;
};

class FRemoteConsoleLogDevice final : public IOutputDevice
{
public:
    FRemoteConsoleLogDevice();
    virtual ~FRemoteConsoleLogDevice();

    virtual void Log(const String& Message) override final;
    virtual void Log(ELogSeverity Severity, const String& Message) override final;

    /** @brief Main thread. Starts recording main-thread log lines. */
    void BeginCapture();

    /** @return Main thread. The lines recorded since BeginCapture, as an array of {severity, message}. */
    FJsonValue EndCapture();

private:
    FJsonValue CapturedLines;
    bool       bIsCapturing;
};

class REMOTECONSOLE_API FRemoteConsoleServer final : public FRunnable
{
public:

    /** @brief Applies the RemoteConsole.* settings. Opens no socket when RemoteConsole.Enable is false. Returns false only when the socket fails to open. */
    static bool Initialize();

    /** @brief Stops the network thread, flushes what it can and closes every socket. */
    static void Release();

    /**
     * @brief Main thread, once per frame. Restarts or stops the server when a RemoteConsole.* setting changed
     *        since the last call, then runs every request that arrived.
     */
    static void Tick();

    /** @brief Called by the RemoteConsole.* CVar delegates. The server restarts with the new settings on the next Tick. */
    static void RequestSettingsRefresh();

    /** @return The running server, or nullptr when disabled or not initialized */
    static FRemoteConsoleServer* Get();

    /** @return Main thread. How many remote console clients are connected right now, zero when the server is not running. */
    static int32 GetNumConnectedClients();

    /** @brief Any thread. Queues a log event for every subscribed client of the running server, if there is one. */
    static void BroadcastLog(ELogSeverity Severity, const String& Message);

public:
    virtual bool  Start() override final;
    virtual int32 Run() override final;
    virtual void  Stop() override final;

private:
    FRemoteConsoleServer(bool bInAllowRemoteConnections);
    ~FRemoteConsoleServer();

    static bool StartFromSettings();
    static void StopInstance();

    bool ListenOn(const FSocketAddress& Address);
    NODISCARD String DescribeListeners() const;
    void ProcessRequests();

    // Network thread
    void AcceptClients(FNetworkSocket* ListenSocket, TArray<String>& OutLogMessages);
    void RejectClient(FNetworkSocket* Socket, const CHAR* Reason);
    void ReceiveFromClient(FRemoteConsoleClient& Client, TArray<FRemoteConsoleRequest>& OutRequests);
    void FlushClient(FRemoteConsoleClient& Client);
    void FlushAllClients(uint32 TimeoutMilliseconds);
    bool ParseLine(FRemoteConsoleClient& Client, const String& Line, FRemoteConsoleRequest& OutRequest);

    // Main thread
    void HandleRequest(const FRemoteConsoleRequest& Request);
    void HandleExec(const FRemoteConsoleRequest& Request);
    void HandleList(const FRemoteConsoleRequest& Request);
    void HandleGet(const FRemoteConsoleRequest& Request);
    void HandleSubscribeLog(const FRemoteConsoleRequest& Request);

    FJsonValue MakeResult(const FRemoteConsoleRequest& Request, bool bSucceeded) const;
    void       SendError(const FRemoteConsoleRequest& Request, const String& Error);
    void       SendResult(const FRemoteConsoleRequest& Request, const FJsonValue& Result);
    void       SendToClient(uint32 ClientId, const String& Line);

    TArray<FNetworkSocket>                   ListenSockets;
    TArray<FSocketAddress>                   ListenAddresses;
    IPlatformThread*                         Thread;
    AtomicBool                               bStopRequested;
    String                                   HelloLine;
    TArray<TUniquePtr<FRemoteConsoleClient>> Clients;
    FCriticalSection                         ClientsCS;
    uint32                                   NextClientId;
    AtomicInt32                              NumConnectedClients;
    TArray<FRemoteConsoleRequest>            PendingRequests;
    FCriticalSection                         PendingRequestsCS;
    const bool                               bAllowRemoteConnections;
    static FRemoteConsoleServer*             Instance;
    static FCriticalSection                  InstanceCS;
    static bool                              bIsInitialized;    // Settings changes before Initialize are applied by Initialize itself
    static bool                              bSettingsChanged;  // Set by the CVar delegates, consumed by Tick
};

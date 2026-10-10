#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/Optional.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/String.h"
#include "Core/Network/NetworkSocket.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/PlatformInterface/IPlatformThread.h"
#include "Core/Threading/Runnable.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "ShaderCompiler/ShaderCompileJob.h"
#include "ShaderCompiler/ShaderCompilerIdentity.h"
#include "ShaderCompiler/Remote/RemoteShaderCompilerProtocol.h"

struct IPlatformEvent;

enum class ERemoteCompileStatus : uint8
{
    Succeeded,

    /** The shader has an error, compiling it anywhere else gives the same result */
    CompileFailed,

    /** No connection, a timeout, or the server cannot serve the language. The caller may compile it itself. */
    Unreachable,
};

struct FRemoteCompileResult
{
    /** An FShaderCode container */
    TArray<uint8>  ShaderCode;

    /** Relative to the asset directory */
    TArray<String> Dependencies;

    /** Compiler output, with paths relative to the asset directory */
    String         Messages;
};

struct FRemoteLanguageInfo
{
    EShaderOutputLanguage   OutputLanguage = EShaderOutputLanguage::Unknown;
    FShaderCompilerIdentity Identity;
    bool                    bForwarded     = false;
};

class SHADERCOMPILER_API FRemoteShaderCompilerClient final : public FRunnable
{
public:
    FRemoteShaderCompilerClient();
    ~FRemoteShaderCompilerClient();

    /**
     * @brief Resolves InHost, connects with FNetworkSocket::ConnectToHost (IPv4 or IPv6) and waits for the Hello.
     *        Closes any earlier connection first, which fails its in-flight requests.
     */
    bool Connect(const String& InHost, uint16 InPort, uint32 TimeoutMilliseconds);
    void Disconnect();

    NODISCARD bool IsConnected() const;
    NODISCARD bool CanCompile(EShaderOutputLanguage OutputLanguage) const;
    NODISCARD TOptional<FShaderCompilerIdentity> GetIdentity(EShaderOutputLanguage OutputLanguage) const;
    NODISCARD TArray<FRemoteLanguageInfo> GetLanguages() const;

    /** @return FSocketAddress::FormatHostAndPort plus the platform, "192.168.1.20:9371 (macOS)" or "[fe80::1]:9371 (macOS)" */
    NODISCARD String GetPeerName() const;

    /** @return Why the last Connect failed, empty after a successful one */
    NODISCARD String GetLastError() const;

    /**
     * @brief Blocking and thread-safe. Files this connection has not sent yet go along with the request.
     * @param Hops Zero from the engine and the batch tool. A server forwarding a request passes its own hop count plus one.
     */
    ERemoteCompileStatus Compile(const FShaderCompileJob& Job, const TArray<FShaderSourceFile>& Files, FRemoteCompileResult& OutResult, uint32 TimeoutMilliseconds, int32 Hops = 0);

public:

    // FRunnable: the network thread flushes SendBuffer, decodes results and wakes the waiting callers
    virtual bool  Start() override final;
    virtual int32 Run() override final;
    virtual void  Stop() override final;

private:
    struct FPendingRequest
    {
        IPlatformEvent*      Event      = nullptr;
        ERemoteCompileStatus Status     = ERemoteCompileStatus::Unreachable;
        bool                 bCompleted = false;
        FRemoteCompileResult Result;
    };

    bool ReceiveHello(uint32 TimeoutMilliseconds);
    void HandleFrame(RemoteShaderCompilerProtocol::FFrame&& Frame);
    void FailAllPending();
    bool FlushSendBuffer();

    String                          Host;
    uint16                          Port;
    FNetworkSocket                  Socket;
    IPlatformThread*                Thread;
    AtomicBool                      bStopRequested;
    AtomicBool                      bConnected;
    String                          PeerPlatform;
    String                          LastError;
    TArray<FRemoteLanguageInfo>     Languages;
    mutable FCriticalSection        LanguagesCS;
    TArray<uint8>                   SendBuffer;
    TSet<String>                    SentFiles;
    FCriticalSection                SendCS;
    TArray<uint8>                   ReceiveBuffer;
    TMap<uint32, FPendingRequest*>  PendingRequests;
    FCriticalSection                PendingCS;
    uint32                          NextRequestId;
};

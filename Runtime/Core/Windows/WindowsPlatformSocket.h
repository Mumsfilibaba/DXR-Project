#pragma once
#include "Core/Windows/Windows.h"
#include "Core/PlatformInterface/IPlatformSocket.h"

class CORE_API FWindowsPlatformSocket final : public IPlatformSocket
{
public:
    static IPlatformSocket* Create(ESocketType Type, ESocketFamily Family);
    static bool   Resolve(const String& Host, uint16 Port, TArray<FSocketAddress>& OutAddresses);
    static bool   ParseAddress(const String& Text, uint16 Port, FSocketAddress& OutAddress);
    static String AddressToString(const FSocketAddress& Address);
    static int32  WaitForRead(IPlatformSocket* const* Sockets, int32 NumSockets, bool* OutReadable, uint32 TimeoutMilliseconds);

public:
    virtual ~FWindowsPlatformSocket();

    virtual bool             Bind(const FSocketAddress& Address) override final;
    virtual bool             Listen(int32 Backlog) override final;
    virtual IPlatformSocket* Accept() override final;
    virtual bool             Connect(const FSocketAddress& Address, uint32 TimeoutMilliseconds) override final;
    virtual ESocketResult    Send(const void* Data, int32 Size, int32& OutBytesSent) override final;
    virtual ESocketResult    Recv(void* Buffer, int32 Size, int32& OutBytesRead) override final;
    virtual ESocketResult    SendTo(const void* Data, int32 Size, const FSocketAddress& Address, int32& OutBytesSent) override final;
    virtual ESocketResult    RecvFrom(void* Buffer, int32 Size, int32& OutBytesRead, FSocketAddress& OutAddress) override final;
    virtual bool             SetNonBlocking(bool bNonBlocking) override final;
    virtual void             Close() override final;
    virtual bool             IsValid() const override final;
    virtual bool             GetPeerAddress(FSocketAddress& OutAddress) const override final;
    virtual bool             GetLocalAddress(FSocketAddress& OutAddress) const override final;

    virtual ESocketType GetType() const override final
    {
        return Type;
    }

    virtual ESocketFamily GetFamily() const override final
    {
        return Family;
    }

private:
    static bool InitializeWinsock();

    FWindowsPlatformSocket(ESocketType InType, ESocketFamily InFamily, UINT_PTR InSocket);

    UINT_PTR      Socket; // SOCKET, kept as UINT_PTR so this header does not need winsock2.h
    ESocketType   Type;
    ESocketFamily Family;
    bool          bIsNonBlocking;
};

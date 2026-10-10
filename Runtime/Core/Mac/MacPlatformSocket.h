#pragma once
#include "Core/PlatformInterface/IPlatformSocket.h"

class CORE_API FMacPlatformSocket final : public IPlatformSocket
{
public:
    static IPlatformSocket* Create(ESocketType Type, ESocketFamily Family);
    static bool   Resolve(const String& Host, uint16 Port, TArray<FSocketAddress>& OutAddresses);
    static bool   ParseAddress(const String& Text, uint16 Port, FSocketAddress& OutAddress);
    static String AddressToString(const FSocketAddress& Address);
    static int32  WaitForRead(IPlatformSocket* const* Sockets, int32 NumSockets, bool* OutReadable, uint32 TimeoutMilliseconds);

public:
    virtual ~FMacPlatformSocket();

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
    FMacPlatformSocket(ESocketType InType, ESocketFamily InFamily, int32 InFileDescriptor);

    int32         FileDescriptor;
    ESocketType   Type;
    ESocketFamily Family;
    bool          bIsNonBlocking;
};

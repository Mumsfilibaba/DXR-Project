#include "Core/Network/NetworkSocket.h"

FNetworkSocket::FNetworkSocket(ESocketType InType)
    : FNonCopyable()
    , PlatformSocket(nullptr)
    , Type(InType)
    , bIsNonBlocking(false)
{
}

FNetworkSocket::FNetworkSocket(FNetworkSocket&& Other)
    : FNonCopyable()
    , PlatformSocket(::Move(Other.PlatformSocket))
    , Type(Other.Type)
    , bIsNonBlocking(Other.bIsNonBlocking)
{
}

FNetworkSocket& FNetworkSocket::operator=(FNetworkSocket&& Other)
{
    if (this != &Other)
    {
        PlatformSocket = ::Move(Other.PlatformSocket);
        Type           = Other.Type;
        bIsNonBlocking = Other.bIsNonBlocking;
    }

    return *this;
}

FNetworkSocket::~FNetworkSocket()
{
    Close();
}

bool FNetworkSocket::Resolve(const String& Host, uint16 Port, TArray<FSocketAddress>& OutAddresses)
{
    return FPlatformSocket::Resolve(Host, Port, OutAddresses);
}

bool FNetworkSocket::ParseAddress(const String& Text, uint16 Port, FSocketAddress& OutAddress)
{
    return FPlatformSocket::ParseAddress(Text, Port, OutAddress);
}

int32 FNetworkSocket::WaitForRead(FNetworkSocket* const* Sockets, int32 NumSockets, bool* OutReadable, uint32 TimeoutMilliseconds)
{
    TArray<IPlatformSocket*> PlatformSockets;
    PlatformSockets.Reserve(NumSockets);

    for (int32 Index = 0; Index < NumSockets; ++Index)
    {
        PlatformSockets.Add(Sockets[Index] ? Sockets[Index]->GetPlatformSocket() : nullptr);
    }

    return FPlatformSocket::WaitForRead(PlatformSockets.Data(), NumSockets, OutReadable, TimeoutMilliseconds);
}

bool FNetworkSocket::CreatePlatformSocket(ESocketFamily Family)
{
    PlatformSocket.Reset(FPlatformSocket::Create(Type, Family));
    if (!PlatformSocket)
    {
        return false;
    }

    if (!PlatformSocket->SetNonBlocking(bIsNonBlocking))
    {
        PlatformSocket.Reset();
        return false;
    }

    return true;
}

bool FNetworkSocket::Bind(const FSocketAddress& Address)
{
    ESocketFamily Family = ESocketFamily::IPv6;
    if (Address.IsIPv4())
    {
        Family = ESocketFamily::IPv4;
    }
    else if (Address.IsUnspecified())
    {
        Family = ESocketFamily::DualStack;
    }

    if (!CreatePlatformSocket(Family) || !PlatformSocket->Bind(Address))
    {
        PlatformSocket.Reset();
        return false;
    }

    return true;
}

bool FNetworkSocket::Listen(int32 Backlog)
{
    return PlatformSocket && PlatformSocket->Listen(Backlog);
}

bool FNetworkSocket::Accept(FNetworkSocket& OutClient)
{
    IPlatformSocket* AcceptedSocket = PlatformSocket ? PlatformSocket->Accept() : nullptr;
    if (!AcceptedSocket)
    {
        return false;
    }

    OutClient = FNetworkSocket(Type);
    OutClient.PlatformSocket.Reset(AcceptedSocket);
    OutClient.bIsNonBlocking = true;
    return true;
}

bool FNetworkSocket::Connect(const FSocketAddress& Address, uint32 TimeoutMilliseconds)
{
    const ESocketFamily Family = Address.IsIPv4() ? ESocketFamily::IPv4 : ESocketFamily::IPv6;
    if (!CreatePlatformSocket(Family) || !PlatformSocket->Connect(Address, TimeoutMilliseconds))
    {
        PlatformSocket.Reset();
        return false;
    }

    return true;
}

bool FNetworkSocket::ConnectToAny(const TArray<FSocketAddress>& Addresses, uint32 TimeoutMilliseconds)
{
    for (const FSocketAddress& Address : Addresses)
    {
        if (Connect(Address, TimeoutMilliseconds))
        {
            return true;
        }
    }

    return false;
}

bool FNetworkSocket::ConnectToHost(const String& Host, uint16 Port, uint32 TimeoutMilliseconds)
{
    TArray<FSocketAddress> Addresses;
    return Resolve(Host, Port, Addresses) && ConnectToAny(Addresses, TimeoutMilliseconds);
}

ESocketResult FNetworkSocket::Send(const void* Data, int32 Size, int32& OutBytesSent)
{
    OutBytesSent = 0;
    return PlatformSocket ? PlatformSocket->Send(Data, Size, OutBytesSent) : ESocketResult::Closed;
}

ESocketResult FNetworkSocket::Recv(void* Buffer, int32 Size, int32& OutBytesRead)
{
    OutBytesRead = 0;
    return PlatformSocket ? PlatformSocket->Recv(Buffer, Size, OutBytesRead) : ESocketResult::Closed;
}

ESocketResult FNetworkSocket::SendTo(const void* Data, int32 Size, const FSocketAddress& Address, int32& OutBytesSent)
{
    OutBytesSent = 0;

    if (!PlatformSocket && !CreatePlatformSocket(ESocketFamily::DualStack) && !CreatePlatformSocket(ESocketFamily::IPv4))
    {
        return ESocketResult::Error;
    }

    return PlatformSocket->SendTo(Data, Size, Address, OutBytesSent);
}

ESocketResult FNetworkSocket::RecvFrom(void* Buffer, int32 Size, int32& OutBytesRead, FSocketAddress& OutAddress)
{
    OutBytesRead = 0;
    return PlatformSocket ? PlatformSocket->RecvFrom(Buffer, Size, OutBytesRead, OutAddress) : ESocketResult::Error;
}

bool FNetworkSocket::SetNonBlocking(bool bNonBlocking)
{
    if (PlatformSocket && !PlatformSocket->SetNonBlocking(bNonBlocking))
    {
        return false;
    }

    bIsNonBlocking = bNonBlocking;
    return true;
}

void FNetworkSocket::Close()
{
    PlatformSocket.Reset();
}

bool FNetworkSocket::IsOpen() const
{
    return PlatformSocket && PlatformSocket->IsValid();
}

bool FNetworkSocket::GetPeerAddress(FSocketAddress& OutAddress) const
{
    return PlatformSocket && PlatformSocket->GetPeerAddress(OutAddress);
}

bool FNetworkSocket::GetLocalAddress(FSocketAddress& OutAddress) const
{
    return PlatformSocket && PlatformSocket->GetLocalAddress(OutAddress);
}

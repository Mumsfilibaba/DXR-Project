#include "Core/Windows/WindowsPlatformSocket.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/OutputDeviceLogger.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

static_assert(sizeof(UINT_PTR) == sizeof(SOCKET), "SOCKET is stored as UINT_PTR");

// Winsock 2.2 is the version every supported Windows ships
static constexpr BYTE GWinsockMajorVersion = 2;
static constexpr BYTE GWinsockMinorVersion = 2;

static constexpr uint32 GMillisecondsPerSecond      = 1000;
static constexpr uint32 GMicrosecondsPerMillisecond = 1000;

static bool SetSocketNonBlocking(SOCKET Socket, bool bNonBlocking)
{
    u_long Mode = bNonBlocking ? 1 : 0;
    return ::ioctlsocket(Socket, FIONBIO, &Mode) == 0;
}

static void DisableNagle(SOCKET Socket)
{
    BOOL bNoDelay = TRUE;
    ::setsockopt(Socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&bNoDelay), sizeof(bNoDelay));
}

static bool IsConnectionClosedError(int Error)
{
    return (Error == WSAECONNRESET) || (Error == WSAECONNABORTED) || (Error == WSAESHUTDOWN);
}

static int32 ToNativeAddress(const FSocketAddress& Address, ESocketFamily Family, sockaddr_storage& OutNative)
{
    OutNative = {};
    if (Family == ESocketFamily::IPv4)
    {
        sockaddr_in& Native4 = reinterpret_cast<sockaddr_in&>(OutNative);
        Native4.sin_family      = AF_INET;
        Native4.sin_port        = ::htons(Address.GetPort());
        Native4.sin_addr.s_addr = ::htonl(Address.GetIPv4());
        return sizeof(sockaddr_in);
    }

    sockaddr_in6& Native6 = reinterpret_cast<sockaddr_in6&>(OutNative);
    Native6.sin6_family = AF_INET6;
    Native6.sin6_port   = ::htons(Address.GetPort());

    Memory::Memcpy(&Native6.sin6_addr, Address.GetBytes(), FSocketAddress::NumBytes);
    return sizeof(sockaddr_in6);
}

static FSocketAddress FromNativeAddress(const sockaddr* Native)
{
    if (Native->sa_family == AF_INET)
    {
        const sockaddr_in* Native4 = reinterpret_cast<const sockaddr_in*>(Native);
        return FSocketAddress::MakeIPv4(::ntohl(Native4->sin_addr.s_addr), ::ntohs(Native4->sin_port));
    }

    const sockaddr_in6* Native6 = reinterpret_cast<const sockaddr_in6*>(Native);
    return FSocketAddress::MakeIPv6(Native6->sin6_addr.u.Byte, ::ntohs(Native6->sin6_port));
}

bool FWindowsPlatformSocket::InitializeWinsock()
{
    static const bool bIsInitialized = []()
    {
        WSADATA Data;
        const int Result = ::WSAStartup(MAKEWORD(GWinsockMajorVersion, GWinsockMinorVersion), &Data);
        if (Result != 0)
        {
            LOG_ERROR("[FWindowsPlatformSocket] WSAStartup failed with error %d", Result);
            return false;
        }

        return true;
    }();

    return bIsInitialized;
}

IPlatformSocket* FWindowsPlatformSocket::Create(ESocketType Type, ESocketFamily Family)
{
    if (!InitializeWinsock())
    {
        return nullptr;
    }

    const bool   bIsStream     = (Type == ESocketType::TCP);
    const int    AddressFamily = (Family == ESocketFamily::IPv4) ? AF_INET : AF_INET6;
    const SOCKET NewSocket     = ::socket(AddressFamily, bIsStream ? SOCK_STREAM : SOCK_DGRAM, bIsStream ? IPPROTO_TCP : IPPROTO_UDP);

    if (NewSocket == INVALID_SOCKET)
    {
        return nullptr;
    }

    if (bIsStream)
    {
        DisableNagle(NewSocket);
    }
    else
    {
        BOOL  bReportConnReset = FALSE;
        DWORD BytesReturned    = 0;
        ::WSAIoctl(NewSocket, SIO_UDP_CONNRESET, &bReportConnReset, sizeof(bReportConnReset), nullptr, 0, &BytesReturned, nullptr, nullptr);
    }

    if (AddressFamily == AF_INET6)
    {
        DWORD bV6Only = (Family == ESocketFamily::IPv6) ? TRUE : FALSE;
        if (::setsockopt(NewSocket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&bV6Only), sizeof(bV6Only)) == SOCKET_ERROR)
        {
            ::closesocket(NewSocket);
            return nullptr;
        }
    }

    return new FWindowsPlatformSocket(Type, Family, static_cast<UINT_PTR>(NewSocket));
}

bool FWindowsPlatformSocket::Resolve(const String& Host, uint16 Port, TArray<FSocketAddress>& OutAddresses)
{
    OutAddresses.Clear();
    if (!InitializeWinsock())
    {
        return false;
    }

    addrinfo Hints = {};
    Hints.ai_family   = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM;

    addrinfo* AddressList = nullptr;
    if ((::getaddrinfo(*Host, nullptr, &Hints, &AddressList) != 0) || !AddressList)
    {
        return false;
    }

    for (const addrinfo* Entry = AddressList; Entry; Entry = Entry->ai_next)
    {
        if ((Entry->ai_family != AF_INET) && (Entry->ai_family != AF_INET6))
        {
            continue;
        }

        FSocketAddress Address = FromNativeAddress(Entry->ai_addr);
        Address.SetPort(Port);
        OutAddresses.AddUnique(Address);
    }

    ::freeaddrinfo(AddressList);
    return !OutAddresses.IsEmpty();
}

bool FWindowsPlatformSocket::ParseAddress(const String& Text, uint16 Port, FSocketAddress& OutAddress)
{
    in_addr IPv4;
    if (::inet_pton(AF_INET, *Text, &IPv4) == 1)
    {
        OutAddress = FSocketAddress::MakeIPv4(::ntohl(IPv4.s_addr), Port);
        return true;
    }

    uint8 IPv6Bytes[FSocketAddress::NumBytes];
    if (::inet_pton(AF_INET6, *Text, IPv6Bytes) == 1)
    {
        OutAddress = FSocketAddress::MakeIPv6(IPv6Bytes, Port);
        return true;
    }

    return false;
}

String FWindowsPlatformSocket::AddressToString(const FSocketAddress& Address)
{
    CHAR Buffer[INET6_ADDRSTRLEN] = {};
    if (Address.IsIPv4())
    {
        in_addr IPv4;
        IPv4.s_addr = ::htonl(Address.GetIPv4());
        ::inet_ntop(AF_INET, &IPv4, Buffer, sizeof(Buffer));
    }
    else
    {
        ::inet_ntop(AF_INET6, Address.GetBytes(), Buffer, sizeof(Buffer));
    }

    return String(Buffer);
}

int32 FWindowsPlatformSocket::WaitForRead(IPlatformSocket* const* Sockets, int32 NumSockets, bool* OutReadable, uint32 TimeoutMilliseconds)
{
    TArray<WSAPOLLFD> PollFds;
    TArray<int32>     PollSocketIndices;

    for (int32 Index = 0; Index < NumSockets; ++Index)
    {
        OutReadable[Index] = false;

        const FWindowsPlatformSocket* PlatformSocket = static_cast<const FWindowsPlatformSocket*>(Sockets[Index]);
        if (PlatformSocket && PlatformSocket->IsValid())
        {
            WSAPOLLFD& PollFd = PollFds.Emplace();
            PollFd.fd      = static_cast<SOCKET>(PlatformSocket->Socket);
            PollFd.events  = POLLRDNORM;
            PollFd.revents = 0;
            PollSocketIndices.Add(Index);
        }
    }

    if (PollFds.IsEmpty())
    {
        ::Sleep(TimeoutMilliseconds);
        return 0;
    }

    const int Result = ::WSAPoll(PollFds.Data(), static_cast<ULONG>(PollFds.Size()), static_cast<INT>(TimeoutMilliseconds));
    if (Result == SOCKET_ERROR)
    {
        return -1;
    }

    int32 NumReadable = 0;
    for (int32 PollIndex = 0; PollIndex < PollFds.Size(); ++PollIndex)
    {
        if (PollFds[PollIndex].revents & (POLLRDNORM | POLLHUP | POLLERR))
        {
            OutReadable[PollSocketIndices[PollIndex]] = true;
            ++NumReadable;
        }
    }

    return NumReadable;
}

FWindowsPlatformSocket::FWindowsPlatformSocket(ESocketType InType, ESocketFamily InFamily, UINT_PTR InSocket)
    : Socket(InSocket)
    , Type(InType)
    , Family(InFamily)
    , bIsNonBlocking(false)
{
}

FWindowsPlatformSocket::~FWindowsPlatformSocket()
{
    Close();
}

bool FWindowsPlatformSocket::Bind(const FSocketAddress& Address)
{
    if (!IsAddressCompatible(Family, Address))
    {
        return false;
    }

    BOOL bExclusive = TRUE;
    ::setsockopt(static_cast<SOCKET>(Socket), SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&bExclusive), sizeof(bExclusive));

    sockaddr_storage Native;
    const int32 NativeLength = ToNativeAddress(Address, Family, Native);
    return ::bind(static_cast<SOCKET>(Socket), reinterpret_cast<const sockaddr*>(&Native), NativeLength) != SOCKET_ERROR;
}

bool FWindowsPlatformSocket::Listen(int32 Backlog)
{
    return ::listen(static_cast<SOCKET>(Socket), Backlog) != SOCKET_ERROR;
}

IPlatformSocket* FWindowsPlatformSocket::Accept()
{
    const SOCKET NewSocket = ::accept(static_cast<SOCKET>(Socket), nullptr, nullptr);
    if (NewSocket == INVALID_SOCKET)
    {
        return nullptr;
    }

    SetSocketNonBlocking(NewSocket, true);
    DisableNagle(NewSocket);

    FWindowsPlatformSocket* AcceptedSocket = new FWindowsPlatformSocket(ESocketType::TCP, Family, static_cast<UINT_PTR>(NewSocket));
    AcceptedSocket->bIsNonBlocking = true;
    return AcceptedSocket;
}

bool FWindowsPlatformSocket::Connect(const FSocketAddress& Address, uint32 TimeoutMilliseconds)
{
    if (!IsAddressCompatible(Family, Address))
    {
        return false;
    }

    sockaddr_storage Native;
    const int32 NativeLength = ToNativeAddress(Address, Family, Native);

    const SOCKET NativeSocket = static_cast<SOCKET>(Socket);
    SetSocketNonBlocking(NativeSocket, true);

    bool bConnected = false;
    if (::connect(NativeSocket, reinterpret_cast<const sockaddr*>(&Native), NativeLength) == 0)
    {
        bConnected = true;
    }
    else if (::WSAGetLastError() == WSAEWOULDBLOCK)
    {
        fd_set WriteSet;
        FD_ZERO(&WriteSet);
        FD_SET(NativeSocket, &WriteSet);

        fd_set ExceptSet;
        FD_ZERO(&ExceptSet);
        FD_SET(NativeSocket, &ExceptSet);

        timeval Timeout;
        Timeout.tv_sec  = static_cast<long>(TimeoutMilliseconds / GMillisecondsPerSecond);
        Timeout.tv_usec = static_cast<long>((TimeoutMilliseconds % GMillisecondsPerSecond) * GMicrosecondsPerMillisecond);

        if ((::select(0, nullptr, &WriteSet, &ExceptSet, &Timeout) > 0) && FD_ISSET(NativeSocket, &WriteSet))
        {
            int32 SocketError  = 0;
            int   OptionLength = sizeof(SocketError);
            ::getsockopt(NativeSocket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&SocketError), &OptionLength);
            bConnected = (SocketError == 0);
        }
    }

    SetSocketNonBlocking(NativeSocket, bIsNonBlocking);
    return bConnected;
}

ESocketResult FWindowsPlatformSocket::Send(const void* Data, int32 Size, int32& OutBytesSent)
{
    OutBytesSent = 0;

    const int Result = ::send(static_cast<SOCKET>(Socket), reinterpret_cast<const char*>(Data), Size, 0);
    if (Result == SOCKET_ERROR)
    {
        const int Error = ::WSAGetLastError();
        if (Error == WSAEWOULDBLOCK)
        {
            return ESocketResult::WouldBlock;
        }

        return IsConnectionClosedError(Error) ? ESocketResult::Closed : ESocketResult::Error;
    }

    OutBytesSent = Result;
    return ESocketResult::Success;
}

ESocketResult FWindowsPlatformSocket::Recv(void* Buffer, int32 Size, int32& OutBytesRead)
{
    OutBytesRead = 0;

    const int Result = ::recv(static_cast<SOCKET>(Socket), reinterpret_cast<char*>(Buffer), Size, 0);
    if (Result == 0)
    {
        return (Type == ESocketType::TCP) ? ESocketResult::Closed : ESocketResult::Success;
    }

    if (Result == SOCKET_ERROR)
    {
        const int Error = ::WSAGetLastError();
        if (Error == WSAEWOULDBLOCK)
        {
            return ESocketResult::WouldBlock;
        }

        return IsConnectionClosedError(Error) ? ESocketResult::Closed : ESocketResult::Error;
    }

    OutBytesRead = Result;
    return ESocketResult::Success;
}

ESocketResult FWindowsPlatformSocket::SendTo(const void* Data, int32 Size, const FSocketAddress& Address, int32& OutBytesSent)
{
    OutBytesSent = 0;
    if (!IsAddressCompatible(Family, Address))
    {
        return ESocketResult::Error;
    }

    sockaddr_storage Native;
    const int32 NativeLength = ToNativeAddress(Address, Family, Native);

    const int Result = ::sendto(static_cast<SOCKET>(Socket), reinterpret_cast<const char*>(Data), Size, 0, reinterpret_cast<const sockaddr*>(&Native), NativeLength);
    if (Result == SOCKET_ERROR)
    {
        return (::WSAGetLastError() == WSAEWOULDBLOCK) ? ESocketResult::WouldBlock : ESocketResult::Error;
    }

    OutBytesSent = Result;
    return ESocketResult::Success;
}

ESocketResult FWindowsPlatformSocket::RecvFrom(void* Buffer, int32 Size, int32& OutBytesRead, FSocketAddress& OutAddress)
{
    OutBytesRead = 0;

    sockaddr_storage Native = {};
    int NativeLength = sizeof(Native);

    const int Result = ::recvfrom(static_cast<SOCKET>(Socket), reinterpret_cast<char*>(Buffer), Size, 0, reinterpret_cast<sockaddr*>(&Native), &NativeLength);
    if (Result == SOCKET_ERROR)
    {
        const int Error = ::WSAGetLastError();
        if (Error == WSAEWOULDBLOCK)
        {
            return ESocketResult::WouldBlock;
        }

        if (Error != WSAEMSGSIZE)
        {
            return ESocketResult::Error;
        }

        OutBytesRead = Size;
    }
    else
    {
        OutBytesRead = Result;
    }

    OutAddress = FromNativeAddress(reinterpret_cast<const sockaddr*>(&Native));
    return ESocketResult::Success;
}

bool FWindowsPlatformSocket::SetNonBlocking(bool bNonBlocking)
{
    if (!SetSocketNonBlocking(static_cast<SOCKET>(Socket), bNonBlocking))
    {
        return false;
    }

    bIsNonBlocking = bNonBlocking;
    return true;
}

void FWindowsPlatformSocket::Close()
{
    if (static_cast<SOCKET>(Socket) != INVALID_SOCKET)
    {
        ::closesocket(static_cast<SOCKET>(Socket));
        Socket = static_cast<UINT_PTR>(INVALID_SOCKET);
    }
}

bool FWindowsPlatformSocket::IsValid() const
{
    return static_cast<SOCKET>(Socket) != INVALID_SOCKET;
}

bool FWindowsPlatformSocket::GetPeerAddress(FSocketAddress& OutAddress) const
{
    sockaddr_storage Native = {};

    int NativeLength = sizeof(Native);
    if (::getpeername(static_cast<SOCKET>(Socket), reinterpret_cast<sockaddr*>(&Native), &NativeLength) == SOCKET_ERROR)
    {
        return false;
    }

    OutAddress = FromNativeAddress(reinterpret_cast<const sockaddr*>(&Native));
    return true;
}

bool FWindowsPlatformSocket::GetLocalAddress(FSocketAddress& OutAddress) const
{
    sockaddr_storage Native = {};

    int NativeLength = sizeof(Native);
    if (::getsockname(static_cast<SOCKET>(Socket), reinterpret_cast<sockaddr*>(&Native), &NativeLength) == SOCKET_ERROR)
    {
        return false;
    }

    OutAddress = FromNativeAddress(reinterpret_cast<const sockaddr*>(&Native));
    return true;
}

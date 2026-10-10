#include "Core/Mac/MacPlatformSocket.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Time/Timespan.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

static bool SetDescriptorNonBlocking(int32 FileDescriptor, bool bNonBlocking)
{
    const int32 Flags = ::fcntl(FileDescriptor, F_GETFL, 0);
    if (Flags < 0)
    {
        return false;
    }

    const int32 NewFlags = bNonBlocking ? (Flags | O_NONBLOCK) : (Flags & ~O_NONBLOCK);
    return ::fcntl(FileDescriptor, F_SETFL, NewFlags) == 0;
}

static void ConfigureDescriptor(int32 FileDescriptor, ESocketType Type)
{
    int32 bEnable = 1;
    if (Type == ESocketType::TCP)
    {
        ::setsockopt(FileDescriptor, IPPROTO_TCP, TCP_NODELAY, &bEnable, sizeof(bEnable));
    }

    ::setsockopt(FileDescriptor, SOL_SOCKET, SO_NOSIGPIPE, &bEnable, sizeof(bEnable));
}

static bool IsConnectionClosedError(int32 Error)
{
    return (Error == ECONNRESET) || (Error == EPIPE) || (Error == ENOTCONN) || (Error == ECONNABORTED);
}

static bool IsWouldBlockError(int32 Error)
{
    return (Error == EAGAIN) || (Error == EWOULDBLOCK) || (Error == EINTR);
}

static socklen_t ToNativeAddress(const FSocketAddress& Address, ESocketFamily Family, sockaddr_storage& OutNative)
{
    OutNative = {};

    if (Family == ESocketFamily::IPv4)
    {
        sockaddr_in& Native4 = reinterpret_cast<sockaddr_in&>(OutNative);
        Native4.sin_family      = AF_INET;
        Native4.sin_port        = htons(Address.GetPort());
        Native4.sin_addr.s_addr = htonl(Address.GetIPv4());
        return sizeof(sockaddr_in);
    }

    sockaddr_in6& Native6 = reinterpret_cast<sockaddr_in6&>(OutNative);
    Native6.sin6_family   = AF_INET6;
    Native6.sin6_port     = htons(Address.GetPort());

    Memory::Memcpy(&Native6.sin6_addr, Address.GetBytes(), FSocketAddress::NumBytes);
    return sizeof(sockaddr_in6);
}

static FSocketAddress FromNativeAddress(const sockaddr* Native)
{
    if (Native->sa_family == AF_INET)
    {
        const sockaddr_in* Native4 = reinterpret_cast<const sockaddr_in*>(Native);
        return FSocketAddress::MakeIPv4(ntohl(Native4->sin_addr.s_addr), ntohs(Native4->sin_port));
    }

    const sockaddr_in6* Native6 = reinterpret_cast<const sockaddr_in6*>(Native);
    return FSocketAddress::MakeIPv6(Native6->sin6_addr.s6_addr, ntohs(Native6->sin6_port));
}

IPlatformSocket* FMacPlatformSocket::Create(ESocketType Type, ESocketFamily Family)
{
    const bool  bIsStream     = (Type == ESocketType::TCP);
    const int32 AddressFamily = (Family == ESocketFamily::IPv4) ? AF_INET : AF_INET6;
    const int32 NewDescriptor = ::socket(AddressFamily, bIsStream ? SOCK_STREAM : SOCK_DGRAM, bIsStream ? IPPROTO_TCP : IPPROTO_UDP);

    if (NewDescriptor < 0)
    {
        return nullptr;
    }

    ConfigureDescriptor(NewDescriptor, Type);

    if (AddressFamily == AF_INET6)
    {
        int32 bV6Only = (Family == ESocketFamily::IPv6) ? 1 : 0;
        if (::setsockopt(NewDescriptor, IPPROTO_IPV6, IPV6_V6ONLY, &bV6Only, sizeof(bV6Only)) != 0)
        {
            ::close(NewDescriptor);
            return nullptr;
        }
    }

    return new FMacPlatformSocket(Type, Family, NewDescriptor);
}

bool FMacPlatformSocket::Resolve(const String& Host, uint16 Port, TArray<FSocketAddress>& OutAddresses)
{
    OutAddresses.Clear();

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

bool FMacPlatformSocket::ParseAddress(const String& Text, uint16 Port, FSocketAddress& OutAddress)
{
    in_addr IPv4;
    if (::inet_pton(AF_INET, *Text, &IPv4) == 1)
    {
        OutAddress = FSocketAddress::MakeIPv4(ntohl(IPv4.s_addr), Port);
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

String FMacPlatformSocket::AddressToString(const FSocketAddress& Address)
{
    CHAR Buffer[INET6_ADDRSTRLEN] = {};
    if (Address.IsIPv4())
    {
        in_addr IPv4;
        IPv4.s_addr = htonl(Address.GetIPv4());
        ::inet_ntop(AF_INET, &IPv4, Buffer, sizeof(Buffer));
    }
    else
    {
        ::inet_ntop(AF_INET6, Address.GetBytes(), Buffer, sizeof(Buffer));
    }

    return String(Buffer);
}

int32 FMacPlatformSocket::WaitForRead(IPlatformSocket* const* Sockets, int32 NumSockets, bool* OutReadable, uint32 TimeoutMilliseconds)
{
    TArray<pollfd> PollFds;
    TArray<int32>  PollSocketIndices;

    for (int32 Index = 0; Index < NumSockets; ++Index)
    {
        OutReadable[Index] = false;

        const FMacPlatformSocket* PlatformSocket = static_cast<const FMacPlatformSocket*>(Sockets[Index]);
        if (PlatformSocket && PlatformSocket->IsValid())
        {
            pollfd& PollFd = PollFds.Emplace();
            PollFd.fd      = PlatformSocket->FileDescriptor;
            PollFd.events  = POLLIN;
            PollFd.revents = 0;
            PollSocketIndices.Add(Index);
        }
    }

    if (PollFds.IsEmpty())
    {
        FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(TimeoutMilliseconds));
        return 0;
    }

    const int32 Result = ::poll(PollFds.Data(), static_cast<nfds_t>(PollFds.Size()), static_cast<int>(TimeoutMilliseconds));
    if (Result < 0)
    {
        return (errno == EINTR) ? 0 : -1;
    }

    int32 NumReadable = 0;
    for (int32 PollIndex = 0; PollIndex < PollFds.Size(); ++PollIndex)
    {
        if (PollFds[PollIndex].revents & (POLLIN | POLLHUP | POLLERR))
        {
            OutReadable[PollSocketIndices[PollIndex]] = true;
            ++NumReadable;
        }
    }

    return NumReadable;
}

FMacPlatformSocket::FMacPlatformSocket(ESocketType InType, ESocketFamily InFamily, int32 InFileDescriptor)
    : FileDescriptor(InFileDescriptor)
    , Type(InType)
    , Family(InFamily)
    , bIsNonBlocking(false)
{
}

FMacPlatformSocket::~FMacPlatformSocket()
{
    Close();
}

bool FMacPlatformSocket::Bind(const FSocketAddress& Address)
{
    if (!IsAddressCompatible(Family, Address))
    {
        return false;
    }

    int32 bReuse = 1;
    ::setsockopt(FileDescriptor, SOL_SOCKET, SO_REUSEADDR, &bReuse, sizeof(bReuse));

    sockaddr_storage Native;
    const socklen_t  NativeLength = ToNativeAddress(Address, Family, Native);
    return ::bind(FileDescriptor, reinterpret_cast<const sockaddr*>(&Native), NativeLength) == 0;
}

bool FMacPlatformSocket::Listen(int32 Backlog)
{
    return ::listen(FileDescriptor, Backlog) == 0;
}

IPlatformSocket* FMacPlatformSocket::Accept()
{
    const int32 NewDescriptor = ::accept(FileDescriptor, nullptr, nullptr);
    if (NewDescriptor < 0)
    {
        return nullptr;
    }

    SetDescriptorNonBlocking(NewDescriptor, true);
    ConfigureDescriptor(NewDescriptor, ESocketType::TCP);

    FMacPlatformSocket* AcceptedSocket = new FMacPlatformSocket(ESocketType::TCP, Family, NewDescriptor);
    AcceptedSocket->bIsNonBlocking = true;
    return AcceptedSocket;
}

bool FMacPlatformSocket::Connect(const FSocketAddress& Address, uint32 TimeoutMilliseconds)
{
    if (!IsAddressCompatible(Family, Address))
    {
        return false;
    }

    sockaddr_storage Native;
    const socklen_t  NativeLength = ToNativeAddress(Address, Family, Native);

    SetDescriptorNonBlocking(FileDescriptor, true);

    bool bConnected = false;
    if (::connect(FileDescriptor, reinterpret_cast<const sockaddr*>(&Native), NativeLength) == 0)
    {
        bConnected = true;
    }
    else if (errno == EINPROGRESS)
    {
        pollfd PollFd = {};
        PollFd.fd     = FileDescriptor;
        PollFd.events = POLLOUT;

        if (::poll(&PollFd, 1, static_cast<int>(TimeoutMilliseconds)) > 0)
        {
            int32     SocketError  = 0;
            socklen_t OptionLength = sizeof(SocketError);

            ::getsockopt(FileDescriptor, SOL_SOCKET, SO_ERROR, &SocketError, &OptionLength);
            bConnected = (SocketError == 0);
        }
    }

    SetDescriptorNonBlocking(FileDescriptor, bIsNonBlocking);
    return bConnected;
}

ESocketResult FMacPlatformSocket::Send(const void* Data, int32 Size, int32& OutBytesSent)
{
    OutBytesSent = 0;

    const ssize_t Result = ::send(FileDescriptor, Data, static_cast<size_t>(Size), 0);
    if (Result < 0)
    {
        if (IsWouldBlockError(errno))
        {
            return ESocketResult::WouldBlock;
        }

        return IsConnectionClosedError(errno) ? ESocketResult::Closed : ESocketResult::Error;
    }

    OutBytesSent = static_cast<int32>(Result);
    return ESocketResult::Success;
}

ESocketResult FMacPlatformSocket::Recv(void* Buffer, int32 Size, int32& OutBytesRead)
{
    OutBytesRead = 0;

    const ssize_t Result = ::recv(FileDescriptor, Buffer, static_cast<size_t>(Size), 0);
    if (Result == 0)
    {
        return (Type == ESocketType::TCP) ? ESocketResult::Closed : ESocketResult::Success;
    }

    if (Result < 0)
    {
        if (IsWouldBlockError(errno))
        {
            return ESocketResult::WouldBlock;
        }

        return IsConnectionClosedError(errno) ? ESocketResult::Closed : ESocketResult::Error;
    }

    OutBytesRead = static_cast<int32>(Result);
    return ESocketResult::Success;
}

ESocketResult FMacPlatformSocket::SendTo(const void* Data, int32 Size, const FSocketAddress& Address, int32& OutBytesSent)
{
    OutBytesSent = 0;
    if (!IsAddressCompatible(Family, Address))
    {
        return ESocketResult::Error;
    }

    sockaddr_storage Native;
    const socklen_t  NativeLength = ToNativeAddress(Address, Family, Native);

    const ssize_t Result = ::sendto(FileDescriptor, Data, static_cast<size_t>(Size), 0, reinterpret_cast<const sockaddr*>(&Native), NativeLength);
    if (Result < 0)
    {
        return IsWouldBlockError(errno) ? ESocketResult::WouldBlock : ESocketResult::Error;
    }

    OutBytesSent = static_cast<int32>(Result);
    return ESocketResult::Success;
}

ESocketResult FMacPlatformSocket::RecvFrom(void* Buffer, int32 Size, int32& OutBytesRead, FSocketAddress& OutAddress)
{
    OutBytesRead = 0;

    sockaddr_storage Native       = {};
    socklen_t        NativeLength = sizeof(Native);

    const ssize_t Result = ::recvfrom(FileDescriptor, Buffer, static_cast<size_t>(Size), 0, reinterpret_cast<sockaddr*>(&Native), &NativeLength);
    if (Result < 0)
    {
        return IsWouldBlockError(errno) ? ESocketResult::WouldBlock : ESocketResult::Error;
    }

    OutBytesRead = static_cast<int32>(Result);
    OutAddress   = FromNativeAddress(reinterpret_cast<const sockaddr*>(&Native));
    return ESocketResult::Success;
}

bool FMacPlatformSocket::SetNonBlocking(bool bNonBlocking)
{
    if (!SetDescriptorNonBlocking(FileDescriptor, bNonBlocking))
    {
        return false;
    }

    bIsNonBlocking = bNonBlocking;
    return true;
}

void FMacPlatformSocket::Close()
{
    if (FileDescriptor >= 0)
    {
        ::close(FileDescriptor);
        FileDescriptor = -1;
    }
}

bool FMacPlatformSocket::IsValid() const
{
    return FileDescriptor >= 0;
}

bool FMacPlatformSocket::GetPeerAddress(FSocketAddress& OutAddress) const
{
    sockaddr_storage Native       = {};
    socklen_t        NativeLength = sizeof(Native);

    if (::getpeername(FileDescriptor, reinterpret_cast<sockaddr*>(&Native), &NativeLength) != 0)
    {
        return false;
    }

    OutAddress = FromNativeAddress(reinterpret_cast<const sockaddr*>(&Native));
    return true;
}

bool FMacPlatformSocket::GetLocalAddress(FSocketAddress& OutAddress) const
{
    sockaddr_storage Native       = {};
    socklen_t        NativeLength = sizeof(Native);

    if (::getsockname(FileDescriptor, reinterpret_cast<sockaddr*>(&Native), &NativeLength) != 0)
    {
        return false;
    }

    OutAddress = FromNativeAddress(reinterpret_cast<const sockaddr*>(&Native));
    return true;
}

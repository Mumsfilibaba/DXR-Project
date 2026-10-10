#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Network/SocketAddress.h"
#include "Core/Platform/PlatformSocket.h"
#include "Core/Templates/Utility/NonCopyable.h"

class CORE_API FNetworkSocket : public FNonCopyable
{
public:

    /** @brief Same as FPlatformSocket::Resolve, for callers that want to tell an unknown host from an unreachable one */
    static bool Resolve(const String& Host, uint16 Port, TArray<FSocketAddress>& OutAddresses);

    /** @brief Same as FPlatformSocket::ParseAddress, so engine code never has to reach into the platform layer */
    static bool ParseAddress(const String& Text, uint16 Port, FSocketAddress& OutAddress);

    /** @brief FPlatformSocket::WaitForRead for network sockets; null entries and sockets without a platform socket are never readable */
    static int32 WaitForRead(FNetworkSocket* const* Sockets, int32 NumSockets, bool* OutReadable, uint32 TimeoutMilliseconds);

public:

    explicit FNetworkSocket(ESocketType InType);
    FNetworkSocket(FNetworkSocket&& Other);
    FNetworkSocket& operator=(FNetworkSocket&& Other);
    ~FNetworkSocket();

    /** @brief Creates a platform socket for the address and binds it: IPv4 for an IPv4 address, DualStack for ::, IPv6 otherwise */
    bool Bind(const FSocketAddress& Address);
    bool Listen(int32 Backlog);

    /** @return False when no connection is pending; otherwise OutClient owns the new connection */
    bool Accept(FNetworkSocket& OutClient);

    /** @brief Creates a platform socket for the address, IPv4 or IPv6, and connects it */
    bool Connect(const FSocketAddress& Address, uint32 TimeoutMilliseconds);

    /** @brief Tries the addresses in order, each with the full timeout, until one connects */
    bool ConnectToAny(const TArray<FSocketAddress>& Addresses, uint32 TimeoutMilliseconds);

    /** @brief Resolve, then ConnectToAny */
    bool ConnectToHost(const String& Host, uint16 Port, uint32 TimeoutMilliseconds);

    ESocketResult Send(const void* Data, int32 Size, int32& OutBytesSent);
    ESocketResult Recv(void* Buffer, int32 Size, int32& OutBytesRead);

    /**
     * @brief UDP. Sends one datagram. Before Bind or Connect there is no platform socket yet, so this creates a
     *        DualStack one, or IPv4 where IPv6 is unavailable, and the OS binds it to a free port on the first send.
     */
    ESocketResult SendTo(const void* Data, int32 Size, const FSocketAddress& Address, int32& OutBytesSent);

    /** @brief UDP. Receives one datagram and its sender; Error before Bind, Connect or SendTo has created a socket */
    ESocketResult RecvFrom(void* Buffer, int32 Size, int32& OutBytesRead, FSocketAddress& OutAddress);

    /** @brief Remembered, and applied to every platform socket this creates from now on */
    bool SetNonBlocking(bool bNonBlocking);

    void Close();

    NODISCARD bool IsOpen() const;

    bool GetPeerAddress(FSocketAddress& OutAddress) const;
    bool GetLocalAddress(FSocketAddress& OutAddress) const;

    NODISCARD ESocketType GetType() const
    {
        return Type;
    }

    /** @return The platform socket for code that needs explicit control, or nullptr before Bind or Connect */
    NODISCARD IPlatformSocket* GetPlatformSocket() const
    {
        return PlatformSocket.Get();
    }

private:
    bool CreatePlatformSocket(ESocketFamily Family);

    TUniquePtr<IPlatformSocket> PlatformSocket;
    ESocketType                 Type;
    bool                        bIsNonBlocking;
};

#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Network/SocketAddress.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

enum class ESocketResult : uint8
{
    Success    = 0,
    WouldBlock = 1,
    Closed     = 2,
    Error      = 3,
};

enum class ESocketType : uint8
{
    TCP = 0, // A stream; Listen and Accept only work on these
    UDP = 1, // Datagrams; SendTo and RecvFrom talk to any peer, and after Connect, Send and Recv talk to that one
};

enum class ESocketFamily : uint8
{
    IPv4      = 0, // IPv4 addresses only
    IPv6      = 1, // IPv6 addresses only
    DualStack = 2, // An IPv6 socket that also takes IPv4 addresses
};

struct CORE_API IPlatformSocket
{
public:

    /** @brief Creates a native socket of the given type and family, or nullptr when the OS refuses. Each platform hides this with its own */
    static IPlatformSocket* Create(ESocketType Type, ESocketFamily Family)
    {
        return nullptr;
    }

    /**
     * @brief Looks up a name, or reads a numeric address, into every IPv4 and IPv6 address it stands for, in the order
     *        the OS prefers. Trying them is left to the caller. Each platform hides this with its own.
     */
    static bool Resolve(const String& Host, uint16 Port, TArray<FSocketAddress>& OutAddresses)
    {
        return false;
    }

    /**
     * @brief Reads a numeric IPv4 ("127.0.0.1") or IPv6 ("::1", "fe80::1") address. Names are not looked up; use
     *        Resolve for those. Each platform hides this with its own.
     */
    static bool ParseAddress(const String& Text, uint16 Port, FSocketAddress& OutAddress)
    {
        return false;
    }

    /** @return The address as text without the port, IPv4 always as a.b.c.d. Each platform hides this with its own. */
    static String AddressToString(const FSocketAddress& Address)
    {
        return String();
    }

    /**
     * @brief Blocks until at least one of the sockets has data to read or a pending connection, or the timeout passes.
     *        A socket whose peer has closed also counts as readable, so the following Recv reports it. Null entries
     *        and closed sockets are never readable. Each platform hides this with its own.
     * @param OutReadable Receives one entry per socket, true when that socket is readable
     * @return The number of readable sockets, zero on timeout and -1 on error
     */
    static int32 WaitForRead(IPlatformSocket* const* Sockets, int32 NumSockets, bool* OutReadable, uint32 TimeoutMilliseconds)
    {
        return -1;
    }

    /** @return True when a socket of the family can bind, connect or send to the address */
    static bool IsAddressCompatible(ESocketFamily Family, const FSocketAddress& Address);

public:
    virtual ~IPlatformSocket() = default;

    /** @brief Fails without logging when the address does not fit the socket's family */
    virtual bool Bind(const FSocketAddress& Address) = 0;

    virtual bool Listen(int32 Backlog) = 0;

    /** @return A new non-blocking socket of the same family for a pending connection, or nullptr when there is none */
    virtual IPlatformSocket* Accept() = 0;

    /** @brief Connects, giving up after the timeout. Keeps the blocking mode. Fails without logging when the address does not fit. */
    virtual bool Connect(const FSocketAddress& Address, uint32 TimeoutMilliseconds) = 0;

    virtual ESocketResult Send(const void* Data, int32 Size, int32& OutBytesSent) = 0;

    /** @brief On a UDP socket a zero-byte read is an empty datagram and reports Success; only TCP reports Closed */
    virtual ESocketResult Recv(void* Buffer, int32 Size, int32& OutBytesRead) = 0;

    /** @brief UDP. Sends one datagram to the address; Error when the address does not fit the socket's family */
    virtual ESocketResult SendTo(const void* Data, int32 Size, const FSocketAddress& Address, int32& OutBytesSent) = 0;

    /**
     * @brief UDP. Receives one datagram and the address it came from. A datagram longer than Size is cut to Size, and a
     *        zero-length datagram is a Success with zero bytes, not Closed.
     */
    virtual ESocketResult RecvFrom(void* Buffer, int32 Size, int32& OutBytesRead, FSocketAddress& OutAddress) = 0;

    virtual bool SetNonBlocking(bool bNonBlocking) = 0;
    virtual void Close() = 0;
    virtual bool IsValid() const = 0;

    virtual bool GetPeerAddress(FSocketAddress& OutAddress) const = 0;

    /** @brief The bound address, which is how a test finds the port the OS picked when binding to port 0 */
    virtual bool GetLocalAddress(FSocketAddress& OutAddress) const = 0;

    virtual ESocketType   GetType()   const = 0;
    virtual ESocketFamily GetFamily() const = 0;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

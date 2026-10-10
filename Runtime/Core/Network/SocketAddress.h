#pragma once
#include "Core/Containers/String.h"

/**
 * An IP address and port. The address is always held as IPv6, with IPv4 addresses stored IPv4-mapped,
 * a.b.c.d as ::ffff:a.b.c.d (RFC 4291 section 2.5.5.2). One type then covers both families, and a dual-stack
 * socket's IPv4 peers compare equal to the same address written as IPv4.
 */

struct CORE_API FSocketAddress
{
public:
    static constexpr int32 NumBytes = 16;

    /** @param HostOrderIPv4 The address in host byte order, so 127.0.0.1 is 0x7F000001 */
    static FSocketAddress MakeIPv4(uint32 HostOrderIPv4, uint16 Port);

    /** @param InBytes Sixteen bytes in network order, which may themselves be an IPv4-mapped address */
    static FSocketAddress MakeIPv6(const uint8* InBytes, uint16 Port);

    /** @return 0.0.0.0, every IPv4 interface */
    static FSocketAddress AnyIPv4(uint16 Port);

    /** @return ::, every interface; FNetworkSocket binds it dual-stack, so IPv4 clients reach it too */
    static FSocketAddress AnyIPv6(uint16 Port);

    static FSocketAddress LoopbackIPv4(uint16 Port);
    static FSocketAddress LoopbackIPv6(uint16 Port);

    /** @return "host:port", with an IPv6 literal bracketed as in "[::1]:9370". Host may also be a name. */
    static String FormatHostAndPort(const String& Host, uint16 Port);

public:

    /** @brief The unspecified IPv6 address and port zero, [::]:0 */
    FSocketAddress();
    ~FSocketAddress();

    /** @return True for an IPv4 address, which is held as ::ffff:a.b.c.d */
    NODISCARD bool IsIPv4() const;

    /** @return True for :: and 0.0.0.0 */
    NODISCARD bool IsUnspecified() const;

    /** @return True for ::1 and anything in 127.0.0.0/8 */
    NODISCARD bool IsLoopback() const;

    /** @return The IPv4 address in host byte order, or zero when this is not an IPv4 address */
    NODISCARD uint32 GetIPv4() const;

    NODISCARD const uint8* GetBytes() const
    {
        return Bytes;
    }

    NODISCARD uint16 GetPort() const
    {
        return Port;
    }

    void SetPort(uint16 InPort)
    {
        Port = InPort;
    }

    /** @return "a.b.c.d:port" for IPv4 and "[address]:port" for IPv6; IPv4 never prints as ::ffff:a.b.c.d */
    NODISCARD String ToString() const;

    /** @return The address alone, without the port or brackets; the platform layer does the formatting */
    NODISCARD String ToAddressString() const;

    NODISCARD bool operator==(const FSocketAddress& Other) const;
    NODISCARD bool operator!=(const FSocketAddress& Other) const;

private:

    // An IPv4-mapped address is ten zero bytes, two 0xFF bytes, then the four IPv4 bytes
    static constexpr int32 MappedMarkerIndex  = 10;
    static constexpr int32 MappedPrefixLength = 12;
    static constexpr uint8 MappedMarkerByte   = 0xFF;
    static constexpr int32 NumIPv4Bytes       = 4;
    static constexpr int32 BitsPerByte        = 8;

    // Every IPv4 loopback address is in 127.0.0.0/8, so only its first octet has to match
    static constexpr uint8 LoopbackFirstOctet  = 127;
    static constexpr int32 IPv4FirstOctetShift = 24;

    // 127.0.0.1 in host byte order
    static constexpr uint32 LoopbackIPv4Address = 0x7F000001;

    uint8  Bytes[NumBytes];
    uint16 Port;
};

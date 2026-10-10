#include "SocketAddressTests.h"

#include <Core/Containers/String.h>
#include <Core/Network/SocketAddress.h>
#include <Core/Platform/PlatformSocket.h>

#include "TestCommon/TestMacros.h"

static constexpr uint16 GTestPort = 9370;

static constexpr uint32 GLoopbackIPv4 = 0x7F000001;
static constexpr uint32 GPrivateIPv4  = 0x0A000001;

static bool Parse(const CHAR* Text, FSocketAddress& OutAddress)
{
    return FPlatformSocket::ParseAddress(Text, GTestPort, OutAddress);
}

static bool ParsesAsLoopback(const CHAR* Text)
{
    FSocketAddress Address;
    return Parse(Text, Address) && Address.IsLoopback();
}

bool SocketAddress_Test()
{
    TEST_BEGIN();

    TEST_SECTION("ParseAddress reads numeric IPv4 and IPv6 addresses");
    {
        FSocketAddress Address;
        TEST_EXPECT(Parse("127.0.0.1", Address));
        TEST_EXPECT(Address.IsIPv4());
        TEST_EXPECT_EQ(Address.GetIPv4(), GLoopbackIPv4);
        TEST_EXPECT_EQ(Address.GetPort(), GTestPort);

        TEST_EXPECT(Parse("::1", Address));
        TEST_EXPECT(!Address.IsIPv4());

        TEST_EXPECT(Parse("::", Address));
        TEST_EXPECT(!Address.IsIPv4());

        TEST_EXPECT(Parse("::ffff:10.0.0.1", Address));
        TEST_EXPECT(Address.IsIPv4());
        TEST_EXPECT_EQ(Address.GetIPv4(), GPrivateIPv4);
    }

    TEST_SECTION("ParseAddress rejects names and malformed addresses");
    {
        FSocketAddress Address;
        TEST_EXPECT(!Parse("localhost", Address));
        TEST_EXPECT(!Parse("256.0.0.1", Address));
        TEST_EXPECT(!Parse("", Address));
    }

    TEST_SECTION("An IPv4 address is the same whether it is made, parsed or parsed as mapped IPv6");
    {
        const FSocketAddress Made = FSocketAddress::MakeIPv4(GLoopbackIPv4, GTestPort);

        FSocketAddress Parsed;
        FSocketAddress ParsedMapped;
        TEST_EXPECT(Parse("127.0.0.1", Parsed));
        TEST_EXPECT(Parse("::ffff:127.0.0.1", ParsedMapped));

        TEST_EXPECT(Made.IsIPv4());
        TEST_EXPECT(ParsedMapped.IsIPv4());
        TEST_EXPECT(Made == Parsed);
        TEST_EXPECT(Made == ParsedMapped);
        TEST_EXPECT(Made == FSocketAddress::LoopbackIPv4(GTestPort));
        TEST_EXPECT(Made != FSocketAddress::LoopbackIPv4(GTestPort + 1));
        TEST_EXPECT(Made != FSocketAddress::LoopbackIPv6(GTestPort));
    }

    TEST_SECTION("IsLoopback covers all of 127.0.0.0/8 and ::1");
    TEST_EXPECT(ParsesAsLoopback("127.0.0.1"));
    TEST_EXPECT(ParsesAsLoopback("127.5.5.5"));
    TEST_EXPECT(ParsesAsLoopback("::1"));
    TEST_EXPECT(!ParsesAsLoopback("10.0.0.1"));
    TEST_EXPECT(!ParsesAsLoopback("::"));
    TEST_EXPECT(!ParsesAsLoopback("::2"));
    TEST_EXPECT(FSocketAddress::LoopbackIPv4(GTestPort).IsLoopback());
    TEST_EXPECT(FSocketAddress::LoopbackIPv6(GTestPort).IsLoopback());

    TEST_SECTION("IsUnspecified is true for :: and 0.0.0.0");
    {
        FSocketAddress Address;
        TEST_EXPECT(Parse("::", Address) && Address.IsUnspecified());
        TEST_EXPECT(Parse("0.0.0.0", Address) && Address.IsUnspecified());
        TEST_EXPECT(Parse("::1", Address) && !Address.IsUnspecified());
        TEST_EXPECT(FSocketAddress::AnyIPv4(GTestPort).IsUnspecified());
        TEST_EXPECT(FSocketAddress::AnyIPv6(GTestPort).IsUnspecified());
        TEST_EXPECT(FSocketAddress::AnyIPv4(GTestPort).IsIPv4());
        TEST_EXPECT(!FSocketAddress::AnyIPv6(GTestPort).IsIPv4());
    }

    TEST_SECTION("ToString brackets IPv6 and never prints IPv4 as ::ffff:");
    {
        TEST_EXPECT(FSocketAddress::LoopbackIPv4(GTestPort).ToString() == "127.0.0.1:9370");
        TEST_EXPECT(FSocketAddress::LoopbackIPv6(GTestPort).ToString() == "[::1]:9370");
        TEST_EXPECT(FSocketAddress::AnyIPv6(GTestPort).ToString() == "[::]:9370");

        FSocketAddress Mapped;
        TEST_EXPECT(Parse("::ffff:10.0.0.1", Mapped));
        TEST_EXPECT(Mapped.ToString() == "10.0.0.1:9370");
        TEST_EXPECT(Mapped.ToAddressString() == "10.0.0.1");
    }

    TEST_SECTION("SetPort changes only the port");
    {
        FSocketAddress Address = FSocketAddress::LoopbackIPv6(GTestPort);
        Address.SetPort(1);
        TEST_EXPECT_EQ(Address.GetPort(), static_cast<uint16>(1));
        TEST_EXPECT(Address.IsLoopback());
        TEST_EXPECT(!Address.IsIPv4());
    }

    TEST_SECTION("FormatHostAndPort brackets only hosts that contain a colon");
    TEST_EXPECT(FSocketAddress::FormatHostAndPort("localhost", 1) == "localhost:1");
    TEST_EXPECT(FSocketAddress::FormatHostAndPort("127.0.0.1", 1) == "127.0.0.1:1");
    TEST_EXPECT(FSocketAddress::FormatHostAndPort("::1", 1) == "[::1]:1");

    TEST_END();
}

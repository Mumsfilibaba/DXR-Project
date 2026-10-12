#include "SocketAddressTests.h"

#include <Core/Containers/String.h>
#include <Core/Network/SocketAddress.h>
#include <Core/Platform/PlatformSocket.h>

#include "TestCommon/TestMacros.h"

static constexpr uint16 GTestPort = 9370;

static constexpr uint32 GLoopbackIPv4 = 0x7F000001;
static constexpr uint32 GPrivateIPv4  = 0x0A000001;

static bool Parse(const CHAR_T* Text, FSocketAddress& OutAddress)
{
    return FPlatformSocket::ParseAddress(Text, GTestPort, OutAddress);
}

static bool ParsesAsLoopback(const CHAR_T* Text)
{
    FSocketAddress Address;
    return Parse(Text, Address) && Address.IsLoopback();
}

bool SocketAddress_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("ParseAddress reads numeric IPv4 and IPv6 addresses"));
    {
        FSocketAddress Address;
        TEST_EXPECT(Parse(TEXT("127.0.0.1"), Address));
        TEST_EXPECT(Address.IsIPv4());
        TEST_EXPECT_EQ(Address.GetIPv4(), GLoopbackIPv4);
        TEST_EXPECT_EQ(Address.GetPort(), GTestPort);

        TEST_EXPECT(Parse(TEXT("::1"), Address));
        TEST_EXPECT(!Address.IsIPv4());

        TEST_EXPECT(Parse(TEXT("::"), Address));
        TEST_EXPECT(!Address.IsIPv4());

        TEST_EXPECT(Parse(TEXT("::ffff:10.0.0.1"), Address));
        TEST_EXPECT(Address.IsIPv4());
        TEST_EXPECT_EQ(Address.GetIPv4(), GPrivateIPv4);
    }

    TEST_SECTION(TEXT("ParseAddress rejects names and malformed addresses"));
    {
        FSocketAddress Address;
        TEST_EXPECT(!Parse(TEXT("localhost"), Address));
        TEST_EXPECT(!Parse(TEXT("256.0.0.1"), Address));
        TEST_EXPECT(!Parse(TEXT(""), Address));
    }

    TEST_SECTION(TEXT("An IPv4 address is the same whether it is made, parsed or parsed as mapped IPv6"));
    {
        const FSocketAddress Made = FSocketAddress::MakeIPv4(GLoopbackIPv4, GTestPort);

        FSocketAddress Parsed;
        FSocketAddress ParsedMapped;
        TEST_EXPECT(Parse(TEXT("127.0.0.1"), Parsed));
        TEST_EXPECT(Parse(TEXT("::ffff:127.0.0.1"), ParsedMapped));

        TEST_EXPECT(Made.IsIPv4());
        TEST_EXPECT(ParsedMapped.IsIPv4());
        TEST_EXPECT(Made == Parsed);
        TEST_EXPECT(Made == ParsedMapped);
        TEST_EXPECT(Made == FSocketAddress::LoopbackIPv4(GTestPort));
        TEST_EXPECT(Made != FSocketAddress::LoopbackIPv4(GTestPort + 1));
        TEST_EXPECT(Made != FSocketAddress::LoopbackIPv6(GTestPort));
    }

    TEST_SECTION(TEXT("IsLoopback covers all of 127.0.0.0/8 and ::1"));
    TEST_EXPECT(ParsesAsLoopback(TEXT("127.0.0.1")));
    TEST_EXPECT(ParsesAsLoopback(TEXT("127.5.5.5")));
    TEST_EXPECT(ParsesAsLoopback(TEXT("::1")));
    TEST_EXPECT(!ParsesAsLoopback(TEXT("10.0.0.1")));
    TEST_EXPECT(!ParsesAsLoopback(TEXT("::")));
    TEST_EXPECT(!ParsesAsLoopback(TEXT("::2")));
    TEST_EXPECT(FSocketAddress::LoopbackIPv4(GTestPort).IsLoopback());
    TEST_EXPECT(FSocketAddress::LoopbackIPv6(GTestPort).IsLoopback());

    TEST_SECTION(TEXT("IsUnspecified is true for :: and 0.0.0.0"));
    {
        FSocketAddress Address;
        TEST_EXPECT(Parse(TEXT("::"), Address) && Address.IsUnspecified());
        TEST_EXPECT(Parse(TEXT("0.0.0.0"), Address) && Address.IsUnspecified());
        TEST_EXPECT(Parse(TEXT("::1"), Address) && !Address.IsUnspecified());
        TEST_EXPECT(FSocketAddress::AnyIPv4(GTestPort).IsUnspecified());
        TEST_EXPECT(FSocketAddress::AnyIPv6(GTestPort).IsUnspecified());
        TEST_EXPECT(FSocketAddress::AnyIPv4(GTestPort).IsIPv4());
        TEST_EXPECT(!FSocketAddress::AnyIPv6(GTestPort).IsIPv4());
    }

    TEST_SECTION(TEXT("ToString brackets IPv6 and never prints IPv4 as ::ffff:"));
    {
        TEST_EXPECT(FSocketAddress::LoopbackIPv4(GTestPort).ToString() == TEXT("127.0.0.1:9370"));
        TEST_EXPECT(FSocketAddress::LoopbackIPv6(GTestPort).ToString() == TEXT("[::1]:9370"));
        TEST_EXPECT(FSocketAddress::AnyIPv6(GTestPort).ToString() == TEXT("[::]:9370"));

        FSocketAddress Mapped;
        TEST_EXPECT(Parse(TEXT("::ffff:10.0.0.1"), Mapped));
        TEST_EXPECT(Mapped.ToString() == TEXT("10.0.0.1:9370"));
        TEST_EXPECT(Mapped.ToAddressString() == TEXT("10.0.0.1"));
    }

    TEST_SECTION(TEXT("SetPort changes only the port"));
    {
        FSocketAddress Address = FSocketAddress::LoopbackIPv6(GTestPort);
        Address.SetPort(1);
        TEST_EXPECT_EQ(Address.GetPort(), static_cast<uint16>(1));
        TEST_EXPECT(Address.IsLoopback());
        TEST_EXPECT(!Address.IsIPv4());
    }

    TEST_SECTION(TEXT("FormatHostAndPort brackets only hosts that contain a colon"));
    TEST_EXPECT(FSocketAddress::FormatHostAndPort(TEXT("localhost"), 1) == TEXT("localhost:1"));
    TEST_EXPECT(FSocketAddress::FormatHostAndPort(TEXT("127.0.0.1"), 1) == TEXT("127.0.0.1:1"));
    TEST_EXPECT(FSocketAddress::FormatHostAndPort(TEXT("::1"), 1) == TEXT("[::1]:1"));

    TEST_END();
}

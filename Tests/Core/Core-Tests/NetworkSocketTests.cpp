#include "NetworkSocketTests.h"

#include <Core/Containers/String.h>
#include <Core/Network/NetworkSocket.h>
#include <Core/Templates/CString.h>

#include "TestCommon/TestMacros.h"

static constexpr uint32 GWaitMilliseconds = 4000;
static constexpr uint16 GAnyPort          = 0;
static constexpr int32 GListenBacklog     = 4;
static constexpr int32 GReceiveBufferSize = 256;

static bool WaitUntilReadable(FNetworkSocket& Socket)
{
    FNetworkSocket* Sockets[] = { &Socket };
    bool bReadable = false;
    return (FNetworkSocket::WaitForRead(Sockets, 1, &bReadable, GWaitMilliseconds) == 1) && bReadable;
}

static String ReceiveText(FNetworkSocket& Socket)
{
    CHAR Buffer[GReceiveBufferSize];
    int32 BytesRead = 0;
    if (!WaitUntilReadable(Socket) || (Socket.Recv(Buffer, sizeof(Buffer), BytesRead) != ESocketResult::Success))
    {
        return String();
    }

    return String(Buffer, BytesRead);
}

static String ReceiveDatagram(FNetworkSocket& Socket)
{
    CHAR Buffer[GReceiveBufferSize];
    int32 BytesRead = 0;
    FSocketAddress From;
    if (!WaitUntilReadable(Socket) || (Socket.RecvFrom(Buffer, sizeof(Buffer), BytesRead, From) != ESocketResult::Success))
    {
        return String();
    }

    return String(Buffer, BytesRead);
}

static bool SendText(FNetworkSocket& Socket, const CHAR* Text)
{
    const int32 Length = CString::Strlen(Text);
    int32 BytesSent = 0;
    return (Socket.Send(Text, Length, BytesSent) == ESocketResult::Success) && (BytesSent == Length);
}

static bool SendDatagram(FNetworkSocket& Socket, const CHAR* Text, const FSocketAddress& Address)
{
    const int32 Length = CString::Strlen(Text);
    int32 BytesSent = 0;
    return (Socket.SendTo(Text, Length, Address, BytesSent) == ESocketResult::Success) && (BytesSent == Length);
}

static uint16 GetLocalPort(const FNetworkSocket& Socket)
{
    FSocketAddress LocalAddress;
    return Socket.GetLocalAddress(LocalAddress) ? LocalAddress.GetPort() : GAnyPort;
}

static bool BindsAs(const FSocketAddress& Address, ESocketFamily ExpectedFamily)
{
    FNetworkSocket Socket(ESocketType::TCP);
    return Socket.Bind(Address) && Socket.GetPlatformSocket() && (Socket.GetPlatformSocket()->GetFamily() == ExpectedFamily);
}

static bool IsIPv6Available()
{
    FNetworkSocket Probe(ESocketType::TCP);
    return Probe.Bind(FSocketAddress::LoopbackIPv6(GAnyPort));
}

bool NetworkSocket_Test()
{
    TEST_BEGIN();

    const bool bHasIPv6 = IsIPv6Available();
    if (!bHasIPv6)
    {
        LOG_WARNING("[NetworkSocket] IPv6 is not available, skipping the IPv6 sections");
    }

    TEST_SECTION("A socket has no platform socket until it is used");
    {
        FNetworkSocket Socket(ESocketType::TCP);
        TEST_EXPECT(!Socket.IsOpen());
        TEST_EXPECT(Socket.GetPlatformSocket() == nullptr);
        TEST_EXPECT_EQ(Socket.GetType(), ESocketType::TCP);
    }

    TEST_SECTION("Bind picks the family from the address");
    TEST_EXPECT(BindsAs(FSocketAddress::LoopbackIPv4(GAnyPort), ESocketFamily::IPv4));
    TEST_EXPECT(BindsAs(FSocketAddress::AnyIPv4(GAnyPort), ESocketFamily::IPv4));
    if (bHasIPv6)
    {
        TEST_EXPECT(BindsAs(FSocketAddress::LoopbackIPv6(GAnyPort), ESocketFamily::IPv6));
        TEST_EXPECT(BindsAs(FSocketAddress::AnyIPv6(GAnyPort), ESocketFamily::DualStack));
    }

    TEST_SECTION("A failed Bind leaves the socket closed");
    {
        FNetworkSocket Holder(ESocketType::TCP);
        TEST_CHECK(Holder.Bind(FSocketAddress::LoopbackIPv4(GAnyPort)));
        TEST_CHECK(Holder.Listen(GListenBacklog));

        FNetworkSocket Second(ESocketType::TCP);
        TEST_EXPECT(!Second.Bind(FSocketAddress::LoopbackIPv4(GetLocalPort(Holder))));
        TEST_EXPECT(!Second.IsOpen());
    }

    TEST_SECTION("ConnectToHost reaches an IPv4-only listener through localhost");
    FNetworkSocket Listener(ESocketType::TCP);
    TEST_CHECK(Listener.SetNonBlocking(true));
    TEST_CHECK(Listener.Bind(FSocketAddress::LoopbackIPv4(GAnyPort)));
    TEST_CHECK(Listener.Listen(GListenBacklog));
    TEST_CHECK(Listener.GetPlatformSocket()->GetFamily() == ESocketFamily::IPv4);
    const uint16 Port = GetLocalPort(Listener);
    TEST_CHECK(Port != GAnyPort);

    FNetworkSocket Client(ESocketType::TCP);
    TEST_CHECK(Client.ConnectToHost("localhost", Port, GWaitMilliseconds));
    TEST_EXPECT(Client.IsOpen());
    {
        FSocketAddress Peer;
        TEST_EXPECT(Client.GetPeerAddress(Peer));
        TEST_EXPECT(Peer == FSocketAddress::LoopbackIPv4(Port));
    }

    TEST_SECTION("Accept fills the socket it is given, which can then send and receive");
    FNetworkSocket Server(ESocketType::TCP);
    TEST_CHECK(WaitUntilReadable(Listener));
    TEST_CHECK(Listener.Accept(Server));
    TEST_EXPECT(Server.IsOpen());
    TEST_EXPECT(SendText(Client, "ping"));
    TEST_EXPECT(ReceiveText(Server) == "ping");
    TEST_EXPECT(SendText(Server, "pong"));
    TEST_EXPECT(ReceiveText(Client) == "pong");

    TEST_SECTION("Accept returns false while no connection is pending");
    {
        FNetworkSocket Nothing(ESocketType::TCP);
        TEST_EXPECT(!Listener.Accept(Nothing));
        TEST_EXPECT(!Nothing.IsOpen());
    }

    TEST_SECTION("A moved socket takes the connection with it");
    {
        FNetworkSocket Moved(::Move(Server));
        TEST_EXPECT(Moved.IsOpen());
        TEST_EXPECT(!Server.IsOpen());
        TEST_EXPECT(SendText(Client, "moved"));
        TEST_EXPECT(ReceiveText(Moved) == "moved");
    }

    TEST_SECTION("SetNonBlocking before Connect carries over to the socket Connect creates");
    {
        FNetworkSocket NonBlockingClient(ESocketType::TCP);
        TEST_CHECK(NonBlockingClient.SetNonBlocking(true));
        TEST_CHECK(NonBlockingClient.Connect(FSocketAddress::LoopbackIPv4(Port), GWaitMilliseconds));

        CHAR Buffer[GReceiveBufferSize];
        int32 BytesRead = 0;
        TEST_EXPECT_EQ(NonBlockingClient.Recv(Buffer, sizeof(Buffer), BytesRead), ESocketResult::WouldBlock);
    }

    TEST_SECTION("ConnectToAny skips addresses that refuse and uses the first that accepts");
    {
        FNetworkSocket Unused(ESocketType::TCP);
        TEST_CHECK(Unused.Bind(FSocketAddress::LoopbackIPv4(GAnyPort)));
        const uint16 ClosedPort = GetLocalPort(Unused);
        Unused.Close();

        TArray<FSocketAddress> Addresses;
        Addresses.Add(FSocketAddress::LoopbackIPv4(ClosedPort));
        Addresses.Add(FSocketAddress::LoopbackIPv4(Port));

        FNetworkSocket AnyClient(ESocketType::TCP);
        TEST_EXPECT(AnyClient.ConnectToAny(Addresses, GWaitMilliseconds));

        FSocketAddress Peer;
        TEST_EXPECT(AnyClient.GetPeerAddress(Peer));
        TEST_EXPECT(Peer == FSocketAddress::LoopbackIPv4(Port));
    }

    TEST_SECTION("Resolve fails for a name that does not exist");
    {
        TArray<FSocketAddress> Addresses;
        TEST_EXPECT(!FNetworkSocket::Resolve("no-such-host.invalid", Port, Addresses));
        TEST_EXPECT(Addresses.IsEmpty());

        FNetworkSocket Unresolved(ESocketType::TCP);
        TEST_EXPECT(!Unresolved.ConnectToHost("no-such-host.invalid", Port, GWaitMilliseconds));
        TEST_EXPECT(!Unresolved.IsOpen());
    }

    TEST_SECTION("WaitForRead treats null entries and unused sockets as never readable");
    {
        FNetworkSocket Unused(ESocketType::TCP);

        FNetworkSocket* Sockets[] = { nullptr, &Unused };
        bool Readable[] = { true, true };
        TEST_EXPECT_EQ(FNetworkSocket::WaitForRead(Sockets, 2, Readable, 0), 0);
        TEST_EXPECT(!Readable[0]);
        TEST_EXPECT(!Readable[1]);
    }

    TEST_SECTION("RecvFrom before any platform socket exists is an Error");
    {
        FNetworkSocket Unbound(ESocketType::UDP);
        CHAR Buffer[GReceiveBufferSize];
        int32 BytesRead = 0;
        FSocketAddress From;
        TEST_EXPECT_EQ(Unbound.RecvFrom(Buffer, sizeof(Buffer), BytesRead, From), ESocketResult::Error);
    }

    TEST_SECTION("An unbound UDP socket's first SendTo creates a dual-stack socket that reaches both families");
    {
        FNetworkSocket ReceiverIPv4(ESocketType::UDP);
        TEST_CHECK(ReceiverIPv4.Bind(FSocketAddress::LoopbackIPv4(GAnyPort)));

        FNetworkSocket Sender(ESocketType::UDP);
        TEST_EXPECT(SendDatagram(Sender, "four", FSocketAddress::LoopbackIPv4(GetLocalPort(ReceiverIPv4))));
        TEST_EXPECT(ReceiveDatagram(ReceiverIPv4) == "four");

        if (bHasIPv6)
        {
            TEST_CHECK(Sender.GetPlatformSocket() != nullptr);
            TEST_EXPECT_EQ(Sender.GetPlatformSocket()->GetFamily(), ESocketFamily::DualStack);

            FNetworkSocket ReceiverIPv6(ESocketType::UDP);
            TEST_CHECK(ReceiverIPv6.Bind(FSocketAddress::LoopbackIPv6(GAnyPort)));
            TEST_EXPECT(SendDatagram(Sender, "six", FSocketAddress::LoopbackIPv6(GetLocalPort(ReceiverIPv6))));
            TEST_EXPECT(ReceiveDatagram(ReceiverIPv6) == "six");
        }
    }

    TEST_END();
}

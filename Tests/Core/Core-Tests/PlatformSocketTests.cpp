#include "PlatformSocketTests.h"

#include <Core/Containers/String.h>
#include <Core/Containers/UniquePtr.h>
#include <Core/Platform/PlatformSocket.h>
#include <Core/Templates/CString.h>

#include "TestCommon/TestMacros.h"

static constexpr uint32 GWaitMilliseconds        = 2000;
static constexpr uint32 GShortWaitMilliseconds   = 50;
static constexpr uint32 GRefusedWaitMilliseconds = 500;
static constexpr uint16 GAnyPort                 = 0;
static constexpr int32  GListenBacklog           = 4;
static constexpr int32  GReceiveBufferSize       = 256;
static constexpr int32  GSmallBufferSize         = 16;
static constexpr int32  GLargeDatagramSize       = 32;
static constexpr int32  GTruncatedReadSize       = 8;

static bool WaitUntilReadable(IPlatformSocket* Socket, uint32 TimeoutMilliseconds)
{
    IPlatformSocket* Sockets[] = { Socket };
    bool bReadable = false;
    return (FPlatformSocket::WaitForRead(Sockets, 1, &bReadable, TimeoutMilliseconds) == 1) && bReadable;
}

static String ReceiveText(IPlatformSocket* Socket)
{
    CHAR Buffer[GReceiveBufferSize];
    int32 BytesRead = 0;
    if (!WaitUntilReadable(Socket, GWaitMilliseconds) || (Socket->Recv(Buffer, sizeof(Buffer), BytesRead) != ESocketResult::Success))
    {
        return String();
    }

    return String(Buffer, BytesRead);
}

static bool SendText(IPlatformSocket* Socket, const CHAR* Text)
{
    const int32 Length = CString::Strlen(Text);
    int32 BytesSent = 0;
    return (Socket->Send(Text, Length, BytesSent) == ESocketResult::Success) && (BytesSent == Length);
}

static uint16 GetLocalPort(IPlatformSocket* Socket)
{
    FSocketAddress LocalAddress;
    return Socket->GetLocalAddress(LocalAddress) ? LocalAddress.GetPort() : GAnyPort;
}

static IPlatformSocket* CreateListener(ESocketFamily Family, const FSocketAddress& Address)
{
    TUniquePtr<IPlatformSocket> Listener(FPlatformSocket::Create(ESocketType::TCP, Family));
    if (!Listener || !Listener->Bind(Address) || !Listener->Listen(GListenBacklog) || !Listener->SetNonBlocking(true))
    {
        return nullptr;
    }

    return Listener.Release();
}

static bool IsIPv6Available()
{
    TUniquePtr<IPlatformSocket> Probe(FPlatformSocket::Create(ESocketType::TCP, ESocketFamily::IPv6));
    return Probe && Probe->Bind(FSocketAddress::LoopbackIPv6(GAnyPort));
}

static bool ConnectIPv4Loopback(uint16 Port)
{
    TUniquePtr<IPlatformSocket> Client(FPlatformSocket::Create(ESocketType::TCP, ESocketFamily::IPv4));
    return Client && Client->Connect(FSocketAddress::LoopbackIPv4(Port), GRefusedWaitMilliseconds);
}

bool PlatformSocket_Test()
{
    TEST_BEGIN();

    const bool bHasIPv6 = IsIPv6Available();
    if (!bHasIPv6)
    {
        LOG_WARNING("[PlatformSocket] IPv6 is not available, skipping the IPv6 sections");
    }

    TEST_SECTION("A listener bound to port 0 reports the port the OS picked");
    TUniquePtr<IPlatformSocket> Listener(CreateListener(ESocketFamily::IPv4, FSocketAddress::LoopbackIPv4(GAnyPort)));
    TEST_CHECK(Listener.IsValid());
    TEST_EXPECT_EQ(Listener->GetType(), ESocketType::TCP);
    TEST_EXPECT_EQ(Listener->GetFamily(), ESocketFamily::IPv4);

    const uint16 Port = GetLocalPort(Listener.Get());
    TEST_CHECK(Port != GAnyPort);

    TEST_SECTION("Accept returns nothing while no connection is pending");
    {
        TUniquePtr<IPlatformSocket> Nothing(Listener->Accept());
        TEST_EXPECT(!Nothing.IsValid());
    }

    TEST_SECTION("A loopback client connects and is accepted as a loopback peer");
    TUniquePtr<IPlatformSocket> Client(FPlatformSocket::Create(ESocketType::TCP, ESocketFamily::IPv4));
    TEST_CHECK(Client.IsValid());
    TEST_CHECK(Client->Connect(FSocketAddress::LoopbackIPv4(Port), GWaitMilliseconds));

    TEST_CHECK(WaitUntilReadable(Listener.Get(), GWaitMilliseconds));
    TUniquePtr<IPlatformSocket> Server(Listener->Accept());
    TEST_CHECK(Server.IsValid());
    TEST_EXPECT_EQ(Server->GetFamily(), ESocketFamily::IPv4);
    {
        FSocketAddress ServerPeer;
        TEST_EXPECT(Server->GetPeerAddress(ServerPeer));
        TEST_EXPECT(ServerPeer.IsIPv4());
        TEST_EXPECT(ServerPeer.IsLoopback());
        TEST_EXPECT(ServerPeer.ToString().StartsWith("127.0.0.1:"));

        FSocketAddress ClientPeer;
        TEST_EXPECT(Client->GetPeerAddress(ClientPeer));
        TEST_EXPECT(ClientPeer == FSocketAddress::LoopbackIPv4(Port));
    }

    TEST_SECTION("Data travels in both directions");
    TEST_EXPECT(SendText(Client.Get(), "ping\n"));
    TEST_EXPECT(ReceiveText(Server.Get()) == "ping\n");
    TEST_EXPECT(SendText(Server.Get(), "pong\n"));
    TEST_EXPECT(ReceiveText(Client.Get()) == "pong\n");

    TEST_SECTION("Waiting on a socket with nothing pending times out");
    {
        IPlatformSocket* Sockets[] = { Server.Get() };
        bool bReadable = true;
        TEST_EXPECT_EQ(FPlatformSocket::WaitForRead(Sockets, 1, &bReadable, GShortWaitMilliseconds), 0);
        TEST_EXPECT(!bReadable);
    }

    TEST_SECTION("A null entry passed to WaitForRead is never readable");
    {
        TEST_EXPECT(SendText(Client.Get(), "x"));

        IPlatformSocket* Sockets[] = { nullptr, Server.Get() };
        bool Readable[] = { true, false };
        TEST_EXPECT_EQ(FPlatformSocket::WaitForRead(Sockets, 2, Readable, GWaitMilliseconds), 1);
        TEST_EXPECT(!Readable[0]);
        TEST_EXPECT(Readable[1]);
        TEST_EXPECT(ReceiveText(Server.Get()) == "x");
    }

    TEST_SECTION("A non-blocking receive with nothing pending would block");
    {
        CHAR Buffer[GSmallBufferSize];
        int32 BytesRead = 0;
        TEST_EXPECT_EQ(Server->Recv(Buffer, sizeof(Buffer), BytesRead), ESocketResult::WouldBlock);
        TEST_EXPECT_EQ(BytesRead, 0);
    }

    TEST_SECTION("Closing one end is reported as Closed on the other");
    Client->Close();
    TEST_EXPECT(!Client->IsValid());
    TEST_EXPECT(WaitUntilReadable(Server.Get(), GWaitMilliseconds));
    {
        CHAR Buffer[GSmallBufferSize];
        int32 BytesRead = 0;
        TEST_EXPECT_EQ(Server->Recv(Buffer, sizeof(Buffer), BytesRead), ESocketResult::Closed);
    }

    TEST_SECTION("A socket refuses addresses its family cannot use");
    {
        TUniquePtr<IPlatformSocket> IPv4Socket(FPlatformSocket::Create(ESocketType::TCP, ESocketFamily::IPv4));
        TEST_CHECK(IPv4Socket.IsValid());
        TEST_EXPECT(!IPv4Socket->Bind(FSocketAddress::LoopbackIPv6(GAnyPort)));
        TEST_EXPECT(!IPv4Socket->Connect(FSocketAddress::LoopbackIPv6(Port), GShortWaitMilliseconds));

        if (bHasIPv6)
        {
            TUniquePtr<IPlatformSocket> IPv6Socket(FPlatformSocket::Create(ESocketType::TCP, ESocketFamily::IPv6));
            TEST_CHECK(IPv6Socket.IsValid());
            TEST_EXPECT(!IPv6Socket->Bind(FSocketAddress::LoopbackIPv4(GAnyPort)));

            TUniquePtr<IPlatformSocket> DualStackSocket(FPlatformSocket::Create(ESocketType::TCP, ESocketFamily::DualStack));
            TEST_CHECK(DualStackSocket.IsValid());
            TEST_EXPECT_EQ(DualStackSocket->GetFamily(), ESocketFamily::DualStack);
            TEST_EXPECT(DualStackSocket->Bind(FSocketAddress::AnyIPv6(GAnyPort)));
        }
    }

    if (bHasIPv6)
    {
        TEST_SECTION("A strict IPv6 listener on :: refuses IPv4 clients");
        {
            TUniquePtr<IPlatformSocket> StrictListener(CreateListener(ESocketFamily::IPv6, FSocketAddress::AnyIPv6(GAnyPort)));
            TEST_CHECK(StrictListener.IsValid());
            TEST_EXPECT(!ConnectIPv4Loopback(GetLocalPort(StrictListener.Get())));
        }

        TEST_SECTION("A dual-stack listener on :: accepts IPv4 clients and reports them as IPv4");
        {
            TUniquePtr<IPlatformSocket> DualStackListener(CreateListener(ESocketFamily::DualStack, FSocketAddress::AnyIPv6(GAnyPort)));
            TEST_CHECK(DualStackListener.IsValid());
            const uint16 DualStackPort = GetLocalPort(DualStackListener.Get());

            TUniquePtr<IPlatformSocket> IPv4Client(FPlatformSocket::Create(ESocketType::TCP, ESocketFamily::IPv4));
            TEST_CHECK(IPv4Client.IsValid());
            TEST_CHECK(IPv4Client->Connect(FSocketAddress::LoopbackIPv4(DualStackPort), GWaitMilliseconds));

            TEST_CHECK(WaitUntilReadable(DualStackListener.Get(), GWaitMilliseconds));
            TUniquePtr<IPlatformSocket> Accepted(DualStackListener->Accept());
            TEST_CHECK(Accepted.IsValid());
            TEST_EXPECT_EQ(Accepted->GetFamily(), ESocketFamily::DualStack);

            FSocketAddress Peer;
            TEST_EXPECT(Accepted->GetPeerAddress(Peer));
            TEST_EXPECT(Peer.IsIPv4());
            TEST_EXPECT(Peer.IsLoopback());
            TEST_EXPECT(Peer.ToString().StartsWith("127.0.0.1:"));
        }

        TEST_SECTION("IPv6 loopback connects, accepts and carries data");
        {
            TUniquePtr<IPlatformSocket> IPv6Listener(CreateListener(ESocketFamily::IPv6, FSocketAddress::LoopbackIPv6(GAnyPort)));
            TEST_CHECK(IPv6Listener.IsValid());
            const uint16 IPv6Port = GetLocalPort(IPv6Listener.Get());

            TUniquePtr<IPlatformSocket> IPv6Client(FPlatformSocket::Create(ESocketType::TCP, ESocketFamily::IPv6));
            TEST_CHECK(IPv6Client.IsValid());
            TEST_CHECK(IPv6Client->Connect(FSocketAddress::LoopbackIPv6(IPv6Port), GWaitMilliseconds));

            TEST_CHECK(WaitUntilReadable(IPv6Listener.Get(), GWaitMilliseconds));
            TUniquePtr<IPlatformSocket> Accepted(IPv6Listener->Accept());
            TEST_CHECK(Accepted.IsValid());

            FSocketAddress Peer;
            TEST_EXPECT(Accepted->GetPeerAddress(Peer));
            TEST_EXPECT(!Peer.IsIPv4());
            TEST_EXPECT(Peer.IsLoopback());
            TEST_EXPECT(Peer.ToString().StartsWith("[::1]:"));

            TEST_EXPECT(SendText(IPv6Client.Get(), "six"));
            TEST_EXPECT(ReceiveText(Accepted.Get()) == "six");
        }
    }

    TEST_SECTION("A UDP datagram reaches a bound socket through a connected one");
    {
        TUniquePtr<IPlatformSocket> Receiver(FPlatformSocket::Create(ESocketType::UDP, ESocketFamily::IPv4));
        TEST_CHECK(Receiver.IsValid());
        TEST_EXPECT_EQ(Receiver->GetType(), ESocketType::UDP);
        TEST_CHECK(Receiver->Bind(FSocketAddress::LoopbackIPv4(GAnyPort)));

        const uint16 ReceiverPort = GetLocalPort(Receiver.Get());
        TEST_CHECK(ReceiverPort != GAnyPort);

        TUniquePtr<IPlatformSocket> Sender(FPlatformSocket::Create(ESocketType::UDP, ESocketFamily::IPv4));
        TEST_CHECK(Sender.IsValid());
        TEST_CHECK(Sender->Connect(FSocketAddress::LoopbackIPv4(ReceiverPort), GWaitMilliseconds));

        TEST_EXPECT(SendText(Sender.Get(), "datagram"));
        TEST_EXPECT(ReceiveText(Receiver.Get()) == "datagram");
    }

    TEST_SECTION("RecvFrom reports who sent the datagram");
    {
        TUniquePtr<IPlatformSocket> Receiver(FPlatformSocket::Create(ESocketType::UDP, ESocketFamily::IPv4));
        TEST_CHECK(Receiver.IsValid());
        TEST_CHECK(Receiver->Bind(FSocketAddress::LoopbackIPv4(GAnyPort)));
        const uint16 ReceiverPort = GetLocalPort(Receiver.Get());

        TUniquePtr<IPlatformSocket> Sender(FPlatformSocket::Create(ESocketType::UDP, ESocketFamily::IPv4));
        TEST_CHECK(Sender.IsValid());
        TEST_CHECK(Sender->Bind(FSocketAddress::LoopbackIPv4(GAnyPort)));
        const uint16 SenderPort = GetLocalPort(Sender.Get());

        int32 BytesSent = 0;
        TEST_EXPECT_EQ(Sender->SendTo("hello", 5, FSocketAddress::LoopbackIPv4(ReceiverPort), BytesSent), ESocketResult::Success);
        TEST_EXPECT_EQ(BytesSent, 5);

        TEST_CHECK(WaitUntilReadable(Receiver.Get(), GWaitMilliseconds));
        CHAR Buffer[GSmallBufferSize];
        int32 BytesRead = 0;
        FSocketAddress From;
        TEST_EXPECT_EQ(Receiver->RecvFrom(Buffer, sizeof(Buffer), BytesRead, From), ESocketResult::Success);
        TEST_EXPECT(String(Buffer, BytesRead) == "hello");
        TEST_EXPECT(From == FSocketAddress::LoopbackIPv4(SenderPort));
    }

    if (bHasIPv6)
    {
        TEST_SECTION("A dual-stack UDP socket receives from IPv4 senders and reports them as IPv4");
        {
            TUniquePtr<IPlatformSocket> Receiver(FPlatformSocket::Create(ESocketType::UDP, ESocketFamily::DualStack));
            TEST_CHECK(Receiver.IsValid());
            TEST_CHECK(Receiver->Bind(FSocketAddress::AnyIPv6(GAnyPort)));
            const uint16 ReceiverPort = GetLocalPort(Receiver.Get());

            TUniquePtr<IPlatformSocket> Sender(FPlatformSocket::Create(ESocketType::UDP, ESocketFamily::IPv4));
            TEST_CHECK(Sender.IsValid());

            int32 BytesSent = 0;
            TEST_EXPECT_EQ(Sender->SendTo("four", 4, FSocketAddress::LoopbackIPv4(ReceiverPort), BytesSent), ESocketResult::Success);

            TEST_CHECK(WaitUntilReadable(Receiver.Get(), GWaitMilliseconds));
            CHAR Buffer[GSmallBufferSize];
            int32 BytesRead = 0;
            FSocketAddress From;
            TEST_EXPECT_EQ(Receiver->RecvFrom(Buffer, sizeof(Buffer), BytesRead, From), ESocketResult::Success);
            TEST_EXPECT(String(Buffer, BytesRead) == "four");
            TEST_EXPECT(From.IsIPv4());
            TEST_EXPECT(From.IsLoopback());
        }
    }

    TEST_SECTION("An IPv4 UDP socket cannot send to an IPv6 address");
    {
        TUniquePtr<IPlatformSocket> Sender(FPlatformSocket::Create(ESocketType::UDP, ESocketFamily::IPv4));
        TEST_CHECK(Sender.IsValid());

        int32 BytesSent = 0;
        TEST_EXPECT_EQ(Sender->SendTo("x", 1, FSocketAddress::LoopbackIPv6(GAnyPort + 1), BytesSent), ESocketResult::Error);
        TEST_EXPECT_EQ(BytesSent, 0);
    }

    TEST_SECTION("A datagram longer than the buffer is cut to fit, and an empty one is a Success");
    {
        TUniquePtr<IPlatformSocket> Receiver(FPlatformSocket::Create(ESocketType::UDP, ESocketFamily::IPv4));
        TEST_CHECK(Receiver.IsValid());
        TEST_CHECK(Receiver->Bind(FSocketAddress::LoopbackIPv4(GAnyPort)));
        const FSocketAddress ReceiverAddress = FSocketAddress::LoopbackIPv4(GetLocalPort(Receiver.Get()));

        TUniquePtr<IPlatformSocket> Sender(FPlatformSocket::Create(ESocketType::UDP, ESocketFamily::IPv4));
        TEST_CHECK(Sender.IsValid());

        CHAR Large[GLargeDatagramSize];
        for (int32 Index = 0; Index < GLargeDatagramSize; ++Index)
        {
            Large[Index] = static_cast<CHAR>('a' + (Index % 26));
        }

        int32 BytesSent = 0;
        TEST_EXPECT_EQ(Sender->SendTo(Large, GLargeDatagramSize, ReceiverAddress, BytesSent), ESocketResult::Success);
        TEST_CHECK(WaitUntilReadable(Receiver.Get(), GWaitMilliseconds));

        CHAR Small[GTruncatedReadSize];
        int32 BytesRead = 0;
        FSocketAddress From;
        TEST_EXPECT_EQ(Receiver->RecvFrom(Small, GTruncatedReadSize, BytesRead, From), ESocketResult::Success);
        TEST_EXPECT_EQ(BytesRead, GTruncatedReadSize);
        TEST_EXPECT(String(Small, BytesRead) == String(Large, GTruncatedReadSize));

        TEST_EXPECT_EQ(Sender->SendTo(Large, 0, ReceiverAddress, BytesSent), ESocketResult::Success);
        TEST_CHECK(WaitUntilReadable(Receiver.Get(), GWaitMilliseconds));

        BytesRead = -1;
        TEST_EXPECT_EQ(Receiver->RecvFrom(Small, GTruncatedReadSize, BytesRead, From), ESocketResult::Success);
        TEST_EXPECT_EQ(BytesRead, 0);
    }

    TEST_SECTION("Binding to a port another listener holds fails");
    {
        TUniquePtr<IPlatformSocket> Second(FPlatformSocket::Create(ESocketType::TCP, ESocketFamily::IPv4));
        TEST_CHECK(Second.IsValid());
        TEST_EXPECT(!Second->Bind(FSocketAddress::LoopbackIPv4(Port)));
    }

    TEST_END();
}

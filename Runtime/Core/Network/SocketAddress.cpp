#include "Core/Network/SocketAddress.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformSocket.h"

FSocketAddress::FSocketAddress()
    : Port(0)
{
    Memory::Memzero(Bytes, NumBytes);
}

FSocketAddress::~FSocketAddress() = default;

FSocketAddress FSocketAddress::MakeIPv4(uint32 HostOrderIPv4, uint16 Port)
{
    FSocketAddress Address;
    Address.Bytes[MappedMarkerIndex]     = MappedMarkerByte;
    Address.Bytes[MappedMarkerIndex + 1] = MappedMarkerByte;

    for (int32 Index = 0; Index < NumIPv4Bytes; ++Index)
    {
        const int32 Shift = (NumIPv4Bytes - 1 - Index) * BitsPerByte;
        Address.Bytes[MappedPrefixLength + Index] = static_cast<uint8>(HostOrderIPv4 >> Shift);
    }

    Address.Port = Port;
    return Address;
}

FSocketAddress FSocketAddress::MakeIPv6(const uint8* InBytes, uint16 Port)
{
    FSocketAddress Address;
    Memory::Memcpy(Address.Bytes, InBytes, NumBytes);
    Address.Port = Port;
    return Address;
}

FSocketAddress FSocketAddress::AnyIPv4(uint16 Port)
{
    return MakeIPv4(0, Port);
}

FSocketAddress FSocketAddress::AnyIPv6(uint16 Port)
{
    FSocketAddress Address;
    Address.Port = Port;
    return Address;
}

FSocketAddress FSocketAddress::LoopbackIPv4(uint16 Port)
{
    return MakeIPv4(LoopbackIPv4Address, Port);
}

FSocketAddress FSocketAddress::LoopbackIPv6(uint16 Port)
{
    FSocketAddress Address;
    Address.Bytes[NumBytes - 1] = 1;
    Address.Port = Port;
    return Address;
}

String FSocketAddress::FormatHostAndPort(const String& Host, uint16 Port)
{
    if (Host.Contains(':'))
    {
        return String::Printf("[%s]:%u", *Host, static_cast<uint32>(Port));
    }

    return String::Printf("%s:%u", *Host, static_cast<uint32>(Port));
}

bool FSocketAddress::IsIPv4() const
{
    for (int32 Index = 0; Index < MappedMarkerIndex; ++Index)
    {
        if (Bytes[Index] != 0)
        {
            return false;
        }
    }

    return (Bytes[MappedMarkerIndex] == MappedMarkerByte) && (Bytes[MappedMarkerIndex + 1] == MappedMarkerByte);
}

bool FSocketAddress::IsUnspecified() const
{
    if (IsIPv4())
    {
        return GetIPv4() == 0;
    }

    for (int32 Index = 0; Index < NumBytes; ++Index)
    {
        if (Bytes[Index] != 0)
        {
            return false;
        }
    }

    return true;
}

bool FSocketAddress::IsLoopback() const
{
    if (IsIPv4())
    {
        return (GetIPv4() >> IPv4FirstOctetShift) == LoopbackFirstOctet;
    }

    return *this == LoopbackIPv6(Port);
}

uint32 FSocketAddress::GetIPv4() const
{
    if (!IsIPv4())
    {
        return 0;
    }

    uint32 HostOrderIPv4 = 0;
    for (int32 Index = 0; Index < NumIPv4Bytes; ++Index)
    {
        HostOrderIPv4 = (HostOrderIPv4 << BitsPerByte) | Bytes[MappedPrefixLength + Index];
    }

    return HostOrderIPv4;
}

String FSocketAddress::ToString() const
{
    return FormatHostAndPort(ToAddressString(), Port);
}

String FSocketAddress::ToAddressString() const
{
    return FPlatformSocket::AddressToString(*this);
}

bool FSocketAddress::operator==(const FSocketAddress& Other) const
{
    return (Port == Other.Port) && (Memory::Memcmp(Bytes, Other.Bytes, NumBytes) == 0);
}

bool FSocketAddress::operator!=(const FSocketAddress& Other) const
{
    return !(*this == Other);
}

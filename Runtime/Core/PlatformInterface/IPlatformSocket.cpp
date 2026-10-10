#include "Core/PlatformInterface/IPlatformSocket.h"

bool IPlatformSocket::IsAddressCompatible(ESocketFamily Family, const FSocketAddress& Address)
{
    switch (Family)
    {
        case ESocketFamily::IPv4: return Address.IsIPv4();
        case ESocketFamily::IPv6: return !Address.IsIPv4();
        default:                  return true;
    }
}

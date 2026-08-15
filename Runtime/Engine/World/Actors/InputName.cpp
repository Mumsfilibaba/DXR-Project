#include "Engine/World/Actors/InputName.h"
#include "Core/Templates/CString.h"

FInputName::FInputName()
    : Name()
    , Hash(0)
{
}

FInputName::FInputName(const CHAR* InName)
    : Name(InName)
    , Hash(HashInputName(InName, static_cast<uint32>(CString::Strlen(InName))))
{
}

uint32 FInputName::HashInputName(const CHAR* Data, uint32 Length)
{
    // FNV-1a 32-bit
    constexpr uint32 OffsetBasis = 2166136261u;
    constexpr uint32 Prime       = 16777619u;

    uint32 Result = OffsetBasis;
    if (Data)
    {
        for (uint32 Index = 0; Index < Length; ++Index)
        {
            Result ^= static_cast<uint32>(static_cast<uint8>(Data[Index]));
            Result *= Prime;
        }
    }

    return Result;
}

bool FInputName::operator==(const FInputName& Other) const
{
    if (Hash != Other.Hash)
    {
        return false;
    }

    return Name == Other.Name;
}

bool FInputName::operator!=(const FInputName& Other) const
{
    return !(*this == Other);
}

#include "Core/Misc/Guid.h"
#include "Core/Platform/PlatformMisc.h"

static int32 HexDigitValue(CHAR Char)
{
    if ((Char >= '0') && (Char <= '9'))
    {
        return Char - '0';
    }
    else if ((Char >= 'a') && (Char <= 'f'))
    {
        return Char - 'a' + 10;
    }
    else if ((Char >= 'A') && (Char <= 'F'))
    {
        return Char - 'A' + 10;
    }

    return -1;
}

FGuid FGuid::Create()
{
    return FPlatformMisc::CreateGuid();
}

bool FGuid::Parse(const String& Text, FGuid& OutGuid)
{
    const bool bIsDashed = (Text.Length() == 36);
    if (!bIsDashed && (Text.Length() != 32))
    {
        return false;
    }

    if (bIsDashed && ((Text[8] != '-') || (Text[13] != '-') || (Text[18] != '-') || (Text[23] != '-')))
    {
        return false;
    }

    uint32 Words[4] = { 0, 0, 0, 0 };

    int32 NumDigits = 0;
    for (int32 Index = 0; Index < Text.Length(); ++Index)
    {
        if (bIsDashed && ((Index == 8) || (Index == 13) || (Index == 18) || (Index == 23)))
        {
            continue;
        }

        const int32 Value = HexDigitValue(Text[Index]);
        if (Value < 0)
        {
            return false;
        }

        uint32& Word = Words[NumDigits / 8];
        Word = (Word << 4) | static_cast<uint32>(Value);
        ++NumDigits;
    }

    OutGuid = FGuid(Words[0], Words[1], Words[2], Words[3]);
    return true;
}

String FGuid::ToString() const
{
    return String::Printf("%08x%08x%08x%08x", A, B, C, D);
}

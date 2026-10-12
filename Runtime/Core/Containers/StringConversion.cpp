#include "Core/Containers/StringConversion.h"

static constexpr uint32 MaxCodePoint = 0x10FFFF;

static bool IsSurrogate(uint32 CodePoint)
{
    return CodePoint >= 0xD800 && CodePoint <= 0xDFFF;
}

static uint32 Decode(const UTF8CHAR** Source, const UTF8CHAR* End)
{
    const uint32 Lead = static_cast<uint8>(**Source);
    ++(*Source);
    if (Lead < 0x80)
    {
        return Lead;
    }

    int32  NumTrailing;
    uint32 CodePoint;
    uint32 FirstMin = 0x80;
    uint32 FirstMax = 0xBF;

    if (Lead >= 0xC2 && Lead <= 0xDF)
    {
        NumTrailing = 1;
        CodePoint   = Lead & 0x1F;
    }
    else if (Lead >= 0xE0 && Lead <= 0xEF)
    {
        NumTrailing = 2;
        CodePoint   = Lead & 0x0F;
        FirstMin    = (Lead == 0xE0) ? 0xA0 : 0x80;
        FirstMax    = (Lead == 0xED) ? 0x9F : 0xBF;
    }
    else if (Lead >= 0xF0 && Lead <= 0xF4)
    {
        NumTrailing = 3;
        CodePoint   = Lead & 0x07;
        FirstMin    = (Lead == 0xF0) ? 0x90 : 0x80;
        FirstMax    = (Lead == 0xF4) ? 0x8F : 0xBF;
    }
    else
    {
        return FStringConversion::ReplacementCharacter;
    }

    for (int32 Index = 0; Index < NumTrailing; ++Index)
    {
        if (*Source == End)
        {
            return FStringConversion::ReplacementCharacter;
        }

        const uint32 Trailing = static_cast<uint8>(**Source);
        const uint32 Min      = (Index == 0) ? FirstMin : 0x80;
        const uint32 Max      = (Index == 0) ? FirstMax : 0xBF;

        if (Trailing < Min || Trailing > Max)
        {
            return FStringConversion::ReplacementCharacter;
        }

        CodePoint = (CodePoint << 6) | (Trailing & 0x3F);
        ++(*Source);
    }

    return CodePoint;
}

static uint32 Decode(const char16_t** Source, const char16_t* End)
{
    const uint32 Unit = **Source;
    ++(*Source);

    if (!IsSurrogate(Unit))
    {
        return Unit;
    }

    if (Unit <= 0xDBFF && *Source != End)
    {
        const uint32 Low = **Source;
        if (Low >= 0xDC00 && Low <= 0xDFFF)
        {
            ++(*Source);
            return 0x10000 + ((Unit - 0xD800) << 10) + (Low - 0xDC00);
        }
    }

    return FStringConversion::ReplacementCharacter;
}

static uint32 Decode(const char32_t** Source, const char32_t*)
{
    const uint32 CodePoint = **Source;
    ++(*Source);
    return (CodePoint > MaxCodePoint || IsSurrogate(CodePoint)) ? FStringConversion::ReplacementCharacter : CodePoint;
}

static int32 Encode(uint32 CodePoint, UTF8CHAR* Dest)
{
    if (CodePoint < 0x80)
    {
        Dest[0] = static_cast<UTF8CHAR>(CodePoint);
        return 1;
    }
    else if (CodePoint < 0x800)
    {
        Dest[0] = static_cast<UTF8CHAR>(0xC0 | (CodePoint >> 6));
        Dest[1] = static_cast<UTF8CHAR>(0x80 | (CodePoint & 0x3F));
        return 2;
    }
    else if (CodePoint < 0x10000)
    {
        Dest[0] = static_cast<UTF8CHAR>(0xE0 | (CodePoint >> 12));
        Dest[1] = static_cast<UTF8CHAR>(0x80 | ((CodePoint >> 6) & 0x3F));
        Dest[2] = static_cast<UTF8CHAR>(0x80 | (CodePoint & 0x3F));
        return 3;
    }
    else
    {
        Dest[0] = static_cast<UTF8CHAR>(0xF0 | (CodePoint >> 18));
        Dest[1] = static_cast<UTF8CHAR>(0x80 | ((CodePoint >> 12) & 0x3F));
        Dest[2] = static_cast<UTF8CHAR>(0x80 | ((CodePoint >> 6) & 0x3F));
        Dest[3] = static_cast<UTF8CHAR>(0x80 | (CodePoint & 0x3F));
        return 4;
    }
}

static int32 Encode(uint32 CodePoint, char16_t* Dest)
{
    if (CodePoint < 0x10000)
    {
        Dest[0] = static_cast<char16_t>(CodePoint);
        return 1;
    }

    CodePoint -= 0x10000;
    Dest[0] = static_cast<char16_t>(0xD800 + (CodePoint >> 10));
    Dest[1] = static_cast<char16_t>(0xDC00 + (CodePoint & 0x3FF));
    return 2;
}

static int32 Encode(uint32 CodePoint, char32_t* Dest)
{
    Dest[0] = static_cast<char32_t>(CodePoint);
    return 1;
}

template<typename ToCharType, typename FromCharType>
static int32 ConvertCodeUnits(ToCharType* Dest, int32 DestLength, const FromCharType* Source, int32 SourceLength)
{
    const FromCharType* End = Source + SourceLength;

    int32 NumWritten = 0;
    while (Source < End)
    {
        ToCharType Units[4];
        const int32 NumUnits = Encode(Decode(&Source, End), Units);

        if (Dest)
        {
            if (NumWritten + NumUnits > DestLength)
            {
                break;
            }

            for (int32 Index = 0; Index < NumUnits; ++Index)
            {
                Dest[NumWritten + Index] = Units[Index];
            }
        }

        NumWritten += NumUnits;
    }

    return NumWritten;
}

int32 FStringConversion::Convert(char16_t* Dest, int32 DestLength, const UTF8CHAR* Source, int32 SourceLength)
{
    return ConvertCodeUnits(Dest, DestLength, Source, SourceLength);
}

int32 FStringConversion::Convert(char32_t* Dest, int32 DestLength, const UTF8CHAR* Source, int32 SourceLength)
{
    return ConvertCodeUnits(Dest, DestLength, Source, SourceLength);
}

int32 FStringConversion::Convert(UTF8CHAR* Dest, int32 DestLength, const char16_t* Source, int32 SourceLength)
{
    return ConvertCodeUnits(Dest, DestLength, Source, SourceLength);
}

int32 FStringConversion::Convert(char32_t* Dest, int32 DestLength, const char16_t* Source, int32 SourceLength)
{
    return ConvertCodeUnits(Dest, DestLength, Source, SourceLength);
}

int32 FStringConversion::Convert(UTF8CHAR* Dest, int32 DestLength, const char32_t* Source, int32 SourceLength)
{
    return ConvertCodeUnits(Dest, DestLength, Source, SourceLength);
}

int32 FStringConversion::Convert(char16_t* Dest, int32 DestLength, const char32_t* Source, int32 SourceLength)
{
    return ConvertCodeUnits(Dest, DestLength, Source, SourceLength);
}

template<typename CharType>
static uint32 DecodeAt(const CharType* Source, int32 SourceLength, int32& OutNumUnits)
{
    const CharType* Cursor = Source;
    const uint32 CodePoint = Decode(&Cursor, Source + SourceLength);

    OutNumUnits = static_cast<int32>(Cursor - Source);
    return CodePoint;
}

template<typename CharType>
static int32 EncodeValid(uint32 CodePoint, CharType* Dest)
{
    return Encode((CodePoint > MaxCodePoint || IsSurrogate(CodePoint)) ? FStringConversion::ReplacementCharacter : CodePoint, Dest);
}

uint32 FStringConversion::DecodeCodePoint(const UTF8CHAR* Source, int32 SourceLength, int32& OutNumUnits)
{
    return DecodeAt(Source, SourceLength, OutNumUnits);
}

uint32 FStringConversion::DecodeCodePoint(const char16_t* Source, int32 SourceLength, int32& OutNumUnits)
{
    return DecodeAt(Source, SourceLength, OutNumUnits);
}

uint32 FStringConversion::DecodeCodePoint(const char32_t* Source, int32 SourceLength, int32& OutNumUnits)
{
    return DecodeAt(Source, SourceLength, OutNumUnits);
}

int32 FStringConversion::EncodeCodePoint(uint32 CodePoint, UTF8CHAR* Dest)
{
    return EncodeValid(CodePoint, Dest);
}

int32 FStringConversion::EncodeCodePoint(uint32 CodePoint, char16_t* Dest)
{
    return EncodeValid(CodePoint, Dest);
}

int32 FStringConversion::EncodeCodePoint(uint32 CodePoint, char32_t* Dest)
{
    return EncodeValid(CodePoint, Dest);
}

int32 FStringConversion::FindPreviousCodePoint(const UTF8CHAR* Source, int32 Index)
{
    if (Index <= 0)
    {
        return 0;
    }

    int32 Start = Index - 1;
    while (Start > 0 && (Index - Start) < MaxCodeUnitsPerCodePoint && (static_cast<uint8>(Source[Start]) & 0xC0) == 0x80)
    {
        --Start;
    }

    int32 NumUnits = 0;
    DecodeAt(Source + Start, Index - Start, NumUnits);
    return (Start + NumUnits == Index) ? Start : Index - 1;
}

int32 FStringConversion::FindPreviousCodePoint(const char16_t* Source, int32 Index)
{
    if (Index <= 0)
    {
        return 0;
    }

    const uint32 Last = Source[Index - 1];
    if (Index >= 2 && Last >= 0xDC00 && Last <= 0xDFFF)
    {
        const uint32 High = Source[Index - 2];
        if (High >= 0xD800 && High <= 0xDBFF)
        {
            return Index - 2;
        }
    }

    return Index - 1;
}

int32 FStringConversion::FindPreviousCodePoint(const char32_t*, int32 Index)
{
    return Index > 0 ? Index - 1 : 0;
}

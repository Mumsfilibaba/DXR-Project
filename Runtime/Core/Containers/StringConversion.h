#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/StaticString.h"
#include "Core/Templates/TypeTraits/Conditional.h"

/*
 * Conversions between the Unicode encodings: UTF-8 (ANSICHAR and UTF8CHAR), UTF-16 (char16_t, and WIDECHAR on Windows)
 * and UTF-32 (char32_t, and WIDECHAR on macOS). Invalid input, such as a truncated UTF-8 sequence or a lone UTF-16
 * surrogate, becomes U+FFFD instead of failing, so a conversion always produces something printable.
 */

struct CORE_API FStringConversion
{
    /** @brief The character every invalid sequence is replaced by */
    static constexpr uint32 ReplacementCharacter = 0xFFFD;

    /**
     * @brief Convert SourceLength code units of Source into Dest, without a null-terminator
     * @param Dest Where to write the result, or nullptr to only count it
     * @param DestLength The number of code units Dest has room for. A code point that does not fit is not written in part.
     * @return Returns the number of code units written, or the number needed when Dest is nullptr
     */
    static int32 Convert(char16_t* Dest, int32 DestLength, const UTF8CHAR* Source, int32 SourceLength);
    static int32 Convert(char32_t* Dest, int32 DestLength, const UTF8CHAR* Source, int32 SourceLength);
    static int32 Convert(UTF8CHAR* Dest, int32 DestLength, const char16_t* Source, int32 SourceLength);
    static int32 Convert(char32_t* Dest, int32 DestLength, const char16_t* Source, int32 SourceLength);
    static int32 Convert(UTF8CHAR* Dest, int32 DestLength, const char32_t* Source, int32 SourceLength);
    static int32 Convert(char16_t* Dest, int32 DestLength, const char32_t* Source, int32 SourceLength);
};

template<typename CharType>
struct TUnicodeCharType;

template<>
struct TUnicodeCharType<ANSICHAR>
{
    using Type = UTF8CHAR;
};

template<>
struct TUnicodeCharType<UTF8CHAR>
{
    using Type = UTF8CHAR;
};

template<>
struct TUnicodeCharType<WIDECHAR>
{
    using Type = typename TConditional<sizeof(WIDECHAR) == sizeof(char16_t), char16_t, char32_t>::Type;
};

template<typename ToCharType, typename FromCharType>
class TStringConversion
{
    using ToUnicodeType   = typename TUnicodeCharType<ToCharType>::Type;
    using FromUnicodeType = typename TUnicodeCharType<FromCharType>::Type;

    inline static constexpr bool bIsSameEncoding = TIsSame<ToUnicodeType, FromUnicodeType>::Value;

public:

    /**
     * @param InSource The text to convert
     * @param InLength The number of code units in InSource, not counting a null-terminator
     * @param bIsNullTerminated InSource[InLength] is a null-terminator, so the text can be used without copying it
     */
    TStringConversion(const FromCharType* InSource, int32 InLength, bool bIsNullTerminated)
        : Source(nullptr)
        , SourceLength(0)
    {
        if constexpr (bIsSameEncoding)
        {
            if (bIsNullTerminated)
            {
                Source       = reinterpret_cast<const ToCharType*>(InSource);
                SourceLength = InLength;
            }
            else
            {
                Converted = TString<ToCharType>(reinterpret_cast<const ToCharType*>(InSource), InLength);
            }
        }
        else
        {
            const FromUnicodeType* From = reinterpret_cast<const FromUnicodeType*>(InSource);

            const int32 ConvertedLength = FStringConversion::Convert(static_cast<ToUnicodeType*>(nullptr), 0, From, InLength);
            Converted.Resize(ConvertedLength);
            FStringConversion::Convert(reinterpret_cast<ToUnicodeType*>(Converted.Data()), ConvertedLength, From, InLength);
        }
    }

    /** @return Returns the converted text, always null-terminated */
    NODISCARD FORCEINLINE const ToCharType* Get() const
    {
        return Source ? Source : *Converted;
    }

    /** @return Returns the number of code units in the converted text, not counting the null-terminator */
    NODISCARD FORCEINLINE int32 Length() const
    {
        return Source ? SourceLength : Converted.Length();
    }

    /** @return Returns the converted text as a string that owns its characters */
    NODISCARD FORCEINLINE TString<ToCharType> ToString() const&
    {
        return Source ? TString<ToCharType>(Source, SourceLength) : Converted;
    }

    NODISCARD FORCEINLINE TString<ToCharType> ToString() &&
    {
        return Source ? TString<ToCharType>(Source, SourceLength) : Move(Converted);
    }

private:
    const ToCharType*   Source;
    int32               SourceLength;
    TString<ToCharType> Converted;
};

template<typename ToCharType, typename FromCharType>
NODISCARD FORCEINLINE TStringConversion<ToCharType, FromCharType> StringCast(const FromCharType* Source)
{
    return TStringConversion<ToCharType, FromCharType>(Source, TCString<FromCharType>::Strlen(Source), true);
}

template<typename ToCharType, typename FromCharType>
NODISCARD FORCEINLINE TStringConversion<ToCharType, FromCharType> StringCast(const FromCharType* Source, int32 Length)
{
    return TStringConversion<ToCharType, FromCharType>(Source, Length, false);
}

template<typename ToCharType, typename FromCharType>
NODISCARD FORCEINLINE TStringConversion<ToCharType, FromCharType> StringCast(const TString<FromCharType>& Source)
{
    return TStringConversion<ToCharType, FromCharType>(*Source, Source.Length(), true);
}

template<typename ToCharType, typename FromCharType>
NODISCARD FORCEINLINE TStringConversion<ToCharType, FromCharType> StringCast(const TStringView<FromCharType>& Source)
{
    return TStringConversion<ToCharType, FromCharType>(Source.Data(), Source.Length(), false);
}

template<typename ToCharType, typename FromCharType, int32 NUM_CHARS>
NODISCARD FORCEINLINE TStringConversion<ToCharType, FromCharType> StringCast(const TStaticString<FromCharType, NUM_CHARS>& Source)
{
    return TStringConversion<ToCharType, FromCharType>(Source.Data(), Source.Length(), true);
}

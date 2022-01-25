#pragma once
#include "StringView.h"

#include "Core/Templates/Identity.h"
#include "Core/Templates/AddReference.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Characters class with a fixed allocated number of characters

template<typename CharType, int32 CharCount>
class TStaticString
{
public:

    static_assert(TIsSame<CharType, char>::Value || TIsSame<CharType, wchar_t>::Value, "Only char and wchar_t is supported for strings");
    static_assert(CharCount > 0, "The number of chars has to be more than zero");

    using ElementType = CharType;
    using SizeType = int32;
    using StringUtils = TStringUtils<CharType>;

    enum
    {
        NPos = SizeType(-1)
    };

    typedef TArrayIterator<TStaticString, CharType>                    IteratorType;
    typedef TArrayIterator<const TStaticString, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<TStaticString, CharType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TStaticString, const CharType> ReverseConstIteratorType;

    /**
     * Create a static string from a formatted string
     * 
     * @param Format: Formatted string
     * @return: Returns the formatted string based on the format string
     */
    static NOINLINE TStaticString MakeFormated(const CharType* Format, ...) noexcept
    {
        TStaticString NewString;

        va_list ArgsList;
        va_start(ArgsList, Format);
        NewString.FormatV(Format, ArgsList);
        va_end(ArgsList);

        return NewString;
    }

    /**
     * Create a static string from a formatted string and an argument-list
     *
     * @param Format: Formatted string
     * @param ArgsList: Argument-list to be formatted based on the format-string
     * @return: Returns the formatted string based on the format string
     */
    static FORCEINLINE void MakeFormatedV(const CharType* Format, va_list ArgsList) noexcept
    {
        TStaticString NewString;
        NewString.FormatV(Format, ArgsList);
        return NewString;
    }

    /**
     * Default constructor
     */
    FORCEINLINE TStaticString() noexcept
        : Characters()
        , Len(0)
    {
        Characters[Len] = StringUtils::Null;
    }

    /**
     * Create a fixed string from a raw string. If the string is longer than the fixed length
     * the string will be shortened to fit.
     * 
     * @param InString: String to copy
     */
    FORCEINLINE TStaticString(const CharType* InString) noexcept
        : Characters()
        , Len(0)
    {
        if (InString)
        {
            CopyFrom(InString, StringUtils::Length(InString));
        }
    }

    /**
     * Create a fixed string from a specified length raw array. If the string is longer than the 
     * fixed length the string will be shortened to fit.
     *
     * @param InString: String to copy
     * @param InLength: Length of the string to copy
     */
    FORCEINLINE explicit TStaticString(const CharType* InString, uint32 InLength) noexcept
        : Characters()
        , Len(0)
    {
        if (InString)
        {
            CopyFrom(InString, InLength);
        }
    }

    /**
     * Create a static string from another string-type. If the string is longer than the
     * fixed length the string will be shortened to fit.
     * 
     * @param InString: String to copy from
     */
    template<typename StringType, typename = typename TEnableIf<TIsTStringType<StringType>::Value>::Type>
    FORCEINLINE explicit TStaticString(const StringType& InString) noexcept
        : Characters()
        , Len(0)
    {
        CopyFrom(InString.CStr(), InString.Length());
    }

    /**
     * Copy Constructor
     * 
     * @param Other: Other string to copy from
     */
    FORCEINLINE TStaticString(const TStaticString& Other) noexcept
        : Characters()
        , Len(0)
    {
        CopyFrom(Other.CStr(), Other.Length());
    }

    /**
     * Move Constructor
     *
     * @param Other: Other string to move from
     */
    FORCEINLINE TStaticString(TStaticString&& Other) noexcept
        : Characters()
        , Len(0)
    {
        MoveFrom(Forward<TStaticString>(Other));
    }

    /**
     * Clears the string
     */
    FORCEINLINE void Clear() noexcept
    {
        Len = 0;
        Characters[Len] = StringUtils::Null;
    }

    /**
     * Appends a character to this string 
     * 
     * @param Char: Character to append
     */
    FORCEINLINE void Append(CharType Char) noexcept
    {
        Assert(Len + 1 < Capacity());

        Characters[Len] = Char;
        Characters[++Len] = StringUtils::Null;
    }

    /**
     * Appends a raw-string to this string
     *
     * @param InString: String to append
     */
    FORCEINLINE void Append(const CharType* InString) noexcept
    {
        Append(InString, StringUtils::Length(InString));
    }

    /**
     * Appends a string of another string-type to this string
     *
     * @param InString: String to append
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value>::Type Append(const StringType& InString) noexcept
    {
        Append(InString.CStr(), InString.Length());
    }

    /**
     * Appends a raw-string to this string with a fixed length
     *
     * @param InString: String to append
     * @param InLength: Length of the string
     */
    FORCEINLINE void Append(const CharType* InString, SizeType InLength) noexcept
    {
        Assert(InString != nullptr);
        Assert(Len + InLength < Capacity());

        StringUtils::Copy(Characters + Len, InString, InLength);

        Len = Len + InLength;
        Characters[Len] = StringUtils::Null;
    }

    /**
     * Resize the string
     * 
     * @param NewLength: New length of the string 
     */
    FORCEINLINE void Resize(SizeType NewLength) noexcept
    {
        Resize(NewLength, CharType());
    }

    /**
     * Resize the string and fill the new elements with a specified character
     *
     * @param NewLength: New length of the string
     * @param FillElement: Element to fill the string with
     */
    FORCEINLINE void Resize(SizeType NewLength, CharType FillElement) noexcept
    {
        Assert(NewLength < CharCount);

        CharType* It = Characters + Len;
        CharType* End = Characters + NewLength;
        while (It != End)
        {
            *(It++) = FillElement;
        }

        Len = NewLength;
        Characters[Len] = StringUtils::Null;
    }

    /**
     * Copy this string into buffer 
     * 
     * @param Buffer: Buffer to fill
     * @param BufferSize: Size of the buffer to fill
     * @param Position: Offset to start copy from
     */
    FORCEINLINE void Copy(CharType* Buffer, SizeType BufferSize, SizeType Position = 0) const noexcept
    {
        Assert(Buffer != nullptr);
        Assert((Position < Len) || (Position == 0));

        SizeType CopySize = NMath::Min(BufferSize, Len - Position);
        StringUtils::Copy(Buffer, Characters + Position, CopySize);
    }

    /**
     * Replace the string with a formatted string (similar to snprintf)
     * 
     * @param Format: Formatted string to replace the string with
     */
    NOINLINE void Format(const CharType* Format, ...) noexcept
    {
        va_list ArgList;
        va_start(ArgList, Format);
        FormatV(Format, ArgList);
        va_end(ArgList);
    }

    /**
     * Replace the string with a formatted string (similar to snprintf)
     *
     * @param Format: Formatted string to replace the string with
     * @param ArgList: Argument list filled with arguments for the formatted string
     */
    FORCEINLINE void FormatV(const CharType* Format, va_list ArgList) noexcept
    {
        SizeType WrittenChars = StringUtils::FormatBufferV(Characters, CharCount - 1, Format, ArgList);
        if (WrittenChars < CharCount)
        {
            Len = WrittenChars;
        }
        else
        {
            Len = CharCount - 1;
        }

        Characters[Len] = StringUtils::Null;
    }

    /**
     * Appends a formatted string to the string
     * 
     * @param Format: Formatted string to append
     */
    NOINLINE void AppendFormat(const CharType* Format, ...) noexcept
    {
        va_list ArgList;
        va_start(ArgList, Format);
        AppendFormatV(Format, ArgList);
        va_end(ArgList);
    }

    /**
     * Appends a formatted string to the string
     *
     * @param Format: Formatted string to append
     * @param ArgList: Argument-list for the formatted string
     */
    FORCEINLINE void AppendFormatV(const CharType* Format, va_list ArgList) noexcept
    {
        const SizeType WrittenChars = StringUtils::FormatBufferV(Characters + Len, CharCount, Format, ArgList);
        const SizeType NewLength = Len + WrittenChars;
        if (NewLength < CharCount)
        {
            Len = NewLength;
        }
        else
        {
            Len = CharCount - 1;
        }

        Characters[Len] = StringUtils::Null;
    }

    /**
     * Convert this string to a lower-case string
     */ 
    FORCEINLINE void ToLowerInline() noexcept
    {
        CharType* It = Characters;
        CharType* End = Characters + Len;
        while (It != End)
        {
            *It = StringUtils::ToLower(*It);
            It++;
        }
    }

    /**
     * Convert this string to a lower-case string and returns a copy
     * 
     * @return: Returns a copy of this string, with all characters in lower-case
     */
    FORCEINLINE TStaticString ToLower() const noexcept
    {
        TStaticString NewString(*this);
        NewString.ToLowerInline();
        return NewString;
    }

    /**
     * Convert this string to a upper-case string
     */
    FORCEINLINE void ToUpperInline() noexcept
    {
        CharType* It = Characters;
        CharType* End = Characters + Len;
        while (It != End)
        {
            *It = StringUtils::ToUpper(*It);
            It++;
        }
    }

    /**
     * Convert this string to a upper-case string and returns a copy
     *
     * @return: Returns a copy of this string, with all characters in upper-case
     */
    FORCEINLINE TStaticString ToUpper() const noexcept
    {
        TStaticString NewString(*this);
        NewString.ToUpperInline();
        return NewString;
    }

    /**
     * Removes whitespace from the beginning and end of the string 
     */
    FORCEINLINE void TrimInline() noexcept
    {
        TrimStartInline();
        TrimEndInline();
    }

    /**
     * Removes whitespace from the beginning and end of the string and returns a copy
     * 
     * @return: Returns a copy of this string with the whitespace removed in the end and beginning
     */
    FORCEINLINE TStaticString Trim() noexcept
    {
        TStaticString NewString(*this);
        NewString.TrimInline();
        return NewString;
    }

    /**
     * Removes whitespace from the beginning of the string 
     */
    FORCEINLINE void TrimStartInline() noexcept
    {
        SizeType Index = 0;
        for (; Index < Len; Index++)
        {
            if (!StringUtils::IsWhiteSpace(Characters[Index]))
            {
                break;
            }
        }

        if (Index > 0)
        {
            Len -= Index;
            CMemory::Memmove(Characters, Characters + Index, SizeInBytes());
        }
    }

    /**
     * Removes whitespace from the beginning of the string and returns a copy
     * 
     * @return: Returns a copy of this string with all the whitespace removed from the beginning
     */
    FORCEINLINE TStaticString TrimStart() noexcept
    {
        TStaticString NewString(*this);
        NewString.TrimStartInline();
        return NewString;
    }

    /**
     * Removes whitespace from the end of the string
     */
    FORCEINLINE void TrimEndInline() noexcept
    {
        for (SizeType Index = Len - 1; Index >= 0; Index--)
        {
            if (StringUtils::IsWhiteSpace(Characters[Index]))
            {
                Len--;
            }
            else
            {
                break;
            }
        }

        Characters[Len] = StringUtils::Null;
    }

    /**
     * Removes whitespace from the end of the string and returns a copy
     *
     * @return: Returns a copy of this string with all the whitespace removed from the end
     */
    FORCEINLINE TStaticString TrimEnd() noexcept
    {
        TStaticString NewString(*this);
        NewString.TrimEndInline();
        return NewString;
    }

    /**
     * Reverses the order of all the characters in the string
     */
    FORCEINLINE void ReverseInline() noexcept
    {
        CharType* Start = Characters;
        CharType* End = Start + Len;
        while (Start < End)
        {
            End--;
            ::Swap<CharType>(*Start, *End);
            Start++;
        }
    }

    /**
     * Reverses the order of all the characters in the string and returns a copy
     * 
     * @return: Returns a string with all the characters reversed
     */
    FORCEINLINE TStaticString Reverse() noexcept
    {
        TStaticString NewString(*this);
        NewString.ReverseInline();
        return NewString;
    }

    /**
     * Compares this string to another string-type
     * 
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, int32>::Type Compare(const StringType& InString) const noexcept
    {
        return Compare(InString.CStr());
    }

    /**
     * Compares this string with a raw-string
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    FORCEINLINE int32 Compare(const CharType* InString) const noexcept
    {
        return StringUtils::Compare(Characters, InString);
    }

    /**
     * Compares this string with a raw-string of a fixed length
     *
     * @param InString: String to compare with
     * @param InLength: Length of the string to compare
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    FORCEINLINE int32 Compare(const CharType* InString, SizeType InLength) const noexcept
    {
        return StringUtils::Compare(Characters, InString, InLength);
    }

    /**
     * Compares this string to another string-type without taking casing into account.
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, int32>::Type CompareNoCase(const StringType& InString) const noexcept
    {
        return CompareNoCase(InString.CStr(), InString.Length());
    }

    /**
     * Compares this string with a raw-string without taking casing into account.
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    FORCEINLINE int32 CompareNoCase(const CharType* InString) const noexcept
    {
        return CompareNoCase(InString, StringUtils::Length(InString));
    }

    /**
     * Compares this string with a raw-string without taking casing into account.
     *
     * @param InString: String to compare with
     * @param InLength: Length of the string to compare
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    FORCEINLINE int32 CompareNoCase(const CharType* InString, SizeType InLength) const noexcept
    {
        if (Len != InLength)
        {
            return -1;
        }

        for (SizeType Index = 0; Index < Len; Index++)
        {
            const CharType TempChar0 = StringUtils::ToLower(Characters[Index]);
            const CharType TempChar1 = StringUtils::ToLower(InString[Index]);

            if (TempChar0 != TempChar1)
            {
                return TempChar0 - TempChar1;
            }
        }

        return 0;
    }

    /**
     * Find the position of the first occurrence of the start of the search-string 
     * 
     * @param InString: String to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    FORCEINLINE SizeType Find(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return Find(InString, StringUtils::Length(InString), Position);
    }

    /**
     * Find the position of the first occurrence of the start of the search-string
     *
     * @param InString: String to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type Find(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return Find(InString, InString.Length(), Position);
    }

    /**
     * Find the position of the first occurrence of the start of the search-string
     *
     * @param InString: String to search
     * @param InString: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    FORCEINLINE SizeType Find(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Assert((Position < Len) || (Position == 0));

        if ((InLength == 0) || StringUtils::IsTerminator(*InString) || (Len == 0))
        {
            return 0;
        }

        const CharType* Start = CStr() + Position;
        const CharType* Result = StringUtils::Find(Start, InString);
        if (!Result)
        {
            return NPos;
        }
        else
        {
            return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
        }
    }

    /**
     * Returns the position of the first occurrence of char
     * 
     * @param Char: Character to search for
     * @param Position: Position to start search at
     * @return: Returns the position of the first occurrence of the char
     */
    FORCEINLINE SizeType Find(CharType Char, SizeType Position = 0) const noexcept
    {
        Assert((Position < Len) || (Position == 0));

        if (StringUtils::IsTerminator(Char) || (Len == 0))
        {
            return 0;
        }

        const CharType* Start = CStr() + Position;
        const CharType* Result = StringUtils::FindChar(Start, Char);
        if (!Result)
        {
            return NPos;
        }
        else
        {
            return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
        }
    }

    /**
     * Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     *
     * @param InString: String to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    FORCEINLINE SizeType ReverseFind(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return ReverseFind(InString, StringUtils::Length(InString), Position);
    }

    /**
     * Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     *
     * @param InString: String to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type ReverseFind(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return ReverseFind(InString, InString.Length(), Position);
    }

    /**
     * Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     * Position is the end, instead of the start as with normal Find.
     * 
     * @param InString: String to search
     * @param InString: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    FORCEINLINE SizeType ReverseFind(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Assert((Position < Len) || (Position == 0));

        if ((InLength == 0) || StringUtils::IsTerminator(*InString) || (Len == 0))
        {
            return Len;
        }

        SizeType Length = (Position == 0) ? Len : NMath::Min(Position, Len);

        const CharType* Start = CStr();
        const CharType* End = Start + Length;
        while (End != Start)
        {
            End--;

            const CharType* SubstringIt = InString;
            for (const CharType* EndIt = End; ; )
            {
                if (*(EndIt++) != *(SubstringIt++))
                {
                    break;
                }
                else if (StringUtils::IsTerminator(*SubstringIt))
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(End - Start));
                }
            }
        }

        return NPos;
    }

    /**
     * Returns the position of the first occurrence of char. Searches the string in reverse.
     *
     * @param Char: Character to search for
     * @param Position: Position to start search at
     * @return: Returns the position of the first occurrence of the char
     */
    FORCEINLINE SizeType ReverseFind(CharType Char, SizeType Position = 0) const noexcept
    {
        Assert((Position < Len) || (Position == 0));

        if (StringUtils::IsTerminator(Char) || (Len == 0))
        {
            return Len;
        }

        const CharType* Result = nullptr;
        const CharType* Start = CStr();
        if (Position == 0)
        {
            Result = StringUtils::ReverseFindChar(Start, Char);
        }
        else
        {
            // TODO: Get rid of const_cast
            CharType* TempCharacters = const_cast<CharType*>(Characters);
            CharType TempChar = TempCharacters[Position + 1];
            TempCharacters[Position + 1] = StringUtils::Null;

            Result = StringUtils::ReverseFindChar(TempCharacters, Char);

            TempCharacters[Position + 1] = TempChar;
        }

        if (!Result)
        {
            return NPos;
        }
        else
        {
            return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
        }
    }

    /**
     * Returns the position of the first occurrence of a character in the search-string that is found
     * 
     * @param InString: String of characters to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string that is found
     */
    FORCEINLINE SizeType FindOneOf(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return FindOneOf(InString, StringUtils::Length(InString), Position);
    }

    /**
     * Returns the position of the first occurrence of a character in the search-string that is found. The search string is of string-type.
     *
     * @param InString: String of characters to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string that is found
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type FindOneOf(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return FindOneOf(InString.CStr(), InString.Length(), Position);
    }

    /**
     * Returns the position of the first occurrence of a character in the search-string that is found
     *
     * @param InString: String of characters to search
     * @param InLength: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string that is found
     */
    FORCEINLINE SizeType FindOneOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Assert((Position < Len) || (Position == 0));

        if ((InLength == 0) || StringUtils::IsTerminator(*InString) || (Len == 0))
        {
            return 0;
        }

        const CharType* Start = CStr() + Position;
        const CharType* Result = StringUtils::FindOneOf(Start, InString);
        if (!Result)
        {
            return NPos;
        }
        else
        {
            return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
        }
    }

    /**
     * Returns the position of the last occurrence of a character in the search-string that is found
     *
     * @param InString: String of characters to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string that is found
     */
    FORCEINLINE SizeType ReverseFindOneOf(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return ReverseFindOneOf(InString, StringUtils::Length(InString), Position);
    }

    /**
     * Returns the position of the last occurrence of a character in the search-string that is found. The search string is of string-type.
     *
     * @param InString: String of characters to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string that is found
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type ReverseFindOneOf(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return ReverseFindOneOf(InString, InString.Length(), Position);
    }

    /**
     * Returns the position of the last occurrence of a character in the search-string that is found
     *
     * @param InString: String of characters to search
     * @param InLength: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string that is found
     */
    FORCEINLINE SizeType ReverseFindOneOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Assert((Position < Len) || (Position == 0));

        if ((InLength == 0) || StringUtils::IsTerminator(*InString) || (Len == 0))
        {
            return Len;
        }

        SizeType Length = (Position == 0) ? Len : NMath::Min(Position, Len);
        SizeType SubstringLength = StringUtils::Length(InString);

        const CharType* Start = CStr();
        const CharType* End = Start + Length;
        while (End != Start)
        {
            End--;

            const CharType* SubstringStart = InString;
            const CharType* SubstringEnd = SubstringStart + SubstringLength;
            while (SubstringStart != SubstringEnd)
            {
                if (*End == *(SubstringStart++))
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(End - Start));
                }
            }
        }

        return NPos;
    }

    /**
     * Returns the position of the first occurrence of a character not a part of the search-string
     * 
     * @param InString: String of characters that should be a part of the string
     * @param Position: Position to start the search at
     * @return: Return position the first character not a part of the search-string
     */
    FORCEINLINE SizeType FindOneNotOf(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return FindOneNotOf(InString, StringUtils::Length(InString), Position);
    }

    /**
     * Returns the position of the first character not a part of the search-string. The string is of a string-type.
     *
     * @param InString: String of characters that should be a part of the string
     * @param Position: Position to start the search at
     * @return: Return position the first character not a part of the search-string
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type FindOneNotOf(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return FindOneNotOf(InString.CStr(), InString.Length(), Position);
    }

    /**
     * Returns the position of the first occurrence of a character not a part of the search-string
     *
     * @param InString: String of characters that should be a part of the string
     * @param InLength: Length of the search-string
     * @param Position: Position to start the search at
     * @return: Return position the first character not a part of the search-string
     */
    FORCEINLINE SizeType FindOneNotOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Assert((Position < Len) || (Position == 0));

        if ((InLength == 0) || StringUtils::IsTerminator(*InString) || (Len == 0))
        {
            return 0;
        }

        SizeType Pos = static_cast<SizeType>(StringUtils::RangeLength(CStr() + Position, InString));
        SizeType Ret = Pos + Position;
        if (Ret >= Len)
        {
            return NPos;
        }
        else
        {
            return Ret;
        }
    }

    /**
     * Returns the position of the last occurrence of a character not a part of the search-string
     *
     * @param InString: String of characters that should be a part of the string
     * @param Position: Position to start the search at
     * @return: Return position the first character not a part of the search-string
     */
    FORCEINLINE SizeType ReverseFindOneNotOf(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return ReverseFindOneNotOf(InString, StringUtils::Length(InString), Position);
    }

    /**
     * Returns the position of the last occurrence of a character not a part of the search-string. The string is of a string-type.
     *
     * @param InString: String of characters that should be a part of the string
     * @param Position: Position to start the search at
     * @return: Return position the first character not a part of the search-string
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type ReverseFindOneNotOf(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return ReverseFindOneNotOf(InString, InString.Length(), Position);
    }

    /**
     * Returns the position of the last occurrence of a character not a part of the search-string
     *
     * @param InString: String of characters that should be a part of the string
     * @param InLength: Length of the search-string
     * @param Position: Position to start the search at
     * @return: Return position the first character not a part of the search-string
     */
    FORCEINLINE SizeType ReverseFindOneNotOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Assert((Position < Len) || (Position == 0));

        if ((InLength == 0) || StringUtils::IsTerminator(*InString) || (Len == 0))
        {
            return Len;
        }

        SizeType Length = (Position == 0) ? Len : NMath::Min(Position, Len);
        SizeType SubstringLength = StringUtils::Length(InString);

        const CharType* Start = CStr();
        const CharType* End = Start + Length;
        while (End != Start)
        {
            End--;

            const CharType* SubstringStart = InString;
            const CharType* SubstringEnd = SubstringStart + SubstringLength;
            while (SubstringStart != SubstringEnd)
            {
                if (*End == *(SubstringStart++))
                {
                    break;
                }
                else if (StringUtils::IsTerminator(*SubstringStart))
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(End - Start));
                }
            }
        }

        return NPos;
    }

    /**
     * Check if the search-string exists within the string 
     * 
     * @param InString: String to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    FORCEINLINE bool Contains(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return (Find(InString, Position) != NPos);
    }

    /**
     * Check if the search-string exists within the string. The string is of a string-type.
     *
     * @param InString: String to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type Contains(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return (Find(InString, Position) != NPos);
    }

    /**
     * Check if the search-string exists within the string
     *
     * @param InString: String to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    FORCEINLINE bool Contains(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        return (Find(InString, InLength, Position) != NPos);
    }

    /**
     * Check if the character exists within the string
     *
     * @param InString: String to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    FORCEINLINE bool Contains(CharType Char, SizeType Position = 0) const noexcept
    {
        return (Find(Char, Position) != NPos);
    }

    /**
     * Check if the one of the characters exists within the string
     *
     * @param InString: String of characters to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    FORCEINLINE bool ContainsOneOf(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return (FindOneOf(InString, Position) != NPos);
    }

    /**
     * Check if the one of the characters exists within the string. The string is of a string-type.
     *
     * @param InString: String of characters to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type ContainsOneOf(const StringType& InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        return (FindOneOf<StringType>(InString, InLength, Position) != NPos);
    }

    /**
     * Check if the one of the characters exists within the string
     *
     * @param InString: String of characters to search for
     * @param InLength: Length of the string with characters to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    FORCEINLINE bool ContainsOneOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        return (FindOneOf(InString, InLength, Position) != NPos);
    }

    /**
     * Removes count characters from position and forward
     * 
     * @param Position: Position to start remove at
     * @param NumCharacters: Number of characters to remove
     */
    FORCEINLINE void Remove(SizeType Position, SizeType NumCharacters) noexcept
    {
        Assert((Position < Len) && (Position + NumCharacters < Len));

        CharType* Dst = Data() + Position;
        CharType* Src = Dst + NumCharacters;

        SizeType Num = Len - (Position + NumCharacters);
        CMemory::Memmove(Dst, Src, Num * sizeof(CharType));
    }

    /**
     * Insert a string at a specified position
     * 
     * @param InString: String to insert
     * @param Position: Position to start the insertion at
     */
    FORCEINLINE void Insert(const CharType* InString, SizeType Position) noexcept
    {
        Insert(InString, StringUtils::Length(InString), Position);
    }

    /**
     * Insert a string at a specified position. The string is of string-type.
     *
     * @param InString: String to insert
     * @param Position: Position to start the insertion at
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value>::Type Insert(const StringType& InString, SizeType Position) noexcept
    {
        Insert(InString.CStr(), InString.Length(), Position);
    }

    /**
     * Insert a string at a specified position
     *
     * @param InString: String to insert
     * @param InLength: Length of the string to insert
     * @param Position: Position to start the insertion at
     */
    FORCEINLINE void Insert(const CharType* InString, SizeType InLength, SizeType Position) noexcept
    {
        Assert((Position < Len) && (Len + InLength < CharCount));

        CharType* Src = Data() + Position;
        CharType* Dst = Src + InLength;

        const uint64 MoveSize = Len - Position;
        StringUtils::Move(Dst, Src, MoveSize);

        StringUtils::Copy(Src, InString, InLength);

        Len += InLength;
        Characters[Len] = StringUtils::Null;
    }

    /**
     * Insert a character at specified position 
     * 
     * @param Char: Character to insert
     * @param Position: Position to insert character at
     */
    FORCEINLINE void Insert(CharType Char, SizeType Position) noexcept
    {
        Assert((Position < Len) && (Len + 1 < CharCount));

        // Make room for string
        CharType* Src = Data() + Position;
        CharType* Dst = Src + 1;

        const uint64 MoveSize = Len - Position;
        StringUtils::Move(Dst, Src, MoveSize);

        // Copy String
        *Src = Char;

        Characters[++Len] = StringUtils::Null;
    }

    /**
     * Replace a part of the string
     * 
     * @param InString: String to replace
     * @param Position: Position to start the replacing at
     */
    FORCEINLINE void Replace(const CharType* InString, SizeType Position) noexcept
    {
        Replace(InString, StringUtils::Length(InString), Position);
    }

    /**
     * Replace a part of the string. String is of string-type.
     *
     * @param InString: String to replace
     * @param Position: Position to start the replacing at
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value>::Type Replace(const StringType& InString, SizeType Position) noexcept
    {
        Replace(InString.CStr(), InString.Length(), Position);
    }

    /**
     * Replace a part of the string
     *
     * @param InString: String to replace
     * @param InLenght: Length of the string to replace
     * @param Position: Position to start the replacing at
     */
    FORCEINLINE void Replace(const CharType* InString, SizeType InLength, SizeType Position) noexcept
    {
        Assert((Position < Len) && (Position + InLength < Len));
        StringUtils::Copy(Data() + Position, InString, InLength);
    }

    /**
     * Replace a character in the string
     * 
     * @param Char: Character to replace
     * @param Position: Position of the character to replace 
     */
    FORCEINLINE void Replace(CharType Char, SizeType Position) noexcept
    {
        Assert((Position < Len));

        CharType* Dest = Data() + Position;
        *Dest = Char;
    }

    /**
     * Insert a new character at the end
     * 
     * @param Char: Character to insert at the end
     */
    FORCEINLINE void Push(CharType Char) noexcept
    {
        Append(Char);
    }

    /**
     * Remove the character at the end
     */
    FORCEINLINE void Pop() noexcept
    {
        Characters[--Len] = StringUtils::Null;
    }

    /**
     * Swap this string with another
     * 
     * @param Other: String to swap with
     */
    FORCEINLINE void Swap(TStaticString& Other)
    {
        TStaticString TempString(Move(*this));
        MoveFrom(Move(Other));
        Other.MoveFrom(Move(TempString));
    }

    /**
     * Create a sub-string of this string
     * 
     * @param Position: Position to start the sub-string at
     * @param NumCharacters: Number of characters in the sub-string
     * @return: Returns a sub-string
     */
    FORCEINLINE TStaticString SubString(SizeType Position, SizeType NumCharacters) const noexcept
    {
        Assert((Position < Len) && (Position + NumCharacters < Len));
        return TStaticString(Characters + Position, NumCharacters);
    }

    /**
     * Create a sub-string view of this string
     *
     * @param Position: Position to start the sub-string at
     * @param NumCharacters: Number of characters in the sub-string
     * @return: Returns a sub-string view
     */
    FORCEINLINE TStringView<CharType> SubStringView(SizeType Position, SizeType NumCharacters) const noexcept
    {
        Assert((Position < Len) && (Position + NumCharacters < Len));
        return TStringView<CharType>(Characters + Position, NumCharacters);
    }

    /**
     * Retrieve a element at a certain index of the string
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE CharType& At(SizeType Index) noexcept
    {
        Assert(Index < Length());
        return Data()[Index];
    }

    /**
     * Retrieve a element at a certain index of the string
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE const CharType& At(SizeType Index) const noexcept
    {
        Assert(Index < Length());
        return Data()[Index];
    }

    /**
     * Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE CharType* Data() noexcept
    {
        return Characters;
    }

    /**
     * Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE const CharType* Data() const noexcept
    {
        return Characters;
    }

    /**
     * Retrieve the string as a c-array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE const CharType* CStr() const noexcept
    {
        return Characters;
    }

    /**
     * Returns the size of the container
     *
     * @return: The current size of the container
     */
    FORCEINLINE SizeType Size() const noexcept
    {
        return Len;
    }

    /**
     * Retrieve the last index that can be used to retrieve an element from the array
     *
     * @return: Returns a the index to the last element of the array
     */
    FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        return (Len > 0) ? (Len - 1) : 0;
    }

    /**
     * Returns the length of the string
     *
     * @return: The current length of the string
     */
    FORCEINLINE SizeType Length() const noexcept
    {
        return Len;
    }

    /**
     * Returns the size of the container in bytes
     *
     * @return: The current size of the container in bytes
     */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Len * sizeof(CharType);
    }

    /**
     * Check if the container contains any elements
     *
     * @return: Returns true if the array is empty or false if it contains elements
     */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (Len == 0);
    }

    /**
     * Retrieve the first element of the array
     *
     * @return: Returns a reference to the first element of the array
     */
    FORCEINLINE ElementType& FirstElement() noexcept
    {
        Assert(!IsEmpty());
        return Data()[0];
    }

    /**
     * Retrieve the first element of the array
     *
     * @return: Returns a reference to the first element of the array
     */
    FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        Assert(!IsEmpty());
        return Data()[0];
    }

    /**
     * Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the array
     */
    FORCEINLINE ElementType& LastElement() noexcept
    {
        Assert(!IsEmpty());
        return Data()[LastElementIndex()];
    }

    /**
     * Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the array
     */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        Assert(!IsEmpty());
        return Data()[LastElementIndex()];
    }

    /**
     * Appends a character to this string
     * 
     * @param RHS: Character to append
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TStaticString& operator+=(CharType RHS) noexcept
    {
        Append(RHS);
        return *this;
    }

    /**
     * Appends a string to this string
     *
     * @param RHS: String to append
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TStaticString& operator+=(const CharType* RHS) noexcept
    {
        Append(RHS);
        return *this;
    }

    /**
     * Appends a string of a string-type to this string
     *
     * @param RHS: String to append
     * @return: Returns a reference to this instance
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, typename TAddReference<TStaticString>::LValue>::Type operator+=(const StringType& RHS) noexcept
    {
        Append<StringType>(RHS);
        return *this;
    }

    /**
     * Bracket-operator to retrieve an element at a certain index
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE CharType& operator[](SizeType Index) noexcept
    {
        return At(Index);
    }

    /**
     * Bracket-operator to retrieve an element at a certain index
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE const CharType& operator[](SizeType Index) const noexcept
    {
        return At(Index);
    }

    /**
     * Assignment operator that takes a raw string
     * 
     * @param RHS: String to copy
     * @return: Return a reference to this instance
     */
    FORCEINLINE TStaticString& operator=(const CharType* RHS) noexcept
    {
        TStaticString(RHS).Swap(*this);
        return *this;
    }

    /**
     * Copy-assignment operator 
     * 
     * @param RHS: String to copy
     * @return: Return a reference to this instance
     */
    FORCEINLINE TStaticString& operator=(const TStaticString& RHS) noexcept
    {
        TStaticString(RHS).Swap(*this);
        return *this;
    }

    /**
     * Assignment operator that takes another string-type
     *
     * @param RHS: String to copy
     * @return: Return a reference to this instance
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, typename TAddReference<TStaticString>::LValue>::Type operator=(const StringType& RHS) noexcept
    {
        TStaticString<StringType, CharCount>(RHS).Swap(*this);
        return *this;
    }

    /**
     * Move-assignment operator
     *
     * @param RHS: String to move
     * @return: Return a reference to this instance
     */
    FORCEINLINE TStaticString& operator=(TStaticString&& RHS) noexcept
    {
        TStaticString(Move(RHS)).Swap(*this);
        return *this;
    }

public:

    /**
     * Retrieve the capacity of the container
     *
     * @return: The capacity of the container
     */
    constexpr SizeType Capacity() const noexcept
    {
        return CharCount;
    }

    /**
     * Retrieve the capacity of the container in bytes
     *
     * @return: The capacity of the container in bytes
     */
    constexpr SizeType CapacityInBytes() const noexcept
    {
        return CharCount * sizeof(CharType);
    }

public:

    /**
     * Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType(*this, Size());
    }

    /**
     * Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType(*this, Size());
    }

    /**
     * Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, Size());
    }

    /**
     * Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType(*this, 0);
    }

    /**
     * Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, Size());
    }

    /**
     * Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, 0);
    }

public:

    /**
     * STL start iterator, same as TArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE IteratorType begin() noexcept
    {
        return StartIterator();
    }

    /**
     * STL end iterator, same as TArray::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    FORCEINLINE IteratorType end() noexcept
    {
        return EndIterator();
    }

    /**
     * STL start iterator, same as TArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    /**
     * STL end iterator, same as TArray::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    FORCEINLINE ConstIteratorType end() const noexcept
    {
        return EndIterator();
    }

private:

    FORCEINLINE void CopyFrom(const CharType* InString, SizeType InLength) noexcept
    {
        Assert(InLength < Capacity());

        StringUtils::Copy(Characters, InString, InLength);
        Len = InLength;
        Characters[Len] = StringUtils::Null;
    }

    FORCEINLINE void MoveFrom(TStaticString&& Other) noexcept
    {
        Len = Other.Len;
        Other.Len = 0;

        CMemory::Memexchange(Characters, Other.Characters, SizeInBytes());
        Characters[Len] = StringUtils::Null;
    }

    CharType Characters[CharCount];
    SizeType Len;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Predefined types

template<uint32 CharCount>
using CStaticString = TStaticString<char, CharCount>;

template<uint32 CharCount>
using WStaticString = TStaticString<wchar_t, CharCount>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TStaticString operators

template<typename CharType, int32 CharCount>
inline TStaticString<CharType, CharCount> operator+(const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    TStaticString<CharType, CharCount> NewString = LHS;
    NewString.Append(RHS);
    return NewString;
}

template<typename CharType, int32 CharCount>
inline TStaticString<CharType, CharCount> operator+(const CharType* LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    TStaticString<CharType, CharCount> NewString = LHS;
    NewString.Append(RHS);
    return NewString;
}

template<typename CharType, int32 CharCount>
inline TStaticString<CharType, CharCount> operator+(const TStaticString<CharType, CharCount>& LHS, const CharType* RHS) noexcept
{
    TStaticString<CharType, CharCount> NewString = LHS;
    NewString.Append(RHS);
    return NewString;
}

template<typename CharType, int32 CharCount>
inline TStaticString<CharType, CharCount> operator+(CharType LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    TStaticString<CharType, CharCount> NewString;
    NewString.Append(LHS);
    NewString.Append(RHS);
    return NewString;
}

template<typename CharType, int32 CharCount>
inline TStaticString<CharType, CharCount> operator+(const TStaticString<CharType, CharCount>& LHS, CharType RHS) noexcept
{
    TStaticString<CharType, CharCount> NewString = LHS;
    NewString.Append(RHS);
    return NewString;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TStaticString comparison operators

template<typename CharType, int32 CharCount>
inline bool operator==(const TStaticString<CharType, CharCount>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) == 0);
}

template<typename CharType, int32 CharCount>
inline bool operator==(const CharType* LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    return (RHS.Compare(LHS) == 0);
}

template<typename CharType, int32 CharCount>
inline bool operator==(const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    return (LHS.Compare(RHS) == 0);
}

template<typename CharType, int32 CharCount>
inline bool operator!=(const TStaticString<CharType, CharCount>& LHS, const CharType* RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CharType, int32 CharCount>
inline bool operator!=(const CharType* LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CharType, int32 CharCount>
inline bool operator!=(const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CharType, int32 CharCount>
inline bool operator<(const TStaticString<CharType, CharCount>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) < 0);
}

template<typename CharType, int32 CharCount>
inline bool operator<(const CharType* LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    return (RHS.Compare(LHS) < 0);
}

template<typename CharType, int32 CharCount>
inline bool operator<(const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    return (LHS.Compare(RHS) < 0);
}

template<typename CharType, int32 CharCount>
inline bool operator<=(const TStaticString<CharType, CharCount>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) <= 0);
}

template<typename CharType, int32 CharCount>
inline bool operator<=(const CharType* LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    return (RHS.Compare(LHS) <= 0);
}

template<typename CharType, int32 CharCount>
inline bool operator<=(const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    return (LHS.Compare(RHS) <= 0);
}

template<typename CharType, int32 CharCount>
inline bool operator>(const TStaticString<CharType, CharCount>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) > 0);
}

template<typename CharType, int32 CharCount>
inline bool operator>(const CharType* LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    return (RHS.Compare(LHS) > 0);
}

template<typename CharType, int32 CharCount>
inline bool operator>(const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    return (LHS.Compare(RHS) > 0);
}

template<typename CharType, int32 CharCount>
inline bool operator>=(const TStaticString<CharType, CharCount>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) >= 0);
}

template<typename CharType, int32 CharCount>
inline bool operator>=(const CharType* LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    return (RHS.Compare(LHS) >= 0);
}

template<typename CharType, int32 CharCount>
inline bool operator>=(const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS) noexcept
{
    return (LHS.Compare(RHS) >= 0);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Add TStaticString to be a string-type

template<typename CharType, int32 CharCount>
struct TIsTStringType<TStaticString<CharType, CharCount>>
{
    enum
    {
        Value = true
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Convert between char and wide

template<int32 CharCount>
inline WStaticString<CharCount> CharToWide(const CStaticString<CharCount>& CharString) noexcept
{
    WStaticString<CharCount> NewString;
    NewString.Resize(CharString.Length());

    mbstowcs(NewString.Data(), CharString.CStr(), CharString.Length());

    return NewString;
}

template<int32 CharCount>
inline CStaticString<CharCount> WideToChar(const WStaticString<CharCount>& WideString) noexcept
{
    CStaticString<CharCount> NewString;
    NewString.Resize(WideString.Length());

    wcstombs(NewString.Data(), WideString.CStr(), WideString.Length());

    return NewString;
}

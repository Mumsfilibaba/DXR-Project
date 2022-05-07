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
     * @brief: Create a static string from a formatted string
     * 
     * @param Format: Formatted string
     * @return: Returns the formatted string based on the format string
     */
    static NOINLINE TStaticString CreateFormated(const CharType* Format, ...) noexcept
    {
        TStaticString NewString;

        va_list ArgsList;
        va_start(ArgsList, Format);
        NewString.FormatArgs(Format, ArgsList);
        va_end(ArgsList);

        return NewString;
    }

    /**
     * @brief: Create a static string from a formatted string and an argument-list
     *
     * @param Format: Formatted string
     * @param ArgsList: Argument-list to be formatted based on the format-string
     * @return: Returns the formatted string based on the format string
     */
    static FORCEINLINE void CreateFormatedArgs(const CharType* Format, va_list ArgsList) noexcept
    {
        TStaticString NewString;
        NewString.FormatArgs(Format, ArgsList);
        return NewString;
    }

    /**
     * @brief: Default constructor
     */
    FORCEINLINE TStaticString() noexcept
        : Characters()
        , Len(0)
    {
        Characters[Len] = StringUtils::Null;
    }

    /**
     * @brief: Create a fixed string from a raw string. If the string is longer than the fixed length
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
     * @brief: Create a fixed string from a specified length raw array. If the string is longer than the 
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
     * @brief: Create a static string from another string-type. If the string is longer than the
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
     * @brief: Copy Constructor
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
     * @brief: Move Constructor
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
     * @brief: Clears the string
     */
    FORCEINLINE void Clear() noexcept
    {
        Len = 0;
        Characters[Len] = StringUtils::Null;
    }

    /**
     * @brief: Appends a character to this string 
     * 
     * @param Char: Character to append
     */
    FORCEINLINE void Append(CharType Char) noexcept
    {
        Check(Len + 1 < Capacity());

        Characters[Len] = Char;
        Characters[++Len] = StringUtils::Null;
    }

    /**
     * @brief: Appends a raw-string to this string
     *
     * @param InString: String to append
     */
    FORCEINLINE void Append(const CharType* InString) noexcept
    {
        Append(InString, StringUtils::Length(InString));
    }

    /**
     * @brief: Appends a string of another string-type to this string
     *
     * @param InString: String to append
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value>::Type Append(const StringType& InString) noexcept
    {
        Append(InString.CStr(), InString.Length());
    }

    /**
     * @brief: Appends a raw-string to this string with a fixed length
     *
     * @param InString: String to append
     * @param InLength: Length of the string
     */
    FORCEINLINE void Append(const CharType* InString, SizeType InLength) noexcept
    {
        Check(InString != nullptr);
        Check(Len + InLength < Capacity());

        StringUtils::Copy(Characters + Len, InString, InLength);

        Len = Len + InLength;
        Characters[Len] = StringUtils::Null;
    }

    /**
     * @brief: Resize the string
     * 
     * @param NewLength: New length of the string 
     */
    FORCEINLINE void Resize(SizeType NewLength) noexcept
    {
        Resize(NewLength, CharType());
    }

    /**
     * @brief: Resize the string and fill the new elements with a specified character
     *
     * @param NewLength: New length of the string
     * @param FillElement: Element to fill the string with
     */
    FORCEINLINE void Resize(SizeType NewLength, CharType FillElement) noexcept
    {
        Check(NewLength < CharCount);

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
     * @brief: Copy this string into buffer 
     * 
     * @param Buffer: Buffer to fill
     * @param BufferSize: Size of the buffer to fill
     * @param Position: Offset to start copy from
     */
    FORCEINLINE void Copy(CharType* Buffer, SizeType BufferSize, SizeType Position = 0) const noexcept
    {
        Check(Buffer != nullptr);
        Check((Position < Len) || (Position == 0));

        SizeType CopySize = NMath::Min(BufferSize, Len - Position);
        StringUtils::Copy(Buffer, Characters + Position, CopySize);
    }

    /**
     * @brief: Replace the string with a formatted string (similar to snprintf)
     * 
     * @param Format: Formatted string to replace the string with
     */
    NOINLINE void Format(const CharType* Format, ...) noexcept
    {
        va_list ArgList;
        va_start(ArgList, Format);
        FormatArgs(Format, ArgList);
        va_end(ArgList);
    }

    /**
     * @brief: Replace the string with a formatted string (similar to snprintf)
     *
     * @param Format: Formatted string to replace the string with
     * @param ArgList: Argument list filled with arguments for the formatted string
     */
    FORCEINLINE void FormatArgs(const CharType* Format, va_list ArgList) noexcept
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
     * @brief: Appends a formatted string to the string
     * 
     * @param Format: Formatted string to append
     */
    NOINLINE void AppendFormat(const CharType* Format, ...) noexcept
    {
        va_list ArgList;
        va_start(ArgList, Format);
        AppendFormatArgs(Format, ArgList);
        va_end(ArgList);
    }

    /**
     * @brief: Appends a formatted string to the string
     *
     * @param Format: Formatted string to append
     * @param ArgList: Argument-list for the formatted string
     */
    FORCEINLINE void AppendFormatArgs(const CharType* Format, va_list ArgList) noexcept
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
     * @brief: Convert this string to a lower-case string
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
     * @brief: Convert this string to a lower-case string and returns a copy
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
     * @brief: Convert this string to a upper-case string
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
     * @brief: Convert this string to a upper-case string and returns a copy
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
     * @brief: Removes whitespace from the beginning and end of the string 
     */
    FORCEINLINE void TrimInline() noexcept
    {
        TrimStartInline();
        TrimEndInline();
    }

    /**
     * @brief: Removes whitespace from the beginning and end of the string and returns a copy
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
     * @brief: Removes whitespace from the beginning of the string 
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
     * @brief: Removes whitespace from the beginning of the string and returns a copy
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
     * @brief: Removes whitespace from the end of the string
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
     * @brief: Removes whitespace from the end of the string and returns a copy
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
     * @brief: Reverses the order of all the characters in the string
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
     * @brief: Reverses the order of all the characters in the string and returns a copy
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
     * @brief: Compares this string to another string-type
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
     * @brief: Compares this string with a raw-string
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    FORCEINLINE int32 Compare(const CharType* InString) const noexcept
    {
        return StringUtils::Compare(Characters, InString);
    }

    /**
     * @brief: Compares this string with a raw-string of a fixed length
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
     * @brief: Compares this string to another string-type without taking casing into account.
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
     * @brief: Compares this string with a raw-string without taking casing into account.
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    FORCEINLINE int32 CompareNoCase(const CharType* InString) const noexcept
    {
        return CompareNoCase(InString, StringUtils::Length(InString));
    }

    /**
     * @brief: Compares this string with a raw-string without taking casing into account.
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
     * @brief: Find the position of the first occurrence of the start of the search-string 
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
     * @brief: Find the position of the first occurrence of the start of the search-string
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
     * @brief: Find the position of the first occurrence of the start of the search-string
     *
     * @param InString: String to search
     * @param InString: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    FORCEINLINE SizeType Find(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Check((Position < Len) || (Position == 0));

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
     * @brief: Returns the position of the first occurrence of char
     * 
     * @param Char: Character to search for
     * @param Position: Position to start search at
     * @return: Returns the position of the first occurrence of the char
     */
    FORCEINLINE SizeType Find(CharType Char, SizeType Position = 0) const noexcept
    {
        Check((Position < Len) || (Position == 0));

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
     * @brief: Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
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
     * @brief: Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
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
     * @brief: Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     * Position is the end, instead of the start as with normal Find.
     * 
     * @param InString: String to search
     * @param InString: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    FORCEINLINE SizeType ReverseFind(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Check((Position < Len) || (Position == 0));

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
     * @brief: Returns the position of the first occurrence of char. Searches the string in reverse.
     *
     * @param Char: Character to search for
     * @param Position: Position to start search at
     * @return: Returns the position of the first occurrence of the char
     */
    FORCEINLINE SizeType ReverseFind(CharType Char, SizeType Position = 0) const noexcept
    {
        Check((Position < Len) || (Position == 0));

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
     * @brief: Returns the position of the first occurrence of a character in the search-string that is found
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
     * @brief: Returns the position of the first occurrence of a character in the search-string that is found. The search string is of string-type.
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
     * @brief: Returns the position of the first occurrence of a character in the search-string that is found
     *
     * @param InString: String of characters to search
     * @param InLength: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string that is found
     */
    FORCEINLINE SizeType FindOneOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Check((Position < Len) || (Position == 0));

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
     * @brief: Returns the position of the last occurrence of a character in the search-string that is found
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
     * @brief: Returns the position of the last occurrence of a character in the search-string that is found. The search string is of string-type.
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
     * @brief: Returns the position of the last occurrence of a character in the search-string that is found
     *
     * @param InString: String of characters to search
     * @param InLength: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string that is found
     */
    FORCEINLINE SizeType ReverseFindOneOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Check((Position < Len) || (Position == 0));

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
     * @brief: Returns the position of the first occurrence of a character not a part of the search-string
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
     * @brief: Returns the position of the first character not a part of the search-string. The string is of a string-type.
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
     * @brief: Returns the position of the first occurrence of a character not a part of the search-string
     *
     * @param InString: String of characters that should be a part of the string
     * @param InLength: Length of the search-string
     * @param Position: Position to start the search at
     * @return: Return position the first character not a part of the search-string
     */
    FORCEINLINE SizeType FindOneNotOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Check((Position < Len) || (Position == 0));

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
     * @brief: Returns the position of the last occurrence of a character not a part of the search-string
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
     * @brief: Returns the position of the last occurrence of a character not a part of the search-string. The string is of a string-type.
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
     * @brief: Returns the position of the last occurrence of a character not a part of the search-string
     *
     * @param InString: String of characters that should be a part of the string
     * @param InLength: Length of the search-string
     * @param Position: Position to start the search at
     * @return: Return position the first character not a part of the search-string
     */
    FORCEINLINE SizeType ReverseFindOneNotOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Check((Position < Len) || (Position == 0));

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
     * @brief: Check if the search-string exists within the string 
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
     * @brief: Check if the search-string exists within the string. The string is of a string-type.
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
     * @brief: Check if the search-string exists within the string
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
     * @brief: Check if the character exists within the string
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
     * @brief: Check if the one of the characters exists within the string
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
     * @brief: Check if the one of the characters exists within the string. The string is of a string-type.
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
     * @brief: Check if the one of the characters exists within the string
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
     * @brief: Removes count characters from position and forward
     * 
     * @param Position: Position to start remove at
     * @param NumCharacters: Number of characters to remove
     */
    FORCEINLINE void Remove(SizeType Position, SizeType NumCharacters) noexcept
    {
        Check((Position < Len) && (Position + NumCharacters < Len));

        CharType* Dst = Data() + Position;
        CharType* Src = Dst + NumCharacters;

        SizeType Num = Len - (Position + NumCharacters);
        CMemory::Memmove(Dst, Src, Num * sizeof(CharType));
    }

    /**
     * @brief: Insert a string at a specified position
     * 
     * @param InString: String to insert
     * @param Position: Position to start the insertion at
     */
    FORCEINLINE void Insert(const CharType* InString, SizeType Position) noexcept
    {
        Insert(InString, StringUtils::Length(InString), Position);
    }

    /**
     * @brief: Insert a string at a specified position. The string is of string-type.
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
     * @brief: Insert a string at a specified position
     *
     * @param InString: String to insert
     * @param InLength: Length of the string to insert
     * @param Position: Position to start the insertion at
     */
    FORCEINLINE void Insert(const CharType* InString, SizeType InLength, SizeType Position) noexcept
    {
        Check((Position < Len) && (Len + InLength < CharCount));

        CharType* Src = Data() + Position;
        CharType* Dst = Src + InLength;

        const uint64 MoveSize = Len - Position;
        StringUtils::Move(Dst, Src, MoveSize);

        StringUtils::Copy(Src, InString, InLength);

        Len += InLength;
        Characters[Len] = StringUtils::Null;
    }

    /**
     * @brief: Insert a character at specified position 
     * 
     * @param Char: Character to insert
     * @param Position: Position to insert character at
     */
    FORCEINLINE void Insert(CharType Char, SizeType Position) noexcept
    {
        Check((Position < Len) && (Len + 1 < CharCount));

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
     * @brief: Replace a part of the string
     * 
     * @param InString: String to replace
     * @param Position: Position to start the replacing at
     */
    FORCEINLINE void Replace(const CharType* InString, SizeType Position) noexcept
    {
        Replace(InString, StringUtils::Length(InString), Position);
    }

    /**
     * @brief: Replace a part of the string. String is of string-type.
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
     * @brief: Replace a part of the string
     *
     * @param InString: String to replace
     * @param InLenght: Length of the string to replace
     * @param Position: Position to start the replacing at
     */
    FORCEINLINE void Replace(const CharType* InString, SizeType InLength, SizeType Position) noexcept
    {
        Check((Position < Len) && (Position + InLength < Len));
        StringUtils::Copy(Data() + Position, InString, InLength);
    }

    /**
     * @brief: Replace a character in the string
     * 
     * @param Char: Character to replace
     * @param Position: Position of the character to replace 
     */
    FORCEINLINE void Replace(CharType Char, SizeType Position) noexcept
    {
        Check((Position < Len));

        CharType* Dest = Data() + Position;
        *Dest = Char;
    }

    /**
     * @brief: Insert a new character at the end
     * 
     * @param Char: Character to insert at the end
     */
    FORCEINLINE void Push(CharType Char) noexcept
    {
        Append(Char);
    }

    /**
     * @brief: Remove the character at the end
     */
    FORCEINLINE void Pop() noexcept
    {
        Characters[--Len] = StringUtils::Null;
    }

    /**
     * @brief: Swap this string with another
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
     * @brief: Create a sub-string of this string
     * 
     * @param Position: Position to start the sub-string at
     * @param NumCharacters: Number of characters in the sub-string
     * @return: Returns a sub-string
     */
    FORCEINLINE TStaticString SubString(SizeType Position, SizeType NumCharacters) const noexcept
    {
        Check((Position < Len) && (Position + NumCharacters < Len));
        return TStaticString(Characters + Position, NumCharacters);
    }

    /**
     * @brief: Create a sub-string view of this string
     *
     * @param Position: Position to start the sub-string at
     * @param NumCharacters: Number of characters in the sub-string
     * @return: Returns a sub-string view
     */
    FORCEINLINE TStringView<CharType> SubStringView(SizeType Position, SizeType NumCharacters) const noexcept
    {
        Check((Position < Len) && (Position + NumCharacters < Len));
        return TStringView<CharType>(Characters + Position, NumCharacters);
    }

    /**
     * @brief: Retrieve a element at a certain index of the string
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE CharType& At(SizeType Index) noexcept
    {
        Check(Index < Length());
        return Data()[Index];
    }

    /**
     * @brief: Retrieve a element at a certain index of the string
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE const CharType& At(SizeType Index) const noexcept
    {
        Check(Index < Length());
        return Data()[Index];
    }

    /**
     * @brief: Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE CharType* Data() noexcept
    {
        return Characters;
    }

    /**
     * @brief: Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE const CharType* Data() const noexcept
    {
        return Characters;
    }

    /**
     * @brief: Retrieve the string as a c-array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE const CharType* CStr() const noexcept
    {
        return Characters;
    }

    /**
     * @brief: Returns the size of the container
     *
     * @return: The current size of the container
     */
    FORCEINLINE SizeType Size() const noexcept
    {
        return Len;
    }

    /**
     * @brief: Retrieve the last index that can be used to retrieve an element from the array
     *
     * @return: Returns a the index to the last element of the array
     */
    FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        return (Len > 0) ? (Len - 1) : 0;
    }

    /**
     * @brief: Returns the length of the string
     *
     * @return: The current length of the string
     */
    FORCEINLINE SizeType Length() const noexcept
    {
        return Len;
    }

    /**
     * @brief: Returns the size of the container in bytes
     *
     * @return: The current size of the container in bytes
     */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Len * sizeof(CharType);
    }

    /**
     * @brief: Check if the container contains any elements
     *
     * @return: Returns true if the array is empty or false if it contains elements
     */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (Len == 0);
    }

    /**
     * @brief: Retrieve the first element of the array
     *
     * @return: Returns a reference to the first element of the array
     */
    FORCEINLINE ElementType& FirstElement() noexcept
    {
        Check(!IsEmpty());
        return Data()[0];
    }

    /**
     * @brief: Retrieve the first element of the array
     *
     * @return: Returns a reference to the first element of the array
     */
    FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        Check(!IsEmpty());
        return Data()[0];
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the array
     */
    FORCEINLINE ElementType& LastElement() noexcept
    {
        Check(!IsEmpty());
        return Data()[LastElementIndex()];
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the array
     */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        Check(!IsEmpty());
        return Data()[LastElementIndex()];
    }

public:

    /**
     * @brief: Appends a character to this string
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
     * @brief: Appends a string to this string
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
     * @brief: Appends a string of a string-type to this string
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
     * @brief: Bracket-operator to retrieve an element at a certain index
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE CharType& operator[](SizeType Index) noexcept
    {
        return At(Index);
    }

    /**
     * @brief: Bracket-operator to retrieve an element at a certain index
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE const CharType& operator[](SizeType Index) const noexcept
    {
        return At(Index);
    }

    /**
     * @brief: Assignment operator that takes a raw string
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
     * @brief: Copy-assignment operator 
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
     * @brief: Assignment operator that takes another string-type
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
     * @brief: Move-assignment operator
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

    friend TStaticString operator+(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        TStaticString NewString = LHS;
        NewString.Append(RHS);
        return NewString;
    }

    friend TStaticString operator+(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        TStaticString NewString = LHS;
        NewString.Append(RHS);
        return NewString;
    }

    friend TStaticString operator+(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        TStaticString NewString = LHS;
        NewString.Append(RHS);
        return NewString;
    }

    friend TStaticString operator+(CharType LHS, const TStaticString& RHS) noexcept
    {
        TStaticString NewString;
        NewString.Append(LHS);
        NewString.Append(RHS);
        return NewString;
    }

    friend TStaticString operator+(const TStaticString& LHS, CharType RHS) noexcept
    {
        TStaticString NewString = LHS;
        NewString.Append(RHS);
        return NewString;
    }

    friend bool operator==(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) == 0);
    }

    friend bool operator==(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        return (RHS.Compare(LHS) == 0);
    }

    friend bool operator==(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        return (LHS.Compare(RHS) == 0);
    }

    friend bool operator!=(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        return !(LHS == RHS);
    }

    friend bool operator!=(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        return !(LHS == RHS);
    }

    friend bool operator!=(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        return !(LHS == RHS);
    }

    friend bool operator<(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) < 0);
    }

    friend bool operator<(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        return (RHS.Compare(LHS) < 0);
    }

    friend bool operator<(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        return (LHS.Compare(RHS) < 0);
    }

    friend bool operator<=(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) <= 0);
    }

    friend bool operator<=(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        return (RHS.Compare(LHS) <= 0);
    }

    friend bool operator<=(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        return (LHS.Compare(RHS) <= 0);
    }

    friend bool operator>(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) > 0);
    }

    friend bool operator>(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        return (RHS.Compare(LHS) > 0);
    }

    friend bool operator>(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        return (LHS.Compare(RHS) > 0);
    }

    friend bool operator>=(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) >= 0);
    }

    friend bool operator>=(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        return (RHS.Compare(LHS) >= 0);
    }

    friend bool operator>=(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        return (LHS.Compare(RHS) >= 0);
    }

public:

    /**
     * @brief: Retrieve the capacity of the container
     *
     * @return: The capacity of the container
     */
    constexpr SizeType Capacity() const noexcept
    {
        return CharCount;
    }

    /**
     * @brief: Retrieve the capacity of the container in bytes
     *
     * @return: The capacity of the container in bytes
     */
    constexpr SizeType CapacityInBytes() const noexcept
    {
        return CharCount * sizeof(CharType);
    }

public:

    /**
     * @brief: Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType(*this, Size());
    }

    /**
     * @brief: Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType(*this, Size());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, Size());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, Size());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, 0);
    }

public:

    /**
     * @brief: STL start iterator, same as TArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE IteratorType begin() noexcept
    {
        return StartIterator();
    }

    /**
     * @brief: STL end iterator, same as TArray::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    FORCEINLINE IteratorType end() noexcept
    {
        return EndIterator();
    }

    /**
     * @brief: STL start iterator, same as TArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    /**
     * @brief: STL end iterator, same as TArray::EndIterator
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
        Check(InLength < Capacity());

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
using StaticString = TStaticString<char, CharCount>;

template<uint32 CharCount>
using WStaticString = TStaticString<wchar_t, CharCount>;

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
inline WStaticString<CharCount> CharToWide(const StaticString<CharCount>& CharString) noexcept
{
    WStaticString<CharCount> NewString;
    NewString.Resize(CharString.Length());

    mbstowcs(NewString.Data(), CharString.CStr(), CharString.Length());

    return NewString;
}

template<int32 CharCount>
inline StaticString<CharCount> WideToChar(const WStaticString<CharCount>& WideString) noexcept
{
    StaticString<CharCount> NewString;
    NewString.Resize(WideString.Length());

    wcstombs(NewString.Data(), WideString.CStr(), WideString.Length());

    return NewString;
}

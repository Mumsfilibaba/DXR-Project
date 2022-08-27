#pragma once
#include "StringView.h"

#include "Core/Templates/Identity.h"
#include "Core/Templates/AddReference.h"

#define STANDARD_STATIC_STRING_LENGTH (128)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TStaticString - Characters class with a fixed allocated number of characters

template<
    typename InCharType,
    int32 NUM_CHARS = STANDARD_STATIC_STRING_LENGTH>
class TStaticString
{
public:
    using CharType = InCharType;
    using SizeType = int32;

    static_assert(
        TIsSame<CharType, CHAR>::Value || TIsSame<CharType, WIDECHAR>::Value, 
        "TStaticString only supports 'CHAR' and 'WIDECHAR'");
    static_assert(
        NUM_CHARS > 0,
        "TStaticString does not support a zero element count");

    typedef TArrayIterator<TStaticString, CharType>                    IteratorType;
    typedef TArrayIterator<const TStaticString, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<TStaticString, CharType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TStaticString, const CharType> ReverseConstIteratorType;

    enum : SizeType { INVALID_INDEX = SizeType(-1) };

public:

    /**
     * @brief: Create a static string from a formatted string and an argument-list
     *
     * @param Format: Formatted string
     * @param Args: Arguments to be formatted based on the format-string
     * @return: Returns the formatted string based on the format string
     */
    template<typename... ArgTypes>
    NODISCARD
    static FORCEINLINE TStaticString CreateFormatted(const CharType* Format, ArgTypes&&... Args) noexcept
    {
        TStaticString NewString;
        NewString.Format(Format, Forward<ArgTypes>(Args)...);
        return NewString;
    }

public:

    /**
     * @brief: Default constructor
     */
    FORCEINLINE TStaticString() noexcept
        : Characters()
        , Length(0)
    {
        Characters[Length] = TChar<CharType>::Zero;
    }

    /**
     * @brief: Create a fixed string from a raw string. If the string is longer than the fixed length
     * the string will be shortened to fit.
     * 
     * @param InString: String to copy
     */
    FORCEINLINE TStaticString(const CharType* InString) noexcept
        : Characters()
        , Length(0)
    {
        if (InString)
        {
            CopyFrom(InString, TCString<CharType>::Strlen(InString));
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
        , Length(0)
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
    template<
        typename StringType, 
        typename = typename TEnableIf<TIsTStringType<StringType>::Value>::Type>
    FORCEINLINE explicit TStaticString(const StringType& InString) noexcept
        : Characters()
        , Length(0)
    {
        CopyFrom(InString.GetCString(), InString.GetLength());
    }

    /**
     * @brief: Copy Constructor
     * 
     * @param Other: Other string to copy from
     */
    FORCEINLINE TStaticString(const TStaticString& Other) noexcept
        : Characters()
        , Length(0)
    {
        CopyFrom(Other.GetCString(), Other.GetLength());
    }

    /**
     * @brief: Move Constructor
     *
     * @param Other: Other string to move from
     */
    FORCEINLINE TStaticString(TStaticString&& Other) noexcept
        : Characters()
        , Length(0)
    {
        MoveFrom(Forward<TStaticString>(Other));
    }

    /**
     * @brief: Clears the string
     */
    FORCEINLINE void Clear() noexcept
    {
        Length = 0;
        Characters[Length] = TChar<CharType>::Zero;
    }

    /**
     * @brief: Appends a character to this string 
     * 
     * @param Char: Character to append
     */
    FORCEINLINE void Append(CharType Char) noexcept
    {
        Check((Length + 1) < GetCapacity());
        Characters[Length]   = Char;
        Characters[++Length] = TChar<CharType>::Zero;
    }

    /**
     * @brief: Appends a raw-string to this string
     *
     * @param InString: String to append
     */
    FORCEINLINE void Append(const CharType* InString) noexcept
    {
        Append(InString, TCString<CharType>::Strlen(InString));
    }

    /**
     * @brief: Appends a string of another string-type to this string
     *
     * @param InString: String to append
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value>::Type Append(const StringType& InString) noexcept
    {
        Append(InString.GetCString(), InString.GetLength());
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
        Check((Length + InLength) < GetCapacity());

        const SizeType MinLength = NMath::Min<SizeType>(NUM_CHARS - Length, InLength);
        TCString<CharType>::Strncpy(Characters + Length, InString, MinLength);

        Length = Length + InLength;
        Characters[Length] = TChar<CharType>::Zero;
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
        Check(NewLength < NUM_CHARS);

        if (NewLength > Length)
        {
            TCString<CharType>::Strnset(Characters + Length, FillElement, NewLength - Length);
        }

        Length = NewLength;
        Characters[Length] = TChar<CharType>::Zero;
    }

    /**
     * @brief: Copy this string into buffer 
     * 
     * @param Buffer: Buffer to fill
     * @param BufferSize: Size of the buffer to fill
     * @param Position: Offset to start copy from
     */
    FORCEINLINE void CopyToBuffer(CharType* Buffer, SizeType BufferSize, SizeType Position = 0) const noexcept
    {
        Check(Buffer != nullptr);
        Check((Position < Length) || (Position == 0));

        const SizeType CopySize = NMath::Min(BufferSize, Length - Position);
        TCString<CharType>::Strncpy(Buffer, Characters + Position, CopySize);
    }

    /**
     * @brief: Replace the string with a formatted string (similar to snprintf)
     *
     * @param InFormat: Formatted string to replace the string with
     * @param Args: Arguments filled for the formatted string
     */
    template<typename... ArgTypes>
    FORCEINLINE void Format(const CharType* InFormat, ArgTypes&&... Args) noexcept
    {
        const SizeType WrittenChars = TCString<CharType>::Snprintf(Characters, NUM_CHARS - 1, InFormat, Forward<ArgTypes>(Args)...);
        if (WrittenChars < NUM_CHARS)
        {
            Length = WrittenChars;
        }
        else
        {
            Length = NUM_CHARS - 1;
        }

        Characters[Length] = TChar<CharType>::Zero;
    }

    /**
     * @brief: Appends a formatted string to the string
     *
     * @param Format: Formatted string to append
     * @param Args: Arguments for the formatted string
     */
    template<typename... ArgTypes>
    FORCEINLINE void AppendFormat(const CharType* InFormat, ArgTypes&&... Args) noexcept
    {
        const SizeType WrittenChars = TCString<CharType>::Snprintf(Characters + Length, NUM_CHARS, InFormat, Forward<ArgTypes>(Args)...);
        const SizeType NewLength    = Length + WrittenChars;
        if (NewLength < NUM_CHARS)
        {
            Length = NewLength;
        }
        else
        {
            Length = NUM_CHARS - 1;
        }

        Characters[Length] = TChar<CharType>::Zero;
    }

    /**
     * @brief: Convert this string to a lower-case string
     */ 
    FORCEINLINE void ToLowerInline() noexcept
    {
        CharType* RESTRICT Current = Characters;
        CharType* RESTRICT End     = Characters + Length;
        while (Current != End)
        {
            *Current = TChar<CharType>::ToLower(*Current);
            Current++;
        }
    }

    /**
     * @brief: Convert this string to a lower-case string and returns a copy
     * 
     * @return: Returns a copy of this string, with all characters in lower-case
     */
    NODISCARD FORCEINLINE TStaticString ToLower() const noexcept
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
        CharType* RESTRICT Current = Characters;
        CharType* RESTRICT End     = Characters + Length;
        while (Current != End)
        {
            *Current = TChar<CharType>::ToUpper(*Current);
            Current++;
        }
    }

    /**
     * @brief: Convert this string to a upper-case string and returns a copy
     *
     * @return: Returns a copy of this string, with all characters in upper-case
     */
    NODISCARD FORCEINLINE TStaticString ToUpper() const noexcept
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
    NODISCARD FORCEINLINE TStaticString Trim() noexcept
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
        CharType* RESTRICT Current = Characters;
        CharType* RESTRICT End     = Characters + Length;
        while (Current != End)
        {
            if (!TChar<CharType>::IsSpace(*Current) && !TChar<CharType>::IsZero(*Current))
            {
                break;
            }

            ++Current;
        }

        const SizeType Index = static_cast<SizeType>(static_cast<intptr>(Current - Characters));
        if (Index > 0)
        {
            Length -= Index;
            TCString<CharType>::Strnmove(Characters, Characters + Index, Length);
        }
    }

    /**
     * @brief: Removes whitespace from the beginning of the string and returns a copy
     * 
     * @return: Returns a copy of this string with all the whitespace removed from the beginning
     */
    NODISCARD FORCEINLINE TStaticString TrimStart() noexcept
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
        CharType* RESTRICT Current = Characters + Length;
        CharType* RESTRICT End     = Characters;
        while (Current != End)
        {
            --Current;

            if (TChar<CharType>::IsSpace(*Current) || TChar<CharType>::IsZero(*Current))
            {
                Length--;
            }
            else
            {
                break;
            }
        }

        Characters[Length] = TChar<CharType>::Zero;
    }

    /**
     * @brief: Removes whitespace from the end of the string and returns a copy
     *
     * @return: Returns a copy of this string with all the whitespace removed from the end
     */
    NODISCARD FORCEINLINE TStaticString TrimEnd() noexcept
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
        CharType* RESTRICT Start = Characters;
        CharType* RESTRICT End   = Start + Length;
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
    NODISCARD FORCEINLINE TStaticString Reverse() noexcept
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
    NODISCARD FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type Compare(const StringType& InString) const noexcept
    {
        return Compare(InString.GetCString());
    }

    /**
     * @brief: Compares this string with a raw-string
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE SizeType Compare(const CharType* InString) const noexcept
    {
        return TCString<CharType>::Strcmp(Characters, InString);
    }

    /**
     * @brief: Compares this string with a raw-string of a fixed length
     *
     * @param InString: String to compare with
     * @param InLength: Length of the string to compare
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE SizeType Compare(const CharType* InString, SizeType InLength) const noexcept
    {
        const SizeType MinLength = NMath::Min<SizeType>(Length, InLength);
        return TCString<CharType>::Strncmp(Characters, InString, MinLength);
    }

    /**
     * @brief: Compares this string to another string-type without taking casing into account.
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    template<typename StringType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type CompareNoCase(const StringType& InString) const noexcept
    {
        return CompareNoCase(InString.GetCString(), InString.GetLength());
    }

    /**
     * @brief: Compares this string with a raw-string without taking casing into account.
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE SizeType CompareNoCase(const CharType* InString) const noexcept
    {
        return TCString<CharType>::Stricmp(Characters, InString);
    }

    /**
     * @brief: Compares this string with a raw-string without taking casing into account.
     *
     * @param InString: String to compare with
     * @param InLength: Length of the string to compare
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE SizeType CompareNoCase(const CharType* InString, SizeType InLength) const noexcept
    {
        const SizeType MinLength = NMath::Min<SizeType>(Length, InLength);
        return TCString<CharType>::Strnicmp(Characters, InString, MinLength);
    }

    /**
     * @brief: Find the position of the first occurrence of the start of the search-string 
     * 
     * @param InString: String to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType Find(const CharType* InString, SizeType Position = 0) const noexcept
    {
        Check((Position < Length) || (Position == 0));

        if (InString == nullptr)
        {
            return INVALID_INDEX;
        }

        if (Length != 0)
        {
            const CharType* RESTRICT Start  = GetCString() + Position;
            const CharType* RESTRICT Result = TCString<CharType>::Strstr(Start, InString);
            if (!Result)
            {
                return INVALID_INDEX;
            }
            else
            {
                return static_cast<SizeType>(static_cast<intptr>(Result - Start));
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
    template<typename StringType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type 
        Find(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return Find(InString, InString.GetLength(), Position);
    }

    /**
     * @brief: Find the position of the first occurrence of the start of the search-string
     *
     * @param InString: String to search
     * @param InString: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType Find(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Check((Position < GetLength()) || (Position == 0));

        if ((InLength == 0) || (GetLength() == 0))
        {
            return 0;
        }

        if (InString == nullptr)
        {
            return INVALID_INDEX;
        }

        const CharType* RESTRICT Current   = GetCString() + Position;
        const CharType* RESTRICT End       = GetCString() + GetLength();
        const CharType* RESTRICT SearchEnd = InString + InLength;
        while (Current != End)
        {
            const CharType* RESTRICT SearchString = InString;
            const CharType* RESTRICT TmpCurrent   = Current;
            while (true)
            {
                if (TChar<CharType>::IsZero(*SearchString) || (SearchString == SearchEnd))
                {
                    return static_cast<SizeType>(static_cast<intptr>(Current - GetCString()));
                }
                else if (*TmpCurrent != *SearchString)
                {
                    break;
                }

                ++TmpCurrent;
                ++SearchString;
            }

            ++Current;
        }

        return INVALID_INDEX;
    }

    /**
     * @brief: Returns the position of the first occurrence of CHAR
     * 
     * @param Char: Character to search for
     * @param Position: Position to start search at
     * @return: Returns the position of the first occurrence of the CHAR
     */
    NODISCARD FORCEINLINE SizeType FindChar(CharType Char, SizeType Position = 0) const noexcept
    {
        Check((Position < Length) || (Position == 0));

        if (Length == 0)
        {
            return 0;
        }

        const CharType* RESTRICT Start   = GetCString() + Position;
        const CharType* RESTRICT Current = Start;
        const CharType* RESTRICT End     = GetCString() + Length;
        while (Current != End)
        {
            if (Char == *Current)
            {
                return static_cast<SizeType>(Current - Start);
            }

            ++Current;
        }

        return INVALID_INDEX;
    }

    /**
     * @brief: Returns the position of the first CHAR that passes the predicate
     *
     * @param Predicate: Predicate that specifies valid chars
     * @param Position: Position to start search at
     * @return: Returns the position of the first occurrence of the CHAR
     */
    template<typename PredicateType>
    NODISCARD FORCEINLINE SizeType FindCharWithPredicate(PredicateType&& Predicate, SizeType Position = 0) const noexcept
    {
        Check((Position < GetLength()) || (Position == 0));

        const SizeType ThisLength = GetLength();
        if (ThisLength == 0)
        {
            return 0;
        }

        const CharType* RESTRICT Start = GetCString() + Position;
        const CharType* RESTRICT Current = Start;
        const CharType* RESTRICT End = GetCString() + ThisLength;
        while (Current != End)
        {
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(Current - Start);
            }

            ++Current;
        }

        return INVALID_INDEX;
    }

    /**
     * @brief: Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     *
     * @param InString: String to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType FindLast(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return FindLast(InString, TCString<CharType>::Strlen(InString), Position);
    }

    /**
     * @brief: Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     *
     * @param InString: String to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type FindLast(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return FindLast(InString, InString.GetLength(), Position);
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
    NODISCARD FORCEINLINE SizeType FindLast(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Check((Position < Length) || (Position == 0));
        Check(InString != nullptr);

        if (Length == 0)
        {
            return 0;
        }

        if (InString != nullptr)
        {
            const CharType* RESTRICT SearchEnd = InString + InLength;
            const CharType* RESTRICT End       = GetCString();
            const CharType* RESTRICT Start     = (Position == 0) ? (End + Length) : (End + Position);
            const CharType* RESTRICT Current   = Start;
            while (Current != End)
            {
                Current--;

                const CharType* RESTRICT SearchString = InString;
                const CharType* RESTRICT TmpCurrent   = Current;
                while (true)
                {
                    if (TChar<CharType>::IsZero(*SearchString) || (SearchString == SearchEnd))
                    {
                        return static_cast<SizeType>(static_cast<intptr>(Current - End));
                    }
                    else if (*TmpCurrent != *SearchString)
                    {
                        break;
                    }

                    ++TmpCurrent;
                    ++SearchString;
                }
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief: Returns the position of the first occurrence of CHAR. Searches the string in reverse.
     *
     * @param Char: Character to search for
     * @param Position: Position to start search at
     * @return: Returns the position of the first occurrence of the CHAR
     */
    NODISCARD FORCEINLINE SizeType FindLastChar(CharType Char, SizeType Position = 0) const noexcept
    {
        Check((Position < Length) || (Position == 0));

        if (Length == 0)
        {
            return 0;
        }

        const CharType* RESTRICT End     = GetCString();
        const CharType* RESTRICT Start   = (Position == 0) ? (End + Length) : (End + Position);
        const CharType* RESTRICT Current = Start;
        while (Current != End)
        {
            --Current;
            if (Char == *Current)
            {
                return static_cast<SizeType>(static_cast<intptr>(Current - End));
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief: Returns the position of the first CHAR that passes the predicate
     *
     * @param Predicate: Predicate that specifies valid chars
     * @param Position: Position to start search at
     * @return: Returns the position of the first occurrence of the CHAR
     */
    template<typename PredicateType>
    NODISCARD FORCEINLINE SizeType FindLastCharWithPredicate(PredicateType&& Predicate, SizeType Position = 0) const noexcept
    {
        Check((Position < Length) || (Position == 0));

        if (Length == 0)
        {
            return 0;
        }

        const CharType* RESTRICT End     = GetCString();
        const CharType* RESTRICT Start   = (Position == 0) ? (End + Length) : (End + Position);
        const CharType* RESTRICT Current = Start;
        while (Current != End)
        {
            --Current;
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(static_cast<intptr>(Current - End));
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief: Check if the search-string exists within the string 
     * 
     * @param InString: String to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return (Find(InString, TCString<CharType>::Strlen(InString), Position) != INVALID_INDEX);
    }

    /**
     * @brief: Check if the search-string exists within the string. The string is of a string-type.
     *
     * @param InString: String to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    template<typename StringType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type Contains(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return (Find(InString, Position) != INVALID_INDEX);
    }

    /**
     * @brief: Check if the search-string exists within the string
     *
     * @param InString: String to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        return (Find(InString, InLength, Position) != INVALID_INDEX);
    }

    /**
     * @brief: Check if the character exists within the string
     *
     * @param InString: String to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(CharType Char, SizeType Position = 0) const noexcept
    {
        return (FindChar(Char, Position) != INVALID_INDEX);
    }

    /**
     * @brief: Check if string begins with a string
     *
     * @param InString: String to test for
     * @return: Returns true if the string begins with InString
     */
    NODISCARD FORCEINLINE bool StartsWith(const CharType* InString) const noexcept
    {
        const SizeType SuffixLength = TCString<CharType>::Strlen(InString);
        return (SuffixLength > 0) && !TCString<CharType>::Strncmp(GetCString(), InString, SuffixLength);
    }

    /**
     * @brief: Check if string begins with a string
     *
     * @param InString: String to test for
     * @return: Returns true if the string begins with InString
     */
    NODISCARD FORCEINLINE bool StartsWithNoCase(const CharType* InString) const noexcept
    {
        const SizeType SuffixLength = TCString<CharType>::Strlen(InString);
        return (SuffixLength > 0) && !TCString<CharType>::Strnicmp(GetCString(), InString, SuffixLength);
    }

    /**
     * @brief: Check if string end with a string
     *
     * @param InString: String to test for
     * @return: Returns true if the string ends with InString
     */
    NODISCARD FORCEINLINE bool EndsWith(const CharType* InString) const noexcept
    {
        if (!InString || TChar<CharType>::IsZero(*InString))
        {
            return false;
        }

        const SizeType SuffixLenght = TCString<CharType>::Strlen(InString);
        if (SuffixLenght > Length)
        {
            return false;
        }

        const CharType* Data = GetCString() + (Length - SuffixLenght);
        return (TCString<CharType>::Strcmp(Data, InString) == 0);
    }

    /**
     * @brief: Check if string end with a string without taking casing into account
     *
     * @param InString: String to test for
     * @return: Returns true if the string ends with InString
     */
    NODISCARD FORCEINLINE bool EndsWithNoCase(const CharType* InString) const noexcept
    {
        if (!InString || TChar<CharType>::IsZero(*InString))
        {
            return false;
        }

        const SizeType SuffixLenght = TCString<CharType>::Strlen(InString);
        if (SuffixLenght > Length)
        {
            return false;
        }

        const CharType* Data = GetCString() + (Length - SuffixLenght);
        return (TCString<CharType>::Stricmp(Data, InString) == 0);
    }

    /**
     * @brief: Removes count characters from position and forward
     * 
     * @param Position: Position to start remove at
     * @param NumCharacters: Number of characters to remove
     */
    FORCEINLINE void Remove(SizeType Position, SizeType NumCharacters) noexcept
    {
        Check((Position < Length) && (Position + NumCharacters < Length));

        CharType* Dst = Characters + Position;
        CharType* Src = Dst + NumCharacters;

        const SizeType Num = Length - (Position + NumCharacters);
        TCString<CharType>::Strmove(Dst, Src, Num);
    }

    /**
     * @brief: Insert a string at a specified position
     * 
     * @param InString: String to insert
     * @param Position: Position to start the insertion at
     */
    FORCEINLINE void Insert(const CharType* InString, SizeType Position) noexcept
    {
        Insert(InString, TCString<CharType>::Strlen(InString), Position);
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
        Insert(InString.GetCString(), InString.GetLength(), Position);
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
        Check((Position < Length) && (Length + InLength < NUM_CHARS));

        CharType* Src = Characters + Position;
        CharType* Dst = Src + InLength;

        const SizeType MoveSize = Length - Position;
        TCString<CharType>::Strnmove(Dst, Src, MoveSize);
        TCString<CharType>::Strncpy(Src, InString, InLength);

        Length += InLength;
        Characters[Length] = TChar<CharType>::Zero;
    }

    /**
     * @brief: Insert a character at specified position 
     * 
     * @param Char: Character to insert
     * @param Position: Position to insert character at
     */
    FORCEINLINE void Insert(CharType Char, SizeType Position) noexcept
    {
        Check((Position < Length) && (Length + 1 < NUM_CHARS));

        // Make room for string
        CharType* Src = Characters + Position;
        CharType* Dst = Src + 1;

        const SizeType MoveSize = Length - Position;
        TCString<CharType>::Strmove(Dst, Src, MoveSize);

        // Copy String
        *Src = Char;
        Characters[++Length] = TChar<CharType>::Zero;
    }

    /**
     * @brief: Replace a part of the string
     * 
     * @param InString: String to replace
     * @param Position: Position to start the replacing at
     */
    FORCEINLINE void Replace(const CharType* InString, SizeType Position) noexcept
    {
        Replace(InString, TCString<CharType>::Strlen(InString), Position);
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
        Replace(InString.GetCString(), InString.GetLength(), Position);
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
        Check((Position < Length) && (Position + InLength < Length));
        
        if (InString)
        {
            TCString<CharType>::Strncpy(Characters + Position, InString, InLength);
        }
    }

    /**
     * @brief: Replace a character in the string
     * 
     * @param Char: Character to replace
     * @param Position: Position of the character to replace 
     */
    FORCEINLINE void Replace(CharType Char, SizeType Position) noexcept
    {
        Check((Position < Length));
        *(Characters + Position) = Char;
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
        if (Length > 0)
        {
            --Length;
            Characters[Length] = TChar<CharType>::Zero;
        }
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
    NODISCARD FORCEINLINE TStaticString SubString(SizeType Position, SizeType NumCharacters) const noexcept
    {
        Check((Position < Length) && (Position + NumCharacters < Length));
        return TStaticString(Characters + Position, NumCharacters);
    }

    /**
     * @brief: Create a sub-string view of this string
     *
     * @param Position: Position to start the sub-string at
     * @param NumCharacters: Number of characters in the sub-string
     * @return: Returns a sub-string view
     */
    NODISCARD FORCEINLINE TStringView<CharType> SubStringView(SizeType Position, SizeType NumCharacters) const noexcept
    {
        Check((Position < Length) && (Position + NumCharacters < Length));
        return TStringView<CharType>(Characters + Position, NumCharacters);
    }

    /**
     * @brief: Retrieve a element at a certain index of the string
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    NODISCARD FORCEINLINE CharType& GetElementAt(SizeType Index) noexcept
    {
        Check(Index < GetLength());
        return Characters[Index];
    }

    /**
     * @brief: Retrieve a element at a certain index of the string
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    NODISCARD FORCEINLINE const CharType& GetElementAt(SizeType Index) const noexcept
    {
        Check(Index < GetLength());
        return Characters[Index];
    }

    /**
     * @brief: Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE CharType* GetData() noexcept
    {
        return Characters;
    }

    /**
     * @brief: Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const CharType* GetData() const noexcept
    {
        return Characters;
    }

    /**
     * @brief: Retrieve the string as a c-array
     *
     * @return: Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const CharType* GetCString() const noexcept
    {
        return Characters;
    }

    /**
     * @brief: Returns the size of the container
     *
     * @return: The current size of the container
     */
    NODISCARD FORCEINLINE SizeType GetSize() const noexcept
    {
        return Length;
    }

    /**
     * @brief: Retrieve the last index that can be used to retrieve an element from the array
     *
     * @return: Returns a the index to the last element of the array
     */
    NODISCARD FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        return (Length > 0) ? (Length - 1) : 0;
    }

    /**
     * @brief: Returns the length of the string
     *
     * @return: The current length of the string
     */
    NODISCARD FORCEINLINE SizeType GetLength() const noexcept
    {
        return Length;
    }

    /**
     * @brief: Returns the size of the container in bytes
     *
     * @return: The current size of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Length * sizeof(CharType);
    }

    /**
     * @brief: Check if the container contains any elements
     *
     * @return: Returns true if the array is empty or false if it contains elements
     */
    NODISCARD FORCEINLINE bool IsEmpty() const noexcept
    {
        return (Length == 0);
    }

    /**
     * @brief: Retrieve the first element of the array
     *
     * @return: Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE CharType& FirstElement() noexcept
    {
        return Characters[0];
    }

    /**
     * @brief: Retrieve the first element of the array
     *
     * @return: Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE const CharType& FirstElement() const noexcept
    {
        return Characters[0];
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE CharType& LastElement() noexcept
    {
        return Characters[LastElementIndex()];
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE const CharType& LastElement() const noexcept
    {
        return Characters[LastElementIndex()];
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
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, typename TAddReference<TStaticString>::LValue>::Type 
        operator+=(const StringType& RHS) noexcept
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
    NODISCARD FORCEINLINE CharType& operator[](SizeType Index) noexcept
    {
        return GetElementAt(Index);
    }

    /**
     * @brief: Bracket-operator to retrieve an element at a certain index
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    NODISCARD FORCEINLINE const CharType& operator[](SizeType Index) const noexcept
    {
        return GetElementAt(Index);
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
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, typename TAddReference<TStaticString>::LValue>::Type
        operator=(const StringType& RHS) noexcept
    {
        TStaticString<StringType, NUM_CHARS>(RHS).Swap(*this);
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
    NODISCARD
    friend FORCEINLINE TStaticString operator+(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        TStaticString NewString = LHS;
        NewString.Append(RHS);
        return NewString;
    }

    NODISCARD
    friend FORCEINLINE TStaticString operator+(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        TStaticString NewString = LHS;
        NewString.Append(RHS);
        return NewString;
    }

    NODISCARD
    friend FORCEINLINE TStaticString operator+(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        TStaticString NewString = LHS;
        NewString.Append(RHS);
        return NewString;
    }

    NODISCARD
    friend FORCEINLINE TStaticString operator+(CharType LHS, const TStaticString& RHS) noexcept
    {
        TStaticString NewString;
        NewString.Append(LHS);
        NewString.Append(RHS);
        return NewString;
    }

    NODISCARD
    friend FORCEINLINE TStaticString operator+(const TStaticString& LHS, CharType RHS) noexcept
    {
        TStaticString NewString = LHS;
        NewString.Append(RHS);
        return NewString;
    }

    NODISCARD
    friend FORCEINLINE bool operator==(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) == 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator==(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        return (RHS.Compare(LHS) == 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator==(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        return (LHS.Compare(RHS) == 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator!=(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        return !(LHS == RHS);
    }

    NODISCARD
    friend FORCEINLINE bool operator!=(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        return !(LHS == RHS);
    }

    NODISCARD
    friend FORCEINLINE bool operator!=(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        return !(LHS == RHS);
    }

    NODISCARD
    friend FORCEINLINE bool operator<(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) < 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator<(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        return (RHS.Compare(LHS) < 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator<(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        return (LHS.Compare(RHS) < 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator<=(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) <= 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator<=(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        return (RHS.Compare(LHS) <= 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator<=(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        return (LHS.Compare(RHS) <= 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator>(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) > 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator>(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        return (RHS.Compare(LHS) > 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator>(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        return (LHS.Compare(RHS) > 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator>=(const TStaticString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) >= 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator>=(const CharType* LHS, const TStaticString& RHS) noexcept
    {
        return (RHS.Compare(LHS) >= 0);
    }

    NODISCARD
    friend FORCEINLINE bool operator>=(const TStaticString& LHS, const TStaticString& RHS) noexcept
    {
        return (LHS.Compare(RHS) >= 0);
    }

public:

    /**
     * @brief: Retrieve the capacity of the container
     *
     * @return: The capacity of the container
     */
    NODISCARD CONSTEXPR SizeType GetCapacity() const noexcept
    {
        return NUM_CHARS;
    }

    /**
     * @brief: Retrieve the capacity of the container in bytes
     *
     * @return: The capacity of the container in bytes
     */
    NODISCARD CONSTEXPR SizeType CapacityInBytes() const noexcept
    {
        return NUM_CHARS * sizeof(CharType);
    }

public:

    /**
     * @brief: Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    NODISCARD FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    NODISCARD FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType(*this, GetSize());
    }

    /**
     * @brief: Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    NODISCARD FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType(*this, GetSize());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, GetSize());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType(*this, 0);
    }

    /**
     * @brief: Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, GetSize());
    }

    /**
     * @brief: Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, 0);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // STL Iterators

    NODISCARD FORCEINLINE IteratorType      begin()       noexcept { return StartIterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept { return StartIterator(); }

    NODISCARD FORCEINLINE IteratorType      end()       noexcept { return EndIterator(); }
    NODISCARD FORCEINLINE ConstIteratorType end() const noexcept { return EndIterator(); }

private:
    FORCEINLINE void CopyFrom(const CharType* InString, SizeType InLength) noexcept
    {
        Check(InLength < GetCapacity());

        TCString<CharType>::Strncpy(Characters, InString, InLength);
        Length = InLength;
        Characters[Length] = TChar<CharType>::Zero;
    }

    FORCEINLINE void MoveFrom(TStaticString&& Other) noexcept
    {
        Length = Other.Length;
        Other.Length = 0;

        FMemory::Memexchange(Characters, Other.Characters, SizeInBytes());
        Characters[Length] = TChar<CharType>::Zero;
    }

    CharType Characters[NUM_CHARS];
    SizeType Length;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Predefined types

template<TSIZE NUM_CHARS = STANDARD_STATIC_STRING_LENGTH>
using FStaticString = TStaticString<CHAR, NUM_CHARS>;

template<TSIZE NUM_CHARS = STANDARD_STATIC_STRING_LENGTH>
using FStaticStringWide = TStaticString<WIDECHAR, NUM_CHARS>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Add TStaticString to be a string-type

template<
    typename CharType,
    int32 NUM_CHARS>
struct TIsTStringType<TStaticString<CharType, NUM_CHARS>>
{
    enum { Value = true };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Convert between CHAR and wide

template<int32 NUM_CHARS>
NODISCARD
inline FStaticStringWide<NUM_CHARS> CharToWide(const FStaticString<NUM_CHARS>& CharString) noexcept
{
    FStaticStringWide<NUM_CHARS> NewString;
    NewString.Resize(CharString.GetLength());
    mbstowcs(NewString.GetData(), CharString.GetCString(), CharString.GetLength());
    return NewString;
}

template<int32 NUM_CHARS>
NODISCARD
inline FStaticString<NUM_CHARS> WideToChar(const FStaticStringWide<NUM_CHARS>& WideString) noexcept
{
    FStaticString<NUM_CHARS> NewString;
    NewString.Resize(WideString.GetLength());
    wcstombs(NewString.GetData(), WideString.GetCString(), WideString.GetLength());
    return NewString;
}

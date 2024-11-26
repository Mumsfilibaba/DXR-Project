#pragma once
#include "Core/Containers/StringView.h"
#include "Core/Templates/TypeTraits.h"

#define STANDARD_STATIC_STRING_LENGTH (128)

template<typename InCharType, int32 NUM_CHARS = STANDARD_STATIC_STRING_LENGTH>
class TStaticString
{
    typedef TChar<InCharType>    FCharType;
    typedef TCString<InCharType> FCStringType;

public:
    typedef int32      SIZETYPE;
    typedef InCharType CHARTYPE;

    static_assert(TIsSame<CHARTYPE, CHAR>::Value || TIsSame<CHARTYPE, WIDECHAR>::Value, "TStaticString only supports 'CHAR' and 'WIDECHAR'");
    static_assert(NUM_CHARS > 0, "TStaticString does not support a zero element count");

    typedef TArrayIterator<TStaticString, CHARTYPE>                    IteratorType;
    typedef TArrayIterator<const TStaticString, const CHARTYPE>        ConstIteratorType;
    typedef TReverseArrayIterator<TStaticString, CHARTYPE>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TStaticString, const CHARTYPE> ReverseConstIteratorType;

    inline static constexpr SIZETYPE INVALID_INDEX = SIZETYPE(~0);

public:

    /**
     * @brief        - Create a static string from a formatted string and an argument list
     * @param Format - Formatted string
     * @param Args   - Arguments to be formatted based on the format string
     * @return       - Returns the formatted string based on the format string
     */
    template<typename... ArgTypes>
    NODISCARD static FORCEINLINE TStaticString CreateFormatted(const CHARTYPE* Format, ArgTypes&&... Args)
    {
        TStaticString NewString;
        NewString.Format(Format, Forward<ArgTypes>(Args)...);
        return NewString;
    }

public:

    /**
     * @brief - Default constructor
     */
    FORCEINLINE TStaticString()
    {
        CharData[StringLength] = 0;
    }

    /**
     * @brief          - Create a fixed string from a raw string. If the string is longer than the fixed length, the string will be shortened to fit.
     * @param InString - String to copy
     */
    FORCEINLINE TStaticString(const CHARTYPE* InString)
    {
        InitializeByCopy(InString, FCStringType::Strlen(InString));
    }

    /**
     * @brief          - Create a fixed string from a specified length raw array. If the string is longer than the fixed length, the string will be shortened to fit.
     * @param InString - String to copy
     * @param InLength - Length of the string to copy
     */
    FORCEINLINE explicit TStaticString(const CHARTYPE* InString, uint32 InLength)
    {
        InitializeByCopy(InString, InLength);
    }

    /**
     * @brief          - Create a static string from another string type. If the string is longer than the fixed length, it will be shortened to fit.
     * @param InString - String to copy from
     */
    template<typename StringType>
    FORCEINLINE explicit TStaticString(const StringType& InString) requires(TIsTStringType<StringType>::Value)
    {
        InitializeByCopy(InString.Data(), InString.Length());
    }

    /**
     * @brief       - Copy Constructor
     * @param Other - Other string to copy from
     */
    FORCEINLINE TStaticString(const TStaticString& Other)
    {
        InitializeByCopy(Other.Data(), Other.Length());
    }

    /**
     * @brief       - Move Constructor
     * @param Other - Other string to move from
     */
    FORCEINLINE TStaticString(TStaticString&& Other)
    {
        MoveFrom(Forward<TStaticString>(Other));
    }

    /**
     * @brief - Resets the string
     */
    FORCEINLINE void Reset()
    {
        StringLength = 0;
        CharData[StringLength] = 0;
    }

    /**
     * @brief      - Appends a character to this string
     * @param Char - Character to append
     */
    FORCEINLINE void Append(CHARTYPE Char)
    {
        if (StringLength + 1 < Capacity())
        {
            CharData[StringLength]   = Char;
            CharData[++StringLength] = 0;
        }
    }

    /**
     * @brief          - Appends a C-string to this string
     * @param InString - String to append
     */
    FORCEINLINE void Append(const CHARTYPE* InString)
    {
        Append(InString, FCStringType::Strlen(InString));
    }

    /**
     * @brief          - Appends a string of another string type to this string
     * @param InString - String to append
     */
    template<typename StringType>
    FORCEINLINE void Append(const StringType& InString) requires(TIsTStringType<StringType>::Value)
    {
        Append(InString.Data(), InString.Length());
    }

    /**
     * @brief          - Appends a C-string to this string with a fixed length
     * @param InString - String to append
     * @param InLength - Length of the string
     */
    FORCEINLINE void Append(const CHARTYPE* InString, SIZETYPE InLength)
    {
        if (InString && InLength > 0)
        {
            const SIZETYPE MinLength = FMath::Min<SIZETYPE>((NUM_CHARS - 1) - StringLength, InLength);
            FCStringType::Strncpy(CharData + StringLength, InString, MinLength);
            StringLength = StringLength + MinLength;
            CharData[StringLength] = 0;
        }
    }

    /**
     * @brief           - Resize the string
     * @param NewLength - New length of the string
     */
    FORCEINLINE void Resize(SIZETYPE NewLength)
    {
        const SIZETYPE MinLength = FMath::Min<SIZETYPE>(NUM_CHARS - 1, NewLength);
        StringLength = MinLength;
        CharData[StringLength] = 0;
    }

    /**
     * @brief            - Copy this string into a buffer
     * @param Buffer     - Buffer to fill
     * @param BufferSize - Size of the buffer to fill
     * @param Position   - Offset to start copy from
     */
    FORCEINLINE void CopyToBuffer(CHARTYPE* Buffer, SIZETYPE BufferSize, SIZETYPE Position = INVALID_INDEX) const
    {
        CHECK(Position < StringLength || Position == 0);
        if (Buffer && BufferSize > 0)
        {
            const SIZETYPE CopySize = FMath::Min(BufferSize, StringLength - Position);
            FCStringType::Strncpy(Buffer, CharData + Position, CopySize);
        }
    }

    /**
     * @brief          - Replace the string with a formatted string (similar to snprintf)
     * @param InFormat - Formatted string to replace the string with
     * @param Args     - Arguments filled for the formatted string
     */
    template<typename... ArgTypes>
    FORCEINLINE void Format(const CHARTYPE* InFormat, ArgTypes&&... Args)
    {
        const SIZETYPE NumWritten = FCStringType::Snprintf(CharData, NUM_CHARS - 1, InFormat, Forward<ArgTypes>(Args)...);
        if (NumWritten < NUM_CHARS)
        {
            StringLength = NumWritten;
        }
        else
        {
            StringLength = NUM_CHARS - 1;
        }

        CharData[StringLength] = 0;
    }

    /**
     * @brief          - Appends a formatted string to the string
     * @param InFormat - Formatted string to append
     * @param Args     - Arguments for the formatted string
     */
    template<typename... ArgTypes>
    FORCEINLINE void AppendFormat(const CHARTYPE* InFormat, ArgTypes&&... Args)
    {
        const SIZETYPE NumWritten = FCStringType::Snprintf(CharData + StringLength, NUM_CHARS, InFormat, Forward<ArgTypes>(Args)...);
        const SIZETYPE NewLength = StringLength + NumWritten;
        if (NewLength < NUM_CHARS)
        {
            StringLength = NewLength;
        }
        else
        {
            StringLength = NUM_CHARS - 1;
        }

        CharData[StringLength] = 0;
    }

    /**
     * @brief - Converts this string to lowercase
     */
    FORCEINLINE void ToLowerInline()
    {
        for (CHARTYPE* RESTRICT Start = CharData, *RESTRICT End = Start + StringLength; Start != End; ++Start)
        {
            *Start = TChar<CHARTYPE>::ToLower(*Start);
        }
    }

    /**
     * @brief  - Converts this string to lowercase and returns a copy
     * @return - Returns a copy of this string, with all characters in lowercase
     */
    NODISCARD FORCEINLINE TStaticString ToLower() const
    {
        TStaticString NewString(*this);
        NewString.ToLowerInline();
        return NewString;
    }

    /**
     * @brief - Converts this string to uppercase
     */
    FORCEINLINE void ToUpperInline()
    {
        for (CHARTYPE* RESTRICT Start = CharData, *RESTRICT End = Start + StringLength; Start != End; ++Start)
        {
            *Start = TChar<CHARTYPE>::ToUpper(*Start);
        }
    }

    /**
     * @brief  - Converts this string to uppercase and returns a copy
     * @return - Returns a copy of this string, with all characters in uppercase
     */
    NODISCARD FORCEINLINE TStaticString ToUpper() const
    {
        TStaticString NewString(*this);
        NewString.ToUpperInline();
        return NewString;
    }

    /**
     * @brief - Removes whitespace from the beginning and end of the string
     */
    FORCEINLINE void TrimInline()
    {
        TrimStartInline();
        TrimEndInline();
    }

    /**
     * @brief  - Removes whitespace from the beginning and end of the string and returns a copy
     * @return - Returns a copy of this string with the whitespace removed from the beginning and end
     */
    NODISCARD FORCEINLINE TStaticString Trim()
    {
        TStaticString NewString(*this);
        NewString.TrimInline();
        return NewString;
    }

    /**
     * @brief - Removes whitespace from the beginning of the string
     */
    FORCEINLINE void TrimStartInline()
    {
        SIZETYPE Index = 0;
        while (FCharType::IsWhitespace(CharData[Index]))
        {
            Index++;
        }

        if (Index)
        {
            StringLength = FMath::Clamp<SIZETYPE>(StringLength - Index, 0, NUM_CHARS - 1);
            FCStringType::Strnmove(CharData, CharData + Index, StringLength);
        }
    }

    /**
     * @brief  - Removes whitespace from the beginning of the string and returns a copy
     * @return - Returns a copy of this string with all the whitespace removed from the beginning
     */
    NODISCARD FORCEINLINE TStaticString TrimStart()
    {
        TStaticString NewString(*this);
        NewString.TrimStartInline();
        return NewString;
    }

    /**
     * @brief - Removes whitespace from the end of the string
     */
    FORCEINLINE void TrimEndInline()
    {
        while (StringLength > 0 && FCharType::IsWhitespace(CharData[StringLength - 1]))
        {
            StringLength--;
        }

        CharData[StringLength] = 0;
    }

    /**
     * @brief  - Removes whitespace from the end of the string and returns a copy
     * @return - Returns a copy of this string with all the whitespace removed from the end
     */
    NODISCARD FORCEINLINE TStaticString TrimEnd()
    {
        TStaticString NewString(*this);
        NewString.TrimEndInline();
        return NewString;
    }

    /**
     * @brief - Reverses the order of all the characters in the string
     */
    FORCEINLINE void ReverseInline()
    {
        const SIZETYPE HalfLength = StringLength / 2;
        for (SIZETYPE Index = 0; Index < HalfLength; ++Index)
        {
            const SIZETYPE ReverseIndex = StringLength - Index - 1;
            const CHARTYPE TempChar = CharData[Index];
            CharData[Index] = CharData[ReverseIndex];
            CharData[ReverseIndex] = TempChar;
        }
    }

    /**
     * @brief  - Reverses the order of all the characters in the string and returns a copy
     * @return - Returns a string with all the characters reversed
     */
    NODISCARD FORCEINLINE TStaticString Reverse()
    {
        TStaticString NewString(*this);
        NewString.ReverseInline();
        return NewString;
    }

    /**
     * @brief          - Compares this string to another string type
     * @param InString - String to compare with
     * @param CaseType - Enum that decides whether the comparison should be case-sensitive or not
     * @return         - Returns the position of the characters that are not equal. The sign determines the difference of the character.
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SIZETYPE Compare(const StringType& InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const requires(TIsTStringType<StringType>::Value)
    {
        return Compare(InString.Data(), InString.Length(), CaseType);
    }

    /**
     * @brief          - Compares this string with a C-string
     * @param InString - String to compare with
     * @param CaseType - Enum that decides whether the comparison should be case-sensitive or not
     * @return         - Returns the position of the characters that are not equal. The sign determines the difference of the character.
     */
    NODISCARD FORCEINLINE SIZETYPE Compare(const CHARTYPE* InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const
    {
        if (CaseType == EStringCaseType::NoCase)
        {
            return static_cast<SIZETYPE>(FCStringType::Stricmp(CharData, InString));
        }
        else
        {
            return static_cast<SIZETYPE>(FCStringType::Strcmp(CharData, InString));
        }
    }

    /**
     * @brief          - Compares this string with a C-string of a fixed length
     * @param InString - String to compare with
     * @param InLength - Length of the string to compare
     * @param CaseType - Enum that decides whether the comparison should be case-sensitive or not
     * @return         - Returns the position of the characters that are not equal. The sign determines the difference of the character.
     */
    NODISCARD FORCEINLINE SIZETYPE Compare(const CHARTYPE* InString, SIZETYPE InLength, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const
    {
        const SIZETYPE MinLength = FMath::Min(Length(), InLength);
        if (CaseType == EStringCaseType::NoCase)
        {
            return static_cast<SIZETYPE>(FCStringType::Strnicmp(CharData, InString, MinLength));
        }
        else
        {
            return static_cast<SIZETYPE>(FCStringType::Strncmp(CharData, InString, MinLength));
        }
    }

    /**
     * @brief          - Compares this string to another string type
     * @param InString - String to compare with
     * @param CaseType - Enum that decides whether the comparison should be case-sensitive or not
     * @return         - Returns true if the strings are equal
     */
    template<typename StringType>
    NODISCARD FORCEINLINE bool Equals(const StringType& InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const requires(TIsTStringType<StringType>::Value)
    {
        return Equals(InString.Data(), InString.Length(), CaseType);
    }

    /**
     * @brief          - Compares this string with a C-string
     * @param InString - String to compare with
     * @param CaseType - Enum that decides whether the comparison should be case-sensitive or not
     * @return         - Returns true if the strings are equal
     */
    NODISCARD FORCEINLINE bool Equals(const CHARTYPE* InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const
    {
        return Equals(InString, FCStringType::Strlen(InString), CaseType);
    }

    /**
     * @brief          - Compares this string with a C-string of a fixed length
     * @param InString - String to compare with
     * @param InLength - Length of the string to compare
     * @param CaseType - Enum that decides whether the comparison should be case-sensitive or not
     * @return         - Returns true if the strings are equal
     */
    NODISCARD FORCEINLINE bool Equals(const CHARTYPE* InString, SIZETYPE InLength, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const
    {
        if (StringLength != InLength)
        {
            return false;
        }

        if (CaseType == EStringCaseType::CaseSensitive)
        {
            return FCStringType::Strncmp(CharData, InString, StringLength) == 0;
        }
        else if (CaseType == EStringCaseType::NoCase)
        {
            return FCStringType::Strnicmp(CharData, InString, StringLength) == 0;
        }
        else
        {
            return false;
        }
    }

    /**
     * @brief          - Find the position of the first occurrence of the start of the search string
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search string
     */
    NODISCARD FORCEINLINE SIZETYPE Find(const CHARTYPE* InString, SIZETYPE Position = INVALID_INDEX) const
    {
        if (!StringLength)
        {
            return 0;
        }

        if (InString == nullptr)
        {
            return INVALID_INDEX;
        }

        SIZETYPE Index = 0;
        if (Position != INVALID_INDEX && StringLength > 0)
        {
            Index += FMath::Clamp(Position, 0, StringLength - 1);
        }

        const CHARTYPE* RESTRICT Result = FCStringType::Strstr(CharData + Index, InString);
        if (!Result)
        {
            return INVALID_INDEX;
        }
        else
        {
            return static_cast<SIZETYPE>(static_cast<ptrint>(Result - CharData));
        }
    }

    /**
     * @brief          - Find the position of the first occurrence of the start of the search string
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SIZETYPE Find(const StringType& InString, SIZETYPE Position = INVALID_INDEX) const requires(TIsTStringType<StringType>::Value)
    {
        return Find(InString.Data(), Position);
    }

    /**
     * @brief          - Returns the position of the first occurrence of a character
     * @param Char     - Character to search for
     * @param Position - Position to start search at
     * @return         - Returns the position of the first occurrence of the character
     */
    NODISCARD FORCEINLINE SIZETYPE FindChar(CHARTYPE Char, SIZETYPE Position = INVALID_INDEX) const
    {
        if (!StringLength)
        {
            return 0;
        }

        const CHARTYPE* RESTRICT Current = CharData;
        if (Position != INVALID_INDEX && StringLength > 0)
        {
            Current += FMath::Clamp(Position, 0, StringLength - 1);
        }

        for (const CHARTYPE* RESTRICT End = CharData + StringLength; Current != End; ++Current)
        {
            if (*Current == Char)
            {
                return static_cast<SIZETYPE>(static_cast<ptrint>(Current - CharData));
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief           - Returns the position of the first character that passes the predicate
     * @param Predicate - Predicate that specifies valid chars
     * @param Position  - Position to start search at
     * @return          - Returns the position of the first occurrence of the character
     */
    template<typename PredicateType>
    NODISCARD FORCEINLINE SIZETYPE FindCharWithPredicate(PredicateType&& Predicate, SIZETYPE Position = INVALID_INDEX) const
    {
        if (!StringLength)
        {
            return 0;
        }

        const CHARTYPE* RESTRICT Current = CharData;
        if (Position != INVALID_INDEX && StringLength > 0)
        {
            Current += FMath::Clamp(Position, 0, StringLength - 1);
        }

        for (const CHARTYPE* RESTRICT End = CharData + StringLength; Current != End; ++Current)
        {
            if (Predicate(*Current))
            {
                return static_cast<SIZETYPE>(static_cast<ptrint>(Current - CharData));
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief          - Find the position of the first occurrence of the start of the search string. Searches the string in reverse.
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search string
     */
    NODISCARD FORCEINLINE SIZETYPE FindLast(const CHARTYPE* InString, SIZETYPE Position = INVALID_INDEX) const
    {
        if (!StringLength)
        {
            return 0;
        }

        if (InString == nullptr)
        {
            return INVALID_INDEX;
        }

        if (Position == INVALID_INDEX || Position > StringLength)
        {
            Position = StringLength;
        }

        const SIZETYPE SearchLength = FCStringType::Strlen(InString);
        for (SIZETYPE Index = Position - SearchLength; Index >= 0; Index--)
        {
            SIZETYPE SearchIndex = 0;
            for (; SearchIndex < SearchLength; ++SearchIndex)
            {
                if (CharData[Index + SearchIndex] != InString[SearchIndex])
                {
                    break;
                }
            }

            if (SearchIndex == SearchLength)
            {
                return Index;
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief          - Find the position of the first occurrence of the start of the search string. Searches the string in reverse.
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SIZETYPE FindLast(const StringType& InString, SIZETYPE Position = INVALID_INDEX) const requires(TIsTStringType<StringType>::Value)
    {
        return FindLast(InString.Data(), Position);
    }

    /**
     * @brief          - Returns the position of the first occurrence of a character. Searches the string in reverse.
     * @param Char     - Character to search for
     * @param Position - Position to start search at
     * @return         - Returns the position of the first occurrence of the character
     */
    NODISCARD FORCEINLINE SIZETYPE FindLastChar(CHARTYPE Char, SIZETYPE Position = INVALID_INDEX) const
    {
        if (!StringLength)
        {
            return 0;
        }

        const CHARTYPE* RESTRICT Current = CharData;
        if (Position == INVALID_INDEX || Position > StringLength)
        {
            Current += StringLength;
        }

        for (const CHARTYPE* RESTRICT End = CharData; Current != End;)
        {
            --Current;
            if (Char == *Current)
            {
                return static_cast<SIZETYPE>(static_cast<ptrint>(Current - End));
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief           - Returns the position of the first character that passes the predicate
     * @param Predicate - Predicate that specifies valid chars
     * @param Position  - Position to start search at
     * @return          - Returns the position of the first occurrence of the character
     */
    template<typename PredicateType>
    NODISCARD FORCEINLINE SIZETYPE FindLastCharWithPredicate(PredicateType&& Predicate, SIZETYPE Position = INVALID_INDEX) const
    {
        if (!StringLength)
        {
            return 0;
        }

        const CHARTYPE* RESTRICT Current = CharData;
        if (Position == INVALID_INDEX || Position > StringLength)
        {
            Current += StringLength;
        }

        for (const CHARTYPE* RESTRICT End = CharData; Current != End;)
        {
            --Current;
            if (Predicate(*Current))
            {
                return static_cast<SIZETYPE>(static_cast<ptrint>(Current - End));
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief          - Check if the search string exists within the string
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(const CHARTYPE* InString, SIZETYPE Position = INVALID_INDEX) const
    {
        return Find(InString, Position) != INVALID_INDEX;
    }

    /**
     * @brief          - Check if the search string exists within the string. The string is of a string type.
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    template<typename StringType>
    NODISCARD FORCEINLINE bool Contains(const StringType& InString, SIZETYPE Position = INVALID_INDEX) const requires(TIsTStringType<StringType>::Value)
    {
        return Find(InString.Data(), Position) != INVALID_INDEX;
    }

    /**
     * @brief          - Check if the character exists within the string
     * @param Char     - Character to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the character is found
     */
    NODISCARD FORCEINLINE bool Contains(CHARTYPE Char, SIZETYPE Position = INVALID_INDEX) const
    {
        return FindChar(Char, Position) != INVALID_INDEX;
    }

    /**
     * @brief            - Check if the string begins with a specified string
     * @param InString   - String to test for
     * @param SearchType - Enum that decides whether the search should be case-sensitive or not
     * @return           - Returns true if the string begins with InString
     */
    NODISCARD FORCEINLINE bool StartsWith(const CHARTYPE* InString, EStringCaseType SearchType = EStringCaseType::CaseSensitive) const
    {
        if (!InString)
        {
            return false;
        }

        const SIZETYPE SuffixLength = FCStringType::Strlen(InString);
        if (SuffixLength > 0)
        {
            if (SearchType == EStringCaseType::CaseSensitive)
            {
                return FCStringType::Strncmp(CharData, InString, SuffixLength) == 0;
            }
            else if (SearchType == EStringCaseType::NoCase)
            {
                return FCStringType::Strnicmp(CharData, InString, SuffixLength) == 0;
            }
        }

        return false;
    }

    /**
     * @brief            - Check if the string ends with a specified string
     * @param InString   - String to test for
     * @param SearchType - Enum that decides whether the search should be case-sensitive or not
     * @return           - Returns true if the string ends with InString
     */
    NODISCARD FORCEINLINE bool EndsWith(const CHARTYPE* InString, EStringCaseType SearchType = EStringCaseType::CaseSensitive) const
    {
        if (!InString)
        {
            return false;
        }

        const SIZETYPE SuffixLength = FCStringType::Strlen(InString);
        if (SuffixLength > 0 && StringLength >= SuffixLength)
        {
            const CHARTYPE* StringData = CharData + (StringLength - SuffixLength);
            if (SearchType == EStringCaseType::CaseSensitive)
            {
                return FCStringType::Strncmp(StringData, InString, SuffixLength) == 0;
            }
            else if (SearchType == EStringCaseType::NoCase)
            {
                return FCStringType::Strnicmp(StringData, InString, SuffixLength) == 0;
            }
        }

        return false;
    }
    
    /**
     * @brief               - Removes count characters from position and forward
     * @param Position      - Position to start removing at
     * @param NumCharacters - Number of characters to remove
     */
    FORCEINLINE void Remove(SIZETYPE Position, SIZETYPE Count)
    {
        CHECK(Position < StringLength && (Position + Count) < StringLength);

        CHARTYPE* Dst = CharData + Position;
        CHARTYPE* Src = Dst + Count;

        const SIZETYPE Num = StringLength - (Position + Count);
        FCStringType::Strmove(Dst, Src, Num);
    }

    /**
     * @brief          - Insert a string at a specified position
     * @param InString - String to insert
     * @param Position - Position to start the insertion at
     */
    FORCEINLINE void Insert(const CHARTYPE* InString, SIZETYPE Position)
    {
        Insert(InString, FCStringType::Strlen(InString), Position);
    }

    /**
     * @brief          - Insert a string at a specified position. The string is of string type.
     * @param InString - String to insert
     * @param Position - Position to start the insertion at
     */
    template<typename StringType>
    FORCEINLINE void Insert(const StringType& InString, SIZETYPE Position) requires(TIsTStringType<StringType>::Value)
    {
        Insert(InString.Data(), InString.Length(), Position);
    }

    /**
     * @brief          - Insert a string at a specified position
     * @param InString - String to insert
     * @param InLength - Length of the string to insert
     * @param Position - Position to start the insertion at
     */
    FORCEINLINE void Insert(const CHARTYPE* InString, SIZETYPE InLength, SIZETYPE Position)
    {
        CHECK((Position < StringLength) && (StringLength + InLength < NUM_CHARS));

        CHARTYPE* Src = CharData + Position;
        CHARTYPE* Dst = Src + InLength;

        const SIZETYPE MoveSize = StringLength - Position;
        FCStringType::Strnmove(Dst, Src, MoveSize);
        FCStringType::Strncpy(Src, InString, InLength);

        StringLength += InLength;
        CharData[StringLength] = 0;
    }

    /**
     * @brief          - Insert a character at a specified position 
     * @param Char     - Character to insert
     * @param Position - Position to insert character at
     */
    FORCEINLINE void Insert(CHARTYPE Char, SIZETYPE Position)
    {
        CHECK((Position < StringLength) && (StringLength + 1 < NUM_CHARS));

        // Make room for character
        CHARTYPE* Src = CharData + Position;
        CHARTYPE* Dst = Src + 1;

        const SIZETYPE MoveSize = StringLength - Position;
        FCStringType::Strmove(Dst, Src, MoveSize);

        // Insert character
        *Src = Char;
        CharData[++StringLength] = 0;
    }

    /**
     * @brief          - Replace a part of the string
     * @param InString - String to replace with
     * @param Position - Position to start the replacing at
     */
    FORCEINLINE void Replace(const CHARTYPE* InString, SIZETYPE Position)
    {
        Replace(InString, FCStringType::Strlen(InString), Position);
    }

    /**
     * @brief          - Replace a part of the string. String is of string type.
     * @param InString - String to replace with
     * @param Position - Position to start the replacing at
     */
    template<typename StringType>
    FORCEINLINE void Replace(const StringType& InString, SIZETYPE Position) requires(TIsTStringType<StringType>::Value)
    {
        Replace(InString.Data(), InString.Length(), Position);
    }

    /**
     * @brief          - Replace a part of the string
     * @param InString - String to replace with
     * @param InLength - Length of the string to replace
     * @param Position - Position to start the replacing at
     */
    FORCEINLINE void Replace(const CHARTYPE* InString, SIZETYPE InLength, SIZETYPE Position)
    {
        CHECK(Position < StringLength && (Position + InLength) < StringLength);      
        if (InString)
        {
            FCStringType::Strncpy(CharData + Position, InString, InLength);
        }
    }

    /**
     * @brief          - Replace a character in the string
     * @param Char     - Character to replace with
     * @param Position - Position of the character to replace 
     */
    FORCEINLINE void Replace(CHARTYPE Char, SIZETYPE Position)
    {
        CHECK(Position < StringLength);
        CharData[Position] = Char;
    }

    /**
     * @brief      - Insert a new character at the end
     * @param Char - Character to insert at the end
     */
    FORCEINLINE void Add(CHARTYPE Char)
    {
        Append(Char);
    }

    /**
     * @brief - Remove the character at the end
     */
    FORCEINLINE void Pop()
    {
        if (StringLength > 0)
        {
            --StringLength;
            CharData[StringLength] = 0;
        }
    }

    /**
     * @brief       - Swap this string with another
     * @param Other - String to swap with
     */
    FORCEINLINE void Swap(TStaticString& Other)
    {
        TStaticString TempString(Move(*this));
        MoveFrom(Move(Other));
        Other.MoveFrom(Move(TempString));
    }

    /**
     * @brief               - Create a substring of this string
     * @param Position      - Position to start the substring at
     * @param NumCharacters - Number of characters in the substring
     * @return              - Returns a substring
     */
    NODISCARD FORCEINLINE TStaticString SubString(SIZETYPE Position, SIZETYPE NumCharacters) const
    {
        CHECK(Position < StringLength && (Position + NumCharacters) < StringLength);
        return TStaticString(CharData + Position, NumCharacters);
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE CHARTYPE* Data()
    {
        return CharData;
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const CHARTYPE* Data() const
    {
        return CharData;
    }

    /**
     * @brief  - Retrieve the string as a C-array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const CHARTYPE* GetCString() const
    {
        return CharData;
    }

    /**
     * @brief  - Returns the size of the container
     * @return - The current size of the container
     */
    NODISCARD FORCEINLINE SIZETYPE Size() const
    {
        return StringLength;
    }

    /**
     * @brief  - Retrieve the last index that can be used to retrieve an element from the array
     * @return - Returns the index to the last element of the array
     */
    NODISCARD FORCEINLINE SIZETYPE LastElementIndex() const
    {
        return (StringLength > 0) ? (StringLength - 1) : 0;
    }

    /**
     * @brief  - Returns the length of the string
     * @return - The current length of the string
     */
    NODISCARD FORCEINLINE SIZETYPE Length() const
    {
        return StringLength;
    }

    /**
     * @brief  - Returns the size of the container in bytes
     * @return - The current size of the container in bytes
     */
    NODISCARD FORCEINLINE SIZETYPE SizeInBytes() const
    {
        return StringLength * sizeof(CHARTYPE);
    }

    /**
     * @brief  - Check if the container contains any elements
     * @return - Returns true if the array is empty or false if it contains elements
     */
    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return StringLength == 0;
    }

    /**
     * @brief  - Retrieve the first element of the array
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE CHARTYPE& FirstElement()
    {
        return CharData[0];
    }

    /**
     * @brief  - Retrieve the first element of the array
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE const CHARTYPE& FirstElement() const
    {
        return CharData[0];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE CHARTYPE& LastElement()
    {
        return CharData[LastElementIndex()];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE const CHARTYPE& LastElement() const
    {
        return CharData[LastElementIndex()];
    }

public:

    /**
     * @brief  - Retrieve the capacity of the container
     * @return - The capacity of the container
     */
    NODISCARD constexpr SIZETYPE Capacity() const
    {
        return NUM_CHARS;
    }

    /**
     * @brief  - Retrieve the capacity of the container in bytes
     * @return - The capacity of the container in bytes
     */
    NODISCARD constexpr SIZETYPE CapacityInBytes() const
    {
        return NUM_CHARS * sizeof(CHARTYPE);
    }

public:

    /**
     * @brief       - Appends a character to this string
     * @param Other - Character to append
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TStaticString& operator+=(CHARTYPE Other)
    {
        Append(Other);
        return *this;
    }

    /**
     * @brief       - Appends a string to this string
     * @param Other - String to append
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TStaticString& operator+=(const CHARTYPE* Other)
    {
        Append(Other);
        return *this;
    }

    /**
     * @brief       - Appends a string of a string type to this string
     * @param Other - String to append
     * @return      - Returns a reference to this instance
     */
    template<typename StringType>
    FORCEINLINE TStaticString& operator+=(const StringType& Other) requires(TIsTStringType<StringType>::Value)
    {
        Append<StringType>(Other);
        return *this;
    }

    /**
     * @brief       - Bracket operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - Returns a reference to the element at the index
     */
    NODISCARD FORCEINLINE CHARTYPE& operator[](SIZETYPE Index)
    {
        CHECK(Index < Length());
        return CharData[Index];
    }

    /**
     * @brief       - Bracket operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - Returns a reference to the element at the index
     */
    NODISCARD FORCEINLINE const CHARTYPE& operator[](SIZETYPE Index) const
    {
        CHECK(Index < Length());
        return CharData[Index];
    }

    /**
     * @brief       - Assignment operator that takes a raw string
     * @param Other - String to copy
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TStaticString& operator=(const CHARTYPE* Other)
    {
        TStaticString(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Copy-assignment operator 
     * @param Other - String to copy
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TStaticString& operator=(const TStaticString& Other)
    {
        TStaticString(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Assignment operator that takes another string type
     * @param Other - String to copy
     * @return      - Returns a reference to this instance
     */
    template<typename StringType>
    FORCEINLINE TStaticString& operator=(const StringType& Other) requires(TIsTStringType<StringType>::Value)
    {
        TStaticString<StringType, NUM_CHARS>(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - String to move
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TStaticString& operator=(TStaticString&& Other)
    {
        TStaticString(Move(Other)).Swap(*this);
        return *this;
    }

public:
    NODISCARD friend FORCEINLINE TStaticString operator+(const TStaticString& LHS, const TStaticString& RHS)
    {
        TStaticString NewString(LHS);
        NewString.Append(RHS);
        return NewString;
    }

    NODISCARD friend FORCEINLINE TStaticString operator+(const CHARTYPE* LHS, const TStaticString& RHS)
    {
        TStaticString NewString(LHS);
        NewString.Append(RHS);
        return NewString;
    }

    NODISCARD friend FORCEINLINE TStaticString operator+(const TStaticString& LHS, const CHARTYPE* RHS)
    {
        TStaticString NewString(LHS);
        NewString.Append(RHS);
        return NewString;
    }

    NODISCARD friend FORCEINLINE TStaticString operator+(CHARTYPE LHS, const TStaticString& RHS)
    {
        TStaticString NewString;
        NewString.Append(LHS);
        NewString.Append(RHS);
        return NewString;
    }

    NODISCARD friend FORCEINLINE TStaticString operator+(const TStaticString& LHS, CHARTYPE RHS)
    {
        TStaticString NewString(LHS);
        NewString.Append(RHS);
        return NewString;
    }

    NODISCARD friend FORCEINLINE bool operator==(const TStaticString& LHS, const CHARTYPE* RHS)
    {
        return LHS.Compare(RHS) == 0;
    }

    NODISCARD friend FORCEINLINE bool operator==(const CHARTYPE* LHS, const TStaticString& RHS)
    {
        return RHS.Compare(LHS) == 0;
    }

    NODISCARD friend FORCEINLINE bool operator==(const TStaticString& LHS, const TStaticString& RHS)
    {
        return LHS.Compare(RHS) == 0;
    }

    NODISCARD friend FORCEINLINE bool operator!=(const TStaticString& LHS, const CHARTYPE* RHS)
    {
        return !(LHS == RHS);
    }

    NODISCARD friend FORCEINLINE bool operator!=(const CHARTYPE* LHS, const TStaticString& RHS)
    {
        return !(LHS == RHS);
    }

    NODISCARD friend FORCEINLINE bool operator!=(const TStaticString& LHS, const TStaticString& RHS)
    {
        return !(LHS == RHS);
    }

    NODISCARD friend FORCEINLINE bool operator<(const TStaticString& LHS, const CHARTYPE* RHS)
    {
        return LHS.Compare(RHS) < 0;
    }

    NODISCARD friend FORCEINLINE bool operator<(const CHARTYPE* LHS, const TStaticString& RHS)
    {
        return RHS.Compare(LHS) < 0;
    }

    NODISCARD friend FORCEINLINE bool operator<(const TStaticString& LHS, const TStaticString& RHS)
    {
        return LHS.Compare(RHS) < 0;
    }

    NODISCARD friend FORCEINLINE bool operator<=(const TStaticString& LHS, const CHARTYPE* RHS)
    {
        return LHS.Compare(RHS) <= 0;
    }

    NODISCARD friend FORCEINLINE bool operator<=(const CHARTYPE* LHS, const TStaticString& RHS)
    {
        return RHS.Compare(LHS) <= 0;
    }

    NODISCARD friend FORCEINLINE bool operator<=(const TStaticString& LHS, const TStaticString& RHS)
    {
        return LHS.Compare(RHS) <= 0;
    }

    NODISCARD friend FORCEINLINE bool operator>(const TStaticString& LHS, const CHARTYPE* RHS)
    {
        return LHS.Compare(RHS) > 0;
    }

    NODISCARD friend FORCEINLINE bool operator>(const CHARTYPE* LHS, const TStaticString& RHS)
    {
        return RHS.Compare(LHS) > 0;
    }

    NODISCARD friend FORCEINLINE bool operator>(const TStaticString& LHS, const TStaticString& RHS)
    {
        return LHS.Compare(RHS) > 0;
    }

    NODISCARD friend FORCEINLINE bool operator>=(const TStaticString& LHS, const CHARTYPE* RHS)
    {
        return LHS.Compare(RHS) >= 0;
    }

    NODISCARD friend FORCEINLINE bool operator>=(const CHARTYPE* LHS, const TStaticString& RHS)
    {
        return RHS.Compare(LHS) >= 0;
    }

    NODISCARD friend FORCEINLINE bool operator>=(const TStaticString& LHS, const TStaticString& RHS)
    {
        return LHS.Compare(RHS) >= 0;
    }

public:

    // Iterators
    NODISCARD FORCEINLINE IteratorType Iterator()
    {
        return IteratorType(*this, 0);
    }

    NODISCARD FORCEINLINE ConstIteratorType ConstIterator() const
    {
        return ConstIteratorType(*this, 0);
    }

    NODISCARD FORCEINLINE ReverseIteratorType ReverseIterator()
    {
        return ReverseIteratorType(*this, Size());
    }

    NODISCARD FORCEINLINE ReverseConstIteratorType ConstReverseIterator() const
    {
        return ReverseConstIteratorType(*this, Size());
    }

public:

    // STL Iterator
    NODISCARD FORCEINLINE IteratorType      begin()       { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const { return ConstIterator(); }
    
    NODISCARD FORCEINLINE IteratorType      end()       { return IteratorType(*this, Size()); }
    NODISCARD FORCEINLINE ConstIteratorType end() const { return ConstIteratorType(*this, Size()); }

private:
    FORCEINLINE void InitializeByCopy(const CHARTYPE* InString, SIZETYPE InLength)
    {
        if (InString && InLength)
        {
            FCStringType::Strncpy(CharData, InString, InLength);
            StringLength = InLength;
            CharData[StringLength] = 0;
        }
    }

    FORCEINLINE void MoveFrom(TStaticString&& Other)
    {
        FCStringType::Strncpy(CharData, Other.CharData, Other.StringLength);
        Other.CharData[0] = 0;

        StringLength = Other.StringLength;
        Other.StringLength     = 0;
        CharData[StringLength] = 0;
    }

    CHARTYPE CharData[NUM_CHARS];
    SIZETYPE StringLength = 0;
};

template<int32 NUM_CHARS = STANDARD_STATIC_STRING_LENGTH>
using FStaticString = TStaticString<CHAR, NUM_CHARS>;

template<int32 NUM_CHARS = STANDARD_STATIC_STRING_LENGTH>
using FStaticStringWide = TStaticString<WIDECHAR, NUM_CHARS>;

template<typename CHARTYPE, int32 NUM_CHARS>
struct TIsTStringType<TStaticString<CHARTYPE, NUM_CHARS>>
{
    inline static constexpr bool Value = true;
};

template<int32 NUM_CHARS>
NODISCARD inline FStaticStringWide<NUM_CHARS> CharToWide(const FStaticString<NUM_CHARS>& CharString)
{
    FStaticStringWide<NUM_CHARS> NewString;
    NewString.Resize(CharString.Length());
    FPlatformString::Mbstowcs(NewString.Data(), CharString.Data(), CharString.Length());
    return NewString;
}

template<int32 NUM_CHARS>
NODISCARD inline FStaticString<NUM_CHARS> WideToChar(const FStaticStringWide<NUM_CHARS>& WideString)
{
    FStaticString<NUM_CHARS> NewString;
    NewString.Resize(WideString.Length());
    FPlatformString::Wcstombs(NewString.Data(), WideString.Data(), WideString.Length());
    return NewString;
}

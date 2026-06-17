#pragma once
#include "Core/Containers/Iterator.h"
#include "Core/Math/Math.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/CString.h"
#include "Core/Templates/ArrayContainer.h"

enum class EStringCaseType
{
    NoCase        = 1,
    CaseSensitive = 2
};

template<typename InCharType>
class TStringView
{
    typedef TCharTraits<InCharType> CharTraitsType;
    typedef TCString<InCharType>    CStringType;

public:
    typedef int32      SizeType;
    typedef InCharType CharType;

    static_assert(TIsSame<CharType, CHAR>::Value || TIsSame<CharType, WIDECHAR>::Value, "TStringView only supports 'CHAR' and 'WIDECHAR'");
    static_assert(TIsSigned<SizeType>::Value, "TStringView only supports a SizeType that's signed");

    typedef TArrayIterator<const TStringView, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<const TStringView, const CharType> ReverseConstIteratorType;

    inline static constexpr SizeType InvalidIndex = SizeType(~0);

public:

    /** @brief Default constructor */
    TStringView() = default;

    /**
     * @brief Create a view from a raw string
     * @param InString String to view
     */
    FORCEINLINE TStringView(const CharType* InString)
        : ViewStart(InString)
        , ViewEnd(InString + CStringType::Strlen(InString))
    {
    }

    /**
     * @brief Create a view from a raw string with a fixed length
     * @param InString String to view
     * @param InLength Length of the string to view
     * @param Offset Offset into the string
     */
    FORCEINLINE explicit TStringView(const CharType* InString, SizeType InLength, SizeType Offset = 0)
        : ViewStart(InString + Offset)
        , ViewEnd(ViewStart + InLength)
    {
    }

    /**
     * @brief Create a view from a string-type
     * @param InString String to view
     */
    template<typename StringType>
    FORCEINLINE explicit TStringView(const StringType& InString) requires(TIsTStringType<StringType>::Value)
        : ViewStart(ArrayContainer::Data(InString))
        , ViewEnd(ArrayContainer::Data(InString) + ArrayContainer::Size(InString))
    {
    }

    /**
     * @brief Copy Constructor
     * @param Other Other view to copy from
     */
    FORCEINLINE TStringView(const TStringView& Other)
        : ViewStart(Other.ViewStart)
        , ViewEnd(Other.ViewEnd)
    {
    }

    /**
     * @brief Move Constructor
     * @param Other Other view to move from
     */
    FORCEINLINE TStringView(TStringView&& Other)
        : ViewStart(Other.ViewStart)
        , ViewEnd(Other.ViewEnd)
    {
        Other.ViewStart = nullptr;
        Other.ViewEnd   = nullptr;
    }

    /**
     * @brief Clears the view
     */
    FORCEINLINE void Clear()
    {
        ViewStart = nullptr;
        ViewEnd   = nullptr;
    }

    /**
     * @brief Copy this string into buffer
     * @param Buffer Buffer to fill
     * @param BufferSize Size of the buffer to fill
     * @param Position Offset to start copy from
     */
    FORCEINLINE void CopyToBuffer(CharType* Buffer, SizeType BufferSize, SizeType Position = InvalidIndex) const
    {
        CHECK(Buffer != nullptr);
        if (Buffer && BufferSize > 0)
        {
            const SizeType CopySize = Math::Min(BufferSize, Length() - Position);
            CStringType::Strncpy(Buffer, ViewStart + Position, CopySize);
        }
    }

    /**
     * @brief Removes whitespace from the beginning and end of the string and returns a copy
     * @return Returns a copy of this string with the whitespace removed at the end and beginning
     */
    NODISCARD FORCEINLINE TStringView Trim()
    {
        TStringView NewStringView(*this);
        NewStringView.TrimInline();
        return NewStringView;
    }

    /**
     * @brief Removes whitespace from the beginning and end of the string
     */
    FORCEINLINE void TrimInline()
    {
        TrimStartInline();
        TrimEndInline();
    }

    /**
     * @brief Removes whitespace from the beginning of the string and returns a copy
     * @return Returns a copy of this string with all the whitespace removed from the beginning
     */
    NODISCARD FORCEINLINE TStringView TrimStart()
    {
        TStringView NewStringView(*this);
        NewStringView.TrimStartInline();
        return NewStringView;
    }

    /**
     * @brief Removes whitespace from the beginning of the string
     */
    FORCEINLINE void TrimStartInline()
    {
        const CharType* RESTRICT Current = ViewStart;
        for (; Current != ViewEnd; ++Current)
        {
            if (!TCharTraits<CharType>::IsWhitespace(*Current))
            {
                break;
            }
        }

        ViewStart = Current;
    }

    /**
     * @brief Removes whitespace from the end of the string and returns a copy
     * @return Returns a copy of this string with all the whitespace removed from the end
     */
    NODISCARD FORCEINLINE TStringView TrimEnd()
    {
        TStringView NewStringView(*this);
        NewStringView.TrimEndInline();
        return NewStringView;
    }

    /**
     * @brief Removes whitespace from the end of the string
     */
    FORCEINLINE void TrimEndInline()
    {
        const CharType* RESTRICT Current = ViewEnd;
        for (; Current > ViewStart; --Current)
        {
            const CharType* RESTRICT CurrentChar = Current - 1;
            if (!TCharTraits<CharType>::IsWhitespace(*CurrentChar))
            {
                break;
            }
        }

        ViewEnd = Current;
    }

    /**
     * @brief Shrink the view from the left and return a copy
     * @param Num Number of characters to trim
     * @return Return a trimmed copy of the view
     */
    NODISCARD FORCEINLINE TStringView ShrinkLeft(int32 Num = 1)
    {
        TStringView NewStringView(*this);
        NewStringView.ShrinkLeftInline(Num);
        return NewStringView;
    }

    /**
     * @brief Shrink the view from the left
     * @param Num Number of characters to trim
     */
    FORCEINLINE void ShrinkLeftInline(int32 Num = 1)
    {
        const SizeType CurrentLength = Length();
        if (CurrentLength <= Num)
        {
            ViewStart = ViewEnd;
        }
        else
        {
            ViewStart += Num;
        }
    }

    /**
     * @brief Shrink the view from the right and return a copy
     * @param Num Number of characters to trim
     * @return Return a trimmed copy of the view
     */
    NODISCARD FORCEINLINE TStringView ShrinkRight(int32 Num = 1)
    {
        TStringView NewStringView(*this);
        NewStringView.ShrinkRightInline(Num);
        return NewStringView;
    }

    /**
     * @brief Shrink the view from the right
     * @param Num Number of characters to trim
     */
    FORCEINLINE void ShrinkRightInline(int32 Num = 1)
    {
        const SizeType CurrentLength = Length();
        if (CurrentLength <= Num)
        {
            ViewEnd = ViewStart;
        }
        else
        {
            ViewEnd -= Num;
        }
    }

    /**
     * @brief Compares this string to another string-type
     * @param InString String to compare with
     * @param CaseType Enum that decides if the comparison should be case-sensitive or not
     * @return Returns the position of the characters that are not equal. The sign determines difference of the character.
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SizeType Compare(const StringType& InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const requires(TIsTStringType<StringType>::Value)
    {
        return Compare(InString.Data(), InString.Length(), CaseType);
    }

    /**
     * @brief Compares this string with a c-string
     * @param InString String to compare with
     * @param CaseType Enum that decides if the comparison should be case-sensitive or not
     * @return Returns the position of the characters that are not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE SizeType Compare(const CharType* InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const
    {
        if (CaseType == EStringCaseType::NoCase)
        {
            return static_cast<SizeType>(CStringType::Stricmp(ViewStart, InString));
        }
        else
        {
            return static_cast<SizeType>(CStringType::Strcmp(ViewStart, InString));
        }
    }

    /**
     * @brief Compares this string with a c-string of a fixed length
     * @param InString String to compare with
     * @param InLength Length of the string to compare
     * @param CaseType Enum that decides if the comparison should be case-sensitive or not
     * @return Returns the position of the characters that are not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE SizeType Compare(const CharType* InString, SizeType InLength, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const
    {
        const SizeType MinLength = Math::Min(Length(), InLength);
        if (CaseType == EStringCaseType::NoCase)
        {
            return static_cast<SizeType>(CStringType::Strnicmp(ViewStart, InString, MinLength));
        }
        else
        {
            return static_cast<SizeType>(CStringType::Strncmp(ViewStart, InString, MinLength));
        }
    }

    /**
     * @brief Compares this string to another string-type
     * @param InString String to compare with
     * @param CaseType Enum that decides if the comparison should be case-sensitive or not
     * @return Returns true if the strings are equal
     */
    template<typename StringType>
    NODISCARD FORCEINLINE bool Equals(const StringType& InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const requires(TIsTStringType<StringType>::Value)
    {
        return Equals(InString.Data(), InString.Length(), CaseType);
    }

    /**
     * @brief Compares this string with a c-string
     * @param InString String to compare with
     * @param CaseType Enum that decides if the comparison should be case-sensitive or not
     * @return Returns true if the strings are equal
     */
    NODISCARD FORCEINLINE bool Equals(const CharType* InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const
    {
        return Equals(InString, CStringType::Strlen(InString), CaseType);
    }

    /**
     * @brief Compares this string with a c-string of a fixed length
     * @param InString String to compare with
     * @param InLength Length of the string to compare
     * @param CaseType Enum that decides if the comparison should be case-sensitive or not
     * @return Returns true if the strings are equal
     */
    NODISCARD FORCEINLINE bool Equals(const CharType* InString, SizeType InLength, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const
    {
        const SizeType CurrentLength = Length();
        if (CurrentLength != InLength)
        {
            return false;
        }

        if (CaseType == EStringCaseType::CaseSensitive)
        {
            return CStringType::Strncmp(ViewStart, InString, CurrentLength) == 0;
        }
        else if (CaseType == EStringCaseType::NoCase)
        {
            return CStringType::Strnicmp(ViewStart, InString, CurrentLength) == 0;
        }
        else
        {
            return false;
        }
    }

    /**
     * @brief Find the position of the first occurrence of the start of the search-string
     * @param InString String to search
     * @param Position Position to start search at
     * @param CaseType Enum that decides if the search should be case-sensitive or not
     * @return Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType Find(const CharType* InString, SizeType Position, EStringCaseType CaseType) const
    {
        const SizeType CurrentLength = Length();
        if (!CurrentLength)
        {
            return 0;
        }

        if (InString == nullptr)
        {
            return InvalidIndex;
        }

        SizeType Index = 0;
        if (Position != InvalidIndex && CurrentLength > 0)
        {
            Index += Math::Clamp(Position, 0, CurrentLength - 1);
        }

        const SizeType SearchLength = CStringType::Strlen(InString);
        if (SearchLength == 0)
        {
            return Index;
        }

        if (SearchLength > CurrentLength || Index > (CurrentLength - SearchLength))
        {
            return InvalidIndex;
        }

        for (; Index <= (CurrentLength - SearchLength); ++Index)
        {
            if (CaseType == EStringCaseType::CaseSensitive)
            {
                if (CStringType::Strncmp(ViewStart + Index, InString, SearchLength) == 0)
                {
                    return Index;
                }
            }
            else if (CaseType == EStringCaseType::NoCase)
            {
                if (CStringType::Strnicmp(ViewStart + Index, InString, SearchLength) == 0)
                {
                    return Index;
                }
            }
            else
            {
                return InvalidIndex;
            }
        }

        return InvalidIndex;
    }

    /**
     * @brief Find the position of the first occurrence of the start of the search-string
     * @param InString String to search
     * @param Position Position to start search at
     * @return Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType Find(const CharType* InString, SizeType Position = InvalidIndex) const
    {
        return Find(InString, Position, EStringCaseType::CaseSensitive);
    }

    /**
     * @brief Find the position of the first occurrence of the start of the search-string
     * @param InString String to search
     * @param CaseType Enum that decides if the search should be case-sensitive or not
     * @param Position Position to start search at
     * @return Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType Find(const CharType* InString, EStringCaseType CaseType, SizeType Position = InvalidIndex) const
    {
        return Find(InString, Position, CaseType);
    }

    /**
     * @brief Find the position of the first occurrence of the start of the search-string
     * @param InString String to search
     * @param Position Position to start search at
     * @return Returns the position of the first character in the search-string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SizeType Find(const StringType& InString, SizeType Position = InvalidIndex) const requires(TIsTStringType<StringType>::Value)
    {
        return Find(InString.Data(), Position);
    }

    /**
     * @brief Find the position of the first occurrence of the start of the search-string
     * @param InString String to search
     * @param CaseType Enum that decides if the search should be case-sensitive or not
     * @param Position Position to start search at
     * @return Returns the position of the first character in the search-string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SizeType Find(const StringType& InString, EStringCaseType CaseType, SizeType Position = InvalidIndex) const requires(TIsTStringType<StringType>::Value)
    {
        return Find(InString.Data(), CaseType, Position);
    }

    /**
     * @brief Returns the position of the first occurrence of CHAR
     * @param Char Character to search for
     * @param Position Position to start search at
     * @return Returns the position of the first occurrence of the CHAR
     */
    NODISCARD FORCEINLINE SizeType FindChar(CharType Char, SizeType Position = InvalidIndex) const
    {
        const SizeType CurrentLength = Length();
        if (!CurrentLength)
        {
            return 0;
        }

        const CharType* RESTRICT Current = ViewStart;
        if (Position != InvalidIndex && CurrentLength > 0)
        {
            Current += Math::Clamp(Position, 0, CurrentLength - 1);
        }

        for (const CharType* RESTRICT End = ViewStart + CurrentLength; Current != End; ++Current)
        {
            if (*Current == Char)
            {
                return static_cast<SizeType>(static_cast<PTR_INT>(Current - ViewStart));
            }
        }

        return InvalidIndex;
    }

    /**
     * @brief Returns the position of the first CHAR that passes the predicate
     * @param Predicate Predicate that specifies valid chars
     * @param Position Position to start search at
     * @return Returns the position of the first occurrence of the CHAR
     */
    template<typename PredicateType>
    NODISCARD FORCEINLINE SizeType FindCharWithPredicate(PredicateType&& Predicate, SizeType Position = InvalidIndex) const
    {
        const SizeType CurrentLength = Length();
        if (!CurrentLength)
        {
            return 0;
        }

        const CharType* RESTRICT Current = ViewStart;
        if (Position != InvalidIndex && CurrentLength > 0)
        {
            Current += Math::Clamp(Position, 0, CurrentLength - 1);
        }

        for (const CharType* RESTRICT End = ViewStart + CurrentLength; Current != End; ++Current)
        {
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(static_cast<PTR_INT>(Current - ViewStart));
            }
        }

        return InvalidIndex;
    }

    /**
     * @brief Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     * @param InString String to search
     * @param Position Position to start search at
     * @param CaseType Enum that decides if the search should be case-sensitive or not
     * @return Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType FindLast(const CharType* InString, SizeType Position, EStringCaseType CaseType) const
    {
        const SizeType CurrentLength = Length();
        if (!CurrentLength)
        {
            return 0;
        }

        if (InString == nullptr)
        {
            return InvalidIndex;
        }

        if (Position == InvalidIndex || Position > CurrentLength)
        {
            Position = CurrentLength;
        }

        const SizeType SearchLength = CStringType::Strlen(InString);
        if (SearchLength == 0)
        {
            return Position;
        }

        if (SearchLength > Position)
        {
            return InvalidIndex;
        }

        for (SizeType Index = Position - SearchLength; Index >= 0; --Index)
        {
            bool bMatch = false;
            if (CaseType == EStringCaseType::CaseSensitive)
            {
                bMatch = CStringType::Strncmp(ViewStart + Index, InString, SearchLength) == 0;
            }
            else if (CaseType == EStringCaseType::NoCase)
            {
                bMatch = CStringType::Strnicmp(ViewStart + Index, InString, SearchLength) == 0;
            }

            if (bMatch)
            {
                return Index;
            }

            if (Index == 0)
            {
                break;
            }
        }

        return InvalidIndex;
    }

    /**
     * @brief Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     * @param InString String to search
     * @param Position Position to start search at
     * @return Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType FindLast(const CharType* InString, SizeType Position = InvalidIndex) const
    {
        return FindLast(InString, Position, EStringCaseType::CaseSensitive);
    }

    /**
     * @brief Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     * @param InString String to search
     * @param CaseType Enum that decides if the search should be case-sensitive or not
     * @param Position Position to start search at
     * @return Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType FindLast(const CharType* InString, EStringCaseType CaseType, SizeType Position = InvalidIndex) const
    {
        return FindLast(InString, Position, CaseType);
    }

    /**
     * @brief Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     * @param InString String to search
     * @param Position Position to start search at
     * @return Returns the position of the first character in the search-string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SizeType FindLast(const StringType& InString, SizeType Position = InvalidIndex) const requires(TIsTStringType<StringType>::Value)
    {
        return FindLast(InString.Data(), Position);
    }

    /**
     * @brief Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     * @param InString String to search
     * @param CaseType Enum that decides if the search should be case-sensitive or not
     * @param Position Position to start search at
     * @return Returns the position of the first character in the search-string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SizeType FindLast(const StringType& InString, EStringCaseType CaseType, SizeType Position = InvalidIndex) const requires(TIsTStringType<StringType>::Value)
    {
        return FindLast(InString.Data(), CaseType, Position);
    }

    /**
     * @brief Returns the position of the first occurrence of CHAR. Searches the string in reverse.
     * @param Char Character to search for
     * @param Position Position to start search at
     * @return Returns the position of the first occurrence of the CHAR
     */
    NODISCARD FORCEINLINE SizeType FindLastChar(CharType Char, SizeType Position = InvalidIndex) const
    {
        const SizeType CurrentLength = Length();
        if (!CurrentLength)
        {
            return 0;
        }

        const CharType* RESTRICT Current = ViewStart;
        if (Position == InvalidIndex || Position > CurrentLength)
        {
            Current += CurrentLength;
        }

        for (const CharType* RESTRICT End = ViewStart; Current != End;)
        {
            --Current;
            if (Char == *Current)
            {
                return static_cast<SizeType>(static_cast<PTR_INT>(Current - End));
            }
        }

        return InvalidIndex;
    }

    /**
     * @brief Returns the position of the first CHAR that passes the predicate
     * @param Predicate Predicate that specifies valid chars
     * @param Position Position to start search at
     * @return Returns the position of the first occurrence of the CHAR
     */
    template<typename PredicateType>
    NODISCARD FORCEINLINE SizeType FindLastCharWithPredicate(PredicateType&& Predicate, SizeType Position = InvalidIndex) const
    {
        const SizeType CurrentLength = Length();
        if (!CurrentLength)
        {
            return 0;
        }

        const CharType* RESTRICT Current = ViewStart;
        if (Position == InvalidIndex || Position > CurrentLength)
        {
            Current += CurrentLength;
        }

        for (const CharType* RESTRICT End = ViewStart; Current != End;)
        {
            --Current;
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(static_cast<PTR_INT>(Current - End));
            }
        }

        return InvalidIndex;
    }

    /**
     * @brief Check if the search-string exists within the view
     * @param InString String to search for
     * @param Position Position to start to search at
     * @return Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(const CharType* InString, SizeType Position = InvalidIndex) const
    {
        return Find(InString, Position) != InvalidIndex;
    }

    /**
     * @brief Check if the search-string exists within the view
     * @param InString String to search for
     * @param CaseType Enum that decides if the search should be case-sensitive or not
     * @param Position Position to start to search at
     * @return Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(const CharType* InString, EStringCaseType CaseType, SizeType Position = InvalidIndex) const
    {
        return Find(InString, CaseType, Position) != InvalidIndex;
    }

    /**
     * @brief Check if the search-string exists within the view. The string is of a string-type.
     * @param InString String to search for
     * @param Position Position to start to search at
     * @return Returns true if the string is found
     */
    template<typename StringType>
    NODISCARD FORCEINLINE bool Contains(const StringType& InString, SizeType Position = InvalidIndex) const requires(TIsTStringType<StringType>::Value)
    {
        return Find(InString, Position) != InvalidIndex;
    }

    /**
     * @brief Check if the search-string exists within the view. The string is of a string-type.
     * @param InString String to search for
     * @param CaseType Enum that decides if the search should be case-sensitive or not
     * @param Position Position to start to search at
     * @return Returns true if the string is found
     */
    template<typename StringType>
    NODISCARD FORCEINLINE bool Contains(const StringType& InString, EStringCaseType CaseType, SizeType Position = InvalidIndex) const requires(TIsTStringType<StringType>::Value)
    {
        return Find(InString, CaseType, Position) != InvalidIndex;
    }

    /**
     * @brief Check if the character exists within the view
     * @param Char Character to search for
     * @param Position Position to start to search at
     * @return Returns true if the character is found
     */
    NODISCARD FORCEINLINE bool Contains(CharType Char, SizeType Position = InvalidIndex) const
    {
        return FindChar(Char, Position) != InvalidIndex;
    }

    /**
     * @brief Check if string begins with a string
     * @param InString String to test for
     * @param InLength Length of the string to test for
     * @param SearchType Type of search to perform, case-sensitive or not
     * @return Returns true if the string begins with InString
     */
    NODISCARD FORCEINLINE bool StartsWith(const CharType* InString, SizeType InLength, EStringCaseType SearchType = EStringCaseType::CaseSensitive) const
    {
        if (!InString)
        {
            return false;
        }

        const SizeType CurrentLength = Length();
        if (InLength > 0 && CurrentLength >= InLength)
        {
            if (SearchType == EStringCaseType::CaseSensitive)
            {
                return CStringType::Strncmp(ViewStart, InString, InLength) == 0;
            }
            else if (SearchType == EStringCaseType::NoCase)
            {
                return CStringType::Strnicmp(ViewStart, InString, InLength) == 0;
            }
        }

        return false;
    }

    /**
     * @brief Check if string begins with a string
     * @param InString String to test for
     * @param SearchType Type of search to perform, case-sensitive or not
     * @return Returns true if the string begins with InString
     */
    NODISCARD FORCEINLINE bool StartsWith(const CharType* InString, EStringCaseType SearchType = EStringCaseType::CaseSensitive) const
    {
        if (!InString)
        {
            return false;
        }

        return StartsWith(InString, CStringType::Strlen(InString), SearchType);
    }

    /**
     * @brief Check if string begins with a string
     * @param InString String to test for
     * @param SearchType Type of search to perform, case-sensitive or not
     * @return Returns true if the string begins with InString
     */
    template<typename StringType>
    NODISCARD FORCEINLINE bool StartsWith(const StringType& InString, EStringCaseType SearchType = EStringCaseType::CaseSensitive) const requires(TIsTStringType<StringType>::Value)
    {
        return StartsWith(InString.Data(), InString.Length(), SearchType);
    }

    /**
     * @brief Check if string ends with a string
     * @param InString String to test for
     * @param InLength Length of the string to test for
     * @param SearchType Type of search to perform, case-sensitive or not
     * @return Returns true if the string ends with InString
     */
    NODISCARD FORCEINLINE bool EndsWith(const CharType* InString, SizeType InLength, EStringCaseType SearchType = EStringCaseType::CaseSensitive) const
    {
        if (!InString)
        {
            return false;
        }

        const SizeType CurrentLength = Length();
        if (InLength > 0 && CurrentLength >= InLength)
        {
            const CharType* StringData = ViewStart + (CurrentLength - InLength);
            if (SearchType == EStringCaseType::CaseSensitive)
            {
                return CStringType::Strncmp(StringData, InString, InLength) == 0;
            }
            else if (SearchType == EStringCaseType::NoCase)
            {
                return CStringType::Strnicmp(StringData, InString, InLength) == 0;
            }
        }

        return false;
    }

    /**
     * @brief Check if string ends with a string
     * @param InString String to test for
     * @param SearchType Type of search to perform, case-sensitive or not
     * @return Returns true if the string ends with InString
     */
    NODISCARD FORCEINLINE bool EndsWith(const CharType* InString, EStringCaseType SearchType = EStringCaseType::CaseSensitive) const
    {
        if (!InString)
        {
            return false;
        }

        return EndsWith(InString, CStringType::Strlen(InString), SearchType);
    }

    /**
     * @brief Check if string ends with a string
     * @param InString String to test for
     * @param SearchType Type of search to perform, case-sensitive or not
     * @return Returns true if the string ends with InString
     */
    template<typename StringType>
    NODISCARD FORCEINLINE bool EndsWith(const StringType& InString, EStringCaseType SearchType = EStringCaseType::CaseSensitive) const requires(TIsTStringType<StringType>::Value)
    {
        return EndsWith(InString.Data(), InString.Length(), SearchType);
    }

    /**
     * @brief Swap this view with another
     * @param Other View to swap with
     */
    FORCEINLINE void Swap(TStringView& Other)
    {
        ::Swap<const CharType*>(ViewStart, Other.ViewStart);
        ::Swap<const CharType*>(ViewEnd, Other.ViewEnd);
    }

    /**
     * @brief Create a sub-string view of this string
     * @param Offset Position to start the sub-string at
     * @param Count Number of characters in the sub-string
     * @return Returns a sub-string view
     */
    NODISCARD FORCEINLINE TStringView SubString(SizeType Offset, SizeType Count) const
    {
        CHECK(Offset < Length() && (Offset + Count <= Length()));
        return TStringView(ViewStart + Offset, Count);
    }

    /**
     * @brief Retrieve the data of the array
     * @return Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const CharType* Data() const
    {
        return ViewStart;
    }

    /**
     * @brief Returns the size of the view
     * @return The current size of the view
     */
    NODISCARD FORCEINLINE SizeType Size() const
    {
        return Length();
    }

    /**
     * @brief Returns the size of the view in bytes
     * @return The current size of the view in bytes
     */
    NODISCARD FORCEINLINE SizeType SizeInBytes() const
    {
        return Size() * sizeof(CharType);
    }

    /**
     * @brief Returns the length of the view
     * @return The current length of the view
     */
    NODISCARD FORCEINLINE SizeType Length() const
    {
        return static_cast<SizeType>(static_cast<PTR_INT>(ViewEnd - ViewStart));
    }

    /**
     * @brief Check if the view contains any elements
     * @return Returns true if the view is empty or false if it contains elements
     */
    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return Length() == 0;
    }

    /**
     * @brief Retrieve the last index that can be used to retrieve an element from the view
     * @return Returns the index to the last element of the view
     */
    NODISCARD FORCEINLINE SizeType LastElementIndex() const
    {
        const SizeType CurrentLength = Length();
        return (CurrentLength > 0) ? (CurrentLength - 1) : 0;
    }

    /**
     * @brief Retrieve the first element of the view
     * @return Returns a reference to the first element of the view
     */
    NODISCARD FORCEINLINE const CharType& FirstElement() const
    {
        CHECK(!IsEmpty());
        return ViewStart[0];
    }

    /**
     * @brief Retrieve the last element of the view
     * @return Returns a reference to the last element of the view
     */
    NODISCARD FORCEINLINE const CharType& LastElement() const
    {
        CHECK(!IsEmpty());
        return ViewStart[LastElementIndex()];
    }

public:

    /**
     * @brief Bracket operator to retrieve an element at a certain index
     * @param Index Index of the element to retrieve
     * @return A reference to the element at the index
     */
    NODISCARD FORCEINLINE const CharType& operator[](SizeType Index) const
    {
        CHECK(Index < Length());
        return ViewStart[Index];
    }

    /**
     * @brief Retrieve a null-terminated string
     * @return Returns a pointer containing a null-terminated string
     */
    NODISCARD FORCEINLINE const CharType* operator*() const
    {
        return (ViewStart == nullptr) ? CStringType::Empty() : ViewStart;
    }

    /**
     * @brief Copy-assignment operator
     * @param Other View to copy
     * @return Returns a reference to this instance
     */
    FORCEINLINE TStringView& operator=(const TStringView& Other)
    {
        TStringView(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief Move-assignment operator
     * @param Other View to move
     * @return Returns a reference to this instance
     */
    FORCEINLINE TStringView& operator=(TStringView&& Other)
    {
        TStringView(Move(Other)).Swap(*this);
        return *this;
    }

public:

    // Iterators
    NODISCARD FORCEINLINE ConstIteratorType Iterator() const
    {
        return ConstIteratorType(*this, 0);
    }

    NODISCARD FORCEINLINE ReverseConstIteratorType ReverseIterator() const
    {
        return ReverseConstIteratorType(*this, Size());
    }

public:

    // STL Iterator
    NODISCARD FORCEINLINE ConstIteratorType begin() const { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType end()   const { return ConstIteratorType(*this, Size()); }

private:
    const CharType* ViewStart{ nullptr };
    const CharType* ViewEnd{ nullptr };
};

using StringView     = TStringView<CHAR>;
using StringViewWide = TStringView<WIDECHAR>;

template<typename CharType>
NODISCARD inline bool operator==(const TStringView<CharType>& LHS, const CharType* RHS)
{
    return LHS.Compare(RHS) == 0;
}

template<typename CharType>
NODISCARD inline bool operator==(const CharType* LHS, const TStringView<CharType>& RHS)
{
    return RHS.Compare(LHS) == 0;
}

template<typename CharType>
NODISCARD inline bool operator==(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS)
{
    return LHS.Compare(RHS) == 0;
}

template<typename CharType>
NODISCARD inline bool operator!=(const TStringView<CharType>& LHS, const CharType* RHS)
{
    return !(LHS == RHS);
}

template<typename CharType>
NODISCARD inline bool operator!=(const CharType* LHS, const TStringView<CharType>& RHS)
{
    return !(LHS == RHS);
}

template<typename CharType>
NODISCARD inline bool operator!=(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS)
{
    return !(LHS == RHS);
}

template<typename CharType>
NODISCARD inline bool operator<(const TStringView<CharType>& LHS, const CharType* RHS)
{
    return LHS.Compare(RHS) < 0;
}

template<typename CharType>
NODISCARD inline bool operator<(const CharType* LHS, const TStringView<CharType>& RHS)
{
    return RHS.Compare(LHS) < 0;
}

template<typename CharType>
NODISCARD inline bool operator<(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS)
{
    return LHS.Compare(RHS) < 0;
}

template<typename CharType>
NODISCARD inline bool operator<=(const TStringView<CharType>& LHS, const CharType* RHS)
{
    return LHS.Compare(RHS) <= 0;
}

template<typename CharType>
NODISCARD inline bool operator<=(const CharType* LHS, const TStringView<CharType>& RHS)
{
    return RHS.Compare(LHS) <= 0;
}

template<typename CharType>
NODISCARD inline bool operator<=(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS)
{
    return LHS.Compare(RHS) <= 0;
}

template<typename CharType>
NODISCARD inline bool operator>(const TStringView<CharType>& LHS, const CharType* RHS)
{
    return LHS.Compare(RHS) > 0;
}

template<typename CharType>
NODISCARD inline bool operator>(const CharType* LHS, const TStringView<CharType>& RHS)
{
    return RHS.Compare(LHS) > 0;
}

template<typename CharType>
NODISCARD inline bool operator>(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS)
{
    return LHS.Compare(RHS) > 0;
}

template<typename CharType>
NODISCARD inline bool operator>=(const TStringView<CharType>& LHS, const CharType* RHS)
{
    return LHS.Compare(RHS) >= 0;
}

template<typename CharType>
NODISCARD inline bool operator>=(const CharType* LHS, const TStringView<CharType>& RHS)
{
    return RHS.Compare(LHS) >= 0;
}

template<typename CharType>
NODISCARD inline bool operator>=(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS)
{
    return LHS.Compare(RHS) >= 0;
}

template<typename CharType>
struct TIsTStringType<TStringView<CharType>>
{
    inline static constexpr bool Value = true;
};

template<typename CharType>
struct TIsContiguousContainer<TStringView<CharType>>
{
    inline static constexpr bool Value = true;
};

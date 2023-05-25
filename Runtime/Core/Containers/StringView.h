#pragma once
#include "Iterator.h"
#include "Core/Math/Math.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/CString.h"
#include "Core/Templates/ArrayContainerHelper.h"

enum class EStringCaseType
{
    NoCase        = 1,
    CaseSensitive = 2
};

template<typename InCharType>
class TStringView
{
    using FCStringType = TCString<InCharType>;
    using FCharType    = TChar<InCharType>;

public:
    using CHARTYPE = InCharType;
    using SizeType = int32;
    
    static_assert(TIsSame<CHARTYPE, CHAR>::Value || TIsSame<CHARTYPE, WIDECHAR>::Value, "TStringView only supports 'CHAR' and 'WIDECHAR'");
    static_assert(TIsSigned<SizeType>::Value, "TStringView only supports a SizeType that's signed");

    typedef TArrayIterator<const TStringView, const CHARTYPE>        ConstIteratorType;
    typedef TReverseArrayIterator<const TStringView, const CHARTYPE> ReverseConstIteratorType;

    enum : SizeType 
    { 
        INVALID_INDEX = SizeType(-1) 
    };

public:
    
    /** @brief - Default constructor */
    TStringView() noexcept = default;

    /**
     * @brief          - Create a view from a raw string
     * @param InString - String to view
     */
    FORCEINLINE TStringView(const CHARTYPE* InString) noexcept
        : ViewStart(InString)
        , ViewEnd(InString + FCStringType::Strlen(InString))
    {
    }

    /**
     * @brief          - Create a view from a raw string with a fixed length
     * @param InString - String to view
     * @param InLength - Length of the string to view
     * @param Offset   - Offset into the string
     */
    FORCEINLINE explicit TStringView(const CHARTYPE* InString, SizeType InLength, SizeType Offset = 0) noexcept
        : ViewStart(InString + Offset)
        , ViewEnd(ViewStart + InLength)
    {
    }

    /**
     * @brief          - Create a view from a string-type 
     * @param InString - String to view
     */
    template<typename StringType>
    FORCEINLINE explicit TStringView(const StringType& InString) noexcept requires(TIsTStringType<StringType>::Value)
        : ViewStart(FArrayContainerHelper::Data(InString))
        , ViewEnd(FArrayContainerHelper::Data(InString) + FArrayContainerHelper::Size(InString))
    {
    }

    /**
     * @brief       - Copy Constructor
     * @param Other - Other view to copy from
     */
    FORCEINLINE TStringView(const TStringView& Other) noexcept
        : ViewStart(Other.ViewStart)
        , ViewEnd(Other.ViewEnd)
    {
    }

    /**
     * @brief       - Move Constructor
     * @param Other - Other view to move from
     */
    FORCEINLINE TStringView(TStringView&& Other) noexcept
        : ViewStart(Other.ViewStart)
        , ViewEnd(Other.ViewEnd)
    {
        Other.ViewStart = nullptr;
        Other.ViewEnd   = nullptr;
    }

    /**
     * @brief - Clears the view
     */
    FORCEINLINE void Clear() noexcept
    {
        ViewStart = nullptr;
        ViewEnd   = nullptr;
    }

    /**
     * @brief            - Copy this string into buffer
     * @param Buffer     - Buffer to fill
     * @param BufferSize - Size of the buffer to fill
     * @param Position   - Offset to start copy from
     */
    FORCEINLINE void CopyToBuffer(CHARTYPE* Buffer, SizeType BufferSize, SizeType Position = INVALID_INDEX) const noexcept
    {
        CHECK(Buffer != nullptr);
        if (Buffer && BufferSize > 0)
        {
            const SizeType CopySize = NMath::Min(BufferSize, Length() - Position);
            FCStringType::Strncpy(Buffer, ViewStart + Position, CopySize);
        }
    }

    /**
     * @brief  - Removes whitespace from the beginning and end of the string and returns a copy
     * @return - Returns a copy of this string with the whitespace removed in the end and beginning
     */
    NODISCARD FORCEINLINE TStringView Trim() noexcept
    {
        TStringView NewStringView(*this);
        NewStringView.TrimInline();
        return NewStringView;
    }

    /**
     * @brief - Removes whitespace from the beginning and end of the string
     */
    FORCEINLINE void TrimInline() noexcept
    {
        TrimStartInline();
        TrimEndInline();
    }

    /**
     * @brief  - Removes whitespace from the beginning of the string and returns a copy
     * @return - Returns a copy of this string with all the whitespace removed from the beginning
     */
    NODISCARD FORCEINLINE TStringView TrimStart() noexcept
    {
        TStringView NewStringView(*this);
        NewStringView.TrimStartInline();
        return NewStringView;
    }

    /**
     * @brief - Removes whitespace from the beginning of the string
     */
    FORCEINLINE void TrimStartInline() noexcept
    {
        const CHARTYPE* RESTRICT Current = ViewStart;
        for (; Current != ViewEnd && *Current; ++Current)
        {
            if (!TChar<CHARTYPE>::IsWhitespace(*Current))
            {
                break;
            }
        }

        ViewStart = Current;
    }

    /**
     * @brief  - Removes whitespace from the end of the string and returns a copy
     * @return - Returns a copy of this string with all the whitespace removed from the end
     */
    NODISCARD FORCEINLINE TStringView TrimEnd() noexcept
    {
        TStringView NewStringView(*this);
        NewStringView.TrimEndInline();
        return NewStringView;
    }

    /**
     * @brief - Removes whitespace from the end of the string
     */
    FORCEINLINE void TrimEndInline() noexcept
    {
        const CHARTYPE* RESTRICT Current = ViewEnd;
        for (; Current != ViewStart && *Current;)
        {
            --Current;
            if (!TChar<CHARTYPE>::IsWhitespace(*Current))
            {
                break;
            }
        }

        ViewEnd = Current;
    }

    /**
     * @brief     - Shrink the view from the left and return a copy
     * @param Num - Number of characters to trim
     * @return    - Return a trimmed copy of the view
     */
    NODISCARD FORCEINLINE TStringView ShrinkLeft(int32 Num = 1) noexcept
    {
        TStringView NewStringView(*this);
        NewStringView.ShrinkLeftInline(Num);
        return NewStringView;
    }

    /**
     * @brief     - Shrink the view from the left
     * @param Num - Number of characters to trim
     */
    FORCEINLINE void ShrinkLeftInline(int32 Num = 1) noexcept
    {
        const CHARTYPE CurrentLength = Length();
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
     * @brief     - Shrink the view from the right and return a copy
     * @param Num - Number of characters to trim
     * @return    - Return a trimmed copy of the view
     */
    NODISCARD FORCEINLINE TStringView ShrinkRight(int32 Num = 1) noexcept
    {
        TStringView NewStringView(*this);
        NewStringView.ShrinkRightInline(Num);
        return NewStringView;
    }

    /**
     * @brief     - Shrink the view from the right
     * @param Num - Number of characters to trim
     */
    FORCEINLINE void ShrinkRightInline(int32 Num = 1) noexcept
    {
        const CHARTYPE CurrentLength = Length();
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
     * @brief          - Compares this string to another string-type
     * @param InString - String to compare with
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SizeType Compare(const StringType& InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept requires(TIsTStringType<StringType>::Value)
    {
        return Compare(InString.Data(), InString.Length(), CaseType);
    }

    /**
     * @brief          - Compares this string with a cstring
     * @param InString - String to compare with
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE SizeType Compare(const CHARTYPE* InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        if (CaseType == EStringCaseType::NoCase)
        {
            return static_cast<SizeType>(FCStringType::Stricmp(ViewStart, InString));
        }
        else
        {
            return static_cast<SizeType>(FCStringType::Strcmp(ViewStart, InString));
        }
    }

    /**
     * @brief          - Compares this string with a cstring of a fixed length
     * @param InString - String to compare with
     * @param InLength - Length of the string to compare
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE SizeType Compare(const CHARTYPE* InString, SizeType InLength, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        const SizeType MinLength = NMath::Min(Length(), InLength);
        if (CaseType == EStringCaseType::NoCase)
        {
            return static_cast<SizeType>(FCStringType::Strnicmp(ViewStart, InString, MinLength));
        }
        else
        {
            return static_cast<SizeType>(FCStringType::Strncmp(ViewStart, InString, MinLength));
        }
    }

    /**
     * @brief          - Compares this string to another string-type
     * @param InString - String to compare with
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns true if the strings are equal
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SizeType Equals(const StringType& InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept requires(TIsTStringType<StringType>::Value)
    {
        return Equals(InString.Data(), InString.Length(), CaseType);
    }

    /**
     * @brief          - Compares this string with a cstring
     * @param InString - String to compare with
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns true if the strings are equal
     */
    NODISCARD FORCEINLINE SizeType Equals(const CHARTYPE* InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        return Equals(InString, FCStringType::Strlen(InString), CaseType);
    }

    /**
     * @brief          - Compares this string with a cstring of a fixed length
     * @param InString - String to compare with
     * @param InLength - Length of the string to compare
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns true if the strings are equal
     */
    NODISCARD FORCEINLINE bool Equals(const CHARTYPE* InString, SizeType InLength, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        const SizeType CurrentLength = Length();
        if (CurrentLength != InLength)
        {
            return false;
        }

        if (CaseType == EStringCaseType::CaseSensitive)
        {
            return FCStringType::Strncmp(ViewStart, InString, CurrentLength) == 0;
        }
        else if (CaseType == EStringCaseType::NoCase)
        {
            return FCStringType::Strnicmp(ViewStart, InString, CurrentLength) == 0;
        }
        else
        {
            return false;
        }
    }

    /**
     * @brief          - Find the position of the first occurrence of the start of the search-string
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType Find(const CHARTYPE* InString, SizeType Position = INVALID_INDEX) const noexcept
    {
        const SizeType CurrentLength = Length();
        if (!CurrentLength)
        {
            return 0;
        }

        if (InString == nullptr)
        {
            return INVALID_INDEX;
        }

        SizeType Index = 0;
        if (Position == INVALID_INDEX && CurrentLength > 0)
        {
            Index += NMath::Clamp(0, CurrentLength - 1, Position);
        }

        const SizeType SearchLength = FCStringType::Strlen(InString);
        for (SizeType Index = Position; Index < CurrentLength; Index++)
        {
            SizeType SearchIndex = 0;
            for (; SearchIndex < InLength; ++SearchIndex)
            {
                if (ViewStart[Index + SearchIndex] != InString[SearchIndex])
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
     * @brief          - Find the position of the first occurrence of the start of the search-string
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search-string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SizeType Find(const StringType& InString, SizeType Position = INVALID_INDEX) const noexcept requires(TIsTStringType<StringType>::Value)
    {
        return Find(InString.Data(), Position);
    }

    /**
     * @brief          - Returns the position of the first occurrence of CHAR
     * @param Char     - Character to search for
     * @param Position - Position to start search at
     * @return         - Returns the position of the first occurrence of the CHAR
     */
    NODISCARD FORCEINLINE SizeType FindChar(CHARTYPE Char, SizeType Position = INVALID_INDEX) const noexcept
    {
        const SizeType CurrentLength = Length();
        if (!CurrentLength)
        {
            return 0;
        }

        const CHARTYPE* RESTRICT Current = ViewStart;
        if (Position != INVALID_INDEX && CurrentLength > 0)
        {
            Current += NMath::Clamp(0, CurrentLength - 1, Position);
        }

        for (const CHARTYPE *RESTRICT End = ViewStart + CurrentLength; Current != End; ++Current)
        {
            if (*Current == Char)
            {
                return static_cast<SizeType>(static_cast<intptr>(Current - ViewStart));
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief           - Returns the position of the first CHAR that passes the predicate
     * @param Predicate - Predicate that specifies valid chars
     * @param Position  - Position to start search at
     * @return          - Returns the position of the first occurrence of the CHAR
     */
    template<typename PredicateType>
    NODISCARD FORCEINLINE SizeType FindCharWithPredicate(PredicateType&& Predicate, SizeType Position = INVALID_INDEX) const noexcept
    {
        const SizeType CurrentLength = Length();
        if (!CurrentLength)
        {
            return 0;
        }

        const CHARTYPE* RESTRICT Current = ViewStart;
        if (Position != INVALID_INDEX && CurrentLength > 0)
        {
            Current += NMath::Clamp(0, CurrentLength - 1, Position);
        }

        for (const CHARTYPE *RESTRICT End = ViewStart + CurrentLength; Current != End; ++Current)
        {
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(static_cast<intptr>(Current - ViewStart));
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief          - Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType FindLast(const CHARTYPE* InString, SizeType Position = INVALID_INDEX) const noexcept
    {
        const SizeType CurrentLength = Length();
        if (!CurrentLength)
        {
            return 0;
        }

        if (InString == nullptr)
        {
            return INVALID_INDEX;
        }

        if (Position == INVALID_INDEX && Position > CurrentLength)
        {
            Position = CurrentLength;
        }

        const SizeType SearchLength = FCStringType::Strlen(InString);
        for (SizeType Index = Position - SearchLength; Index >= 0; Index++)
        {
            SizeType SearchIndex = 0;
            for (; SearchIndex < InLength; ++SearchIndex)
            {
                if (ViewStart[Index + SearchIndex] != InString[SearchIndex])
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
     * @brief          - Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search-string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SizeType FindLast(const StringType& InString, SizeType Position = INVALID_INDEX) const noexcept requires(TIsTStringType<StringType>::Value)
    {
        return FindLast(InString.Data(), Position);
    }

    /**
     * @brief          - Returns the position of the first occurrence of CHAR. Searches the string in reverse.
     * @param Char     - Character to search for
     * @param Position - Position to start search at
     * @return         - Returns the position of the first occurrence of the CHAR
     */
    NODISCARD FORCEINLINE SizeType FindLastChar(CHARTYPE Char, SizeType Position = INVALID_INDEX) const noexcept
    {
        const SizeType CurrentLength = Length();
        if (!CurrentLength)
        {
            return 0;
        }

        const CHARTYPE* RESTRICT Current = ViewStart;
        if (Position != INVALID_INDEX && CurrentLength > 0)
        {
            Current += NMath::Clamp(0, CurrentLength - 1, Position);
        }

        for (const CHARTYPE* RESTRICT End = ViewStart; Current != End;)
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
     * @brief           - Returns the position of the first CHAR that passes the predicate
     * @param Predicate - Predicate that specifies valid chars
     * @param Position  - Position to start search at
     * @return          - Returns the position of the first occurrence of the CHAR
     */
    template<typename PredicateType>
    NODISCARD FORCEINLINE SizeType FindLastCharWithPredicate(PredicateType&& Predicate, SizeType Position = INVALID_INDEX) const noexcept
    {
        const SizeType CurrentLength = Length();
        if (!CurrentLength)
        {
            return 0;
        }

        const CHARTYPE* RESTRICT Current = ViewStart;
        if (Position != INVALID_INDEX && CurrentLength > 0)
        {
            Current += NMath::Clamp(0, CurrentLength - 1, Position);
        }

        for (const CHARTYPE* RESTRICT End = ViewStart; Current != End;)
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
     * @brief          - Check if the search-string exists within the view
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(const CHARTYPE* InString, SizeType Position = INVALID_INDEX) const noexcept
    {
        return Find(InString, FCStringType::Strlen(InString), Position) != INVALID_INDEX;
    }

    /**
     * @brief          - Check if the search-string exists within the view. The string is of a string-type.
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    template<typename StringType>
    NODISCARD FORCEINLINE bool Contains(const StringType& InString, SizeType Position = INVALID_INDEX) const noexcept requires(TIsTStringType<StringType>::Value)
    {
        return Find(InString, Position) != INVALID_INDEX;
    }

    /**
     * @brief          - Check if the character exists within the view
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(CHARTYPE Char, SizeType Position = INVALID_INDEX) const noexcept
    {
        return FindChar(Char, Position) != INVALID_INDEX;
    }

    /**
     * @brief            - Check if string begins with a string
     * @param InString   - String to test for
     * @param SearchType - Type of search to perform, case-sensitive or not
     * @return           - Returns true if the string begins with InString
     */
    NODISCARD FORCEINLINE bool StartsWith(const CHARTYPE* InString, EStringCaseType SearchType = EStringCaseType::CaseSensitive) const noexcept
    {
        if (!InString)
        {
            return false;
        }

        const SizeType SuffixLength = FCStringType::Strlen(InString);
        if (SuffixLength > 0)
        {
            if (SearchType == EStringCaseType::CaseSensitive)
            {
                return FCStringType::Strncmp(ViewStart, InString, SuffixLength) == 0;
            }
            else if (SearchType == EStringCaseType::NoCase)
            {
                return FCStringType::Strnicmp(ViewStart, InString, SuffixLength) == 0;
            }
        }

        return false;
    }

    /**
     * @brief            - Check if string end with a string
     * @param InString   - String to test for
     * @param SearchType - Type of search to perform, case-sensitive or not
     * @return           - Returns true if the string ends with InString
     */
    NODISCARD FORCEINLINE bool EndsWith(const CHARTYPE* InString, EStringCaseType SearchType = EStringCaseType::CaseSensitive) const noexcept
    {
        if (!InString)
        {
            return false;
        }

        const SizeType CurrentLength = Length();
        const SizeType SuffixLength  = FCStringType::Strlen(InString);
        if (SuffixLength > 0 && CurrentLength > SuffixLength)
        {
            const CHARTYPE* StringData = ViewStart + (CurrentLength - SuffixLength);
            if (SearchType == EStringCaseType::CaseSensitive)
            {
                return FCStringType::Strncmp(ViewStart, InString, SuffixLength) == 0;
            }
            else if (SearchType == EStringCaseType::NoCase)
            {
                return FCStringType::Strnicmp(ViewStart, InString, SuffixLength) == 0;
            }
        }

        return false;
    }

    /**
     * @brief       - Swap this view with another
     * @param Other - String to swap with
     */
    FORCEINLINE void Swap(TStringView& Other) noexcept
    {
        ::Swap<const CHARTYPE*>(ViewStart, Other.ViewStart);
        ::Swap<const CHARTYPE*>(ViewEnd, Other.ViewEnd);
    }

    /**
    * @brief               - Create a sub-string view of this string
    * @param Position      - Position to start the sub-string at
    * @param NumCharacters - Number of characters in the sub-string
    * @return              - Returns a sub-string view
    */
    NODISCARD FORCEINLINE TStringView SubString(SizeType Offset, SizeType Count) const noexcept
    {
        CHECK(Count < Length() && (Offset + Count <= Length()));
        return TStringView(ViewStart + Offset, Count);
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const CHARTYPE* Data() const noexcept
    {
        return ViewStart;
    }

    /**
     * @brief  - Retrieve a null-terminated string
     * @return - Returns a pointer containing a null-terminated string
     */
    NODISCARD FORCEINLINE const CHARTYPE* GetCString() const noexcept
    {
        return (ViewStart == nullptr) ? FCStringType::Empty() : ViewStart;
    }

    /**
     * @brief  - Returns the size of the view
     * @return - The current size of the view
     */
    NODISCARD FORCEINLINE SizeType Size() const noexcept
    {
        return Length();
    }

    /**
     * @brief  - Returns the size of the container in bytes
     * @return - The current size of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Size() * sizeof(CHARTYPE);
    }

    /**
     * @brief  - Returns the length of the view
     * @return - The current length of the view
     */
    NODISCARD FORCEINLINE SizeType Length() const noexcept
    {
        return static_cast<SizeType>(static_cast<intptr>(ViewEnd - ViewStart));
    }

    /**
     * @brief  - Check if the container contains any elements
     * @return - Returns true if the array is empty or false if it contains elements
     */
    NODISCARD FORCEINLINE bool IsEmpty() const noexcept
    {
        return Length() == 0;
    }

    /**
     * @brief  - Retrieve the last index that can be used to retrieve an element from the view
     * @return - Returns a the index to the last element of the view
     */
    NODISCARD FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        const SizeType CurrentLength = Length();
        return (CurrentLength > 0) ? (CurrentLength - 1) : 0;
    }

    /**
     * @brief  - Retrieve the first element of the view
     * @return - Returns a reference to the first element of the view
     */
    NODISCARD FORCEINLINE const CHARTYPE& FirstElement() const noexcept
    {
        CHECK(!IsEmpty());
        return ViewStart[0];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the view
     */
    NODISCARD FORCEINLINE const CHARTYPE& LastElement() const noexcept
    {
        CHECK(!IsEmpty());
        return ViewStart[LastElementIndex()];
    }

public:

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const CHARTYPE& operator[](SizeType Index) const noexcept
    {
        CHECK(Index < Length());
        return ViewStart[Index];
    }

    /**
     * @brief       - Copy-assignment operator
     * @param Other - String to copy
     * @return      - Return a reference to this instance
     */
    FORCEINLINE TStringView& operator=(const TStringView& Other) noexcept
    {
        TStringView(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - String to move
     * @return      - Return a reference to this instance
     */
    FORCEINLINE TStringView& operator=(TStringView&& Other) noexcept
    {
        TStringView(::Move(Other)).Swap(*this);
        return *this;
    }

public: // Iterators

    /**
     * @brief  - Retrieve an iterator to the beginning of the array
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType Iterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ReverseIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, Size());
    }

public: // STL Iterator
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType end()   const noexcept { return ConstIteratorType(*this, Size()); }

private:
    const CHARTYPE* ViewStart{ nullptr };
    const CHARTYPE* ViewEnd{ nullptr };
};


using FStringView     = TStringView<CHAR>;
using FStringViewWide = TStringView<WIDECHAR>;


template<typename CHARTYPE>
NODISCARD inline bool operator==(const TStringView<CHARTYPE>& LHS, const CHARTYPE* RHS) noexcept
{
    return LHS.Compare(RHS) == 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator==(const CHARTYPE* LHS, const TStringView<CHARTYPE>& RHS) noexcept
{
    return RHS.Compare(LHS) == 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator==(const TStringView<CHARTYPE>& LHS, const TStringView<CHARTYPE>& RHS) noexcept
{
    return LHS.Compare(RHS) == 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator!=(const TStringView<CHARTYPE>& LHS, const CHARTYPE* RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CHARTYPE>
NODISCARD inline bool operator!=(const CHARTYPE* LHS, const TStringView<CHARTYPE>& RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CHARTYPE>
NODISCARD inline bool operator!=(const TStringView<CHARTYPE>& LHS, const TStringView<CHARTYPE>& RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CHARTYPE>
NODISCARD inline bool operator<(const TStringView<CHARTYPE>& LHS, const CHARTYPE* RHS) noexcept
{
    return LHS.Compare(RHS) < 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator<(const CHARTYPE* LHS, const TStringView<CHARTYPE>& RHS) noexcept
{
    return RHS.Compare(LHS) < 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator<(const TStringView<CHARTYPE>& LHS, const TStringView<CHARTYPE>& RHS) noexcept
{
    return LHS.Compare(RHS) < 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator<=(const TStringView<CHARTYPE>& LHS, const CHARTYPE* RHS) noexcept
{
    return LHS.Compare(RHS) <= 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator<=(const CHARTYPE* LHS, const TStringView<CHARTYPE>& RHS) noexcept
{
    return RHS.Compare(LHS) <= 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator<=(const TStringView<CHARTYPE>& LHS, const TStringView<CHARTYPE>& RHS) noexcept
{
    return LHS.Compare(RHS) <= 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator>(const TStringView<CHARTYPE>& LHS, const CHARTYPE* RHS) noexcept
{
    return LHS.Compare(RHS) > 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator>(const CHARTYPE* LHS, const TStringView<CHARTYPE>& RHS) noexcept
{
    return RHS.Compare(LHS) > 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator>(const TStringView<CHARTYPE>& LHS, const TStringView<CHARTYPE>& RHS) noexcept
{
    return LHS.Compare(RHS) > 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator>=(const TStringView<CHARTYPE>& LHS, const CHARTYPE* RHS) noexcept
{
    return LHS.Compare(RHS) >= 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator>=(const CHARTYPE* LHS, const TStringView<CHARTYPE>& RHS) noexcept
{
    return RHS.Compare(LHS) >= 0;
}

template<typename CHARTYPE>
NODISCARD inline bool operator>=(const TStringView<CHARTYPE>& LHS, const TStringView<CHARTYPE>& RHS) noexcept
{
    return LHS.Compare(RHS) >= 0;
}


template<typename CHARTYPE>
struct TIsTStringType<TStringView<CHARTYPE>>
{
    inline static constexpr bool Value = true;
};

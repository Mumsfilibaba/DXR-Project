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
public:
    using CharType = InCharType;
    using SizeType = int32;
    
    static_assert(TIsSame<CharType, CHAR>::Value || TIsSame<CharType, WIDECHAR>::Value, "TStringView only supports 'CHAR' and 'WIDECHAR'");
    static_assert(TIsSigned<SizeType>::Value, "TStringView only supports a SizeType that's signed");

    typedef TArrayIterator<const TStringView, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<const TStringView, const CharType> ReverseConstIteratorType;

    enum : SizeType 
    { 
        INVALID_INDEX = SizeType(-1) 
    };

public:
    
    /**
     * @brief - Default constructor
     */
    FORCEINLINE TStringView() noexcept
        : ViewStart(nullptr)
        , ViewEnd(nullptr)
    {
    }

    /**
     * @brief          - Create a view from a raw string
     * @param InString - String to view
     */
    FORCEINLINE TStringView(const CharType* InString) noexcept
        : ViewStart(InString)
        , ViewEnd(InString + TCString<CharType>::Strlen(InString))
    {
    }

    /**
     * @brief          - Create a view from a raw string with a fixed length
     * @param InString - String to view
     * @param InLength - Length of the string to view
     * @param Offset   - Offset into the string
     */
    FORCEINLINE explicit TStringView(const CharType* InString, SizeType InLength, SizeType Offset = 0) noexcept
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
    FORCEINLINE void CopyToBuffer(CharType* Buffer, SizeType BufferSize, SizeType Position = 0) const noexcept
    {
        CHECK(Buffer != nullptr);
        CHECK((Position < Length()) || (Position == 0));
        const SizeType CurrentLen = Length();
        const SizeType CopySize   = NMath::Min(BufferSize, CurrentLen - Position);
        TCString<CharType>::Strncpy(Buffer, ViewStart + Position, CopySize);
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
        const CharType* RESTRICT Current = ViewStart;
        for (; Current != ViewEnd && *Current; ++Current)
        {
            if (!TChar<CharType>::IsSpace(*Current))
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
        const CharType* RESTRICT Current = ViewEnd;
        for (; Current != ViewStart && *Current;)
        {
            --Current;
            if (!TChar<CharType>::IsSpace(*Current))
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
        const CharType CurrentLength = Length();
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
        const CharType CurrentLength = Length();
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
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    template<typename StringType>
    NODISCARD FORCEINLINE int32 Compare(const StringType& InString) const noexcept requires(TIsTStringType<StringType>::Value)
    {
        return Compare(InString.GetCString(), InString.Length());
    }

    /**
     * @brief          - Compares this string with a raw-string
     * @param InString - String to compare with
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE int32 Compare(const CharType* InString) const noexcept
    {
        return Compare(InString, TCString<CharType>::Strlen(InString));
    }

    /**
     * @brief          - Compares this string with a raw-string of a fixed length
     * @param InString - String to compare with
     * @param InLength - Length of the string to compare
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE int32 Compare(const CharType* InString, SizeType InLength) const noexcept
    {
        const SizeType CurrentLength = Length();
        if (CurrentLength != InLength)
        {
            return -1;
        }
        else if (CurrentLength == 0)
        {
            return 0;
        }

        for (const CharType* RESTRICT Current = ViewStart; Current != ViewEnd; ++Current)
        {
            if (*Current != *InString)
            {
                return *Current - *InString;
            }

            InString++;
        }

        return 0;
    }

    /**
     * @brief          - Compares this string to another string-type without taking casing into account.
     * @param InString - String to compare with
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    template<typename StringType>
    NODISCARD FORCEINLINE int32 CompareNoCase(const StringType& InString) const noexcept requires(TIsTStringType<StringType>::Value)
    {
        return CompareNoCase(InString.GetCString(), InString.Length());
    }

    /**
     * @brief          - Compares this string with a raw-string without taking casing into account.
     * @param InString - String to compare with
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE int32 CompareNoCase(const CharType* InString) const noexcept
    {
        return CompareNoCase(InString, TCString<CharType>::Strlen(InString));
    }

    /**
     * @brief          - Compares this string with a raw-string without taking casing into account.
     * @param InString - String to compare with
     * @param InLength - Length of the string to compare
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE int32 CompareNoCase(const CharType* InString, SizeType InLength) const noexcept
    {
        const SizeType CurrentLength = Length();
        if (CurrentLength != InLength)
        {
            return -1;
        }
        else if (CurrentLength == 0)
        {
            return 0;
        }

        for (const CharType* RESTRICT Current = ViewStart; Current != ViewEnd; ++Current)
        {
            if (TChar<CharType>::ToLower(*Current) != TChar<CharType>::ToLower(*InString))
            {
                return *Current - *InString;
            }

            InString++;
        }

        return 0;
    }

    /**
     * @brief          - Find the position of the first occurrence of the start of the search-string
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType Find(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return Find(InString, TCString<CharType>::Strlen(InString), Position);
    }

    /**
     * @brief          - Find the position of the first occurrence of the start of the search-string
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search-string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE SizeType Find(const StringType& InString, SizeType Position = 0) const noexcept requires(TIsTStringType<StringType>::Value)
    {
        return Find(InString.Data(), InString.Length(), Position);
    }

    /**
     * @brief          - Find the position of the first occurrence of the start of the search-string
     * @param InString - String to search
     * @param InString - Length of the search-string
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType Find(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        CHECK((Position < Length()) || (Position == 0));

        if (InLength == 0 || Length() == 0)
        {
            return 0;
        }

        if (InString == nullptr)
        {
            return INVALID_INDEX;
        }

        for (const CharType* RESTRICT SearchEnd = InString + InLength, *RESTRICT Current = GetCString() + Position; Current != ViewEnd; ++Current)
        {
            const CharType* RESTRICT SearchString = InString;
            const CharType* RESTRICT TempCurrent  = Current;
            while (true)
            {
                if (*SearchString == 0 || SearchString == SearchEnd)
                {
                    return static_cast<SizeType>(static_cast<intptr>(Current - GetCString()));
                }
                else if (*TempCurrent != *SearchString)
                {
                    break;
                }

                ++TempCurrent;
                ++SearchString;
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief          - Returns the position of the first occurrence of CHAR
     * @param Char     - Character to search for
     * @param Position - Position to start search at
     * @return         - Returns the position of the first occurrence of the CHAR
     */
    NODISCARD FORCEINLINE SizeType FindChar(CharType Char, SizeType Position = 0) const noexcept
    {
        CHECK((Position < Length()) || (Position == 0));

        const SizeType CurrentLength = Length();
        if (CurrentLength == 0)
        {
            return 0;
        }

        for (const CharType* RESTRICT Start = GetCString() + Position, *RESTRICT Current = Start, *RESTRICT End = GetCString() + CurrentLength; Current != End; ++Current)
        {
            if (Char == *Current)
            {
                return static_cast<SizeType>(Current - Start);
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
    NODISCARD FORCEINLINE SizeType FindCharWithPredicate(PredicateType&& Predicate, SizeType Position = 0) const noexcept
    {
        CHECK((Position < Length()) || (Position == 0));

        const SizeType CurrentLength = Length();
        if (CurrentLength == 0)
        {
            return 0;
        }

        for (const CharType* RESTRICT Start = GetCString() + Position, *RESTRICT Current = Start, *RESTRICT End = GetCString() + CurrentLength; Current != End; ++Current)
        {
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(Current - Start);
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
    NODISCARD FORCEINLINE SizeType FindLast(const CharType* InString, SizeType Position = 0) const noexcept
    {
        CHECK((Position < Length()) || (Position == 0));
        CHECK(InString != nullptr);

        const SizeType CurrentLength = Length();
        if (CurrentLength == 0)
        {
            return 0;
        }

        if (!Position)
        {
            Position = CurrentLength;
        }

        for (const CharType* RESTRICT End = GetCString(), *RESTRICT Start = End + Position, *RESTRICT Current = Start; Current != End; ++Current)
        {
            Current--;

            const CharType* RESTRICT SearchString = InString;
            const CharType* RESTRICT TempCurrent  = Current;
            while (true)
            {
                if ((*SearchString == 0))
                {
                    return static_cast<SizeType>(static_cast<intptr>(Current - End));
                }
                else if (*TempCurrent != *SearchString)
                {
                    break;
                }

                ++TempCurrent;
                ++SearchString;
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
    NODISCARD FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type FindLast(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return FindLast(InString, InString.Length(), Position);
    }

    /**
     * @brief - Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     *     Position is the end, instead of the start as with normal Find.
     * 
     * @param InString - String to search
     * @param InString - Length of the search-string
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType FindLast(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        CHECK((Position < Length()) || (Position == 0));
        CHECK(InString != nullptr);

        const SizeType CurrentLength = Length();
        if (CurrentLength == 0)
        {
            return 0;
        }

        if (!Position)
        {
            Position = CurrentLength;
        }

        for (const CharType* RESTRICT SearchEnd = InString + InLength, *RESTRICT End = GetCString(), *RESTRICT Start = End + Position, *RESTRICT Current = Start; Current != End;)
        {
            Current--;

            const CharType* RESTRICT SearchString = InString;
            const CharType* RESTRICT TempCurrent  = Current;
            while (true)
            {
                if ((*SearchString == 0) || (SearchString == SearchEnd))
                {
                    return static_cast<SizeType>(static_cast<intptr>(Current - End));
                }
                else if (*TempCurrent != *SearchString)
                {
                    break;
                }

                ++TempCurrent;
                ++SearchString;
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief          - Returns the position of the first occurrence of CHAR. Searches the string in reverse.
     * @param Char     - Character to search for
     * @param Position - Position to start search at
     * @return         - Returns the position of the first occurrence of the CHAR
     */
    NODISCARD FORCEINLINE SizeType FindLastChar(CharType Char, SizeType Position = 0) const noexcept
    {
        CHECK((Position < Length()) || (Position == 0));

        const SizeType CurrentLength = Length();
        if (CurrentLength == 0)
        {
            return 0;
        }

        if (!Position)
        {
            Position = CurrentLength;
        }

        for (const CharType* RESTRICT End = GetCString(), *RESTRICT Start = End + Position, *RESTRICT Current = Start; Current != End;)
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
    NODISCARD FORCEINLINE SizeType FindLastCharWithPredicate(PredicateType&& Predicate, SizeType Position = 0) const noexcept
    {
        CHECK((Position < Length()) || (Position == 0));

        const SizeType CurrentLength = Length();
        if (CurrentLength == 0)
        {
            return 0;
        }

        if (!Position)
        {
            Position = Length;
        }

        for (const CharType* RESTRICT End = GetCString(), *RESTRICT Start = End + Position, *RESTRICT Current = Start; Current != End;)
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
    NODISCARD FORCEINLINE bool Contains(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return Find(InString, TCString<CharType>::Strlen(InString), Position) != INVALID_INDEX;
    }

    /**
     * @brief          - Check if the search-string exists within the view. The string is of a string-type.
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    template<typename StringType>
    NODISCARD FORCEINLINE bool Contains(const StringType& InString, SizeType Position = 0) const noexcept requires(TIsTStringType<StringType>::Value)
    {
        return Find(InString, Position) != INVALID_INDEX;
    }

    /**
     * @brief          - Check if the search-string exists within the view
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        return Find(InString, InLength, Position) != INVALID_INDEX;
    }

    /**
     * @brief          - Check if the character exists within the view
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(CharType Char, SizeType Position = 0) const noexcept
    {
        return FindChar(Char, Position) != INVALID_INDEX;
    }

    /**
     * @brief          - Check if string begins with a string
     * @param InString - String to test for
     * @return         - Returns true if the string begins with InString
     */
    NODISCARD FORCEINLINE bool StartsWith(const CharType* InString) const noexcept
    {
        const SizeType SuffixLength = TCString<CharType>::Strlen(InString);
        return (SuffixLength > 0) && !TCString<CharType>::Strncmp(GetCString(), InString, SuffixLength);
    }

    /**
     * @brief          - Check if string begins with a string
     * @param InString - String to test for
     * @return         - Returns true if the string begins with InString
     */
    NODISCARD FORCEINLINE bool StartsWithNoCase(const CharType* InString) const noexcept
    {
        const SizeType SuffixLength = TCString<CharType>::Strlen(InString);
        return (SuffixLength > 0) && !TCString<CharType>::Strnicmp(GetCString(), InString, SuffixLength);
    }

    /**
     * @brief          - Check if string end with a string
     * @param InString - String to test for
     * @return         - Returns true if the string ends with InString
     */
    NODISCARD FORCEINLINE bool EndsWith(const CharType* InString) const noexcept
    {
        if (!InString || (*InString != 0))
        {
            return false;
        }

        const SizeType CurrentLength = Length();
        const SizeType SuffixLength = TCString<CharType>::Strlen(InString);
        if (SuffixLength > CurrentLength)
        {
            return false;
        }

        const CharType* TempData = ViewStart + (CurrentLength - SuffixLength);
        return TCString<CharType>::Strcmp(TempData, InString) == 0;
    }

    /**
     * @brief          - Check if string end with a string without taking casing into account
     * @param InString - String to test for
     * @return         - Returns true if the string ends with InString
     */
    NODISCARD FORCEINLINE bool EndsWithNoCase(const CharType* InString) const noexcept
    {
        if (!InString || *InString == 0)
        {
            return false;
        }

		const SizeType CurrentLength = Length();
		const SizeType SuffixLength = TCString<CharType>::Strlen(InString);
		if (SuffixLength > CurrentLength)
		{
			return false;
		}

		const CharType* TempData = ViewStart + (CurrentLength - SuffixLength);
		return TCString<CharType>::Stricmp(TempData, InString) == 0;
    }

    /**
     * @brief       - Swap this view with another
     * @param Other - String to swap with
     */
    FORCEINLINE void Swap(TStringView& Other) noexcept
    {
        ::Swap<const CharType*>(ViewStart, Other.ViewStart);
        ::Swap<const CharType*>(ViewEnd, Other.ViewEnd);
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
    NODISCARD FORCEINLINE const CharType* Data() const noexcept
    {
        return ViewStart;
    }

    /**
     * @brief  - Retrieve a null-terminated string
     * @return - Returns a pointer containing a null-terminated string
     */
    NODISCARD FORCEINLINE const CharType* GetCString() const noexcept
    {
        return (ViewStart == nullptr) ? TCString<CharType>::Empty() : ViewStart;
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
        return Size() * sizeof(CharType);
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
    NODISCARD FORCEINLINE const CharType& FirstElement() const noexcept
    {
        CHECK(!IsEmpty());
        return *ViewStart;
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the view
     */
    NODISCARD FORCEINLINE const CharType& LastElement() const noexcept
    {
        CHECK(!IsEmpty());
        return *(ViewStart + LastElementIndex());
    }

public:

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const CharType& operator[](SizeType Index) const noexcept
    {
        CHECK(Index < Length());
        return *(ViewStart + Index);
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
    const CharType* ViewStart = nullptr;
    const CharType* ViewEnd   = nullptr;
};


using FStringView     = TStringView<CHAR>;
using FStringViewWide = TStringView<WIDECHAR>;


template<typename CharType>
NODISCARD
inline bool operator==(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) == 0);
}

template<typename CharType>
NODISCARD
inline bool operator==(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) == 0);
}

template<typename CharType>
NODISCARD
inline bool operator==(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) == 0);
}

template<typename CharType>
NODISCARD
inline bool operator!=(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CharType>
NODISCARD
inline bool operator!=(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CharType>
NODISCARD
inline bool operator!=(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CharType>
NODISCARD
inline bool operator<(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) < 0);
}

template<typename CharType>
NODISCARD
inline bool operator<(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) < 0);
}

template<typename CharType>
NODISCARD
inline bool operator<(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) < 0);
}

template<typename CharType>
NODISCARD
inline bool operator<=(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) <= 0);
}

template<typename CharType>
NODISCARD
inline bool operator<=(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) <= 0);
}

template<typename CharType>
NODISCARD
inline bool operator<=(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) <= 0);
}

template<typename CharType>
NODISCARD
inline bool operator>(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) > 0);
}

template<typename CharType>
NODISCARD
inline bool operator>(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) > 0);
}

template<typename CharType>
NODISCARD
inline bool operator>(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) > 0);
}

template<typename CharType>
NODISCARD
inline bool operator>=(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) >= 0);
}

template<typename CharType>
NODISCARD
inline bool operator>=(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) >= 0);
}

template<typename CharType>
NODISCARD
inline bool operator>=(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) >= 0);
}


template<typename CharType>
struct TIsTStringType<TStringView<CharType>>
{
    inline static constexpr bool Value = true;
};

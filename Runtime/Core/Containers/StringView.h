#pragma once
#include "Iterator.h"

#include "Core/Math/Math.h"
#include "Core/Templates/IsTStringType.h"
#include "Core/Templates/IsSame.h"
#include "Core/Templates/Move.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/IsTStringType.h"
#include "Core/Templates/CString.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TStringView - Class for viewing a string similar to std::string_view

template<typename InCharType>
class TStringView
{
public:
    using CharType = InCharType;
    using SizeType = int32;
    
    static_assert(
        TIsSame<CharType, CHAR>::Value || TIsSame<CharType, WIDECHAR>::Value,
        "TStringView only supports 'CHAR' and 'WIDECHAR'");
    static_assert(
        TIsSigned<SizeType>::Value,
        "TStringView only supports a SizeType that's signed");

    /* Iterators */
    typedef TArrayIterator<const TStringView, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<const TStringView, const CharType> ReverseConstIteratorType;

    enum { INVALID_INDEX = SizeType(-1) };

public:
    
    /**
     * @brief: Default constructor
     */
    FORCEINLINE TStringView() noexcept
        : ViewStart(nullptr)
        , ViewEnd(nullptr)
    { }

    /**
     * @brief: Create a view from a raw string
     * 
     * @param InString: String to view
     */
    FORCEINLINE TStringView(const CharType* InString) noexcept
        : ViewStart(InString)
        , ViewEnd(InString + TCString<CharType>::Strlen(InString))
    { }

    /**
     * @brief: Create a view from a raw string with a fixed length
     *
     * @param InString: String to view
     * @param InLength: Length of the string to view
     */
    FORCEINLINE explicit TStringView(const CharType* InString, SizeType InLength) noexcept
        : ViewStart(InString)
        , ViewEnd(InString + InLength)
    { }

    /**
     * @brief: Create a view from a string-type 
     * 
     * @param InString: String to view
     */
    template<
        typename StringType,
        typename = typename TEnableIf<TIsTStringType<StringType>::Value>::Type>
    FORCEINLINE explicit TStringView(const StringType& InString) noexcept
        : ViewStart(InString.GetCString())
        , ViewEnd(InString.GetCString() + InString.GetLength())
    { }

    /**
     * @brief: Copy Constructor
     *
     * @param Other: Other view to copy from
     */
    FORCEINLINE TStringView(const TStringView& Other) noexcept
        : ViewStart(Other.ViewStart)
        , ViewEnd(Other.ViewEnd)
    { }

    /**
     * @brief: Move Constructor
     *
     * @param Other: Other view to move from
     */
    FORCEINLINE TStringView(TStringView&& Other) noexcept
        : ViewStart(Other.ViewStart)
        , ViewEnd(Other.ViewEnd)
    {
        Other.ViewStart = nullptr;
        Other.ViewEnd   = nullptr;
    }

    /**
     * @brief: Clears the view
     */
    FORCEINLINE void Clear() noexcept
    {
        ViewStart = nullptr;
        ViewEnd   = nullptr;
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
        Check((Position < GetLength()) || (Position == 0));

        const SizeType CopySize = NMath::Min(BufferSize, GetLength() - Position);
        TCString<CharType>::Strncpy(Buffer, ViewStart + Position, CopySize);
    }

    /**
     * @brief: Removes whitespace from the beginning and end of the string and returns a copy
     *
     * @return: Returns a copy of this string with the whitespace removed in the end and beginning
     */
    NODISCARD FORCEINLINE TStringView Trim() noexcept
    {
        TStringView NewStringView(*this);
        NewStringView.TrimInline();
        return NewStringView;
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
     * @brief: Removes whitespace from the beginning of the string and returns a copy
     *
     * @return: Returns a copy of this string with all the whitespace removed from the beginning
     */
    NODISCARD FORCEINLINE TStringView TrimStart() noexcept
    {
        TStringView NewStringView(*this);
        NewStringView.TrimStartInline();
        return NewStringView;
    }

    /**
     * @brief: Removes whitespace from the beginning of the string
     */
    FORCEINLINE void TrimStartInline() noexcept
    {
        const CharType* Current = ViewStart;
        while (Current != ViewEnd)
        {
            if (!TChar<CharType>::IsSpace(*Current) && !TChar<CharType>::IsZero(*Current))
            {
                break;
            }

            ++Current;
        }

        ViewStart = Current;
    }

    /**
     * @brief: Removes whitespace from the end of the string and returns a copy
     *
     * @return: Returns a copy of this string with all the whitespace removed from the end
     */
    NODISCARD FORCEINLINE TStringView TrimEnd() noexcept
    {
        TStringView NewStringView(*this);
        NewStringView.TrimEndInline();
        return NewStringView;
    }

    /**
     * @brief: Removes whitespace from the end of the string
     */
    FORCEINLINE void TrimEndInline() noexcept
    {
        const CharType* Current = ViewEnd;
        while (Current != ViewStart)
        {
            Current--;
            if (!TChar<CharType>::IsSpace(*Current) && !TChar<CharType>::IsZero(*Current))
            {
                break;
            }
        }

        ViewEnd = Current;
    }

    /**
     * @brief: Shrink the view from the left and return a copy
     * 
     * @param Num: Number of characters to trim
     * @return: Return a trimmed copy of the view
     */
    NODISCARD FORCEINLINE TStringView ShrinkLeft(int32 Num = 1) noexcept
    {
        TStringView NewStringView(*this);
        NewStringView.ShrinkLeftInline(Num);
        return NewStringView;
    }

    /**
     * @brief: Shrink the view from the left
     *
     * @param Num: Number of characters to trim
     */
    FORCEINLINE void ShrinkLeftInline(int32 Num = 1) noexcept
    {
        if (GetLength() <= Num)
        {
            ViewStart = ViewEnd;
        }
        else
        {
            ViewStart += Num;
        }
    }

    /**
     * @brief: Shrink the view from the right and return a copy
     *
     * @param Num: Number of characters to trim
     * @return: Return a trimmed copy of the view
     */
    NODISCARD FORCEINLINE TStringView ShrinkRight(int32 Num = 1) noexcept
    {
        TStringView NewStringView(*this);
        NewStringView.ShrinkRightInline(Num);
        return NewStringView;
    }

    /**
     * @brief: Shrink the view from the right
     *
     * @param Num: Number of characters to trim
     */
    FORCEINLINE void ShrinkRightInline(int32 Num = 1) noexcept
    {
        if (GetLength() <= Num)
        {
            ViewEnd = ViewStart;
        }
        else
        {
            ViewEnd -= Num;
        }
    }

    /**
     * @brief: Compares this string to another string-type
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    template<typename StringType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, int32>::Type Compare(const StringType& InString) const noexcept
    {
        return Compare(InString.GetCString(), InString.GetLength());
    }

    /**
     * @brief: Compares this string with a raw-string
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE int32 Compare(const CharType* InString) const noexcept
    {
        return Compare(InString, TCString<CharType>::Strlen(InString));
    }

    /**
     * @brief: Compares this string with a raw-string of a fixed length
     *
     * @param InString: String to compare with
     * @param InLength: Length of the string to compare
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE int32 Compare(const CharType* InString, SizeType InLength) const noexcept
    {
        const SizeType Length = GetLength();
        if (Length != InLength)
        {
            return -1;
        }
        else if (Length == 0)
        {
            return 0;
        }

        const CharType* Current = ViewStart;
        while (Current != ViewEnd)
        {
            if (*Current != *InString)
            {
                return *Current - *InString;
            }

            Current++;
            InString++;
        }

        return 0;
    }

    /**
     * @brief: Compares this string to another string-type without taking casing into account.
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    template<typename StringType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, int32>::Type CompareNoCase(const StringType& InString) const noexcept
    {
        return CompareNoCase(InString.GetCString(), InString.GetLength());
    }

    /**
     * @brief: Compares this string with a raw-string without taking casing into account.
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE int32 CompareNoCase(const CharType* InString) const noexcept
    {
        return CompareNoCase(InString, TCString<CharType>::Strlen(InString));
    }

    /**
     * @brief: Compares this string with a raw-string without taking casing into account.
     *
     * @param InString: String to compare with
     * @param InLength: Length of the string to compare
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE int32 CompareNoCase(const CharType* InString, SizeType InLength) const noexcept
    {
        const SizeType Length = GetLength();
        if (Length != InLength)
        {
            return -1;
        }
        else if (Length == 0)
        {
            return 0;
        }

        const CharType* Current = ViewStart;
        while (Current != ViewEnd)
        {
            if (TChar<CharType>::ToLower(*Current) != TChar<CharType>::ToLower(*InString))
            {
                return *Current - *InString;
            }

            Current++;
            InString++;
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
    NODISCARD FORCEINLINE SizeType Find(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return Find(InString, TCString<CharType>::Strlen(InString), Position);
    }

    /**
     * @brief: Find the position of the first occurrence of the start of the search-string
     *
     * @param InString: String to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type Find(const StringType& InString, SizeType Position = 0) const noexcept
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
        const CharType* RESTRICT SearchEnd = InString + InLength;
        while (Current != ViewEnd)
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
        Check((Position < GetLength()) || (Position == 0));

        const SizeType Length = GetLength();
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

        const SizeType Length = GetLength();
        if (Length == 0)
        {
            return 0;
        }

        const CharType* RESTRICT Start   = GetCString() + Position;
        const CharType* RESTRICT Current = Start;
        const CharType* RESTRICT End     = GetCString() + Length;
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
        Check((Position < GetLength()) || (Position == 0));

        SizeType Length = GetLength();
        if (Length == 0)
        {
            return 0;
        }

        if (Position != 0)
        {
            Length = NMath::Min(Position, Length);
        }

        if (InString != nullptr)
        {
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
                    if (TChar<CharType>::IsZero(*SearchString))
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
        Check((Position < GetLength()) || (Position == 0));

        SizeType Length = GetLength();
        if ((InLength == 0) || (Length == 0))
        {
            return Length;
        }

        if (Position != 0)
        {
            Length = NMath::Min(Position, Length);
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
        Check((Position < GetLength()) || (Position == 0));

        const SizeType Length = GetLength();
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
        Check((Position < GetLength()) || (Position == 0));

        const SizeType Length = GetLength();
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
     * @brief: Check if the search-string exists within the view
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
     * @brief: Check if the search-string exists within the view. The string is of a string-type.
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
     * @brief: Check if the search-string exists within the view
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
     * @brief: Check if the character exists within the view
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

        const SizeType Length = GetLength();
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

        const SizeType Length = GetLength();
        const SizeType SuffixLenght = TCString<CharType>::Strlen(InString);
        if (SuffixLenght > Length)
        {
            return false;
        }

        const CharType* Data = GetCString() + (Length - SuffixLenght);
        return (TCString<CharType>::Stricmp(Data, InString) == 0);
    }

    /**
     * @brief: Check if the container contains any elements
     *
     * @return: Returns true if the array is empty or false if it contains elements
     */
    NODISCARD FORCEINLINE bool IsEmpty() const noexcept
    {
        return (GetLength() == 0);
    }

    /**
     * @brief: Retrieve the first element of the view
     *
     * @return: Returns a reference to the first element of the view
     */
    NODISCARD FORCEINLINE const CharType& FirstElement() const noexcept
    {
        Check(!IsEmpty());
        return *ViewStart;
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the view
     */
    NODISCARD FORCEINLINE const CharType& LastElement() const noexcept
    {
        Check(!IsEmpty());
        return *(ViewStart + LastElementIndex());
    }

    /**
     * @brief: Retrieve a element at a certain index of the view
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    NODISCARD FORCEINLINE const CharType& GetElementAt(SizeType Index) const noexcept
    {
        Check(Index < GetLength());
        return *(ViewStart + Index);
    }

    /**
     * @brief: Swap this view with another
     *
     * @param Other: String to swap with
     */
    FORCEINLINE void Swap(TStringView& Other) noexcept
    {
        ::Swap<const CharType*>(ViewStart, Other.ViewStart);
        ::Swap<const CharType*>(ViewEnd  , Other.ViewEnd);
    }

    /**
     * @brief: Retrieve the last index that can be used to retrieve an element from the view
     *
     * @return: Returns a the index to the last element of the view
     */
    NODISCARD FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        const SizeType Length = GetLength();
        return (Length > 0) ? (Length - 1) : 0;
    }

    /**
     * @brief: Returns the size of the view
     *
     * @return: The current size of the view
     */
    NODISCARD FORCEINLINE SizeType GetSize() const noexcept
    {
        return GetLength();
    }

    /**
     * @brief: Returns the length of the view
     *
     * @return: The current length of the view
     */
    NODISCARD FORCEINLINE SizeType GetLength() const noexcept
    {
        return static_cast<SizeType>(static_cast<intptr>(ViewEnd - ViewStart));
    }

    /**
     * @brief: Returns the size of the container in bytes
     *
     * @return: The current size of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return GetSize() * sizeof(CharType);
    }

    /**
     * @brief: Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const CharType* GetData() const noexcept
    {
        return ViewStart;
    }

    /**
     * @brief: Retrieve a null-terminated string
     *
     * @return: Returns a pointer containing a null-terminated string
     */
    NODISCARD FORCEINLINE const CharType* GetCString() const noexcept
    {
        return (ViewStart == nullptr) ? TCString<CharType>::Empty() : ViewStart;
    }

    /**
     * @brief: Create a sub-string view of this string
     *
     * @param Position: Position to start the sub-string at
     * @param NumCharacters: Number of characters in the sub-string
     * @return: Returns a sub-string view
     */
    NODISCARD FORCEINLINE TStringView SubStringView(SizeType Offset, SizeType Count) const noexcept
    {
        Check((Count < GetLength()) && (Offset + Count < GetLength()));
        return TStringView(GetData() + Offset, Count);
    }

public:

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
     * @brief: Copy-assignment operator
     *
     * @param RHS: String to copy
     * @return: Return a reference to this instance
     */
    FORCEINLINE TStringView& operator=(const TStringView& RHS) noexcept
    {
        TStringView(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator
     *
     * @param RHS: String to move
     * @return: Return a reference to this instance
     */
    FORCEINLINE TStringView& operator=(TStringView&& RHS) noexcept
    {
        TStringView(Move(RHS)).Swap(*this);
        return *this;
    }

public:

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

    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept { return StartIterator(); }
    NODISCARD FORCEINLINE ConstIteratorType end()   const noexcept { return EndIterator(); }

private:
    const CharType* ViewStart = nullptr;
    const CharType* ViewEnd   = nullptr;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Predefined types

using FStringView     = TStringView<CHAR>;
using FStringViewWide = TStringView<WIDECHAR>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Operators

template<typename CharType>
inline NODISCARD bool operator==(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) == 0);
}

template<typename CharType>
inline NODISCARD bool operator==(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) == 0);
}

template<typename CharType>
inline NODISCARD bool operator==(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) == 0);
}

template<typename CharType>
inline NODISCARD bool operator!=(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CharType>
inline NODISCARD bool operator!=(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CharType>
inline NODISCARD bool operator!=(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CharType>
inline NODISCARD bool operator<(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) < 0);
}

template<typename CharType>
inline NODISCARD bool operator<(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) < 0);
}

template<typename CharType>
inline NODISCARD bool operator<(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) < 0);
}

template<typename CharType>
inline NODISCARD bool operator<=(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) <= 0);
}

template<typename CharType>
inline NODISCARD bool operator<=(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) <= 0);
}

template<typename CharType>
inline NODISCARD bool operator<=(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) <= 0);
}

template<typename CharType>
inline NODISCARD bool operator>(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) > 0);
}

template<typename CharType>
inline NODISCARD bool operator>(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) > 0);
}

template<typename CharType>
inline NODISCARD bool operator>(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) > 0);
}

template<typename CharType>
inline NODISCARD bool operator>=(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) >= 0);
}

template<typename CharType>
inline NODISCARD bool operator>=(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) >= 0);
}

template<typename CharType>
inline NODISCARD bool operator>=(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) >= 0);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Add TStringView to be a string-type

template<typename CharType>
struct TIsTStringType<TStringView<CharType>>
{
    enum { Value = true };
};

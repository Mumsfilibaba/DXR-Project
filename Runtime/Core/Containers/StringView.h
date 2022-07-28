#pragma once
#include "Iterator.h"

#include "Core/Math/Math.h"
#include "Core/Templates/IsSame.h"
#include "Core/Templates/Move.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/IsTStringType.h"
#include "Core/Templates/StringUtils.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TStringView - Class for viewing a string similar to std::string_view

template<typename CharType>
class TStringView
{
public:
    using ElementType = CharType;
    using SizeType    = int32;
    using StringUtils = TCString<CharType>;
    
    static_assert(TIsSame<CharType, char>::Value || TIsSame<CharType, wchar_t>::Value, "Only char and wchar_t is supported for strings");

    enum
    {
        NPos = SizeType(-1)
    };

    typedef TArrayIterator<const TStringView, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<const TStringView, const CharType> ReverseConstIteratorType;

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
        , ViewEnd(InString + StringUtils::Length(InString))
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
    template<typename StringType, typename = typename TEnableIf<TIsTStringType<StringType>::Value>::Type>
    FORCEINLINE explicit TStringView(const StringType& InString) noexcept
        : ViewStart(InString.CStr())
        , ViewEnd(InString.CStr() + InString.Length())
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
        Other.ViewEnd = nullptr;
    }

    /**
     * @brief: Clears the view
     */
    FORCEINLINE void Clear() noexcept
    {
        ViewStart = nullptr;
        ViewEnd = nullptr;
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
        Check((Position < Length()) || (Position == 0));

        SizeType CopySize = NMath::Min(BufferSize, Length() - Position);
        StringUtils::Copy(Buffer, ViewStart + Position, CopySize);
    }

    /**
     * @brief: Removes whitespace from the beginning and end of the string and returns a copy
     *
     * @return: Returns a copy of this string with the whitespace removed in the end and beginning
     */
    FORCEINLINE TStringView Trim() noexcept
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
    FORCEINLINE TStringView TrimStart() noexcept
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
        const CharType* ViewIterator = ViewStart;
        while (ViewIterator != ViewEnd)
        {
            if (!StringUtils::IsWhiteSpace(*(ViewIterator++)))
            {
                break;
            }
            else
            {
                ViewStart++;
            }
        }
    }

    /**
     * @brief: Removes whitespace from the end of the string and returns a copy
     *
     * @return: Returns a copy of this string with all the whitespace removed from the end
     */
    FORCEINLINE TStringView TrimEnd() noexcept
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
        const CharType* ViewIterator = ViewEnd;
        while (ViewIterator != ViewStart)
        {
            ViewIterator--;
            if (!StringUtils::IsWhiteSpace(*ViewIterator))
            {
                break;
            }
            else
            {
                ViewEnd--;
            }
        }
    }

    /**
     * @brief: Shrink the view from the left and return a copy
     * 
     * @param Num: Number of characters to trim
     * @return: Return a trimmed copy of the view
     */
    FORCEINLINE TStringView ShrinkLeft(int32 Num = 1) noexcept
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
        if (Length() <= Num)
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
    FORCEINLINE TStringView ShrinkRight(int32 Num = 1) noexcept
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
        if (Length() <= Num)
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
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, int32>::Type Compare(const StringType& InString) const noexcept
    {
        return Compare(InString.CStr(), InString.Length());
    }

    /**
     * @brief: Compares this string with a raw-string
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    FORCEINLINE int32 Compare(const CharType* InString) const noexcept
    {
        return Compare(InString, StringUtils::Length(InString));
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
        SizeType ThisLength = Length();
        if (ThisLength != InLength)
        {
            return -1;
        }
        else if (ThisLength == 0)
        {
            return 0;
        }

        const CharType* Start = ViewStart;
        while (Start != ViewEnd)
        {
            if (*Start != *InString)
            {
                return *Start - *InString;
            }

            Start++;
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
        SizeType ThisLength = Length();
        if (ThisLength != InLength)
        {
            return -1;
        }
        else if (ThisLength == 0)
        {
            return 0;
        }

        const CharType* Start = ViewStart;
        while (Start != ViewEnd)
        {
            const CharType TempChar0 = StringUtils::ToLower(*Start);
            const CharType TempChar1 = StringUtils::ToLower(*InString);

            if (TempChar0 != TempChar1)
            {
                return TempChar0 - TempChar1;
            }

            Start++;
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
        Check((Position < Length()) || (Position == 0));

        if ((InLength == 0) || StringUtils::IsTerminator(*InString) || (Length() == 0))
        {
            return 0;
        }

        const CharType* Start = ViewStart + Position;
        while (Start != ViewEnd)
        {
            const CharType* SubstringIt = InString;
            for (const CharType* It = Start; ; )
            {
                if (*(It++) != *(SubstringIt++))
                {
                    break;
                }
                else if (StringUtils::IsTerminator(*SubstringIt))
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(Start - ViewStart));
                }
            }

            Start++;
        }

        return NPos;
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
        Check((Position < Length()) || (Position == 0));

        if (StringUtils::IsTerminator(Char) || (Length() == 0))
        {
            return 0;
        }

        const CharType* Start = ViewStart + Position;
        while (Start != ViewEnd)
        {
            if (*Start == Char)
            {
                // If terminator is reached we have found the full substring in out string
                return static_cast<SizeType>(static_cast<intptr_t>(Start - ViewStart));
            }

            Start++;
        }

        return NPos;
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
        Check((Position < Length()) || (Position == 0));

        SizeType ThisLength = Length();
        if ((InLength == 0) || StringUtils::IsTerminator(*InString) || (ThisLength == 0))
        {
            return ThisLength;
        }

        if (Position != 0)
        {
            ThisLength = NMath::Min(Position, ThisLength);
        }

        const CharType* End = ViewStart + ThisLength;
        while (End != ViewStart)
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
                    return static_cast<SizeType>(static_cast<intptr_t>(End - ViewStart));
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
        Check((Position < Length()) || (Position == 0));

        SizeType ThisLength = Length();
        if (StringUtils::IsTerminator(Char) || (ThisLength == 0))
        {
            return ThisLength;
        }

        if (Position != 0)
        {
            ThisLength = NMath::Min(Position, ThisLength);
        }

        const CharType* End = ViewStart + ThisLength;
        while (End != ViewStart)
        {
            if (*(--End) == Char)
            {
                return static_cast<SizeType>(static_cast<intptr_t>(End - ViewStart));
            }
        }

        return NPos;
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
        Check((Position < Length()) || (Position == 0));

        if ((InLength == 0) || StringUtils::IsTerminator(*InString) || (Length() == 0))
        {
            return 0;
        }

        const CharType* Start = ViewStart + Position;
        while (Start != ViewEnd)
        {
            const CharType* SubstringStart = InString;
            const CharType* SubstringEnd = SubstringStart + InLength;
            while (SubstringStart != SubstringEnd)
            {
                if (*(SubstringStart++) == *Start)
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(Start - ViewStart));
                }
            }

            Start++;
        }

        return NPos;
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
        Check((Position < Length()) || (Position == 0));

        SizeType ThisLength = Length();
        if ((InLength == 0) || StringUtils::IsTerminator(*InString) || (ThisLength == 0))
        {
            return ThisLength;
        }

        if (Position != 0)
        {
            ThisLength = NMath::Min(Position, ThisLength);
        }

        SizeType SubstringLength = StringUtils::Length(InString);

        const CharType* ViewIterator = ViewStart + ThisLength;
        while (ViewIterator != ViewStart)
        {
            ViewIterator--;

            const CharType* SubstringStart = InString;
            const CharType* SubstringEnd = SubstringStart + SubstringLength;
            while (SubstringStart != SubstringEnd)
            {
                if (*ViewIterator == *(SubstringStart++))
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(ViewIterator - ViewStart));
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
        Check((Position < Length()) || (Position == 0));

        if ((InLength == 0) || StringUtils::IsTerminator(*InString) || (Length() == 0))
        {
            return 0;
        }

        const CharType* Start = ViewStart + Position;
        while (Start != ViewEnd)
        {
            const CharType* SubstringStart = InString;
            const CharType* SubstringEnd = SubstringStart + InLength;
            while (SubstringStart != SubstringEnd)
            {
                if (*(SubstringStart++) == *Start)
                {
                    break;
                }
                else if (StringUtils::IsTerminator(*SubstringStart))
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(Start - ViewStart));
                }
            }

            Start++;
        }

        return NPos;
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
        Check((Position < Length()) || (Position == 0));

        SizeType ThisLength = Length();
        if ((InLength == 0) || StringUtils::IsTerminator(*InString) || (ThisLength == 0))
        {
            return ThisLength;
        }

        if (Position != 0)
        {
            ThisLength = NMath::Min(Position, ThisLength);
        }

        SizeType SubstringLength = StringUtils::Length(InString);

        const CharType* ViewIterator = ViewStart + ThisLength;
        while (ViewIterator != ViewStart)
        {
            ViewIterator--;

            const CharType* SubstringStart = InString;
            const CharType* SubstringEnd = SubstringStart + SubstringLength;
            while (SubstringStart != SubstringEnd)
            {
                if (*ViewIterator == *(SubstringStart++))
                {
                    break;
                }
                else if (StringUtils::IsTerminator(*SubstringStart))
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(ViewIterator - ViewStart));
                }
            }
        }

        return NPos;
    }

    /**
     * @brief: Check if the search-string exists within the view
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
     * @brief: Check if the search-string exists within the view. The string is of a string-type.
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
     * @brief: Check if the search-string exists within the view
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
     * @brief: Check if the character exists within the view
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
     * @brief: Check if the one of the characters exists within the view
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
     * @brief: Check if the one of the characters exists within the view. The string is of a string-type.
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
     * @brief: Check if the one of the characters exists within the view
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
     * @brief: Check if the container contains any elements
     *
     * @return: Returns true if the array is empty or false if it contains elements
     */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (Length() == 0);
    }

    /**
     * @brief: Retrieve the first element of the view
     *
     * @return: Returns a reference to the first element of the view
     */
    FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        Check(!IsEmpty());
        return Data()[0];
    }

    /**
     * @brief: Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the view
     */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        Check(!IsEmpty());
        return Data()[LastElementIndex()];
    }

    /**
     * @brief: Retrieve a element at a certain index of the view
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
     * @brief: Swap this view with another
     *
     * @param Other: String to swap with
     */
    FORCEINLINE void Swap(TStringView& Other) noexcept
    {
        ::Swap<const CharType*>(ViewStart, Other.ViewStart);
        ::Swap<const CharType*>(ViewEnd, Other.ViewEnd);
    }

    /**
     * @brief: Retrieve the last index that can be used to retrieve an element from the view
     *
     * @return: Returns a the index to the last element of the view
     */
    FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        SizeType Len = Length();
        return (Len > 0) ? (Len - 1) : 0;
    }

    /**
     * @brief: Returns the size of the view
     *
     * @return: The current size of the view
     */
    FORCEINLINE SizeType Size() const noexcept
    {
        return Length();
    }

    /**
     * @brief: Returns the length of the view
     *
     * @return: The current length of the view
     */
    FORCEINLINE SizeType Length() const noexcept
    {
        return static_cast<SizeType>(static_cast<intptr_t>(ViewEnd - ViewStart));
    }

    /**
     * @brief: Returns the size of the container in bytes
     *
     * @return: The current size of the container in bytes
     */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Size() * sizeof(CharType);
    }

    /**
     * @brief: Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE const CharType* Data() const noexcept
    {
        return ViewStart;
    }

    /**
     * @brief: Retrieve a null-terminated string
     *
     * @return: Returns a pointer containing a null-terminated string
     */
    FORCEINLINE const CharType* CStr() const noexcept
    {
        return (ViewStart == nullptr) ? StringUtils::Empty() : ViewStart;
    }

    /**
     * @brief: Create a sub-string view of this string
     *
     * @param Position: Position to start the sub-string at
     * @param NumCharacters: Number of characters in the sub-string
     * @return: Returns a sub-string view
     */
    FORCEINLINE TStringView SubStringView(SizeType Offset, SizeType Count) const noexcept
    {
        Check((Count < Length()) && (Offset + Count < Length()));
        return TStringView(Data() + Offset, Count);
    }

public:

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
    const CharType* ViewStart = nullptr;
    const CharType* ViewEnd = nullptr;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Predefined types

using FStringView   = TStringView<char>;
using FFWStringView = TStringView<wchar_t>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Operators

template<typename CharType>
inline bool operator==(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) == 0);
}

template<typename CharType>
inline bool operator==(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) == 0);
}

template<typename CharType>
inline bool operator==(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) == 0);
}

template<typename CharType>
inline bool operator!=(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CharType>
inline bool operator!=(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CharType>
inline bool operator!=(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return !(LHS == RHS);
}

template<typename CharType>
inline bool operator<(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) < 0);
}

template<typename CharType>
inline bool operator<(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) < 0);
}

template<typename CharType>
inline bool operator<(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) < 0);
}

template<typename CharType>
inline bool operator<=(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) <= 0);
}

template<typename CharType>
inline bool operator<=(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) <= 0);
}

template<typename CharType>
inline bool operator<=(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) <= 0);
}

template<typename CharType>
inline bool operator>(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) > 0);
}

template<typename CharType>
inline bool operator>(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) > 0);
}

template<typename CharType>
inline bool operator>(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) > 0);
}

template<typename CharType>
inline bool operator>=(const TStringView<CharType>& LHS, const CharType* RHS) noexcept
{
    return (LHS.Compare(RHS) >= 0);
}

template<typename CharType>
inline bool operator>=(const CharType* LHS, const TStringView<CharType>& RHS) noexcept
{
    return (RHS.Compare(LHS) >= 0);
}

template<typename CharType>
inline bool operator>=(const TStringView<CharType>& LHS, const TStringView<CharType>& RHS) noexcept
{
    return (LHS.Compare(RHS) >= 0);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Add TStringView to be a string-type

template<typename CharType>
struct TIsTStringType<TStringView<CharType>>
{
    enum
    {
        Value = true
    };
};

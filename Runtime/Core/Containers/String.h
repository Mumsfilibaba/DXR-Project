#pragma once
#include "Array.h"
#include "StringView.h"

#include "Core/Templates/TypeTraits.h"

#if defined(__OBJC__)
    #include <Foundation/Foundation.h>
#endif

#define STRING_USE_INLINE_ALLOCATOR (1)
#define STRING_FORMAT_BUFFER_SIZE   (512)

#if STRING_USE_INLINE_ALLOCATOR 
#define STRING_ALLOCATOR_INLINE_ELEMENTS (16)

// Use a small static buffer for small strings
template<typename CharType>
using TStringAllocator = TInlineArrayAllocator<CharType, STRING_ALLOCATOR_INLINE_ELEMENTS>;
#else

// No preallocated bytes for strings
template<typename CharType>
using TStringAllocator = TDefaultArrayAllocator<CharType>;
#endif

template<typename InCharType>
class TString
{
public:
    using CharType    = InCharType;
    using SizeType    = int32;
    using StorageType = TArray<CharType, TStringAllocator<CharType>>;

    static_assert(
        TIsSame<CharType, CHAR>::Value || TIsSame<CharType, WIDECHAR>::Value,
        "TString only supports 'CHAR' and 'WIDECHAR'");
    
    static_assert(TIsSigned<SizeType>::Value, "TString only supports a SizeType that's signed");

    typedef TArrayIterator<TString, CharType>                    IteratorType;
    typedef TArrayIterator<const TString, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<TString, CharType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TString, const CharType> ReverseConstIteratorType;
    
    enum : SizeType { INVALID_INDEX = SizeType(-1) };

public:
    
    /**
     * @brief          - Create a string from a formatted string
     * @param InFormat - Formatted string
     * @param Args     - Arguments for the formatted string
     * @return         - Returns the formatted string based on the format string
     */
    template<typename... ArgTypes>
    NODISCARD static FORCEINLINE TString CreateFormatted(const CharType* InFormat, ArgTypes&&... Args) noexcept
    {
        TString NewString;
        NewString.Format(InFormat, ::Forward<ArgTypes>(Args)...);
        return NewString;
    }

public:
    
    /**
     * @brief - Default constructor
     */
    FORCEINLINE TString() noexcept
        : Data()
    { }

    /**
     * @brief          - Create a string from a raw array
     * @param InString - String to copy
     */
    FORCEINLINE TString(const CharType* InString) noexcept
        : Data()
    {
        if (InString)
        {
            CopyFrom(InString, TCString<CharType>::Strlen(InString));
        }
    }

    /**
     * @brief          - Create a string with a specific reserved length
     * @param InLength - Length of the string
     */
    FORCEINLINE explicit TString(SizeType InLength) noexcept
        : Data(InLength + 1)
    { }

    /**
     * @brief          - Create a string from a specified length raw array
     * @param InString - String to copy
     * @param InLength - Length of the string to copy
     */
    FORCEINLINE explicit TString(const CharType* InString, SizeType InLength) noexcept
        : Data()
    {
        if (InString)
        {
            CopyFrom(InString, InLength);
        }
    }

    /**
     * @brief          - Create a static string from another string-type
     * @param InString - String to copy from
     */
    template<
        typename StringType,
        typename = typename TEnableIf<TIsTStringType<StringType>::Value>::Type>
    FORCEINLINE explicit TString(const StringType& InString) noexcept
        : Data()
    {
        CopyFrom(InString.GetCString(), InString.GetLength());
    }

    /**
     * @brief       - Copy Constructor
     * @param Other - Other string to copy from
     */
    FORCEINLINE TString(const TString& Other) noexcept
        : Data()
    {
        CopyFrom(Other.GetCString(), Other.GetLength());
    }

    /**
     * @brief       - Move Constructor
     * @param Other - Other string to move from
     */
    FORCEINLINE TString(TString&& Other) noexcept
        : Data()
    {
        MoveFrom(::Forward<TString>(Other));
    }

#if defined(__OBJC__)
    FORCEINLINE TString(NSString* InString) noexcept
        : Characters()
    {
        const CharType* RawString = nullptr;
        if CONSTEXPR (TIsSame<CharType, CHAR>::Value)
        {
            RawString = reinterpret_cast<const CharType*>([InString cStringUsingEncoding:NSUTF8StringEncoding]);
        }
        else
        {
            RawString = reinterpret_cast<const CharType*>([InString cStringUsingEncoding:NSUTF32LittleEndianStringEncoding]);
        }
        
        CopyFrom(RawString, InString.length);
    }
#endif

    /**
     * @brief - Clears the string
     */
    FORCEINLINE void Clear(bool bFreeMemory = false) noexcept
    {
        if (bFreeMemory)
        {
            Data.Clear(true);
        }
        else if (!Data.IsEmpty())
        {
            Data.Reset(1);
            Data[0] = TChar<CharType>::Null;
        }
    }
    
    /**
     * @brief - Resets the container, but does not deallocate the memory. Takes an optional parameter to
     *     default construct a new amount of elements.
     * 
     * @param NewLength - Number of elements to construct
     */
    FORCEINLINE void Reset(SizeType NewLength = 0) noexcept
    {
        const SizeType NewSizeWithZero = (NewLength > 0) ? (NewLength + 1) : 0;
        Data.Reset(NewSizeWithZero);

        if (CharType* TempData = Data.GetData())
        {
            TempData[0] = TChar<CharType>::Null;
        }
    }

    /**
     * @brief - Resets the container, but does not deallocate the memory. Takes an optional parameter to
     *     default construct a new amount of elements from a single element.
     * 
     * @param NewLength  - Number of elements to construct
     * @param CharToFill - Character to fill the new String with
     */
    FORCEINLINE void Reset(SizeType NewLength, const CharType CharToFill) noexcept
    {
        const SizeType NewSizeWithZero = (NewLength > 0) ? (NewLength + 1) : 0;
        Data.Reset(NewSizeWithZero);

        if (CharType* TempData = Data.GetData())
        {
            TCString<CharType>::Strnset(TempData, CharToFill, NewLength);
            TempData[NewLength] = TChar<CharType>::Null;
        }
    }

    /**
     * @brief          - Resets the container, but does not deallocate the memory. Takes in pointer to copy-construct elements from.
     * @param InString - Raw string to copy from
     * @param InLength - Length of the string
     */
    FORCEINLINE void Reset(const CharType* InString, SizeType InLength) noexcept
    {
        CHECK(InLength == 0 || (InString && InLength > 0));

        const SizeType NewSizeWithZero = (InLength > 0) ? (InLength + 1) : 0;
        Data.Reset(NewSizeWithZero);

        if (CharType* TempData = Data.GetData())
        {
            TCString<CharType>::Strncpy(TempData, InString, InLength);
            TempData[InLength] = TChar<CharType>::Null;
        }
    }

    /**
     * @brief - Resets the container, but does not deallocate the memory. Creates a new array from
     *     another array which can be of another type of array.
     * 
     * @param InputArray - Array to copy-construct from
     */
    template<typename ArrayType>
    FORCEINLINE typename TEnableIf<TIsTArrayType<ArrayType>::Value>::Type Reset(const ArrayType& InputArray) noexcept
    {
        Reset(InputArray.GetData(), InputArray.GetSize());
    }

    /**
     * @brief            - Resets the container by moving elements from another string to this one.
     * @param InputArray - Array to copy-construct elements from
     */
    FORCEINLINE void Reset(TString&& InputArray) noexcept
    {
        MoveFrom(::Forward<TString>(InputArray));
    }

    /**
     * @brief      - Appends a character to this string
     * @param Char - Character to append
     */
    FORCEINLINE void Append(CharType Char) noexcept
    {
        const auto PreviousLength  = GetLength();
        const auto NewSizeWithZero = PreviousLength + 2;
        Data.Resize(NewSizeWithZero);

        CharType* TempData = Data.GetData();
        TempData[PreviousLength]     = Char;
        TempData[PreviousLength + 1] = TChar<CharType>::Null;
    }

    /**
     * @brief          - Appends a raw-string to this string
     * @param InString - String to append
     */
    FORCEINLINE void Append(const CharType* InString) noexcept
    {
        Append(InString, TCString<CharType>::Strlen(InString));
    }

    /**
     * @brief          - Appends a string of another string-type to this string
     * @param InString - String to append
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value>::Type Append(const StringType& InString) noexcept
    {
        Append(InString.GetCString(), InString.GetLength());
    }

    /**
     * @brief          - Appends a raw-string to this string with a fixed length
     * @param InString - String to append
     * @param InLength - Length of the string
     */
    FORCEINLINE void Append(const CharType* InString, SizeType InLength) noexcept
    {
        CHECK(InLength == 0 || (InString && InLength > 0));

        const auto PreviousLength  = GetLength();
        const auto NewSizeWithZero = PreviousLength + InLength + 1;
        Data.Resize(NewSizeWithZero);

        CharType* TempData = Data.GetData() + PreviousLength;
        TCString<CharType>::Strncpy(TempData, InString, InLength);
        TempData[InLength] = TChar<CharType>::Null;
    }

    /**
     * @brief           - Resize the string
     * @param NewLength - New length of the string
     */
    FORCEINLINE void Resize(SizeType NewLength) noexcept
    {
        const auto NewSizeWithZero = (NewLength > 0) ? (NewLength + 1) : 0;
        Data.Resize(NewSizeWithZero);

        if (CharType* TempData = Data.GetData())
        {
            TempData[NewLength] = TChar<CharType>::Null;
        }
    }

    /**
     * @brief             - Resize the string and fill the new elements with a specified character
     * @param NewLength   - New length of the string
     * @param FillElement - Element to fill the string with
     */
    FORCEINLINE void Resize(SizeType NewLength, CharType FillElement) noexcept
    {
        const auto NewSizeWithZero = (NewLength > 0) ? (NewLength + 1) : 0;
        Data.Pop();
        Data.Resize(NewSizeWithZero, FillElement);

        if (CharType* TempData = Data.GetData())
        {
            TempData[NewLength] = TChar<CharType>::Null;
        }
    }

    /**
     * @brief             - Reserve storage space
     * @param NewCapacity - New capacity of the string
     */
    FORCEINLINE void Reserve(SizeType NewCapacity) noexcept
    {
        Data.Reserve(NewCapacity);
    }

    /**
     * @brief            - Copy this string into buffer
     * @param Buffer     - Buffer to fill
     * @param BufferSize - Size of the buffer to fill
     * @param Position   - Offset to start copy from
     */
    FORCEINLINE void CopyToBuffer(CharType* Buffer, SizeType BufferSize, SizeType Position = 0) const noexcept
    {
        CHECK(Position < GetLength() || Position == 0);
        CHECK(Buffer && BufferSize > 0);

        const SizeType CopySize = NMath::Min(BufferSize, GetLength() - Position);
        TCString<CharType>::Strncpy(Buffer, Data.GetData() + Position, CopySize);
    }

    /**
     * @brief         - Replace the string with a formatted string (similar to snprintf)
     * @param Format  - Formatted string to replace the string with
     * @param ArgList - Argument list filled with arguments for the formatted string
     */
    template<typename... ArgTypes>
    inline void Format(const CharType* InFormat, ArgTypes&&... Args) noexcept
    {
        CharType Buffer[STRING_FORMAT_BUFFER_SIZE];
        SizeType BufferSize = STRING_FORMAT_BUFFER_SIZE;

        CharType* DynamicBuffer = nullptr;
        CharType* WrittenString = Buffer;
        
        // Start by printing to the static buffer
        SizeType WrittenChars = TCString<CharType>::Snprintf(WrittenString, BufferSize, InFormat, ::Forward<ArgTypes>(Args)...);
       
        // In case the buffer size is to small, increase the buffer size with a dynamic allocation until we have enough space
        while ((WrittenChars > BufferSize) || (WrittenChars == -1))
        {
            BufferSize   += WrittenChars;
            DynamicBuffer = reinterpret_cast<CharType*>(FMemory::Realloc(DynamicBuffer, BufferSize * sizeof(CharType)));
            WrittenString = DynamicBuffer;
            WrittenChars  = TCString<CharType>::Snprintf(WrittenString, BufferSize, InFormat, ::Forward<ArgTypes>(Args)...);
        }

        const int32 WrittenLength = TCString<CharType>::Strlen(WrittenString);
        Data.Reset(WrittenString, WrittenLength);

        if (DynamicBuffer)
        {
            FMemory::Free(DynamicBuffer);
        }

        Data.Emplace(TChar<CharType>::Null);
    }

    /**
     * @brief         - Appends a formatted string to the string
     * @param Format  - Formatted string to append
     * @param ArgList - Argument-list for the formatted string
     */
    template<typename... ArgTypes>
    inline void AppendFormat(const CharType* InFormat, ArgTypes&&... Args) noexcept
    {
        CharType Buffer[STRING_FORMAT_BUFFER_SIZE];
        SizeType BufferSize = STRING_FORMAT_BUFFER_SIZE;

        CharType* DynamicBuffer = nullptr;
        CharType* WrittenString = Buffer;

        // Start by printing to the static buffer
        SizeType WrittenChars = TCString<CharType>::Snprintf(WrittenString, BufferSize, InFormat, ::Forward<ArgTypes>(Args)...);
        
        // In case the buffer size is to small, increase the buffer size with a dynamic allocation until we have enough space
        while ((WrittenChars > BufferSize) || (WrittenChars == -1))
        {
            BufferSize   += WrittenChars;
            DynamicBuffer = reinterpret_cast<CharType*>(FMemory::Realloc(DynamicBuffer, BufferSize * sizeof(CharType)));
            WrittenString = DynamicBuffer;
            WrittenChars  = TCString<CharType>::Snprintf(WrittenString, BufferSize, InFormat, ::Forward<ArgTypes>(Args)...);
        }

        Data.Pop();
        Data.Append(WrittenString, WrittenChars);

        if (DynamicBuffer)
        {
            FMemory::Free(DynamicBuffer);
        }

        Data.Emplace(TChar<CharType>::Null);
    }

    /**
     * @brief - Convert this string to a lower-case string
     */
    FORCEINLINE void ToLowerInline() noexcept
    {
        CharType* RESTRICT Start = Data.GetData();
        CharType* RESTRICT End   = Start + Data.GetSize();
        while (Start != End)
        {
            *Start = TChar<CharType>::ToLower(*Start);
            ++Start;
        }
    }

    /**
     * @brief  - Convert this string to a lower-case string and returns a copy
     * @return - Returns a copy of this string, with all characters in lower-case
     */
    FORCEINLINE TString ToLower() const noexcept
    {
        TString NewString(*this);
        NewString.ToLowerInline();
        return NewString;
    }

    /**
     * @brief - Convert this string to a upper-case string
     */
    FORCEINLINE void ToUpperInline() noexcept
    {
        CharType* RESTRICT Start = Data.GetData();
        CharType* RESTRICT End   = Start + Data.GetSize();
        while (Start != End)
        {
            *Start = TChar<CharType>::ToUpper(*Start);
            ++Start;
        }
    }

    /**
     * @brief  - Convert this string to a upper-case string and returns a copy
     * @return - Returns a copy of this string, with all characters in upper-case
     */
    NODISCARD FORCEINLINE TString ToUpper() const noexcept
    {
        TString NewString(*this);
        NewString.ToUpperInline();
        return NewString;
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
     * @brief  - Removes whitespace from the beginning and end of the string and returns a copy
     * @return - Returns a copy of this string with the whitespace removed in the end and beginning
     */
    NODISCARD FORCEINLINE TString Trim() noexcept
    {
        TString NewString(*this);
        NewString.TrimInline();
        return NewString;
    }

    /**
     * @brief  - Removes whitespace from the beginning of the string and returns a copy
     * @return - Returns a copy of this string with all the whitespace removed from the beginning
     */
    NODISCARD FORCEINLINE TString TrimStart() noexcept
    {
        TString NewString(*this);
        NewString.TrimStartInline();
        return NewString;
    }

    /**
     * @brief - Removes whitespace from the beginning of the string
     */
    FORCEINLINE void TrimStartInline() noexcept
    {
        const CharType* RESTRICT Start = Data.GetData();
        const CharType* RESTRICT End   = Data.GetData() + GetLength();
        while (Start != End)
        {
            if (!TChar<CharType>::IsSpace(*Start) && !TChar<CharType>::IsZero(*Start))
            {
                break;
            }

            ++Start;
        }

        const SizeType Count = static_cast<SizeType>(Start - Data.GetData());
        if (Count > 0)
        {
            Data.RemoveRangeAt(0, Count);
        }
    }

    /**
     * @brief  - Removes whitespace from the end of the string and returns a copy
     * @return - Returns a copy of this string with all the whitespace removed from the end
     */
    NODISCARD FORCEINLINE TString TrimEnd() noexcept
    {
        TString NewString(*this);
        NewString.TrimEndInline();
        return NewString;
    }

    /**
     * @brief - Removes whitespace from the end of the string
     */
    FORCEINLINE void TrimEndInline() noexcept
    {
        const CharType* RESTRICT Start   = Data.GetData() + GetLength();
        const CharType* RESTRICT Current = Start;
        const CharType* RESTRICT End     = Data.GetData();
        while (Current != End)
        {
            --Current;
            if (!TChar<CharType>::IsSpace(*Current) && !TChar<CharType>::IsZero(*Current))
            {
                break;
            }
        }

        // NOT comparing with zero since smallest count is one (Since decrement is done before break)
        const SizeType Count = static_cast<SizeType>(Start - Current);
        if (Count > 1)
        {
            Data.PopRange(Count);
            Data.Emplace(TChar<CharType>::Null);
        }
    }

    /**
     * @brief  - Reverses the order of all the characters in the string and returns a copy
     * @return - Returns a string with all the characters reversed
     */
    NODISCARD FORCEINLINE TString Reverse() noexcept
    {
        TString NewString(*this);
        NewString.ReverseInline();
        return NewString;
    }

    /**
     * @brief - Reverses the order of all the characters in the string
     */
    FORCEINLINE void ReverseInline() noexcept
    {
        CharType* RESTRICT Start = Data.GetData();
        CharType* RESTRICT End   = Start + GetLength();
        while (Start < End)
        {
            --End;
            ::Swap<CharType>(*Start, *End);
            ++Start;
        }
    }

    /**
     * @brief          - Compares this string to another string-type
     * @param InString - String to compare with
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    template<typename StringType>
    NODISCARD FORCEINLINE typename TEnableIf<
            TIsTStringType<StringType>::Value,
            SizeType
        >::Type Compare(const StringType& InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        return Compare(InString.GetCString(), InString.GetLength(), CaseType);
    }

    /**
     * @brief          - Compares this string with a raw-string
     * @param InString - String to compare with
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE SizeType Compare(const CharType* InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        if (CaseType == EStringCaseType::NoCase)
        {
            return static_cast<SizeType>(TCString<CharType>::Stricmp(Data.GetData(), InString));
        }
        else
        {
            return static_cast<SizeType>(TCString<CharType>::Strcmp(Data.GetData(), InString));
        }
    }

    /**
     * @brief          - Compares this string with a raw-string of a fixed length
     * @param InString - String to compare with
     * @param InLength - Length of the string to compare
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE SizeType Compare(const CharType* InString, SizeType InLength, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        const SizeType MinLength = NMath::Min(GetLength(), InLength);
        if (CaseType == EStringCaseType::NoCase)
        {
            return static_cast<SizeType>(TCString<CharType>::Strnicmp(Data.GetData(), InString, MinLength));
        }
        else
        {
            return static_cast<SizeType>(TCString<CharType>::Strncmp(Data.GetData(), InString, MinLength));
        }
    }

    /**
     * @brief          - Compares this string to another string-type
     * @param InString - String to compare with
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns true if the strings are equal
     */
    template<typename StringType>
    NODISCARD FORCEINLINE typename TEnableIf<
            TIsTStringType<StringType>::Value,
            SizeType
        >::Type Equals(const StringType& InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        return Equals(InString.GetCString(), InString.GetLength(), CaseType);
    }

    /**
     * @brief          - Compares this string with a raw-string
     * @param InString - String to compare with
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns true if the strings are equal
     */
    NODISCARD FORCEINLINE SizeType Equals(const CharType* InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        return Equals(InString, TCString<CharType>::Strlen(InString), CaseType);
    }

    /**
     * @brief          - Compares this string with a raw-string of a fixed length
     * @param InString - String to compare with
     * @param InLength - Length of the string to compare
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns true if the strings are equal
     */
    NODISCARD FORCEINLINE bool Equals(const CharType* InString, SizeType InLength, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        const auto Length = GetLength();
        if (Length != InLength)
        {
            return false;
        }

        if (CaseType == EStringCaseType::CaseSensitive)
        {
            return (TCString<CharType>::Strncmp(Data.GetData(), InString, Length) == 0);
        }
        else if (CaseType == EStringCaseType::NoCase)
        {
            return (TCString<CharType>::Strnicmp(Data.GetData(), InString, Length) == 0);
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
    NODISCARD FORCEINLINE SizeType Find(const CharType* InString, SizeType Position = 0) const noexcept
    {
        CHECK((Position < GetLength()) || (Position == 0));

        if (InString == nullptr)
        {
            return INVALID_INDEX;
        }

        if (GetLength() != 0)
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
     * @brief          - Find the position of the first occurrence of the start of the search-string
     * @param InString - String to search
     * @param InString - Length of the search-string
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType Find(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        CHECK((Position < GetLength()) || (Position == 0));

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
     * @brief          - Find the position of the first occurrence of the start of the search-string
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search-string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type Find(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return Find(InString.GetCString(), Position);
    }

    /**
     * @brief          - Returns the position of the first occurrence of CHAR
     * @param Char     - Character to search for
     * @param Position - Position to start search at
     * @return         - Returns the position of the first occurrence of the CHAR
     */
    NODISCARD FORCEINLINE SizeType FindChar(CharType Char, SizeType Position = 0) const noexcept
    {
        CHECK((Position < GetLength()) || (Position == 0));

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
     * @brief           - Returns the position of the first CHAR that passes the predicate
     * @param Predicate - Predicate that specifies valid chars
     * @param Position  - Position to start search at
     * @return          - Returns the position of the first occurrence of the CHAR
     */
    template<typename PredicateType>
    NODISCARD FORCEINLINE SizeType FindCharWithPredicate(PredicateType&& Predicate, SizeType Position = 0) const noexcept
    {
        CHECK((Position < GetLength()) || (Position == 0));

        const SizeType ThisLength = GetLength();
        if (ThisLength == 0)
        {
            return 0;
        }

        const CharType* RESTRICT Start   = GetCString() + Position;
        const CharType* RESTRICT Current = Start;
        const CharType* RESTRICT End     = GetCString() + ThisLength;
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
     * @brief          - Find the position of the last occurrence of the start of the search-string
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search-string
     */
    NODISCARD FORCEINLINE SizeType FindLast(const CharType* InString, SizeType Position = 0) const noexcept
    {
        CHECK((Position < GetLength()) || (Position == 0));
        CHECK(InString != nullptr);

        const SizeType ThisLength = GetLength();
        if (ThisLength == 0)
        {
            return 0;
        }

        if (InString != nullptr)
        {
            const CharType* RESTRICT End     = GetCString();
            const CharType* RESTRICT Start   = (Position == 0) ? (End + ThisLength) : (End + Position);
            const CharType* RESTRICT Current = Start;
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
     * @brief          - Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     * @param InString - String to search
     * @param Position - Position to start search at
     * @return         - Returns the position of the first character in the search-string
     */
    template<typename StringType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type FindLast(const StringType& InString, SizeType Position = 0) const noexcept
    {
        return FindLast(InString, InString.GetLength(), Position);
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
        CHECK((Position < GetLength()) || (Position == 0));
        CHECK(InString != nullptr);

        const SizeType ThisLength = GetLength();
        if (ThisLength == 0)
        {
            return 0;
        }

        if (InString != nullptr)
        {
            const CharType* RESTRICT SearchEnd = InString + InLength;
            const CharType* RESTRICT End       = GetCString();
            const CharType* RESTRICT Start     = (Position == 0) ? (End + ThisLength) : (End + Position);
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
     * @brief          - Returns the position of the first occurrence of CHAR. Searches the string in reverse.
     * @param Char     - Character to search for
     * @param Position - Position to start search at
     * @return         - Returns the position of the first occurrence of the CHAR
     */
    NODISCARD FORCEINLINE SizeType FindLastChar(CharType Char, SizeType Position = 0) const noexcept
    {
        CHECK((Position < GetLength()) || (Position == 0));

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
     * @brief           - Returns the position of the first CHAR that passes the predicate
     * @param Predicate - Predicate that specifies valid chars
     * @param Position  - Position to start search at
     * @return          - Returns the position of the first occurrence of the CHAR
     */
    template<typename PredicateType>
    NODISCARD FORCEINLINE SizeType FindLastCharWithPredicate(PredicateType&& Predicate, SizeType Position = 0) const noexcept
    {
        CHECK((Position < GetLength()) || (Position == 0));

        const SizeType ThisLength = GetLength();
        if (ThisLength == 0)
        {
            return 0;
        }

        const CharType* RESTRICT End     = GetCString();
        const CharType* RESTRICT Start   = (Position == 0) ? (End + ThisLength) : (End + Position);
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
     * @brief          - Check if the search-string exists within the string
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(const CharType* InString, SizeType InOffset = 0) const noexcept
    {
        return (Find(InString, TCString<CharType>::Strlen(InString), InOffset) != INVALID_INDEX);
    }

    /**
     * @brief          - Check if the search-string exists within the string. The string is of a string-type.
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    template<typename StringType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type Contains(const StringType& InString, SizeType InOffset = 0) const noexcept
    {
        return (Find(InString, InOffset) != INVALID_INDEX);
    }

    /**
     * @brief          - Check if the search-string exists within the string
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(const CharType* InString, SizeType InLength, SizeType InOffset = 0) const noexcept
    {
        return (Find(InString, InLength, InOffset) != INVALID_INDEX);
    }

    /**
     * @brief          - Check if the character exists within the string
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(CharType Char, SizeType InOffset = 0) const noexcept
    {
        return (FindChar(Char, InOffset) != INVALID_INDEX);
    }

    /**
     * @brief          - Check if string begins with a string
     * @param InString - String to test for
     * @return         - Returns true if the string begins with InString
     */
    NODISCARD FORCEINLINE bool StartsWith(const CharType* InString) const noexcept
    {
        const SizeType SuffixLength = TCString<CharType>::Strlen(InString);
        return (SuffixLength > 0) && (TCString<CharType>::Strncmp(GetCString(), InString, SuffixLength) == 0);
    }

    /**
     * @brief          - Check if string begins with a string
     * @param InString - String to test for
     * @return         - Returns true if the string begins with InString
     */
    NODISCARD FORCEINLINE bool StartsWithNoCase(const CharType* InString) const noexcept
    {
        const SizeType SuffixLength = TCString<CharType>::Strlen(InString);
        return (SuffixLength > 0) && (TCString<CharType>::Strnicmp(GetCString(), InString, SuffixLength) == 0);
    }

    /**
     * @brief          - Check if string end with a string
     * @param InString - String to test for
     * @return         - Returns true if the string ends with InString
     */
    NODISCARD FORCEINLINE bool EndsWith(const CharType* InString) const noexcept
    {
        if (!InString || TChar<CharType>::IsZero(*InString))
        {
            return false;
        }

        const SizeType Length       = GetLength();
        const SizeType SuffixLenght = TCString<CharType>::Strlen(InString);
        if (SuffixLenght > Length)
        {
            return false;
        }

        const CharType* TempData = Data.GetData() + (Length - SuffixLenght);
        return (TCString<CharType>::Strcmp(TempData, InString) == 0);
    }

    /**
     * @brief          - Check if string end with a string without taking casing into account
     * @param InString - String to test for
     * @return         - Returns true if the string ends with InString
     */
    NODISCARD FORCEINLINE bool EndsWithNoCase(const CharType* InString) const noexcept
    {
        if (!InString || TChar<CharType>::IsZero(*InString))
        {
            return false;
        }

        const SizeType Length       = GetLength();
        const SizeType SuffixLength = TCString<CharType>::Strlen(InString);
        if (SuffixLength > Length)
        {
            return false;
        }

        const CharType* TempData = Data.GetData() + (Length - SuffixLength);
        return (TCString<CharType>::Stricmp(TempData, InString) == 0);
    }

    /**
     * @brief               - Removes count characters from position and forward
     * @param Position      - Position to start remove at
     * @param NumCharacters - Number of characters to remove
     */
    FORCEINLINE void Remove(SizeType Position, SizeType Count) noexcept
    {
        CHECK(Position < GetLength() && (Position + Count <= GetLength()));
        Data.RemoveRangeAt(Position, Count);
    }

    /**
     * @brief          - Insert a string at a specified position
     * @param InString - String to insert
     * @param Position - Position to start the insertion at
     */
    FORCEINLINE void Insert(const CharType* InString, SizeType Position) noexcept
    {
        Insert(InString, TCString<CharType>::Strlen(InString), Position);
    }

    /**
     * @brief          - Insert a string at a specified position. The string is of string-type.
     * @param InString - String to insert
     * @param Position - Position to start the insertion at
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value>::Type Insert(const StringType& InString, SizeType Position) noexcept
    {
        Insert(InString.GetCString(), InString.GetLength(), Position);
    }

    /**
     * @brief          - Insert a string at a specified position
     * @param InString - String to insert
     * @param InLength - Length of the string to insert
     * @param Position - Position to start the insertion at
     */
    FORCEINLINE void Insert(const CharType* InString, SizeType InLength, SizeType Position) noexcept
    {
        CHECK(Position <= GetLength());
        Data.Insert(Position, InString, InLength);
    }

    /**
     * @brief          - Insert a character at specified position
     * @param Char     - Character to insert
     * @param Position - Position to insert character at
     */
    FORCEINLINE void Insert(CharType Char, SizeType Position) noexcept
    {
        CHECK(Position <= GetLength());
        Data.Insert(Position, Char);
    }

    /**
     * @brief          - Replace a part of the string
     * @param InString - String to replace
     * @param Position - Position to start the replacing at
     */
    FORCEINLINE void Replace(const CharType* InString, SizeType Position) noexcept
    {
        Replace(InString, TCString<CharType>::Strlen(InString), Position);
    }

    /**
     * @brief          - Replace a part of the string. String is of string-type.
     * @param InString - String to replace
     * @param Position - Position to start the replacing at
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value>::Type Replace(const StringType& InString, SizeType Position) noexcept
    {
        Replace(InString.GetCString(), InString.GetLength(), Position);
    }

    /**
     * @brief          - Replace a part of the string
     * @param InString - String to replace
     * @param InLenght - Length of the string to replace
     * @param Position - Position to start the replacing at
     */
    FORCEINLINE void Replace(const CharType* InString, SizeType InLength, SizeType Position) noexcept
    {
        CHECK(Position < GetLength() && (Position + InLength < GetLength()));
        TCString<CharType>::Strncpy(Data.GetData() + Position, InString, InLength);
    }

    /**
     * @brief          - Replace a character in the string
     * @param Char     - Character to replace
     * @param Position - Position of the character to replace
     */
    FORCEINLINE void Replace(CharType Char, SizeType Position = 0) noexcept
    {
        CHECK((Position < GetLength()));
        Data[Position] = Char;
    }

    /**
     * @brief      - Insert a new character at the end
     * @param Char - Character to insert at the end
     */
    FORCEINLINE void Push(CharType Char) noexcept
    {
        Append(Char);
    }

    /**
     * @brief - Remove the character at the end
     */
    FORCEINLINE void Pop() noexcept
    {
        const SizeType Length = GetLength();
        if (Length)
        {
            Data.Pop();
            Data[Length] = TChar<CharType>::Null;
        }
    }

    /**
     * @brief       - Swap this string with another
     * @param Other - String to swap with
     */
    FORCEINLINE void Swap(TString& Other)
    {
        TString TempString(::Move(*this));
        MoveFrom(::Move(Other));
        Other.MoveFrom(::Move(TempString));
    }

    /**
     * @brief               - Create a sub-string of this string
     * @param Position      - Position to start the sub-string at
     * @param NumCharacters - Number of characters in the sub-string
     * @return              - Returns a sub-string
     */
    NODISCARD FORCEINLINE TString SubString(SizeType Offset, SizeType Count) const noexcept
    {
        CHECK((Offset < GetLength()) && (Offset + Count < GetLength()));
        return TString(Data.GetData() + Offset, Count);
    }

     /**
      * @brief               - Create a StringView of this string
      * @param Position      - Position to start the sub-string at
      * @param NumCharacters - Number of characters in the sub-string
      * @return              - Returns a sub-string view
      */
    NODISCARD FORCEINLINE TStringView<CharType> SubStringView(SizeType Offset, SizeType Count) const noexcept
    {
        CHECK((Offset < GetLength()) && (Offset + Count < GetLength()));
        return TStringView<CharType>(Data.GetData() + Offset, Count);
    }

    /**
     * @brief       - Retrieve a element at a certain index of the string
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE CharType& GetElementAt(SizeType Index) noexcept
    {
        CHECK(Index < GetLength());
        return Data[Index];
    }

    /**
     * @brief       - Retrieve a element at a certain index of the string
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const CharType& GetElementAt(SizeType Index) const noexcept
    {
        CHECK(Index < GetLength());
        return Data[Index];
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE CharType* GetData() noexcept
    {
        return Data.GetData();
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const CharType* GetData() const noexcept
    {
        return Data.GetData();
    }

    /**
     * @brief  - Retrieve a null-terminated string
     * @return - Returns a pointer containing a null-terminated string
     */
    NODISCARD FORCEINLINE const CharType* GetCString() const noexcept
    {
        return !Data.IsEmpty() ? Data.GetData() : TCString<CharType>::Empty();
    }

#if defined(__OBJC__)
    /**
     * @brief  - Retrieve a null-terminated string
     * @return - Returns a pointer containing a null-terminated string
     */
    NODISCARD FORCEINLINE NSString* GetNSString() const noexcept
    {
        if CONSTEXPR (TIsSame<CharType, CHAR>::Value)
        {
            return [[[NSString alloc] initWithBytes:Characters.GetData() length:GetLength() * sizeof(CharType) encoding:NSUTF8StringEncoding] autorelease];
        }
        else
        {
            return [[[NSString alloc] initWithBytes:Characters.GetData() length:GetLength() * sizeof(CharType) encoding:NSUTF32LittleEndianStringEncoding] autorelease];
        }
    }
#endif
    
    /**
     * @brief  - Returns the size of the container
     * @return - The current size of the container
     */
    NODISCARD FORCEINLINE SizeType GetSize() const noexcept
    {
        const SizeType Size = Data.GetSize();
        return (Size > 0) ? (Size - 1) : 0;
    }

    /**
     * @brief  - Retrieve the last index that can be used to retrieve an element from the array
     * @return - Returns a the index to the last element of the array
     */
    NODISCARD FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        return GetSize();
    }

    /**
     * @brief  - Returns the length of the string
     * @return - The current length of the string
     */
    NODISCARD FORCEINLINE SizeType GetLength() const noexcept
    {
        return GetSize();
    }

    /**
     * @brief  - Returns the capacity of the container
     * @return - The current capacity of the container
     */
    NODISCARD FORCEINLINE SizeType GetCapacity() const noexcept
    {
        return Data.GetCapacity();
    }

    /**
     * @brief  - Returns the capacity of the container in bytes
     * @return - The current capacity of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType CapacityInBytes() const noexcept
    {
        return Data.GetCapacity() * sizeof(CharType);
    }

    /**
     * @brief  - Returns the size of the container in bytes
     * @return - The current size of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return GetLength() * sizeof(CharType);
    }

    /**
     * @brief  - Check if the container contains any elements
     * @return - Returns true if the array is empty or false if it contains elements
     */
    NODISCARD FORCEINLINE bool IsEmpty() const noexcept
    {
        return (GetLength() == 0);
    }

    /**
     * @brief  - Retrieve the first element of the array
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE CharType& FirstElement() noexcept
    {
        return Data[0];
    }

    /**
     * @brief  - Retrieve the first element of the array
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE const CharType& FirstElement() const noexcept
    {
        return Data[0];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE CharType& LastElement() noexcept
    {
        return Data[LastElementIndex()];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE const CharType& LastElement() const noexcept
    {
        return Data[LastElementIndex()];
    }

public:

    /**
     * @brief       - Appends a character to this string
     * @param Other - Character to append
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TString& operator+=(CharType Other) noexcept
    {
        Append(Other);
        return *this;
    }

    /**
     * @brief       - Appends a raw-string to this string
     * @param Other - String to append
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TString& operator+=(const CharType* Other) noexcept
    {
        Append(Other);
        return *this;
    }

    /**
     * @brief       - Appends a string of a string-type to this string
     * @param Other - String to append
     * @return      - Returns a reference to this instance
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<
            TIsTStringType<StringType>::Value,
            typename TAddLValueReference<TString>::Type
        >::Type operator+=(const StringType& Other) noexcept
    {
        Append<StringType>(Other);
        return *this;
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE CharType& operator[](SizeType Index) noexcept
    {
        return GetElementAt(Index);
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const CharType& operator[](SizeType Index) const noexcept
    {
        return GetElementAt(Index);
    }

    /**
     * @brief       - Assignment operator that takes a raw string
     * @param Other - String to copy
     * @return      - Return a reference to this instance
     */
    FORCEINLINE TString& operator=(const CharType* Other) noexcept
    {
        TString(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Copy-assignment operator
     * @param Other - String to copy
     * @return      - Return a reference to this instance
     */
    FORCEINLINE TString& operator=(const TString& Other) noexcept
    {
        TString(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Copy-assignment operator that takes another string-type
     * @param Other - String to copy
     * @return      - Return a reference to this instance
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<
            TIsTStringType<StringType>::Value,
            typename TAddLValueReference<TString>::Type
        >::Type operator=(const StringType& Other) noexcept
    {
        TString<StringType>(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - String to move
     * @return      - Return a reference to this instance
     */
    FORCEINLINE TString& operator=(TString&& Other) noexcept
    {
        TString(::Move(Other)).Swap(*this);
        return *this;
    }
    
public:
    NODISCARD friend FORCEINLINE TString operator+(const TString& LHS, const TString& RHS) noexcept
    {
        const auto LHSLength = LHS.GetLength();
        const auto RHSLength = RHS.GetLength();
        const auto CombinedLength = LHSLength + RHSLength;

        TString NewString(CombinedLength);

        CharType* TempData = NewString.GetData();
        TCString<CharType>::Strncpy(TempData, LHS.GetCString(), LHSLength);
        TCString<CharType>::Strncpy(TempData + LHSLength, RHS.GetCString(), RHSLength);

        TempData[CombinedLength] = TChar<CharType>::Null;
        return NewString;
    }

    NODISCARD friend FORCEINLINE TString operator+(const CharType* LHS, const TString& RHS) noexcept
    {
        const auto LHSLength = TCString<CharType>::Strlen(LHS);
        const auto RHSLength = RHS.GetLength();
        const auto CombinedLength = LHSLength + RHSLength;

        TString NewString(CombinedLength);

        CharType* TempData = NewString.GetData();
        TCString<CharType>::Strncpy(TempData, LHS, LHSLength);
        TCString<CharType>::Strncpy(TempData + LHSLength, RHS.GetCString(), RHSLength);

        TempData[CombinedLength] = TChar<CharType>::Null;
        return NewString;
    }

    NODISCARD friend FORCEINLINE TString operator+(const TString& LHS, const CharType* RHS) noexcept
    {
        const auto LHSLength = LHS.GetLength();
        const auto RHSLength = TCString<CharType>::Strlen(RHS);
        const auto CombinedLength = LHSLength + RHSLength;

        TString NewString(CombinedLength);

        CharType* TempData = NewString.GetData();
        TCString<CharType>::Strncpy(TempData, LHS.GetCString(), LHSLength);
        TCString<CharType>::Strncpy(TempData + LHSLength, RHS, RHSLength);

        TempData[CombinedLength] = TChar<CharType>::Null;
        return NewString;
    }

    NODISCARD friend FORCEINLINE TString operator+(CharType LHS, const TString& RHS) noexcept
    {
        const auto RHSLength      = RHS.GetLength();
        const auto CombinedLength = RHSLength + 1;

        TString NewString(CombinedLength);

        CharType* TempData = NewString.GetData();
        TempData[0] = LHS;

        TCString<CharType>::Strncpy(TempData + 1, RHS.GetCString(), RHSLength);
        TempData[CombinedLength] = TChar<CharType>::Null;
        return NewString;
    }

    NODISCARD friend FORCEINLINE TString operator+(const TString& LHS, CharType RHS) noexcept
    {
        const auto LHSLength      = LHS.GetLength();
        const auto CombinedLength = LHSLength + 1;

        TString NewString(CombinedLength);

        CharType* TempData = NewString.GetData();

        TCString<CharType>::Strncpy(TempData, LHS.GetCString(), LHSLength);
        TempData[LHSLength]      = RHS;
        TempData[CombinedLength] = TChar<CharType>::Null;
        return NewString;
    }
    
    NODISCARD friend FORCEINLINE bool operator==(const TString& LHS, const CharType* RHS) noexcept
    {
        return LHS.Equals(RHS);
    }

    NODISCARD friend FORCEINLINE bool operator==(const CharType* LHS, const TString& RHS) noexcept
    {
        return RHS.Equals(LHS);
    }

    NODISCARD friend FORCEINLINE bool operator==(const TString& LHS, const TString& RHS) noexcept
    {
        return LHS.Equals(RHS);
    }

    NODISCARD friend FORCEINLINE bool operator!=(const TString& LHS, const CharType* RHS) noexcept
    {
        return !(LHS == RHS);
    }

    NODISCARD friend FORCEINLINE bool operator!=(const CharType* LHS, const TString& RHS) noexcept
    {
        return !(LHS == RHS);
    }

    NODISCARD friend FORCEINLINE bool operator!=(const TString& LHS, const TString& RHS) noexcept
    {
        return !(LHS == RHS);
    }

    NODISCARD friend FORCEINLINE bool operator<(const TString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) < 0);
    }

    NODISCARD friend FORCEINLINE bool operator<(const CharType* LHS, const TString& RHS) noexcept
    {
        return (RHS.Compare(LHS) < 0);
    }

    NODISCARD friend FORCEINLINE bool operator<(const TString& LHS, const TString& RHS) noexcept
    {
        return (LHS.Compare(RHS) < 0);
    }

    NODISCARD friend FORCEINLINE bool operator<=(const TString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) <= 0);
    }

    NODISCARD friend FORCEINLINE bool operator<=(const CharType* LHS, const TString& RHS) noexcept
    {
        return (RHS.Compare(LHS) <= 0);
    }

    NODISCARD friend FORCEINLINE bool operator<=(const TString& LHS, const TString& RHS) noexcept
    {
        return (LHS.Compare(RHS) <= 0);
    }

    NODISCARD friend FORCEINLINE bool operator>(const TString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) > 0);
    }

    NODISCARD friend FORCEINLINE bool operator>(const CharType* LHS, const TString& RHS) noexcept
    {
        return (RHS.Compare(LHS) > 0);
    }

    NODISCARD friend FORCEINLINE bool operator>(const TString& LHS, const TString& RHS) noexcept
    {
        return (LHS.Compare(RHS) > 0);
    }

    NODISCARD friend FORCEINLINE bool operator>=(const TString& LHS, const CharType* RHS) noexcept
    {
        return (LHS.Compare(RHS) >= 0);
    }

    NODISCARD friend FORCEINLINE bool operator>=(const CharType* LHS, const TString& RHS) noexcept
    {
        return (RHS.Compare(LHS) >= 0);
    }

    NODISCARD friend FORCEINLINE bool operator>=(const TString& LHS, const TString& RHS) noexcept
    {
        return (LHS.Compare(RHS) >= 0);
    }

public:

    /**
      * @brief  - Retrieve an iterator to the beginning of the array
      * @return - A iterator that points to the first element
      */
    NODISCARD FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief   - Retrieve an iterator to the end of the array
     * @return  - A iterator that points to the element past the end
     */
    NODISCARD FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType(*this, GetSize());
    }

    /**
     * @brief   - Retrieve an iterator to the beginning of the array
     * @return  - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief   - Retrieve an iterator to the end of the array
     * @return  - A iterator that points to the element past the end
     */
    NODISCARD FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType(*this, GetSize());
    }

    /**
     * @brief   - Retrieve an reverse-iterator to the end of the array
     * @return  - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, GetSize());
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the start of the array
     * @return - A reverse-iterator that points to the element before the first element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, GetSize());
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the start of the array
     * @return - A reverse-iterator that points to the element before the first element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, 0);
    }

public:
    NODISCARD FORCEINLINE IteratorType      begin()       noexcept { return StartIterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept { return StartIterator(); }

    NODISCARD FORCEINLINE IteratorType      end()       noexcept { return EndIterator(); }
    NODISCARD FORCEINLINE ConstIteratorType end() const noexcept { return EndIterator(); }

private:
    FORCEINLINE void CopyFrom(const CharType* InString, SizeType InLength) noexcept
    {
        const SizeType NewSizeWithZero = (InLength > 0) ? (InLength + 1) : 0;
        Data.Reset(NewSizeWithZero);

        CharType* TempData = Data.GetData();
        TCString<CharType>::Strncpy(TempData, InString, InLength);
        TempData[InLength] = TChar<CharType>::Null;
    }

    FORCEINLINE void MoveFrom(TString&& Other) noexcept
    {
        Data = ::Move(Other.Data);
    }

    StorageType Data;
};


using FString     = TString<CHAR>;
using FStringWide = TString<WIDECHAR>;


// TODO: Investigate this one
//template<typename CharType>
//struct TIsReallocatable<TString<CharType>>
//{
//    enum { Value = true };
//};

template<typename CharType>
struct TIsTStringType<TString<CharType>>
{
    enum { Value = true };
};

NODISCARD inline FStringWide CharToWide(const FStringView& CharString) noexcept
{
    FStringWide NewString;
    NewString.Resize(CharString.GetLength());
    mbstowcs(NewString.GetData(), CharString.GetCString(), CharString.GetLength());
    return NewString;
}

NODISCARD inline FStringWide CharToWide(const FString& CharString) noexcept
{
    FStringWide NewString;
    NewString.Resize(CharString.GetLength());
    mbstowcs(NewString.GetData(), CharString.GetCString(), CharString.GetLength());
    return NewString;
}

NODISCARD inline FString WideToChar(const FStringViewWide& WideString) noexcept
{
    FString NewString;
    NewString.Resize(WideString.GetLength());
    wcstombs(NewString.GetData(), WideString.GetCString(), WideString.GetLength());
    return NewString;
}

NODISCARD inline FString WideToChar(const FStringWide& WideString) noexcept
{
    FString NewString;
    NewString.Resize(WideString.GetLength());
    wcstombs(NewString.GetData(), WideString.GetCString(), WideString.GetLength());
    return NewString;
}


template<typename CharType>
struct TStringHasher
{
    // Jenkins's one_at_a_time hash: https://en.wikipedia.org/wiki/Jenkins_hash_function
    FORCEINLINE size_t operator()(const TString<CharType>& String) const
    {
        // TODO: Investigate how good is this for wide chars

        const auto* Key    = String.GetCString();
        const int32 Length = String.GetLength();

        int32  Index = 0;
        uint64 Hash  = 0;
        while (Index != Length)
        {
            Hash += Key[Index++];
            Hash += Hash << 10;
            Hash ^= Hash >> 6;
        }

        Hash += Hash << 3;
        Hash ^= Hash >> 11;
        Hash += Hash << 15;
        return Hash;
    }
};


using FStringHasher     = TStringHasher<CHAR>;
using FStringHasherWide = TStringHasher<WIDECHAR>;


template<typename T>
struct TTypeToString
{
    NODISCARD static FORCEINLINE typename TEnableIf<TIsArithmetic<T>::Value, FString>::Type ToString(T Element)
    {
        return FString::CreateFormatted(TFormatSpecifier<typename TRemoveCV<T>::Type>::GetStringSpecifier(), Element);
    }

    //NODISCARD static FORCEINLINE typename TEnableIf<TIsPointer<T>::Value, FString>::Type ToString(T Element)
    //{
    //    return FString::CreateFormatted("%p", Element);
    //}
};

template<>
NODISCARD FORCEINLINE FString TTypeToString<bool>::ToString(bool bElement)
{
    return FString(bElement ? "true" : "false");
}


template<typename T>
struct TTypeToStringWide
{
    NODISCARD static FORCEINLINE typename TEnableIf<TIsArithmetic<T>::Value, FStringWide>::Type ToString(T Element)
    {
        return FStringWide::CreateFormatted(TFormatSpecifierWide<typename TRemoveCV<T>::Type>::GetStringSpecifier(), Element);
    }

    //NODISCARD static FORCEINLINE typename TEnableIf<TIsPointer<T>::Value, FStringWide>::Type ToString(T Element)
    //{
    //    return FStringWide::CreateFormatted(L"%p", Element);
    //}
};

template<>
NODISCARD FORCEINLINE FStringWide TTypeToStringWide<bool>::ToString(bool bElement)
{
    return FStringWide(bElement ? L"true" : L"false");
}


template<typename T>
struct TTypeFromString
{
    static FORCEINLINE bool FromString(const FString& String, T& OutElement);
};

template<>
FORCEINLINE bool TTypeFromString<FString>::FromString(const FString& String, FString& OutElement)
{
    OutElement = String;
    return true;
}

template<>
FORCEINLINE bool TTypeFromString<int32>::FromString(const FString& String, int32& OutElement)
{
    CHAR* End;
    OutElement = FCString::Strtoi(String.GetCString(), &End, 10);
    if (*End)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromString<int64>::FromString(const FString& String, int64& OutElement)
{
    CHAR* End;
    OutElement = FCString::Strtoi64(String.GetCString(), &End, 10);
    if (*End)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromString<uint32>::FromString(const FString& String, uint32& OutElement)
{
    CHAR* End;
    OutElement = FCString::Strtoui(String.GetCString(), &End, 10);
    if (*End)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromString<uint64>::FromString(const FString& String, uint64& OutElement)
{
    CHAR* End;
    OutElement = FCString::Strtoui64(String.GetCString(), &End, 10);
    if (*End)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromString<float>::FromString(const FString& String, float& OutElement)
{
    CHAR* End;
    OutElement = FCString::Strtof(String.GetCString(), &End);
    if (*End)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromString<double>::FromString(const FString& String, double& OutElement)
{
    CHAR* End;
    OutElement = FCString::Strtod(String.GetCString(), &End);
    if (*End)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromString<bool>::FromString(const FString& String, bool& OutElement)
{
    if (FCString::Stricmp(String.GetCString(), "true"))
    {
        OutElement = true;
        return true;
    }
    else if (FCString::Stricmp(String.GetCString(), "false"))
    {
        OutElement = false;
        return true;
    }

    CHAR* End;
    OutElement = static_cast<bool>(FCString::Strtoi(String.GetCString(), &End, 10));
    if (*End)
    {
        return true;
    }

    return false;
}


template<typename T>
struct TTypeFromStringWide
{
    static FORCEINLINE bool FromString(const FStringWide& String, T& OutElement);
};

template<>
FORCEINLINE bool TTypeFromStringWide<FStringWide>::FromString(const FStringWide& String, FStringWide& OutElement)
{
    OutElement = String;
    return true;
}

template<>
FORCEINLINE bool TTypeFromStringWide<int32>::FromString(const FStringWide& String, int32& OutElement)
{
    WIDECHAR* End;
    OutElement = FCStringWide::Strtoi(String.GetCString(), &End, 10);
    if (*End)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromStringWide<int64>::FromString(const FStringWide& String, int64& OutElement)
{
    WIDECHAR* End;
    OutElement = FCStringWide::Strtoi64(String.GetCString(), &End, 10);
    if (*End)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromStringWide<uint32>::FromString(const FStringWide& String, uint32& OutElement)
{
    WIDECHAR* End;
    OutElement = FCStringWide::Strtoui(String.GetCString(), &End, 10);
    if (*End)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromStringWide<uint64>::FromString(const FStringWide& String, uint64& OutElement)
{
    WIDECHAR* End;
    OutElement = FCStringWide::Strtoui64(String.GetCString(), &End, 10);
    if (*End)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromStringWide<float>::FromString(const FStringWide& String, float& OutElement)
{
    WIDECHAR* End;
    OutElement = FCStringWide::Strtof(String.GetCString(), &End);
    if (*End)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromStringWide<double>::FromString(const FStringWide& String, double& OutElement)
{
    WIDECHAR* End;
    OutElement = FCStringWide::Strtod(String.GetCString(), &End);
    if (*End)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromStringWide<bool>::FromString(const FStringWide& String, bool& OutElement)
{
    if (FCStringWide::Stricmp(String.GetCString(), L"true"))
    {
        OutElement = true;
        return true;
    }
    else if (FCStringWide::Stricmp(String.GetCString(), L"false"))
    {
        OutElement = false;
        return true;
    }

    WIDECHAR* End;
    OutElement = static_cast<bool>(FCStringWide::Strtoi(String.GetCString(), &End, 10));
    if (*End)
    {
        return true;
    }

    return false;
}

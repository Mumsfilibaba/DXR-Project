#pragma once
#include "Array.h"
#include "StringView.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/ArrayContainerHelper.h"

#if defined(__OBJC__)
    #include <Foundation/Foundation.h>
#endif

#define STRING_USE_INLINE_ALLOCATOR (1)
#define STRING_FORMAT_BUFFER_SIZE   (512)

#if STRING_USE_INLINE_ALLOCATOR 
    // Use a small static buffer for small strings
    #define STRING_ALLOCATOR_INLINE_ELEMENTS (16)

    template<typename CHARTYPE>
    using TStringAllocator = TInlineArrayAllocator<CHARTYPE, STRING_ALLOCATOR_INLINE_ELEMENTS>;
#else
    // No preallocated bytes for strings
    template<typename CHARTYPE>
    using TStringAllocator = TDefaultArrayAllocator<CHARTYPE>;
#endif

template<typename InCharType>
class TString
{
    using FCStringType = TCString<InCharType>;
    using FCharType    = TChar<InCharType>;

public:
    using CHARTYPE    = InCharType;
    using SizeType    = int32;
    using StorageType = TArray<CHARTYPE, TStringAllocator<CHARTYPE>>;

    static_assert(TIsSame<CHARTYPE, CHAR>::Value || TIsSame<CHARTYPE, WIDECHAR>::Value, "TString only supports 'CHAR' and 'WIDECHAR'");
    static_assert(TIsSigned<SizeType>::Value, "TString only supports a SizeType that's signed");

    typedef TArrayIterator<TString, CHARTYPE>                    IteratorType;
    typedef TArrayIterator<const TString, const CHARTYPE>        ConstIteratorType;
    typedef TReverseArrayIterator<TString, CHARTYPE>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TString, const CHARTYPE> ReverseConstIteratorType;
    
    enum : SizeType 
    { 
        INVALID_INDEX = SizeType(-1) 
    };

public:
    
    /**
     * @brief          - Create a string from a formatted string
     * @param InFormat - Formatted string
     * @param Args     - Arguments for the formatted string
     * @return         - Returns the formatted string based on the format string
     */
    template<typename... ArgTypes>
    NODISCARD static FORCEINLINE TString CreateFormatted(const CHARTYPE* InFormat, ArgTypes... Args) noexcept
    {
        TString NewString;
        NewString.Format(InFormat, ::Forward<ArgTypes>(Args)...);
        return NewString;
    }

public:
    
    /** @brief - Default constructor */
    TString() = default;

    /**
     * @brief          - Create a string from a c-string
     * @param InString - String to copy
     */
    FORCEINLINE TString(const CHARTYPE* InString) noexcept
    {
        InitializeByCopy(InString, FCStringType::Strlen(InString));
    }

    /**
     * @brief          - Create a string from a c-string
     * @param InSlack  - Extra number of characters to allocate
     * @param InString - String to copy
     */
    FORCEINLINE TString(SizeType InSlack, const CHARTYPE* InString) noexcept
    {
        InitializeWithSlack(InString, FCStringType::Strlen(InString), InSlack);
    }

    /**
     * @brief          - Create a string with a specific reserved length
     * @param InLength - Length of the string
     */
    FORCEINLINE explicit TString(SizeType InLength) noexcept
        : CharData(InLength + 1)
    {
    }

    /**
     * @brief          - Create a string from a specified length c-string
     * @param InString - String to copy
     * @param InLength - Length of the string to copy
     */
    FORCEINLINE explicit TString(const CHARTYPE* InString, SizeType InLength) noexcept
    {
        InitializeByCopy(InString, InLength);
    }

    /**
     * @brief          - Create a static string from another string-type
     * @param InString - String to copy from
     */
    template<typename StringType>
    FORCEINLINE explicit TString(const StringType& InString) noexcept requires(TIsTStringType<StringType>::Value)
    {
        InitializeByCopy(InString.Data(), InString.Length());
    }

    /**
     * @brief       - Copy Constructor
     * @param Other - Other string to copy from
     */
    FORCEINLINE TString(const TString& Other) noexcept
    {
        InitializeByCopy(Other.Data(), Other.Length());
    }

    /**
     * @brief         - Copy Constructor
     * @param Other   - Other string to copy from
     * @param InSlack - Extra chars that get allocated
     */
    FORCEINLINE TString(const TString& Other, SizeType InSlack) noexcept
    {
        InitializeWithSlack(Other.Data(), Other.Length(), InSlack);
    }

    /**
     * @brief       - Move Constructor
     * @param Other - Other string to move from
     */
    FORCEINLINE TString(TString&& Other) noexcept
        : CharData(::Move(Other.CharData))
    {
    }

#if defined(__OBJC__)
    FORCEINLINE TString(NSString* InString) noexcept
    {
        const CHARTYPE* CastString = nullptr;
        if constexpr (TIsSame<CHARTYPE, CHAR>::Value)
        {
            CastString = reinterpret_cast<const CHARTYPE*>([InString cStringUsingEncoding:NSUTF8StringEncoding]);
        }
        else
        {
            CastString = reinterpret_cast<const CHARTYPE*>([InString cStringUsingEncoding:NSUTF32LittleEndianStringEncoding]);
        }
        
        InitializeByCopy(CastString, InString.length);
    }
#endif

    /**
     * @brief - Clears the string
     */
    FORCEINLINE void Clear(bool bRemoveSlack = false) noexcept
    {
        if (bRemoveSlack)
        {
            CharData.Clear(true);
        }
        else if (!CharData.IsEmpty())
        {
            CharData.Reset(1);
            CharData[0] = 0;
        }
    }
    
    /**
     * @brief           - Resets the container, but does not deallocate the memory. Takes an optional parameter to default construct a new amount of elements.
     * @param NewLength - Number of elements to construct
     */
    FORCEINLINE void Reset(SizeType NewLength = 0) noexcept
    {
        const SizeType NewSizeWithZero = NewLength ? (NewLength + 1) : 0;
        CharData.Reset(NewSizeWithZero);
        if (NewSizeWithZero)
        {
            CharData[0] = 0;
        }
    }

    /**
     * @brief          - Resets the container, but does not deallocate the memory. Takes in pointer to copy-construct elements from.
     * @param InString - Raw string to copy from
     * @param InLength - Length of the string
     */
    FORCEINLINE void Reset(const CHARTYPE* InString, SizeType InLength) noexcept
    {
        CHECK(InLength == 0 || (InString && InLength > 0));

        const SizeType NewSizeWithZero = (InLength) ? (InLength + 1) : 0;
        CharData.Reset(NewSizeWithZero);
        if (NewSizeWithZero)
        {
            FCStringType::Strncpy(CharData.Data(), InString, InLength);
            CharData[InLength] = 0;
        }
    }

    /**
     * @brief            - Resets the container, but does not deallocate the memory. Creates a new array from another array which can be of another type of array.
     * @param InputArray - Array to copy-construct from
     */
    template<typename ArrayType>
    FORCEINLINE void Reset(const ArrayType& InputArray) noexcept requires(TIsTArrayType<ArrayType>::Value)
    {
        Reset(FArrayContainerHelper::Data(InputArray), FArrayContainerHelper::Size(InputArray));
    }

    /**
     * @brief      - Appends a character to this string
     * @param Char - Character to append
     */
    FORCEINLINE void Append(CHARTYPE Char) noexcept
    {
		// Need to take into account that there is a possibility that the only current character is null
        const SizeType NumUninitialized = CharData.IsEmpty() ? 2 : 1;
		const SizeType OldLength = Length();
        CharData.AppendUninitialized(NumUninitialized);
        CharData[OldLength]     = Char;
        CharData[OldLength + 1] = 0;
    }

    /**
     * @brief          - Appends a c-string to this string
     * @param InString - String to append
     */
    FORCEINLINE void Append(const CHARTYPE* InString) noexcept
    {
        Append(InString, FCStringType::Strlen(InString));
    }

    /**
     * @brief          - Appends a string of another string-type to this string
     * @param InString - String to append
     */
    template<typename StringType>
    FORCEINLINE void Append(const StringType& InString) noexcept requires(TIsTStringType<StringType>::Value)
    {
        Append(InString.Data(), InString.Length());
    }

    /**
     * @brief          - Appends a c-string to this string with a fixed length
     * @param InString - String to append
     * @param InLength - Length of the string
     */
    FORCEINLINE void Append(const CHARTYPE* InString, SizeType InLength) noexcept
    {
        if (InString && InLength > 0)
        {
            // Need to take into account that there is a possibility that the only current character is null
            const SizeType NumUninitialized = CharData.IsEmpty() ? InLength + 1: InLength;
            const SizeType OldLength = Length();
            CharData.AppendUninitialized(NumUninitialized);
            FCStringType::Strncpy(CharData.Data() + OldLength, InString, InLength);
            CharData[Length()] = 0;
        }
    }

    /**
     * @brief           - Resize the string
     * @param NewLength - New length of the string
     */
    FORCEINLINE void Resize(SizeType NewLength) noexcept
    {
        const SizeType NewSizeWithZero = (NewLength) ? (NewLength + 1) : 0;
        CharData.Resize(NewSizeWithZero);
        if (NewSizeWithZero)
        {
            CharData[NewLength] = 0;
        }
    }

    /**
     * @brief             - Reserve storage space
     * @param NewCapacity - New capacity of the string
     */
    FORCEINLINE void Reserve(SizeType NewCapacity) noexcept
    {
        CharData.Reserve(NewCapacity);
        CharData[Length()] = 0;
    }

    /**
     * @brief            - Copy this string into buffer
     * @param Buffer     - Buffer to fill
     * @param BufferSize - Size of the buffer to fill
     * @param Position   - Offset to start copy from
     */
    FORCEINLINE void CopyToBuffer(CHARTYPE* Buffer, SizeType BufferSize, SizeType Position = INVALID_INDEX) const noexcept
    {
        CHECK(Position < Length() || Position == 0);
        if (Buffer && BufferSize > 0)
        {
            const SizeType CopySize = FMath::Min(BufferSize, Length() - Position);
            FCStringType::Strncpy(Buffer, CharData.Data() + Position, CopySize);
        }
    }

    /**
     * @brief         - Replace the string with a formatted string (similar to snprintf)
     * @param Format  - Formatted string to replace the string with
     * @param ArgList - Argument list filled with arguments for the formatted string
     */
    template<typename... ArgTypes>
    inline void Format(const CHARTYPE* InFormat, ArgTypes&&... Args) noexcept
    {
        CHARTYPE Buffer[STRING_FORMAT_BUFFER_SIZE];
        SizeType BufferSize = STRING_FORMAT_BUFFER_SIZE;

        CHARTYPE* DynamicBuffer = nullptr;
        CHARTYPE* WrittenString = Buffer;
        
        // Start by printing to the static buffer
        SizeType WrittenChars = FCStringType::Snprintf(WrittenString, BufferSize, InFormat, ::Forward<ArgTypes>(Args)...);
       
        // In case the buffer size is to small, increase the buffer size with a dynamic allocation until we have enough space
        while ((WrittenChars > BufferSize) || (WrittenChars == -1))
        {
            BufferSize   += WrittenChars;
            DynamicBuffer = reinterpret_cast<CHARTYPE*>(FMemory::Realloc(DynamicBuffer, BufferSize * sizeof(CHARTYPE)));
            WrittenString = DynamicBuffer;
            WrittenChars  = FCStringType::Snprintf(WrittenString, BufferSize, InFormat, ::Forward<ArgTypes>(Args)...);
        }

        const SizeType WrittenLength = FCStringType::Strlen(WrittenString);
        CharData.Reset(WrittenLength + 1);
        FCStringType::Strncpy(CharData.Data(), WrittenString, WrittenLength);
        CharData[WrittenLength] = 0;

        if (DynamicBuffer)
        {
            FMemory::Free(DynamicBuffer);
        }
    }

    /**
     * @brief         - Appends a formatted string to the string
     * @param Format  - Formatted string to append
     * @param ArgList - Argument-list for the formatted string
     */
    template<typename... ArgTypes>
    inline void AppendFormat(const CHARTYPE* InFormat, ArgTypes&&... Args) noexcept
    {
        CHARTYPE Buffer[STRING_FORMAT_BUFFER_SIZE];
        SizeType BufferSize = STRING_FORMAT_BUFFER_SIZE;

        CHARTYPE* DynamicBuffer = nullptr;
        CHARTYPE* WrittenString = Buffer;

        // Start by printing to the static buffer
        SizeType WrittenChars = FCStringType::Snprintf(WrittenString, BufferSize, InFormat, ::Forward<ArgTypes>(Args)...);
        
        // In case the buffer size is to small, increase the buffer size with a dynamic allocation until we have enough space
        while ((WrittenChars > BufferSize) || (WrittenChars == -1))
        {
            BufferSize   += WrittenChars;
            DynamicBuffer = reinterpret_cast<CHARTYPE*>(FMemory::Realloc(DynamicBuffer, BufferSize * sizeof(CHARTYPE)));
            WrittenString = DynamicBuffer;
            WrittenChars  = FCStringType::Snprintf(WrittenString, BufferSize, InFormat, ::Forward<ArgTypes>(Args)...);
        }

        const SizeType WrittenLength = FCStringType::Strlen(WrittenString);
        const SizeType OldSize       = Length();
        const SizeType NewSize       = (OldSize > 0) ? WrittenLength : WrittenLength + 1;
        CharData.AppendUninitialized(NewSize);

        FCStringType::Strncpy(CharData.Data() + OldSize, WrittenString, WrittenLength);
        CharData[OldSize + WrittenLength] = 0;

        if (DynamicBuffer)
        {
            FMemory::Free(DynamicBuffer);
        }
    }

    /**
     * @brief - Convert this string to a lower-case string
     */
    FORCEINLINE void ToLowerInline() noexcept
    {
		for (CHARTYPE* RESTRICT Start = CharData.Data(), *RESTRICT End = Start + Length(); Start != End; ++Start)
		{
            *Start = FCharType::ToLower(*Start);
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
        for (CHARTYPE* RESTRICT Start = CharData.Data(), *RESTRICT End = Start + Length(); Start != End; ++Start)
		{
            *Start = FCharType::ToUpper(*Start);
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
        SizeType Index = 0;
        while (FCharType::IsWhitespace(CharData[Index]))
        {
            Index++;
        }

        CharData.RemoveAt(0, Index);
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
        const SizeType OldLength = Length();
        SizeType NewLength = OldLength;
        while (NewLength > 0 && FCharType::IsWhitespace(CharData[NewLength - 1]))
        {
            NewLength--;
        }

        CharData.RemoveAt(NewLength + 1, OldLength - NewLength);
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
        const SizeType CurrentLength = Length();
		const SizeType HalfLength = CurrentLength / 2;

        CHARTYPE* StringData = CharData.Data();
		for (SizeType Index = 0; Index < HalfLength; ++Index)
		{
			const SizeType ReverseIndex = CurrentLength - Index - 1;
			const CHARTYPE TempChar = StringData[Index];
            StringData[Index] = StringData[ReverseIndex];
            StringData[ReverseIndex] = TempChar;
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
     * @brief          - Compares this string with a c-string
     * @param InString - String to compare with
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE SizeType Compare(const CHARTYPE* InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        if (CaseType == EStringCaseType::NoCase)
        {
            return static_cast<SizeType>(FCStringType::Stricmp(CharData.Data(), InString));
        }
        else
        {
            return static_cast<SizeType>(FCStringType::Strcmp(CharData.Data(), InString));
        }
    }

    /**
     * @brief          - Compares this string with a c-string of a fixed length
     * @param InString - String to compare with
     * @param InLength - Length of the string to compare
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    NODISCARD FORCEINLINE SizeType Compare(const CHARTYPE* InString, SizeType InLength, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        const SizeType MinLength = FMath::Min(Length(), InLength);
        if (CaseType == EStringCaseType::NoCase)
        {
            return static_cast<SizeType>(FCStringType::Strnicmp(CharData.Data(), InString, MinLength));
        }
        else
        {
            return static_cast<SizeType>(FCStringType::Strncmp(CharData.Data(), InString, MinLength));
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
     * @brief          - Compares this string with a c-string
     * @param InString - String to compare with
     * @param CaseType - Enum that decides if the comparison should be case-sensitive or not
     * @return         - Returns true if the strings are equal
     */
    NODISCARD FORCEINLINE SizeType Equals(const CHARTYPE* InString, EStringCaseType CaseType = EStringCaseType::CaseSensitive) const noexcept
    {
        return Equals(InString, FCStringType::Strlen(InString), CaseType);
    }

    /**
     * @brief          - Compares this string with a c-string of a fixed length
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
            return FCStringType::Strncmp(CharData.Data(), InString, CurrentLength) == 0;
        }
        else if (CaseType == EStringCaseType::NoCase)
        {
            return FCStringType::Strnicmp(CharData.Data(), InString, CurrentLength) == 0;
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
            Index += FMath::Clamp(0, CurrentLength - 1, Position);
        }

        const CHARTYPE* RESTRICT Result = FCStringType::Strstr(CharData.Data() + Index, InString);
        if (!Result)
        {
            return INVALID_INDEX;
        }
        else
        {
            return static_cast<SizeType>(static_cast<intptr>(Result - CharData.Data()));
        }
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

        const CHARTYPE* RESTRICT Current = CharData.Data();
        if (Position != INVALID_INDEX && CurrentLength > 0)
        {
            Current += FMath::Clamp(0, CurrentLength - 1, Position);
        }

        for (const CHARTYPE* RESTRICT End = CharData.Data() + CurrentLength; Current != End; ++Current)
        {
            if (*Current == Char)
            {
                return static_cast<SizeType>(static_cast<intptr>(Current - CharData.Data()));
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

        const CHARTYPE* RESTRICT Current = CharData.Data();
        if (Position != INVALID_INDEX && CurrentLength > 0)
        {
            Current += FMath::Clamp(0, CurrentLength - 1, Position);
        }

        for (const CHARTYPE *RESTRICT End = CharData.Data() + CurrentLength; Current != End; ++Current)
        {
            if (Predicate(*Current))
            {
                return static_cast<SizeType>(static_cast<intptr>(Current - CharData.Data()));
            }
        }

        return INVALID_INDEX;
    }

    /**
     * @brief          - Find the position of the last occurrence of the start of the search-string
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

        if (Position == INVALID_INDEX || Position > CurrentLength)
        {
            Position = CurrentLength;
        }

        const SizeType SearchLength = FCStringType::Strlen(InString);
        for (SizeType Index = Position - SearchLength; Index >= 0; Index--)
        {
            SizeType SearchIndex = 0;
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

        const CHARTYPE* RESTRICT Current = CharData.Data();
		if (Position == INVALID_INDEX || Position > CurrentLength)
		{
            Current += CurrentLength;
		}

        for (const CHARTYPE* RESTRICT End = CharData.Data(); Current != End;)
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

        const CHARTYPE* RESTRICT Current = CharData.Data();
		if (Position == INVALID_INDEX || Position > CurrentLength)
		{
			Current += CurrentLength;
		}

        for (const CHARTYPE* RESTRICT End = CharData.Data(); Current != End;)
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
    NODISCARD FORCEINLINE bool Contains(const CHARTYPE* InString, SizeType Position = INVALID_INDEX) const noexcept
    {
        return Find(InString, Position) != INVALID_INDEX;
    }

    /**
     * @brief          - Check if the search-string exists within the string. The string is of a string-type.
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    template<typename StringType>
    NODISCARD FORCEINLINE bool Contains(const StringType& InString, SizeType Position = INVALID_INDEX) const noexcept requires(TIsTStringType<StringType>::Value)
    {
        return Find(InString.Data(), Position) != INVALID_INDEX;
    }

    /**
     * @brief          - Check if the character exists within the string
     * @param InString - String to search for
     * @param Position - Position to start to search at
     * @return         - Returns true if the string is found
     */
    NODISCARD FORCEINLINE bool Contains(CHARTYPE Char, SizeType Position = 0) const noexcept
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
                return FCStringType::Strncmp(CharData.Data(), InString, SuffixLength) == 0;
            }
            else if (SearchType == EStringCaseType::NoCase)
            {
                return FCStringType::Strnicmp(CharData.Data(), InString, SuffixLength) == 0;
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
            const CHARTYPE* StringData = CharData.Data() + (CurrentLength - SuffixLength);
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
     * @param Position      - Position to start remove at
     * @param NumCharacters - Number of characters to remove
     */
    FORCEINLINE void Remove(SizeType Position, SizeType Count) noexcept
    {
        CHECK(Position < Length() && (Position + Count) <= Length());
        CharData.RemoveAt(Position, Count);
    }

    /**
     * @brief          - Insert a string at a specified position
     * @param InString - String to insert
     * @param Position - Position to start the insertion at
     */
    FORCEINLINE void Insert(const CHARTYPE* InString, SizeType Position) noexcept
    {
        Insert(InString, FCStringType::Strlen(InString), Position);
    }

    /**
     * @brief          - Insert a string at a specified position. The string is of string-type.
     * @param InString - String to insert
     * @param Position - Position to start the insertion at
     */
    template<typename StringType>
    FORCEINLINE void Insert(const StringType& InString, SizeType Position) noexcept requires(TIsTStringType<StringType>::Value)
    {
        Insert(InString.Data(), InString.Length(), Position);
    }

    /**
     * @brief          - Insert a string at a specified position
     * @param InString - String to insert
     * @param InLength - Length of the string to insert
     * @param Position - Position to start the insertion at
     */
    FORCEINLINE void Insert(const CHARTYPE* InString, SizeType InLength, SizeType Position) noexcept
    {
        CHECK(Position <= Length());
        CharData.Insert(Position, InString, InLength);
    }

    /**
     * @brief          - Insert a character at specified position
     * @param Char     - Character to insert
     * @param Position - Position to insert character at
     */
    FORCEINLINE void Insert(CHARTYPE Char, SizeType Position) noexcept
    {
        CHECK(Position <= Length());
        CharData.Insert(Position, Char);
    }

    /**
     * @brief          - Replace a part of the string
     * @param InString - String to replace
     * @param Position - Position to start the replacing at
     */
    FORCEINLINE void Replace(const CHARTYPE* InString, SizeType Position) noexcept
    {
        Replace(InString, FCStringType::Strlen(InString), Position);
    }

    /**
     * @brief          - Replace a part of the string. String is of string-type.
     * @param InString - String to replace
     * @param Position - Position to start the replacing at
     */
    template<typename StringType>
    FORCEINLINE void Replace(const StringType& InString, SizeType Position) noexcept requires(TIsTStringType<StringType>::Value)
    {
        Replace(InString.Data(), InString.Length(), Position);
    }

    /**
     * @brief          - Replace a part of the string
     * @param InString - String to replace
     * @param InLenght - Length of the string to replace
     * @param Position - Position to start the replacing at
     */
    FORCEINLINE void Replace(const CHARTYPE* InString, SizeType InLength, SizeType Position) noexcept
    {
        CHECK(Position < Length() && (Position + InLength) < Length());
        FCStringType::Strncpy(CharData.Data() + Position, InString, InLength);
    }

    /**
     * @brief          - Replace a character in the string
     * @param Char     - Character to replace
     * @param Position - Position of the character to replace
     */
    FORCEINLINE void Replace(CHARTYPE Char, SizeType Position = INVALID_INDEX) noexcept
    {
        CHECK(Position < Length());
        CharData[Position] = Char;
    }

    /**
     * @brief      - Insert a new character at the end
     * @param Char - Character to insert at the end
     */
    FORCEINLINE void Add(CHARTYPE Char) noexcept
    {
        Append(Char);
    }

    /**
     * @brief - Remove the character at the end
     */
    FORCEINLINE void Pop() noexcept
    {
        const SizeType CurrentLength = Length();
        if (CurrentLength > 0)
        {
            CharData.Pop();
            CharData[CurrentLength] = 0;
        }
    }

    /**
     * @brief       - Swap this string with another
     * @param Other - String to swap with
     */
    FORCEINLINE void Swap(TString& Other)
    {
        StorageType TempData(::Move(CharData));
        CharData = ::Move(Other.CharData);
        Other.CharData = ::Move(TempData);
    }

    /**
     * @brief               - Create a sub-string of this string
     * @param Position      - Position to start the sub-string at
     * @param NumCharacters - Number of characters in the sub-string
     * @return              - Returns a sub-string
     */
    NODISCARD FORCEINLINE TString SubString(SizeType Offset, SizeType Count) const noexcept
    {
        CHECK(Offset < Length() && (Offset + Count < Length()));
        return TString(CharData.Data() + Offset, Count);
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE CHARTYPE* Data() noexcept
    {
        return CharData.Data();
    }

    /**
     * @brief  - Retrieve the data of the array
     * @return - Returns a pointer to the data of the array
     */
    NODISCARD FORCEINLINE const CHARTYPE* Data() const noexcept
    {
        return CharData.Data();
    }

    /**
     * @brief  - Retrieve a null-terminated string
     * @return - Returns a pointer containing a null-terminated string
     */
    NODISCARD FORCEINLINE const CHARTYPE* GetCString() const noexcept
    {
        return !CharData.IsEmpty() ? CharData.Data() : FCStringType::Empty();
    }

#if defined(__OBJC__)
    /**
     * @brief  - Retrieve a null-terminated string
     * @return - Returns a pointer containing a null-terminated string
     */
    NODISCARD FORCEINLINE NSString* GetNSString() const noexcept
    {
        if constexpr (TIsSame<CHARTYPE, CHAR>::Value)
        {
            return [[[NSString alloc] initWithBytes:CharData.Data() length:Length() * sizeof(CHARTYPE) encoding:NSUTF8StringEncoding] autorelease];
        }
        else
        {
            return [[[NSString alloc] initWithBytes:CharData.Data() length:Length() * sizeof(CHARTYPE) encoding:NSUTF32LittleEndianStringEncoding] autorelease];
        }
    }
#endif
    
    /**
     * @brief  - Returns the size of the container
     * @return - The current size of the container
     */
    NODISCARD FORCEINLINE SizeType Size() const noexcept
    {
        const SizeType CurrentSize = CharData.Size();
        return (CurrentSize > 0) ? (CurrentSize - 1) : 0;
    }

    /**
     * @brief  - Returns the size of the container in bytes
     * @return - The current size of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Length() * sizeof(CHARTYPE);
    }

    /**
     * @brief  - Returns the length of the string
     * @return - The current length of the string
     */
    NODISCARD FORCEINLINE SizeType Length() const noexcept
    {
        return Size();
    }

    /**
     * @brief  - Returns the capacity of the container
     * @return - The current capacity of the container
     */
    NODISCARD FORCEINLINE SizeType Capacity() const noexcept
    {
        return CharData.Capacity();
    }

    /**
     * @brief  - Returns the capacity of the container in bytes
     * @return - The current capacity of the container in bytes
     */
    NODISCARD FORCEINLINE SizeType CapacityInBytes() const noexcept
    {
        return CharData.Capacity() * sizeof(CHARTYPE);
    }

    /**
     * @brief  - Check if the container contains any elements
     * @return - Returns true if the array is empty or false if it contains elements
     */
    NODISCARD FORCEINLINE bool IsEmpty() const noexcept
    {
        // Returns true if there only is one element due to null-terminator
        return CharData.Size() <= 1;
    }

    /**
     * @brief  - Retrieve the last index that can be used to retrieve an element from the array
     * @return - Returns a the index to the last element of the array
     */
    NODISCARD FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        return Size();
    }

    /**
     * @brief  - Retrieve the first element of the array
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE CHARTYPE& FirstElement() noexcept
    {
        return CharData[0];
    }

    /**
     * @brief  - Retrieve the first element of the array
     * @return - Returns a reference to the first element of the array
     */
    NODISCARD FORCEINLINE const CHARTYPE& FirstElement() const noexcept
    {
        return CharData[0];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE CHARTYPE& LastElement() noexcept
    {
        const SizeType TempLastIndex = LastElementIndex();
        return CharData[TempLastIndex];
    }

    /**
     * @brief  - Retrieve the last element of the array
     * @return - Returns a reference to the last element of the array
     */
    NODISCARD FORCEINLINE const CHARTYPE& LastElement() const noexcept
    {
        const SizeType TempLastIndex = LastElementIndex();
        return CharData[TempLastIndex];
    }

public:

    /**
     * @brief       - Appends a character to this string
     * @param Other - Character to append
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TString& operator+=(CHARTYPE Other) noexcept
    {
        Append(Other);
        return *this;
    }

    /**
     * @brief       - Appends a c-string to this string
     * @param Other - String to append
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TString& operator+=(const CHARTYPE* Other) noexcept
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
    FORCEINLINE TString& operator+=(const StringType& Other) noexcept requires(TIsTStringType<StringType>::Value)
    {
        Append<StringType>(Other);
        return *this;
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE CHARTYPE& operator[](SizeType Index) noexcept
    {
        CHECK(Index < Length());
        return CharData[Index];
    }

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE const CHARTYPE& operator[](SizeType Index) const noexcept
    {
        CHECK(Index < Length());
        return CharData[Index];
    }

    /**
     * @brief       - Assignment operator that takes a raw string
     * @param Other - String to copy
     * @return      - Return a reference to this instance
     */
    FORCEINLINE TString& operator=(const CHARTYPE* Other) noexcept
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
    FORCEINLINE TString& operator=(const StringType& Other) noexcept requires(TIsTStringType<StringType>::Value)
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
        TString NewString(LHS, RHS.Length());
        NewString.Append(RHS);
        return NewString;
    }

    NODISCARD friend FORCEINLINE TString operator+(const CHARTYPE* LHS, const TString& RHS) noexcept
    {
        TString NewString(RHS.Length(), LHS);
        NewString.Append(RHS);
        return NewString;
    }

    NODISCARD friend FORCEINLINE TString operator+(const TString& LHS, const CHARTYPE* RHS) noexcept
    {
        const SizeType AppendLength = FCStringType::Strlen(RHS);
        TString NewString(LHS, AppendLength);
        NewString.Append(RHS, AppendLength);
        return NewString;
    }

    NODISCARD friend FORCEINLINE TString operator+(CHARTYPE LHS, const TString& RHS) noexcept
    {
        // Allocate a long enough string
        const SizeType NewLength = RHS.Length() + 1;
        TString NewString(NewLength);
        
        // Copy the Char and String directly to avoid overhead
        CHARTYPE* StringData = NewString.Data();
        StringData[0] = LHS;
        FCStringType::Strncpy(StringData + 1, RHS.Data(), RHS.Length());
        StringData[NewLength] = 0;
        return NewString;
    }

    NODISCARD friend FORCEINLINE TString operator+(const TString& LHS, CHARTYPE RHS) noexcept
    {
        TString NewString(LHS, 1);
        NewString.Append(RHS);
        return NewString;
    }
    
    NODISCARD friend FORCEINLINE bool operator==(const TString& LHS, const CHARTYPE* RHS) noexcept
    {
        return LHS.Equals(RHS);
    }

    NODISCARD friend FORCEINLINE bool operator==(const CHARTYPE* LHS, const TString& RHS) noexcept
    {
        return RHS.Equals(LHS);
    }

    NODISCARD friend FORCEINLINE bool operator==(const TString& LHS, const TString& RHS) noexcept
    {
        return LHS.Equals(RHS);
    }

    NODISCARD friend FORCEINLINE bool operator!=(const TString& LHS, const CHARTYPE* RHS) noexcept
    {
        return !(LHS == RHS);
    }

    NODISCARD friend FORCEINLINE bool operator!=(const CHARTYPE* LHS, const TString& RHS) noexcept
    {
        return !(LHS == RHS);
    }

    NODISCARD friend FORCEINLINE bool operator!=(const TString& LHS, const TString& RHS) noexcept
    {
        return !(LHS == RHS);
    }

    NODISCARD friend FORCEINLINE bool operator<(const TString& LHS, const CHARTYPE* RHS) noexcept
    {
        return LHS.Compare(RHS) < 0;
    }

    NODISCARD friend FORCEINLINE bool operator<(const CHARTYPE* LHS, const TString& RHS) noexcept
    {
        return RHS.Compare(LHS) < 0;
    }

    NODISCARD friend FORCEINLINE bool operator<(const TString& LHS, const TString& RHS) noexcept
    {
        return LHS.Compare(RHS) < 0;
    }

    NODISCARD friend FORCEINLINE bool operator<=(const TString& LHS, const CHARTYPE* RHS) noexcept
    {
        return LHS.Compare(RHS) <= 0;
    }

    NODISCARD friend FORCEINLINE bool operator<=(const CHARTYPE* LHS, const TString& RHS) noexcept
    {
        return RHS.Compare(LHS) <= 0;
    }

    NODISCARD friend FORCEINLINE bool operator<=(const TString& LHS, const TString& RHS) noexcept
    {
        return LHS.Compare(RHS) <= 0;
    }

    NODISCARD friend FORCEINLINE bool operator>(const TString& LHS, const CHARTYPE* RHS) noexcept
    {
        return LHS.Compare(RHS) > 0;
    }

    NODISCARD friend FORCEINLINE bool operator>(const CHARTYPE* LHS, const TString& RHS) noexcept
    {
        return RHS.Compare(LHS) > 0;
    }

    NODISCARD friend FORCEINLINE bool operator>(const TString& LHS, const TString& RHS) noexcept
    {
        return LHS.Compare(RHS) > 0;
    }

    NODISCARD friend FORCEINLINE bool operator>=(const TString& LHS, const CHARTYPE* RHS) noexcept
    {
        return LHS.Compare(RHS) >= 0;
    }

    NODISCARD friend FORCEINLINE bool operator>=(const CHARTYPE* LHS, const TString& RHS) noexcept
    {
        return RHS.Compare(LHS) >= 0;
    }

    NODISCARD friend FORCEINLINE bool operator>=(const TString& LHS, const TString& RHS) noexcept
    {
        return LHS.Compare(RHS) >= 0;
    }

public: // Iterators

    /**
     * @brief  - Retrieve an iterator to the beginning of the array
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE IteratorType Iterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an iterator to the beginning of the array
     * @return - A iterator that points to the first element
     */
    NODISCARD FORCEINLINE ConstIteratorType ConstIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseIteratorType ReverseIterator() noexcept
    {
        return ReverseIteratorType(*this, Size());
    }

    /**
     * @brief  - Retrieve an reverse-iterator to the end of the array
     * @return - A reverse-iterator that points to the last element
     */
    NODISCARD FORCEINLINE ReverseConstIteratorType ConstReverseterator() const noexcept
    {
        return ReverseConstIteratorType(*this, Size());
    }

public: // STL Iterator
    NODISCARD FORCEINLINE IteratorType      begin()       noexcept { return Iterator(); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const noexcept { return ConstIterator(); }
    
    NODISCARD FORCEINLINE IteratorType      end()       noexcept { return IteratorType(*this, Length()); }
    NODISCARD FORCEINLINE ConstIteratorType end() const noexcept { return ConstIteratorType(*this, Length()); }

private:
    FORCEINLINE void InitializeByCopy(const CHARTYPE* InString, SizeType InLength) noexcept
    {
        if (InString && InLength)
        {
            CharData.AppendUninitialized(InLength + 1);
            FCStringType::Strncpy(CharData.Data(), InString, InLength);
            CharData[InLength] = 0;
        }
    }

    FORCEINLINE void InitializeWithSlack(const CHARTYPE* InString, SizeType InLength, SizeType InSlack) noexcept
    {
        if (InString && InLength)
        {
            CharData.Reserve(InLength + InSlack + 1);
            CharData.AppendUninitialized(InLength + 1);
            FCStringType::Strncpy(CharData.Data(), InString, InLength);
            CharData[InLength] = 0;
        }
    }

    StorageType CharData;
};

using FString     = TString<CHAR>;
using FStringWide = TString<WIDECHAR>;

// TODO: Investigate this one
//template<typename CHARTYPE>
//struct TIsReallocatable<TString<CHARTYPE>>
//{
//    inline static constexpr bool Value = true;
//};

template<typename CHARTYPE>
struct TIsTStringType<TString<CHARTYPE>>
{
    inline static constexpr bool Value = true;
};

template<typename CHARTYPE>
struct TIsContiguousContainer<TString<CHARTYPE>>
{
    inline static constexpr bool Value = true;
};

NODISCARD inline FStringWide CharToWide(const FStringView& CharString) noexcept
{
    FStringWide NewString;
    NewString.Resize(CharString.Length());
    FPlatformString::Mbstowcs(NewString.Data(), CharString.Data(), CharString.Length());
    return NewString;
}

NODISCARD inline FStringWide CharToWide(const FString& CharString) noexcept
{
    FStringWide NewString;
    NewString.Resize(CharString.Length());
    FPlatformString::Mbstowcs(NewString.Data(), CharString.Data(), CharString.Length());
    return NewString;
}

NODISCARD inline FString WideToChar(const FStringViewWide& WideString) noexcept
{
    FString NewString;
    NewString.Resize(WideString.Length());
    FPlatformString::Wcstombs(NewString.Data(), WideString.Data(), WideString.Length());
    return NewString;
}

NODISCARD inline FString WideToChar(const FStringWide& WideString) noexcept
{
    FString NewString;
    NewString.Resize(WideString.Length());
    FPlatformString::Wcstombs(NewString.Data(), WideString.Data(), WideString.Length());
    return NewString;
}


template<typename CHARTYPE>
struct TStringHasher
{
    // Jenkins's one_at_a_time hash: https://en.wikipedia.org/wiki/Jenkins_hash_function
    FORCEINLINE size_t operator()(const TString<CHARTYPE>& String) const
    {
        // TODO: Investigate how good is this for wide chars

        const auto* Key    = String.GetCString();
        const int32 Length = String.Length();

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
    const CHAR* Start = String.GetCString();
    OutElement = FCString::Strtoi(Start, &End, 10);
    if (End != Start)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromString<int64>::FromString(const FString& String, int64& OutElement)
{
    CHAR* End;
    const CHAR* Start = String.GetCString();
    OutElement = FCString::Strtoi64(Start, &End, 10);
    if (End != Start)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromString<uint32>::FromString(const FString& String, uint32& OutElement)
{
    CHAR* End;
    const CHAR* Start = String.GetCString();
    OutElement = FCString::Strtoui(Start, &End, 10);
    if (End != Start)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromString<uint64>::FromString(const FString& String, uint64& OutElement)
{
    CHAR* End;
    const CHAR* Start = String.GetCString();
    OutElement = FCString::Strtoui64(Start, &End, 10);
    if (End != Start)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromString<float>::FromString(const FString& String, float& OutElement)
{
    CHAR* End;
    const CHAR* Start = String.GetCString();
    OutElement = FCString::Strtof(Start, &End);
    if (End != Start)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromString<double>::FromString(const FString& String, double& OutElement)
{
    CHAR* End;
    const CHAR* Start = String.GetCString();
    OutElement = FCString::Strtod(Start, &End);
    if (End != Start)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromString<bool>::FromString(const FString& String, bool& OutElement)
{
    const CHAR* Start = String.GetCString();
    if (FCString::Stricmp(Start, "true") == 0)
    {
        OutElement = true;
        return true;
    }
    else if (FCString::Stricmp(Start, "false") == 0)
    {
        OutElement = false;
        return true;
    }

    CHAR* End;
    OutElement = static_cast<bool>(FCString::Strtoi(Start, &End, 10));
    if (End != Start)
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
    const WIDECHAR* Start = String.GetCString();
    OutElement = FCStringWide::Strtoi(Start, &End, 10);
    if (End != Start)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromStringWide<int64>::FromString(const FStringWide& String, int64& OutElement)
{
    WIDECHAR* End;
    const WIDECHAR* Start = String.GetCString();
    OutElement = FCStringWide::Strtoi64(Start, &End, 10);
    if (End != Start)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromStringWide<uint32>::FromString(const FStringWide& String, uint32& OutElement)
{
    WIDECHAR* End;
    const WIDECHAR* Start = String.GetCString();
    OutElement = FCStringWide::Strtoui(Start, &End, 10);
    if (End != Start)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromStringWide<uint64>::FromString(const FStringWide& String, uint64& OutElement)
{
    WIDECHAR* End;
    const WIDECHAR* Start = String.GetCString();
    OutElement = FCStringWide::Strtoui64(Start, &End, 10);
    if (End != Start)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromStringWide<float>::FromString(const FStringWide& String, float& OutElement)
{
    WIDECHAR* End;
    const WIDECHAR* Start = String.GetCString();
    OutElement = FCStringWide::Strtof(Start, &End);
    if (End != Start)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromStringWide<double>::FromString(const FStringWide& String, double& OutElement)
{
    WIDECHAR* End;
    const WIDECHAR* Start = String.GetCString();
    OutElement = FCStringWide::Strtod(Start, &End);
    if (End != Start)
    {
        return true;
    }

    return false;
}

template<>
FORCEINLINE bool TTypeFromStringWide<bool>::FromString(const FStringWide& String, bool& OutElement)
{
    const WIDECHAR* Start = String.GetCString();
    if (FCStringWide::Stricmp(Start, L"true") == 0)
    {
        OutElement = true;
        return true;
    }
    else if (FCStringWide::Stricmp(Start, L"false") == 0)
    {
        OutElement = false;
        return true;
    }

    WIDECHAR* End;
    OutElement = static_cast<bool>(FCStringWide::Strtoi(Start, &End, 10));
    if (End != Start)
    {
        return true;
    }

    return false;
}


template<typename T>
struct TTryParseType
{
    static FORCEINLINE bool TryParse(const FString& InString);
};

template<>
FORCEINLINE bool TTryParseType<int32>::TryParse(const FString& InString)
{
    CHAR* End;
    const CHAR* Start = InString.GetCString();
    FCString::Strtoi(Start, &End, 10);
    return (End != Start);
}

template<>
FORCEINLINE bool TTryParseType<uint32>::TryParse(const FString& InString)
{
    CHAR* End;
    const CHAR* Start = InString.GetCString();
    FCString::Strtoui(Start, &End, 10);
    return (End != Start);
}

template<>
FORCEINLINE bool TTryParseType<int64>::TryParse(const FString& InString)
{
    CHAR* End;
    const CHAR* Start = InString.GetCString();
    FCString::Strtoi64(Start, &End, 10);
    return (End != Start);
}

template<>
FORCEINLINE bool TTryParseType<uint64>::TryParse(const FString& InString)
{
    CHAR* End;
    const CHAR* Start = InString.GetCString();
    FCString::Strtoui64(Start, &End, 10);
    return (End != Start);
}

template<>
FORCEINLINE bool TTryParseType<float>::TryParse(const FString& InString)
{
    CHAR* End;
    const CHAR* Start = InString.GetCString();
    FCString::Strtof(Start, &End);
    return (End != Start);
}

template<>
FORCEINLINE bool TTryParseType<double>::TryParse(const FString& InString)
{
    CHAR* End;
    const CHAR* Start = InString.GetCString();
    FCString::Strtof(Start, &End);
    return (End != Start);
}

template<>
FORCEINLINE bool TTryParseType<bool>::TryParse(const FString& InString)
{
    const CHAR* Start = InString.GetCString();
    if (FCString::Stricmp(Start, "true") == 0)
    {
        return true;
    }
    else if (FCString::Stricmp(Start, "false") == 0)
    {
        return true;
    }

    CHAR* End;
    FCString::Strtoi64(Start, &End, 10);
    return (End != Start);
}

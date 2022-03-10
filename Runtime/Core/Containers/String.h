#pragma once
#include "Array.h"
#include "StringView.h"

#include "Core/Templates/Identity.h"
#include "Core/Templates/AddReference.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Config

#define STRING_USE_INLINE_ALLOCATOR (1)
#define STRING_FORMAT_BUFFER_SIZE   (256)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Allocator type

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TString - String class with a dynamic length

template<typename CharType>
class TString
{
public:

    static_assert(TIsSame<CharType, char>::Value || TIsSame<CharType, wchar_t>::Value, "Only char and wchar_t is supported for strings");

    using ElementType = CharType;
    using SizeType = int32;
    using StringMisc = TStringMisc<ElementType>;
    using ViewType = TStringView<ElementType>;
    using StorageType = TArray<CharType, TStringAllocator<CharType>>;

    enum
    {
        NPos = SizeType(-1)
    };

    typedef TArrayIterator<TString, CharType>                    IteratorType;
    typedef TArrayIterator<const TString, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<TString, CharType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TString, const CharType> ReverseConstIteratorType;

    /**
     * Create a string from a formatted string
     *
     * @param Format: Formatted string
     * @return: Returns the formatted string based on the format string
     */
    static NOINLINE TString MakeFormated(const CharType* Format, ...) noexcept
    {
        TString NewString;

        va_list ArgsList;
        va_start(ArgsList, Format);
        NewString.FormatV(Format, ArgsList);
        va_end(ArgsList);

        return NewString;
    }

    /**
     * Create a string from a formatted string and an argument-list
     *
     * @param Format: Formatted string
     * @param ArgsList: Argument-list to be formatted based on the format-string
     * @return: Returns the formatted string based on the format string
     */
    static FORCEINLINE TString MakeFormatedV(const CharType* Format, va_list ArgsList) noexcept
    {
        TString NewString;
        NewString.FormatV(Format, ArgsList);
        return NewString;
    }

    /**
     * Default constructor
     */
    FORCEINLINE TString() noexcept
        : Characters()
    {
    }

    /**
     * Create a string from a raw array
     *
     * @param InString: String to copy
     */
    FORCEINLINE TString(const CharType* InString) noexcept
        : Characters()
    {
        if (InString)
        {
            CopyFrom(InString, StringMisc::Length(InString));
        }
    }

    /**
     * Create a string from a specified length raw array
     *
     * @param InString: String to copy
     * @param InLength: Length of the string to copy
     */
    FORCEINLINE explicit TString(const CharType* InString, uint32 InLength) noexcept
        : Characters()
    {
        if (InString)
        {
            CopyFrom(InString, InLength);
        }
    }

    /**
     * Create a static string from another string-type
     *
     * @param InString: String to copy from
     */
    template<typename StringType, typename = typename TEnableIf<TIsTStringType<StringType>::Value>::Type>
    FORCEINLINE explicit TString(const StringType& InString) noexcept
        : Characters()
    {
        CopyFrom(InString.CStr(), InString.Length());
    }

    /**
     * Copy Constructor
     *
     * @param Other: Other string to copy from
     */
    FORCEINLINE TString(const TString& Other) noexcept
        : Characters()
    {
        CopyFrom(Other.CStr(), Other.Length());
    }

    /**
     * Move Constructor
     *
     * @param Other: Other string to move from
     */
    FORCEINLINE TString(TString&& Other) noexcept
        : Characters()
    {
        MoveFrom(Forward<TString>(Other));
    }

    /**
     * Empties the storage 
     */
    FORCEINLINE void Empty() noexcept
    {
        Characters.Empty();
    }

    /**
     * Clears the string
     */
    FORCEINLINE void Clear() noexcept
    {
        if (!Characters.IsEmpty())
        {
            Characters.Resize(1);
            Characters[0] = StringMisc::Null;
        }
    }

    /**
     * Appends a character to this string
     *
     * @param Char: Character to append
     */
    FORCEINLINE void Append(CharType Char) noexcept
    {
        if (!Characters.IsEmpty())
        {
            Characters.Pop();
        }

        Characters.Emplace(Char);
        Characters.Emplace(StringMisc::Null);
    }

    /**
     * Appends a raw-string to this string
     *
     * @param InString: String to append
     */
    FORCEINLINE void Append(const CharType* InString) noexcept
    {
        Append(InString, StringMisc::Length(InString));
    }

    /**
     * Appends a string of another string-type to this string
     *
     * @param InString: String to append
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value>::Type Append(const StringType& InString) noexcept
    {
        Append(InString.CStr(), InString.Length());
    }

    /**
     * Appends a raw-string to this string with a fixed length
     *
     * @param InString: String to append
     * @param InLength: Length of the string
     */
    FORCEINLINE void Append(const CharType* InString, SizeType InLength) noexcept
    {
        Assert(InString != nullptr);

        if (!Characters.IsEmpty())
        {
            Characters.Pop();
        }

        Characters.Append(InString, InLength);
        Characters.Emplace(StringMisc::Null);
    }

    /**
     * Resize the string
     *
     * @param NewLength: New length of the string
     */
    FORCEINLINE void Resize(SizeType NewSize) noexcept
    {
        Resize(NewSize, CharType());
    }

    /**
     * Resize the string and fill the new elements with a specified character
     *
     * @param NewLength: New length of the string
     * @param FillElement: Element to fill the string with
     */
    FORCEINLINE void Resize(SizeType NewSize, CharType FillElement) noexcept
    {
        if (!Characters.IsEmpty())
        {
            Characters.Pop();
        }

        Characters.Resize(NewSize, FillElement);
        Characters.Emplace(StringMisc::Null);
    }

    /**
     * Reserve storage space
     *
     * @param NewCapacity: New capacity of the string
     */
    FORCEINLINE void Reserve(SizeType NewCapacity) noexcept
    {
        Characters.Reserve(NewCapacity);
    }

    /**
     * Copy this string into buffer
     *
     * @param Buffer: Buffer to fill
     * @param BufferSize: Size of the buffer to fill
     * @param Position: Offset to start copy from
     */
    FORCEINLINE void Copy(CharType* Buffer, SizeType BufferSize, SizeType Position = 0) const noexcept
    {
        Assert((Position < Length()) || (Position == 0));

        if (Buffer && (BufferSize > 0))
        {
            SizeType CopySize = NMath::Min(BufferSize, Length() - Position);
            StringMisc::Copy(Buffer, Characters.Data() + Position, CopySize);
        }
    }

    /**
     * Replace the string with a formatted string (similar to snprintf)
     *
     * @param Format: Formatted string to replace the string with
     */
    NOINLINE void Format(const CharType* Format, ...) noexcept
    {
        va_list ArgList;
        va_start(ArgList, Format);
        FormatV(Format, ArgList);
        va_end(ArgList);
    }

    /**
     * Replace the string with a formatted string (similar to snprintf)
     *
     * @param Format: Formatted string to replace the string with
     * @param ArgList: Argument list filled with arguments for the formatted string
     */
    FORCEINLINE void FormatV(const CharType* Format, va_list ArgsList) noexcept
    {
        CharType Buffer[STRING_FORMAT_BUFFER_SIZE];
        SizeType BufferSize = STRING_FORMAT_BUFFER_SIZE;

        CharType* DynamicBuffer = nullptr;
        CharType* WrittenString = Buffer;

        SizeType WrittenChars = StringMisc::FormatBufferV(WrittenString, BufferSize, Format, ArgsList);
        while ((WrittenChars > BufferSize) || (WrittenChars == -1))
        {
            BufferSize += BufferSize;

            DynamicBuffer = reinterpret_cast<CharType*>(CMemory::Realloc(DynamicBuffer, BufferSize * sizeof(CharType)));
            WrittenString = DynamicBuffer;

            WrittenChars = StringMisc::FormatBufferV(WrittenString, BufferSize, Format, ArgsList);
        }

        int32 WrittenLength = StringMisc::Length(WrittenString);
        Characters.Reset(WrittenString, WrittenLength);

        if (DynamicBuffer)
        {
            CMemory::Free(DynamicBuffer);
        }

        Characters.Emplace(StringMisc::Null);
    }

    /**
     * Appends a formatted string to the string
     *
     * @param Format: Formatted string to append
     */
    NOINLINE void AppendFormat(const CharType* Format, ...) noexcept
    {
        va_list ArgList;
        va_start(ArgList, Format);
        AppendFormatV(Format, ArgList);
        va_end(ArgList);
    }

    /**
     * Appends a formatted string to the string
     *
     * @param Format: Formatted string to append
     * @param ArgList: Argument-list for the formatted string
     */
    FORCEINLINE void AppendFormatV(const CharType* Format, va_list ArgsList) noexcept
    {
        CharType Buffer[STRING_FORMAT_BUFFER_SIZE];
        SizeType BufferSize = STRING_FORMAT_BUFFER_SIZE;

        CharType* DynamicBuffer = nullptr;
        CharType* WrittenString = Buffer;

        SizeType WrittenChars = StringMisc::FormatBufferV(WrittenString, BufferSize, Format, ArgsList);
        while ((WrittenChars > BufferSize) || (WrittenChars == -1))
        {
            BufferSize += BufferSize;

            DynamicBuffer = reinterpret_cast<CharType*>(CMemory::Realloc(DynamicBuffer, BufferSize * sizeof(CharType)));
            WrittenString = DynamicBuffer;

            WrittenChars = StringMisc::FormatBufferV(WrittenString, BufferSize, Format, ArgsList);
        }

        Characters.Pop();
        Characters.Append(WrittenString, WrittenChars);

        if (DynamicBuffer)
        {
            CMemory::Free(DynamicBuffer);
        }

        Characters.Emplace(StringMisc::Null);
    }

    /**
     * Convert this string to a lower-case string
     */
    FORCEINLINE void ToLowerInline() noexcept
    {
        CharType* It = Characters.Data();
        CharType* End = It + Characters.Size();
        while (It != End)
        {
            *It = StringMisc::ToLower(*It);
            It++;
        }
    }

    /**
     * Convert this string to a lower-case string and returns a copy
     *
     * @return: Returns a copy of this string, with all characters in lower-case
     */
    FORCEINLINE TString ToLower() const noexcept
    {
        TString NewString(*this);
        NewString.ToLowerInline();
        return NewString;
    }

    /**
     * Convert this string to a upper-case string
     */
    FORCEINLINE void ToUpperInline() noexcept
    {
        CharType* It = Characters.Data();
        CharType* End = It + Characters.Size();
        while (It != End)
        {
            *It = StringMisc::ToUpper(*It);
            It++;
        }
    }

    /**
     * Convert this string to a upper-case string and returns a copy
     *
     * @return: Returns a copy of this string, with all characters in upper-case
     */
    FORCEINLINE TString ToUpper() const noexcept
    {
        TString NewString(*this);
        NewString.ToUpperInline();
        return NewString;
    }

    /**
     * Removes whitespace from the beginning and end of the string
     */
    FORCEINLINE void TrimInline() noexcept
    {
        TrimStartInline();
        TrimEndInline();
    }

    /**
     * Removes whitespace from the beginning and end of the string and returns a copy
     *
     * @return: Returns a copy of this string with the whitespace removed in the end and beginning
     */
    FORCEINLINE TString Trim() noexcept
    {
        TString NewString(*this);
        NewString.TrimInline();
        return NewString;
    }

    /**
     * Removes whitespace from the beginning of the string and returns a copy
     *
     * @return: Returns a copy of this string with all the whitespace removed from the beginning
     */
    FORCEINLINE TString TrimStart() noexcept
    {
        TString NewString(*this);
        NewString.TrimStartInline();
        return NewString;
    }

    /**
     * Removes whitespace from the beginning of the string
     */
    FORCEINLINE void TrimStartInline() noexcept
    {
        SizeType Count = 0;
        SizeType ThisLength = Length();

        CharType* Start = Characters.Data();
        CharType* End = Start + ThisLength;
        while (Start != End)
        {
            if (StringMisc::IsWhiteSpace(*(Start++)))
            {
                Count++;
            }
            else
            {
                break;
            }
        }

        if (Count > 0)
        {
            Characters.RemoveRangeAt(0, Count);
        }
    }

    /**
     * Removes whitespace from the end of the string and returns a copy
     *
     * @return: Returns a copy of this string with all the whitespace removed from the end
     */
    FORCEINLINE TString TrimEnd() noexcept
    {
        TString NewString(*this);
        NewString.TrimEndInline();
        return NewString;
    }

    /**
     * Removes whitespace from the end of the string
     */
    FORCEINLINE void TrimEndInline() noexcept
    {
        SizeType LastIndex = LastElementIndex();
        SizeType Index = LastIndex;
        for (; Index >= 0; Index--)
        {
            if (!StringMisc::IsWhiteSpace(Characters[Index]))
            {
                break;
            }
        }

        SizeType Count = (LastIndex > Index) ? (LastIndex - Index) : 0;
        if (Count > 0)
        {
            Characters.PopRange(Count + 1);
            Characters.Emplace(StringMisc::Null);
        }
    }

    /**
     * Reverses the order of all the characters in the string and returns a copy
     *
     * @return: Returns a string with all the characters reversed
     */
    FORCEINLINE TString Reverse() noexcept
    {
        TString NewString(*this);
        NewString.ReverseInline();
        return NewString;
    }

    /**
     * Reverses the order of all the characters in the string
     */
    FORCEINLINE void ReverseInline() noexcept
    {
        CharType* Start = Characters.Data();
        CharType* End = Start + Length();
        while (Start < End)
        {
            End--;
            ::Swap<CharType>(*Start, *End);
            Start++;
        }
    }

    /**
     * Compares this string to another string-type
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
     * Compares this string with a raw-string
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    FORCEINLINE int32 Compare(const CharType* InString) const noexcept
    {
        return StringMisc::Compare(Characters.Data(), InString);
    }

    /**
     * Compares this string with a raw-string of a fixed length
     *
     * @param InString: String to compare with
     * @param InLength: Length of the string to compare
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    FORCEINLINE int32 Compare(const CharType* InString, SizeType InLength) const noexcept
    {
        return StringMisc::Compare(Characters.Data(), InString, InLength);
    }

    /**
     * Compares this string to another string-type without taking casing into account.
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
     * Compares this string with a raw-string without taking casing into account.
     *
     * @param InString: String to compare with
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    FORCEINLINE int32 CompareNoCase(const CharType* InString) const noexcept
    {
        return CompareNoCase(InString, StringMisc::Length(InString));
    }

    /**
     * Compares this string with a raw-string without taking casing into account.
     *
     * @param InString: String to compare with
     * @param InLength: Length of the string to compare
     * @return: Returns the position of the characters that is not equal. The sign determines difference of the character.
     */
    FORCEINLINE int32 CompareNoCase(const CharType* InString, SizeType InLength) const noexcept
    {
        Assert(InString != nullptr);

        SizeType Len = Length();
        if (Len != InLength)
        {
            return -1;
        }

        const CharType* Start = Characters.Data();
        const CharType* End = Start + Len;
        while (Start != End)
        {
            const CharType TempChar0 = StringMisc::ToLower(*(Start++));
            const CharType TempChar1 = StringMisc::ToLower(*(InString++));

            if (TempChar0 != TempChar1)
            {
                return TempChar0 - TempChar1;
            }
        }

        return 0;
    }

    /**
     * Find the position of the first occurrence of the start of the search-string
     *
     * @param InString: String to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    FORCEINLINE SizeType Find(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return Find(InString, StringMisc::Length(InString), Position);
    }

    /**
     * Find the position of the first occurrence of the start of the search-string
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
     * Find the position of the first occurrence of the start of the search-string
     *
     * @param InString: String to search
     * @param InString: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    FORCEINLINE SizeType Find(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Assert((Position < Length()) || (Position == 0));

        if ((InLength == 0) || StringMisc::IsTerminator(*InString) || (Length() == 0))
        {
            return 0;
        }

        const CharType* Start = CStr() + Position;
        const CharType* Result = StringMisc::Find(Start, InString);
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
     * Returns the position of the first occurrence of char
     *
     * @param Char: Character to search for
     * @param Position: Position to start search at
     * @return: Returns the position of the first occurrence of the char
     */
    FORCEINLINE SizeType Find(CharType Char, SizeType Position = 0) const noexcept
    {
        Assert((Position < Length()) || (Position == 0));

        if (StringMisc::IsTerminator(Char) || (Length() == 0))
        {
            return 0;
        }

        const CharType* Start = CStr() + Position;
        const CharType* Result = StringMisc::FindChar(Start, Char);
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
     * Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     *
     * @param InString: String to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    FORCEINLINE SizeType ReverseFind(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return ReverseFind(InString, StringMisc::Length(InString), Position);
    }

    /**
     * Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
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
     * Find the position of the first occurrence of the start of the search-string. Searches the string in reverse.
     * Position is the end, instead of the start as with normal Find.
     *
     * @param InString: String to search
     * @param InString: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string
     */
    FORCEINLINE SizeType ReverseFind(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Assert((Position < Length()) || (Position == 0));
        Assert(InString != nullptr);

        SizeType Len = Length();
        if ((InLength == 0) || StringMisc::IsTerminator(*InString) || (Len == 0))
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
                else if (StringMisc::IsTerminator(*SubstringIt))
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(End - Start));
                }
            }
        }

        return NPos;
    }

    /**
     * Returns the position of the first occurrence of char. Searches the string in reverse.
     *
     * @param Char: Character to search for
     * @param Position: Position to start search at
     * @return: Returns the position of the first occurrence of the char
     */
    FORCEINLINE SizeType ReverseFind(CharType Char, SizeType Position = 0) const noexcept
    {
        Assert((Position < Length()) || (Position == 0));

        SizeType Len = Length();
        if (StringMisc::IsTerminator(Char) || (Len == 0))
        {
            return Len;
        }

        const CharType* Result = nullptr;
        const CharType* Start = CStr();
        if (Position == 0)
        {
            Result = StringMisc::ReverseFindChar(Start, Char);
        }
        else
        {
            // TODO: Get rid of const_cast
            CharType* TempCharacters = const_cast<CharType*>(Characters.Data());
            CharType TempChar = TempCharacters[Position + 1];
            TempCharacters[Position + 1] = StringMisc::Null;

            Result = StringMisc::ReverseFindChar(TempCharacters, Char);

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
     * Returns the position of the first occurrence of a character in the search-string that is found
     *
     * @param InString: String of characters to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string that is found
     */
    FORCEINLINE SizeType FindOneOf(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return FindOneOf(InString, StringMisc::Length(InString), Position);
    }

    /**
     * Returns the position of the first occurrence of a character in the search-string that is found. The search string is of string-type.
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
     * Returns the position of the first occurrence of a character in the search-string that is found
     *
     * @param InString: String of characters to search
     * @param InLength: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string that is found
     */
    FORCEINLINE SizeType FindOneOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Assert((Position < Length()) || (Position == 0));

        SizeType Len = Length();
        if ((InLength == 0) || StringMisc::IsTerminator(*InString) || (Len == 0))
        {
            return 0;
        }

        const CharType* Start = CStr() + Position;
        const CharType* Result = StringMisc::FindOneOf(Start, InString);
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
     * Returns the position of the last occurrence of a character in the search-string that is found
     *
     * @param InString: String of characters to search
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string that is found
     */
    FORCEINLINE SizeType ReverseFindOneOf(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return ReverseFindOneOf(InString, StringMisc::Length(InString), Position);
    }

    /**
     * Returns the position of the last occurrence of a character in the search-string that is found. The search string is of string-type.
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
     * Returns the position of the last occurrence of a character in the search-string that is found
     *
     * @param InString: String of characters to search
     * @param InLength: Length of the search-string
     * @param Position: Position to start search at
     * @return: Returns the position of the first character in the search-string that is found
     */
    FORCEINLINE SizeType ReverseFindOneOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Assert((Position < Length()) || (Position == 0));

        SizeType ThisLength = Length();
        if ((InLength == 0) || StringMisc::IsTerminator(*InString) || (ThisLength == 0))
        {
            return ThisLength;
        }

        if (Position != 0)
        {
            ThisLength = NMath::Min(Position, ThisLength);
        }

        SizeType SubstringLength = StringMisc::Length(InString);

        const CharType* Start = CStr();
        const CharType* End = Start + ThisLength;
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
     * Returns the position of the first occurrence of a character not a part of the search-string
     *
     * @param InString: String of characters that should be a part of the string
     * @param Position: Position to start the search at
     * @return: Return position the first character not a part of the search-string
     */
    FORCEINLINE SizeType FindOneNotOf(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return FindOneNotOf(InString, StringMisc::Length(InString), Position);
    }

    /**
     * Returns the position of the first character not a part of the search-string. The string is of a string-type.
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
     * Returns the position of the first occurrence of a character not a part of the search-string
     *
     * @param InString: String of characters that should be a part of the string
     * @param InLength: Length of the search-string
     * @param Position: Position to start the search at
     * @return: Return position the first character not a part of the search-string
     */
    FORCEINLINE SizeType FindOneNotOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Assert((Position < Length()) || (Position == 0));

        SizeType Len = Length();
        if ((InLength == 0) || StringMisc::IsTerminator(*InString) || (Len == 0))
        {
            return 0;
        }

        SizeType Pos = static_cast<SizeType>(StringMisc::RangeLength(CStr() + Position, InString));
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
     * Returns the position of the last occurrence of a character not a part of the search-string
     *
     * @param InString: String of characters that should be a part of the string
     * @param Position: Position to start the search at
     * @return: Return position the first character not a part of the search-string
     */
    FORCEINLINE SizeType ReverseFindOneNotOf(const CharType* InString, SizeType Position = 0) const noexcept
    {
        return ReverseFindOneNotOf(InString, StringMisc::Length(InString), Position);
    }

    /**
     * Returns the position of the last occurrence of a character not a part of the search-string. The string is of a string-type.
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
     * Returns the position of the last occurrence of a character not a part of the search-string
     *
     * @param InString: String of characters that should be a part of the string
     * @param InLength: Length of the search-string
     * @param Position: Position to start the search at
     * @return: Return position the first character not a part of the search-string
     */
    FORCEINLINE SizeType ReverseFindOneNotOf(const CharType* InString, SizeType InLength, SizeType Position = 0) const noexcept
    {
        Assert((Position < Length()) || (Position == 0));

        SizeType ThisLength = Length();
        if ((InLength == 0) || StringMisc::IsTerminator(*InString) || (ThisLength == 0))
        {
            return ThisLength;
        }

        if (Position != 0)
        {
            ThisLength = NMath::Min(Position, ThisLength);
        }

        SizeType SubstringLength = StringMisc::Length(InString);

        const CharType* Start = CStr();
        const CharType* End = Start + ThisLength;
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
                else if (StringMisc::IsTerminator(*SubstringStart))
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(End - Start));
                }
            }
        }

        return NPos;
    }

    /**
     * Check if the search-string exists within the string
     *
     * @param InString: String to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    FORCEINLINE bool Contains(const CharType* InString, SizeType InOffset = 0) const noexcept
    {
        return (Find(InString, InOffset) != NPos);
    }

    /**
     * Check if the search-string exists within the string. The string is of a string-type.
     *
     * @param InString: String to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type Contains(const StringType& InString, SizeType InOffset = 0) const noexcept
    {
        return (Find(InString, InOffset) != NPos);
    }

    /**
     * Check if the search-string exists within the string
     *
     * @param InString: String to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    FORCEINLINE bool Contains(const CharType* InString, SizeType InLength, SizeType InOffset = 0) const noexcept
    {
        return (Find(InString, InLength, InOffset) != NPos);
    }

    /**
     * Check if the character exists within the string
     *
     * @param InString: String to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    FORCEINLINE bool Contains(CharType Char, SizeType InOffset = 0) const noexcept
    {
        return (Find(Char, InOffset) != NPos);
    }

    /**
     * Check if the one of the characters exists within the string
     *
     * @param InString: String of characters to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    FORCEINLINE bool ContainsOneOf(const CharType* InString, SizeType InOffset = 0) const noexcept
    {
        return (FindOneOf(InString, InOffset) != NPos);
    }

    /**
     * Check if the one of the characters exists within the string. The string is of a string-type.
     *
     * @param InString: String of characters to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type ContainsOneOf(const StringType& InString, SizeType InLength, SizeType InOffset = 0) const noexcept
    {
        return (FindOneOf<StringType>(InString, InLength, InOffset) != NPos);
    }

    /**
     * Check if the one of the characters exists within the string
     *
     * @param InString: String of characters to search for
     * @param InLength: Length of the string with characters to search for
     * @param Position: Position to start to search at
     * @return: Returns true if the string is found
     */
    FORCEINLINE bool ContainsOneOf(const CharType* InString, SizeType InLength, SizeType InOffset = 0) const noexcept
    {
        return (FindOneOf(InString, InLength, InOffset) != NPos);
    }

    /**
     * Removes count characters from position and forward
     *
     * @param Position: Position to start remove at
     * @param NumCharacters: Number of characters to remove
     */
    FORCEINLINE void Remove(SizeType Position, SizeType Count) noexcept
    {
        Assert((Position < Length()) && (Position + Count < Length()));
        Characters.RemoveRangeAt(Position, Count);
    }

    /**
     * Insert a string at a specified position
     *
     * @param InString: String to insert
     * @param Position: Position to start the insertion at
     */
    FORCEINLINE void Insert(const CharType* InString, SizeType Position) noexcept
    {
        Insert(InString, StringMisc::Length(InString), Position);
    }

    /**
     * Insert a string at a specified position. The string is of string-type.
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
     * Insert a string at a specified position
     *
     * @param InString: String to insert
     * @param InLength: Length of the string to insert
     * @param Position: Position to start the insertion at
     */
    FORCEINLINE void Insert(const CharType* InString, SizeType InLength, SizeType Position) noexcept
    {
        Assert((Position <= Length()));

        SizeType ThisLength = Length();
        if (Position == ThisLength || Characters.IsEmpty())
        {
            Append(InString, InLength);
        }
        else
        {
            Characters.Insert(Position, InString, InLength);
        }
    }

    /**
     * Insert a character at specified position
     *
     * @param Char: Character to insert
     * @param Position: Position to insert character at
     */
    FORCEINLINE void Insert(CharType Char, SizeType Position) noexcept
    {
        Insert(&Char, 1, Position);
    }

    /**
     * Replace a part of the string
     *
     * @param InString: String to replace
     * @param Position: Position to start the replacing at
     */
    FORCEINLINE void Replace(const CharType* InString, SizeType Position) noexcept
    {
        Replace(InString, StringMisc::Length(InString), Position);
    }

    /**
     * Replace a part of the string. String is of string-type.
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
     * Replace a part of the string
     *
     * @param InString: String to replace
     * @param InLenght: Length of the string to replace
     * @param Position: Position to start the replacing at
     */
    FORCEINLINE void Replace(const CharType* InString, SizeType InLength, SizeType Position) noexcept
    {
        Assert((Position < Length()) && (Position + InLength < Length()));
        StringMisc::Copy(Data() + Position, InString, InLength);
    }

    /**
     * Replace a character in the string
     *
     * @param Char: Character to replace
     * @param Position: Position of the character to replace
     */
    FORCEINLINE void Replace(CharType Char, SizeType Position = 0) noexcept
    {
        Assert((Position < Length()));

        CharType* PositionPtr = Characters.Data() + Position;
        *PositionPtr = Char;
    }

    /**
     * Insert a new character at the end
     *
     * @param Char: Character to insert at the end
     */
    FORCEINLINE void Push(CharType Char) noexcept
    {
        Append(Char);
    }

    /**
     * Remove the character at the end
     */
    FORCEINLINE void Pop() noexcept
    {
        SizeType Len = Length();
        Characters[Len] = StringMisc::Null;
        Characters.Pop();
    }

    /**
     * Swap this string with another
     *
     * @param Other: String to swap with
     */
    FORCEINLINE void Swap(TString& Other)
    {
        TString TempString(Move(*this));
        MoveFrom(Move(Other));
        Other.MoveFrom(Move(TempString));
    }

    /**
     * Create a sub-string of this string
     *
     * @param Position: Position to start the sub-string at
     * @param NumCharacters: Number of characters in the sub-string
     * @return: Returns a sub-string
     */
    FORCEINLINE TString SubString(SizeType Offset, SizeType Count) const noexcept
    {
        Assert((Offset < Length()) && (Offset + Count < Length()));
        return TString(Characters.Data() + Offset, Count);
    }

     /**
      * Create a sub-string view of this string
      *
      * @param Position: Position to start the sub-string at
      * @param NumCharacters: Number of characters in the sub-string
      * @return: Returns a sub-string view
      */
    FORCEINLINE ViewType SubStringView(SizeType Offset, SizeType Count) const noexcept
    {
        Assert((Offset < Length()) && (Offset + Count < Length()));
        return TStringView<CharType>(Characters.Data() + Offset, Count);
    }

    /**
     * Retrieve a element at a certain index of the string
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE CharType& At(SizeType Index) noexcept
    {
        Assert(Index < Length());
        return Data()[Index];
    }

    /**
     * Retrieve a element at a certain index of the string
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE const CharType& At(SizeType Index) const noexcept
    {
        Assert(Index < Length());
        return Data()[Index];
    }

    /**
     * Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE CharType* Data() noexcept
    {
        return Characters.Data();
    }

    /**
     * Retrieve the data of the array
     *
     * @return: Returns a pointer to the data of the array
     */
    FORCEINLINE const CharType* Data() const noexcept
    {
        return Characters.Data();
    }

    /**
     * Retrieve a null-terminated string
     *
     * @return: Returns a pointer containing a null-terminated string
     */
    FORCEINLINE const CharType* CStr() const noexcept
    {
        return (!Characters.IsEmpty()) ? Characters.Data() : StringMisc::Empty();
    }

    /**
     * Returns the size of the container
     *
     * @return: The current size of the container
     */
    FORCEINLINE SizeType Size() const noexcept
    {
        SizeType Len = Characters.Size();
        return (Len > 0) ? (Len - 1) : 0; // Characters contains null-terminator
    }

    /**
     * Retrieve the last index that can be used to retrieve an element from the array
     *
     * @return: Returns a the index to the last element of the array
     */
    FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        SizeType Len = Size();
        return (Len > 0) ? (Len - 1) : 0;
    }

    /**
     * Returns the length of the string
     *
     * @return: The current length of the string
     */
    FORCEINLINE SizeType Length() const noexcept
    {
        return Size();
    }

    /**
     * Returns the capacity of the container
     *
     * @return: The current capacity of the container
     */
    FORCEINLINE SizeType Capacity() const noexcept
    {
        return Characters.Capacity();
    }

    /**
     * Returns the capacity of the container in bytes
     *
     * @return: The current capacity of the container in bytes
     */
    FORCEINLINE SizeType CapacityInBytes() const noexcept
    {
        return Characters.Capacity() * sizeof(CharType);
    }

    /**
     * Returns the size of the container in bytes
     *
     * @return: The current size of the container in bytes
     */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Length() * sizeof(CharType);
    }

    /**
     * Check if the container contains any elements
     *
     * @return: Returns true if the array is empty or false if it contains elements
     */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (Length() == 0);
    }

    /**
     * Retrieve the first element of the array
     *
     * @return: Returns a reference to the first element of the array
     */
    FORCEINLINE ElementType& FirstElement() noexcept
    {
        Assert(!IsEmpty());
        return Data()[0];
    }

    /**
     * Retrieve the first element of the array
     *
     * @return: Returns a reference to the first element of the array
     */
    FORCEINLINE const ElementType& FirstElement() const noexcept
    {
        Assert(!IsEmpty());
        return Data()[0];
    }

    /**
     * Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the array
     */
    FORCEINLINE ElementType& LastElement() noexcept
    {
        Assert(!IsEmpty());
        return Data()[LastElementIndex()];
    }

    /**
     * Retrieve the last element of the array
     *
     * @return: Returns a reference to the last element of the array
     */
    FORCEINLINE const ElementType& LastElement() const noexcept
    {
        Assert(!IsEmpty());
        return Data()[LastElementIndex()];
    }

public:

    /**
     * Appends a character to this string
     *
     * @param Rhs: Character to append
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TString& operator+=(CharType Rhs) noexcept
    {
        Append(Rhs);
        return *this;
    }

    /**
     * Appends a raw-string to this string
     *
     * @param Rhs: String to append
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TString& operator+=(const CharType* Rhs) noexcept
    {
        Append(Rhs);
        return *this;
    }

    /**
     * Appends a string of a string-type to this string
     *
     * @param Rhs: String to append
     * @return: Returns a reference to this instance
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, typename TAddReference<TString>::LValue>::Type operator+=(const StringType& Rhs) noexcept
    {
        Append<StringType>(Rhs);
        return *this;
    }

    /**
     * Bracket-operator to retrieve an element at a certain index
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE CharType& operator[](SizeType Index) noexcept
    {
        return At(Index);
    }

    /**
     * Bracket-operator to retrieve an element at a certain index
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    FORCEINLINE const CharType& operator[](SizeType Index) const noexcept
    {
        return At(Index);
    }

    /**
     * Assignment operator that takes a raw string
     *
     * @param InString: String to copy
     * @return: Return a reference to this instance
     */
    FORCEINLINE TString& operator=(const CharType* Rhs) noexcept
    {
        TString(Rhs).Swap(*this);
        return *this;
    }

    /**
     * Copy-assignment operator
     *
     * @param Rhs: String to copy
     * @return: Return a reference to this instance
     */
    FORCEINLINE TString& operator=(const TString& Rhs) noexcept
    {
        TString(Rhs).Swap(*this);
        return *this;
    }

    /**
     * Copy-assignment operator that takes another string-type
     *
     * @param Rhs: String to copy
     * @return: Return a reference to this instance
     */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, typename TAddReference<TString>::LValue>::Type operator=(const StringType& Rhs) noexcept
    {
        TString<StringType>(Rhs).Swap(*this);
        return *this;
    }

    /**
     * Move-assignment operator
     *
     * @param Rhs: String to move
     * @return: Return a reference to this instance
     */
    FORCEINLINE TString& operator=(TString&& Rhs) noexcept
    {
        TString(Move(Rhs)).Swap(*this);
        return *this;
    }

public:

    /**
      * Retrieve an iterator to the beginning of the array
      *
      * @return: A iterator that points to the first element
      */
    FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType(*this, 0);
    }

    /**
     * Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType(*this, Size());
    }

    /**
     * Retrieve an iterator to the beginning of the array
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType(*this, 0);
    }

    /**
     * Retrieve an iterator to the end of the array
     *
     * @return: A iterator that points to the element past the end
     */
    FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType(*this, Size());
    }

    /**
     * Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType(*this, Size());
    }

    /**
     * Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType(*this, 0);
    }

    /**
     * Retrieve an reverse-iterator to the end of the array
     *
     * @return: A reverse-iterator that points to the last element
     */
    FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, Size());
    }

    /**
     * Retrieve an reverse-iterator to the start of the array
     *
     * @return: A reverse-iterator that points to the element before the first element
     */
    FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType(*this, 0);
    }

public:

    /**
     * STL start iterator, same as TArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE IteratorType begin() noexcept
    {
        return StartIterator();
    }

    /**
     * STL end iterator, same as TArray::EndIterator
     *
     * @return: A iterator that points past the last element
     */
    FORCEINLINE IteratorType end() noexcept
    {
        return EndIterator();
    }

    /**
     * STL start iterator, same as TArray::StartIterator
     *
     * @return: A iterator that points to the first element
     */
    FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    /**
     * STL end iterator, same as TArray::EndIterator
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
        Characters.Resize(InLength);
        StringMisc::Copy(Characters.Data(), InString, InLength);
        Characters.Emplace(StringMisc::Null);
    }

    FORCEINLINE void MoveFrom(TString&& Other) noexcept
    {
        Characters = Move(Other.Characters);
    }

    StorageType Characters;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Predefined types

using String = TString<char>;
using WString = TString<wchar_t>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TString Operators

template<typename CharType>
inline TString<CharType> operator+(const TString<CharType>& Lhs, const TString<CharType>& Rhs) noexcept
{
    const typename TString<CharType>::SizeType CombinedSize = Lhs.Length() + Rhs.Length();

    TString<CharType> NewString;
    NewString.Reserve(CombinedSize);
    NewString.Append(Lhs);
    NewString.Append(Rhs);
    return NewString;
}

template<typename CharType>
inline TString<CharType> operator+(const CharType* Lhs, const TString<CharType>& Rhs) noexcept
{
    const typename TString<CharType>::SizeType CombinedSize = TStringMisc<CharType>::Length(Lhs) + Rhs.Length();

    TString<CharType> NewString;
    NewString.Reserve(CombinedSize);
    NewString.Append(Lhs);
    NewString.Append(Rhs);
    return NewString;
}

template<typename CharType>
inline TString<CharType> operator+(const TString<CharType>& Lhs, const CharType* Rhs) noexcept
{
    const typename TString<CharType>::SizeType CombinedSize = Lhs.Length() + TStringMisc<CharType>::Length(Rhs);

    TString<CharType> NewString;
    NewString.Reserve(CombinedSize);
    NewString.Append(Lhs);
    NewString.Append(Rhs);
    return NewString;
}

template<typename CharType>
inline TString<CharType> operator+(CharType Lhs, const TString<CharType>& Rhs) noexcept
{
    const typename TString<CharType>::SizeType CombinedSize = Rhs.Length() + 1;

    TString<CharType> NewString;
    NewString.Reserve(CombinedSize);
    NewString.Append(Lhs);
    NewString.Append(Rhs);
    return NewString;
}

template<typename CharType>
inline TString<CharType> operator+(const TString<CharType>& Lhs, CharType Rhs) noexcept
{
    const typename TString<CharType>::SizeType CombinedSize = Lhs.Length() + 1;

    TString<CharType> NewString;
    NewString.Reserve(CombinedSize);
    NewString.Append(Lhs);
    NewString.Append(Rhs);
    return NewString;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TString comparison operators

template<typename CharType>
inline bool operator==(const TString<CharType>& Lhs, const CharType* Rhs) noexcept
{
    return (Lhs.Compare(Rhs) == 0);
}

template<typename CharType>
inline bool operator==(const CharType* Lhs, const TString<CharType>& Rhs) noexcept
{
    return (Rhs.Compare(Lhs) == 0);
}

template<typename CharType>
inline bool operator==(const TString<CharType>& Lhs, const TString<CharType>& Rhs) noexcept
{
    return (Lhs.Compare(Rhs) == 0);
}

template<typename CharType>
inline bool operator!=(const TString<CharType>& Lhs, const CharType* Rhs) noexcept
{
    return !(Lhs == Rhs);
}

template<typename CharType>
inline bool operator!=(const CharType* Lhs, const TString<CharType>& Rhs) noexcept
{
    return !(Lhs == Rhs);
}

template<typename CharType>
inline bool operator!=(const TString<CharType>& Lhs, const TString<CharType>& Rhs) noexcept
{
    return !(Lhs == Rhs);
}

template<typename CharType>
inline bool operator<(const TString<CharType>& Lhs, const CharType* Rhs) noexcept
{
    return (Lhs.Compare(Rhs) < 0);
}

template<typename CharType>
inline bool operator<(const CharType* Lhs, const TString<CharType>& Rhs) noexcept
{
    return (Rhs.Compare(Lhs) < 0);
}

template<typename CharType>
inline bool operator<(const TString<CharType>& Lhs, const TString<CharType>& Rhs) noexcept
{
    return (Lhs.Compare(Rhs) < 0);
}

template<typename CharType>
inline bool operator<=(const TString<CharType>& Lhs, const CharType* Rhs) noexcept
{
    return (Lhs.Compare(Rhs) <= 0);
}

template<typename CharType>
inline bool operator<=(const CharType* Lhs, const TString<CharType>& Rhs) noexcept
{
    return (Rhs.Compare(Lhs) <= 0);
}

template<typename CharType>
inline bool operator<=(const TString<CharType>& Lhs, const TString<CharType>& Rhs) noexcept
{
    return (Lhs.Compare(Rhs) <= 0);
}

template<typename CharType>
inline bool operator>(const TString<CharType>& Lhs, const CharType* Rhs) noexcept
{
    return (Lhs.Compare(Rhs) > 0);
}

template<typename CharType>
inline bool operator>(const CharType* Lhs, const TString<CharType>& Rhs) noexcept
{
    return (Rhs.Compare(Lhs) > 0);
}

template<typename CharType>
inline bool operator>(const TString<CharType>& Lhs, const TString<CharType>& Rhs) noexcept
{
    return (Lhs.Compare(Rhs) > 0);
}

template<typename CharType>
inline bool operator>=(const TString<CharType>& Lhs, const CharType* Rhs) noexcept
{
    return (Lhs.Compare(Rhs) >= 0);
}

template<typename CharType>
inline bool operator>=(const CharType* Lhs, const TString<CharType>& Rhs) noexcept
{
    return (Rhs.Compare(Lhs) >= 0);
}

template<typename CharType>
inline bool operator>=(const TString<CharType>& Lhs, const TString<CharType>& Rhs) noexcept
{
    return (Lhs.Compare(Rhs) >= 0);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Add TString to be a string-type

template<typename CharType>
struct TIsTStringType<TString<CharType>>
{
    enum
    {
        Value = true
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// char and wide conversion functions

inline WString CharToWide(const String& CharString) noexcept
{
    WString NewString;
    NewString.Resize(CharString.Length());

    mbstowcs(NewString.Data(), CharString.CStr(), CharString.Length());

    return NewString;
}

inline String WideToChar(const WString& WideString) noexcept
{
    String NewString;
    NewString.Resize(WideString.Length());

    wcstombs(NewString.Data(), WideString.CStr(), WideString.Length());

    return NewString;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Hashing for strings

template<typename TChar>
struct TStringHasher
{
    // Jenkins's one_at_a_time hash: https://en.wikipedia.org/wiki/Jenkins_hash_function
    FORCEINLINE size_t operator()(const TString<TChar>& String) const
    {
        // TODO: Investigate how good is this for wide chars

        const TChar* Key = String.CStr();
        int32 Length = String.Length();

        int32  Index = 0;
        uint64 Hash = 0;
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Predefined hashing types

using SStringHasher     = TStringHasher<char>;
using SWideStringHasher = TStringHasher<wchar_t>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper for converting to a string

template<typename T>
typename TEnableIf<TIsFloatingPoint<T>::Value, String>::Type ToString(T Element)
{
    return String::MakeFormated("%f", Element);
}

template<typename T>
typename TEnableIf<TNot<TIsFloatingPoint<T>>::Value, String>::Type ToString(T Element);

template<>
inline String ToString<int32>(int32 Element)
{
    return String::MakeFormated("%d", Element);
}

template<>
inline String ToString<int64>(int64 Element)
{
    return String::MakeFormated("%lld", Element);
}

template<>
inline String ToString<uint32>(uint32 Element)
{
    return String::MakeFormated("%u", Element);
}

template<>
inline String ToString<uint64>(uint64 Element)
{
    return String::MakeFormated("%llu", Element);
}

template<>
inline String ToString<bool>(bool bElement)
{
    return String(bElement ? "true" : "false");
}

template<typename T>
typename TEnableIf<TIsFloatingPoint<T>::Value, WString>::Type ToWideString(T Element)
{
    return WString::MakeFormated(L"%f", Element);
}

template<typename T>
typename TEnableIf<TNot<TIsFloatingPoint<T>>::Value, WString>::Type ToWideString(T Element);

template<>
inline WString ToWideString<int32>(int32 Element)
{
    return WString::MakeFormated(L"%d", Element);
}

template<>
inline WString ToWideString<int64>(int64 Element)
{
    return WString::MakeFormated(L"%lld", Element);
}

template<>
inline WString ToWideString<uint32>(uint32 Element)
{
    return WString::MakeFormated(L"%u", Element);
}

template<>
inline WString ToWideString<uint64>(uint64 Element)
{
    return WString::MakeFormated(L"%llu", Element);
}

template<>
inline WString ToWideString<bool>(bool bElement)
{
    return WString(bElement ? L"true" : L"false");
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper for converting from a string

template<typename T>
inline typename TEnableIf<TNot<TIsIntegerNotBool<T>>::Value, bool>::Type FromString(const String& Value, T& OutElement);

template<typename T>
inline typename TEnableIf<TIsIntegerNotBool<T>::Value, bool>::Type FromString(const String& Value, T& OutElement)
{
    // TODO: Improve with more than base 10
    char* End;
    OutElement = CStringParse::ParseInt<T>(Value.CStr(), &End, 10);
    return (*End != 0);
}

template<>
inline bool FromString<float>(const String& Value, float& OutElement)
{
    char* End;
    OutElement = CStringParse::ParseFloat(Value.CStr(), &End);
    return (*End != 0);
}

template<>
inline bool FromString<double>(const String& Value, double& OutElement)
{
    char* End;
    OutElement = CStringParse::ParseDouble(Value.CStr(), &End);
    return (*End != 0);
}

template<>
inline bool FromString<bool>(const String& Value, bool& OutElement)
{
    char* End;
    OutElement = static_cast<bool>(CStringParse::ParseInt32(Value.CStr(), &End, 10));
    if (*End)
    {
        return true;
    }

    return false;
}

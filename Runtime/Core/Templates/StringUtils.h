#pragma once
#include "Core/Memory/Memory.h"

#include <cstring>
#include <cwchar>
#include <cctype>
#include <cwctype>
#include <cstdarg>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers for characters */

template<typename CharType>
class TStringUtils;

template<>
class TStringUtils<char>
{
public:

    using CharType = char;
    using Pointer = CharType*;
    using ConstPointer = const Pointer;

    static constexpr CharType Null = '\0';

    /**
     * Is the character a space
     * 
     * @param Char: Character to compare
     * @return: Returns true or false weather if the character is a space or not
     */
    static FORCEINLINE bool IsSpace(CharType Char) noexcept
    {
        return !!isspace(static_cast<int>(Char));
    }

    /**
     * Is the character a null-terminator
     *
     * @param Char: Character to compare
     * @return: Returns true or false weather if the character is a null-terminator or not
     */
    static FORCEINLINE bool IsTerminator(CharType Char) noexcept
    {
        return (Char == Null);
    }

    /**
     * Is the character a whitespace or null-terminator
     *
     * @param Char: Character to compare
     * @return: Returns true or false weather if the character is a null-terminator, whitespace or neither
     */
    static FORCEINLINE bool IsWhiteSpace(CharType Char) noexcept
    {
        return IsSpace(Char) || IsTerminator(Char);
    }

    /**
     * Search for a sub-string within a string
     * 
     * @param String: String to search
     * @param Substring: Sub-string to search for
     * @return: Returns the pointer to the first character in sub-string in the string, or nullptr if not found
     */
    static FORCEINLINE const CharType* Find(const CharType* String, const CharType* Substring) noexcept
    {
        return strstr(String, Substring);
    }

    /**
     * Finds the first occurrence of one of the characters in the set
     * 
     * @param String: String to search 
     * @param Set: Set of characters to search for
     * @return: Returns the pointer to the first character in string that is a part of the set, or nullptr if not found
     */
    static FORCEINLINE const CharType* FindOneOf(const CharType* String, const CharType* Set) noexcept
    {
        return strpbrk(String, Set);
    }

    /**
     * Finds the length of the range of characters only from the set
     * 
     * @param String: String to search
     * @param Set: Set of characters to search for
     * @return: Returns the length of the first range of characters in string that is only a part of the set
     */
    static FORCEINLINE int32 RangeLength(const CharType* String, const CharType* Set) noexcept
    {
        return static_cast<int32>(strspn(String, Set));
    }

    /**
     * Finds character in string
     * 
     * @param String: String to search
     * @param Char: Character to search for
     * @return: Returns the pointer to the first occurrence of char in the string, or nullptr if not found
     */
    static FORCEINLINE const CharType* FindChar(const CharType* String, CharType Char) noexcept
    {
        return strchr(String, Char);
    }

    /**
     * Finds character in string by searching backwards
     * 
     * @param String: String to search
     * @param Char: Character to search for 
     * @return: Returns the pointer to the first occurrence of char in the string, or nullptr if not found
     */
    static FORCEINLINE const CharType* ReverseFindChar(const CharType* String, CharType Char) noexcept
    {
        return strrchr(String, Char);
    }

    /**
     * Retrieve the length of a string
     * 
     * @param String: String to retrieve length of
     * @return: Returns the length of the string, not including null-terminator
     */
    static FORCEINLINE int32 Length(const CharType* String) noexcept
    {
        return (String != nullptr) ? static_cast<int32>(strlen(String)) : 0;
    }

    /**
     * Format string-buffer
     * 
     * @param Buffer: Buffer to store formatted string in
     * @param BufferLength: Length of the buffer 
     * @param Format: Formatted string to print into the buffer
     * @return: Returns the number of characters that were printed into the buffer
     */
    static int32 FormatBuffer(CharType* Buffer, int32 BufferLength, const CharType* Format, ...) noexcept
    {
        va_list Args;
        va_start(Args, Format);
        int32 Result = FormatBufferV(Buffer, BufferLength, Format, Args);
        va_end(Args);

        return Result;
    }

    /**
     * Format string-buffer with argument-list
     *
     * @param Buffer: Buffer to store formatted string in
     * @param BufferLength: Length of the buffer
     * @param Format: Formatted string to print into the buffer
     * @return: Returns the number of characters that were printed into the buffer
     */
    static FORCEINLINE int32 FormatBufferV(CharType* Buffer, int32 Len, const CharType* Format, va_list Args) noexcept
    {
        return vsnprintf(Buffer, Len, Format, Args);
    }

    /**
     * Convert character to lower-case
     * 
     * @param Char: Character to convert
     * @return: Returns the character in lower case
     */
    static FORCEINLINE CharType ToLower(CharType Char) noexcept
    {
        return static_cast<CharType>(tolower(static_cast<int>(Char)));
    }

    /**
     * Convert character to upper-case
     *
     * @param Char: Character to convert
     * @return: Returns the character in upper case
     */
    static FORCEINLINE CharType ToUpper(CharType Char) noexcept
    {
        return static_cast<CharType>(toupper(static_cast<int>(Char)));
    }

    /**
     * Copy two strings
     * 
     * @param Dest: String to copy to
     * @param Source: String to copy from
     * @return: Returns the pointer to the destination string
     */
    static FORCEINLINE CharType* Copy(CharType* Dest, const CharType* Source) noexcept
    {
        return Copy(Dest, Source, Length(Source));
    }

    /**
     * Copy two strings
     *
     * @param Dest: String to copy to
     * @param Source: String to copy from
     * @param InLength: Number of characters to copy
     * @return: Returns the pointer to the destination string
     */
    static FORCEINLINE CharType* Copy(CharType* Dest, const CharType* Source, uint64 InLength) noexcept
    {
        return reinterpret_cast<CharType*>(CMemory::Memcpy(Dest, Source, InLength * sizeof(CharType)));
    }

    /**
     * Move characters from one string to another
     *
     * @param Dest: String to move to
     * @param Source: String to move from
     * @return: Returns the destination string
     */
    static FORCEINLINE CharType* Move(CharType* Dest, const CharType* Source) noexcept
    {
        return Move(Dest, Source, Length(Source));
    }

    /**
     * Move characters from one string to another
     * 
     * @param Dest: String to move to
     * @param Source: String to move from
     * @param InLength: Number of characters to move
     * @return: Returns the destination string
     */
    static FORCEINLINE CharType* Move(CharType* Dest, const CharType* Source, uint64 InLength) noexcept
    {
        return reinterpret_cast<CharType*>(CMemory::Memmove(Dest, Source, InLength * sizeof(CharType)));
    }

    /**
     * Compare two strings 
     * 
     * @param LHS: Left-hand side to compare
     * @param RHS: Right-hand side to compare
     * @return: Returns zero if equal, otherwise the position of the character that is not equal
     */
    static FORCEINLINE int32 Compare(const CharType* LHS, const CharType* RHS) noexcept
    {
        return static_cast<int32>(strcmp(LHS, RHS));
    }

    /**
     * Compare two strings
     *
     * @param LHS: Left-hand side to compare
     * @param RHS: Right-hand side to compare
     * @param InLength: Length of the strings to compare
     * @return: Returns zero if equal, otherwise the position of the character that is not equal
     */
    static FORCEINLINE int32 Compare(const CharType* LHS, const CharType* RHS, uint64 InLength) noexcept
    {
        return static_cast<int32>(strncmp(LHS, RHS, InLength));
    }

    /**
     * Retrieve the empty string 
     * 
     * @return: Returns an empty string
     */
    static FORCEINLINE const CharType* Empty() noexcept
    {
        return "";
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Specialization for wide-chars

template<>
class TStringUtils<wchar_t>
{
public:

    using CharType = wchar_t;
    using Pointer = CharType*;
    using ConstPointer = const Pointer;

    static constexpr CharType Null = L'\0';

    /**
     * Is the character a space
     *
     * @param Char: Character to compare
     * @return: Returns true or false weather if the character is a space or not
     */
    static FORCEINLINE bool IsSpace(CharType Char) noexcept
    {
        return !!iswspace(static_cast<wint_t>(Char));
    }

    /**
     * Is the character a null-terminator
     *
     * @param Char: Character to compare
     * @return: Returns true or false weather if the character is a null-terminator or not
     */
    static FORCEINLINE bool IsTerminator(CharType Char) noexcept
    {
        return (Char == Null);
    }

    /**
     * Is the character a whitespace or null-terminator
     *
     * @param Char: Character to compare
     * @return: Returns true or false weather if the character is a null-terminator, whitespace or neither
     */
    static FORCEINLINE bool IsWhiteSpace(CharType Char) noexcept
    {
        return IsSpace(Char) || IsTerminator(Char);
    }

    /**
     * Search for a sub-string within a string
     *
     * @param String: String to search
     * @param Substring: Sub-string to search for
     * @return: Returns the pointer to the first character in sub-string in the string, or nullptr if not found
     */
    static FORCEINLINE const CharType* Find(const CharType* String, const CharType* Substring) noexcept
    {
        return wcsstr(String, Substring);
    }

    /**
     * Finds the first occurrence of one of the characters in the set
     *
     * @param String: String to search
     * @param Set: Set of characters to search for
     * @return: Returns the pointer to the first character in string that is a part of the set, or nullptr if not found
     */
    static FORCEINLINE const CharType* FindOneOf(const CharType* String, const CharType* Set) noexcept
    {
        return wcspbrk(String, Set);
    }

    /**
     * Finds the length of the range of characters only from the set
     *
     * @param String: String to search
     * @param Set: Set of characters to search for
     * @return: Returns the length of the first range of characters in string that is only a part of the set
     */
    static FORCEINLINE const CharType* FindChar(const CharType* String, CharType Char) noexcept
    {
        return wcschr(String, Char);
    }

    /**
     * Finds character in string
     *
     * @param String: String to search
     * @param Char: Character to search for
     * @return: Returns the pointer to the first occurrence of char in the string, or nullptr if not found
     */
    static FORCEINLINE const CharType* ReverseFindChar(const CharType* String, CharType Char) noexcept
    {
        return wcsrchr(String, Char);
    }

    /**
     * Finds character in string by searching backwards
     *
     * @param String: String to search
     * @param Char: Character to search for
     * @return: Returns the pointer to the first occurrence of char in the string, or nullptr if not found
     */
    static FORCEINLINE int32 RangeLength(const CharType* String, const CharType* Set) noexcept
    {
        return static_cast<int32>(wcsspn(String, Set));
    }

    /**
     * Retrieve the length of a string
     *
     * @param String: String to retrieve length of
     * @return: Returns the length of the string, not including null-terminator
     */
    static FORCEINLINE int32 Length(const CharType* String) noexcept
    {
        return (String != nullptr) ? static_cast<int32>(wcslen(String)) : 0;
    }

    /**
     * Format string-buffer
     *
     * @param Buffer: Buffer to store formatted string in
     * @param BufferLength: Length of the buffer
     * @param Format: Formatted string to print into the buffer
     * @return: Returns the number of characters that were printed into the buffer
     */
    static int32 FormatBuffer(CharType* Buffer, int32 BufferLength, const CharType* Format, ...) noexcept
    {
        va_list Args;
        va_start(Args, Format);
        int32 Result = FormatBufferV(Buffer, BufferLength, Format, Args);
        va_end(Args);

        return Result;
    }

    /**
     * Format string-buffer with argument-list
     *
     * @param Buffer: Buffer to store formatted string in
     * @param BufferLength: Length of the buffer
     * @param Format: Formatted string to print into the buffer
     * @return: Returns the number of characters that were printed into the buffer
     */
    static FORCEINLINE int32 FormatBufferV(CharType* Data, int32 Len, const CharType* Format, va_list Args) noexcept
    {
        return vswprintf(Data, Len, Format, Args);
    }

     /**
      * Convert character to lower-case
      *
      * @param Char: Character to convert
      * @return: Returns the character in lower case
      */
    static FORCEINLINE CharType ToLower(CharType Char) noexcept
    {
        return static_cast<CharType>(towlower(static_cast<wint_t>(Char)));
    }

    /**
     * Convert character to upper-case
     *
     * @param Char: Character to convert
     * @return: Returns the character in upper case
     */
    static FORCEINLINE CharType ToUpper(CharType Char) noexcept
    {
        return static_cast<CharType>(towupper(static_cast<wint_t>(Char)));
    }

    /**
     * Copy two strings
     *
     * @param Dest: String to copy to
     * @param Source: String to copy from
     * @return: Returns the pointer to the destination string
     */
    static FORCEINLINE CharType* Copy(CharType* Dest, const CharType* Source) noexcept
    {
        return Copy(Dest, Source, Length(Source));
    }

    /**
     * Copy two strings
     *
     * @param Dest: String to copy to
     * @param Source: String to copy from
     * @param InLength: Number of characters to copy
     * @return: Returns the pointer to the destination string
     */
    static FORCEINLINE CharType* Copy(CharType* Dest, const CharType* Source, uint64 Length) noexcept
    {
        return reinterpret_cast<CharType*>(CMemory::Memcpy(Dest, Source, Length * sizeof(CharType)));
    }

    /**
     * Move characters from one string to another
     *
     * @param Dest: String to move to
     * @param Source: String to move from
     * @return: Returns the destination string
     */
    static FORCEINLINE CharType* Move(CharType* Dest, const CharType* Source) noexcept
    {
        return Move(Dest, Source, Length(Source));
    }

    /**
     * Move characters from one string to another
     *
     * @param Dest: String to move to
     * @param Source: String to move from
     * @param InLength: Number of characters to move
     * @return: Returns the destination string
     */
    static FORCEINLINE CharType* Move(CharType* Dest, const CharType* Source, uint64 Length) noexcept
    {
        return reinterpret_cast<CharType*>(CMemory::Memmove(Dest, Source, Length * sizeof(CharType)));
    }

    /**
     * Compare two strings
     *
     * @param LHS: Left-hand side to compare
     * @param RHS: Right-hand side to compare
     * @return: Returns zero if equal, otherwise the position of the character that is not equal
     */
    static FORCEINLINE int32 Compare(const CharType* LHS, const CharType* RHS) noexcept
    {
        return static_cast<int32>(wcscmp(LHS, RHS));
    }

    /**
     * Compare two strings
     *
     * @param LHS: Left-hand side to compare
     * @param RHS: Right-hand side to compare
     * @param InLength: Length of the strings to compare
     * @return: Returns zero if equal, otherwise the position of the character that is not equal
     */
    static FORCEINLINE int32 Compare(const CharType* LHS, const CharType* RHS, uint64 Length) noexcept
    {
        return static_cast<int32>(wcsncmp(LHS, RHS, Length));
    }

    /**
     * Retrieve the empty string
     *
     * @return: Returns an empty string
     */
    static FORCEINLINE const CharType* Empty() noexcept
    {
        return L"";
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Predefined types

using CStringUtils = TStringUtils<char>;
using WStringUtils = TStringUtils<wchar_t>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers for StringParsing

template<typename CharType>
class TStringParse;

template<>
class TStringParse<char>
{
public:

    using CharType = char;

    template<typename T>
    static typename TEnableIf<TIsIntegerNotBool<T>::Value, T>::Type ParseInt(const CharType* String, CharType** End, int32 Base);

    /**
     * Parse a int8
     * 
     * @param String: String to parse from
     * @param End: Pointer that gets set to the character after the parsed value in the string
     * @param Base: Base of the value to parse, set to zero if the base should be determined
     * @return: Returns the parsed value on success, zero otherwise
     */
    static FORCEINLINE int8 ParseInt8(const CharType* String, CharType** End, int32 Base)
    {
        return static_cast<int8>(strtol(String, End, Base));
    }

    /**
     * Parse a int16
     *
     * @param String: String to parse from
     * @param End: Pointer that gets set to the character after the parsed value in the string
     * @param Base: Base of the value to parse, set to zero if the base should be determined
     * @return: Returns the parsed value on success, zero otherwise
     */
    static FORCEINLINE int16 ParseInt16(const CharType* String, CharType** End, int32 Base)
    {
        return static_cast<int16>(strtol(String, End, Base));
    }

    /**
     * Parse a int32
     *
     * @param String: String to parse from
     * @param End: Pointer that gets set to the character after the parsed value in the string
     * @param Base: Base of the value to parse, set to zero if the base should be determined
     * @return: Returns the parsed value on success, zero otherwise
     */
    static FORCEINLINE int32 ParseInt32(const CharType* String, CharType** End, int32 Base)
    {
        return static_cast<int32>(strtol(String, End, Base));
    }

    /**
     * Parse a int64
     *
     * @param String: String to parse from
     * @param End: Pointer that gets set to the character after the parsed value in the string
     * @param Base: Base of the value to parse, set to zero if the base should be determined
     * @return: Returns the parsed value on success, zero otherwise
     */
    static FORCEINLINE int64 ParseInt64(const CharType* String, CharType** End, int32 Base)
    {
        return static_cast<int64>(strtoll(String, End, Base));
    }

    /**
     * Parse a uint8
     *
     * @param String: String to parse from
     * @param End: Pointer that gets set to the character after the parsed value in the string
     * @param Base: Base of the value to parse, set to zero if the base should be determined
     * @return: Returns the parsed value on success, zero otherwise
     */
    static FORCEINLINE uint8 ParseUint8(const CharType* String, CharType** End, int32 Base)
    {
        return static_cast<uint8>(strtoul(String, End, Base));
    }

    /**
     * Parse a uint16
     *
     * @param String: String to parse from
     * @param End: Pointer that gets set to the character after the parsed value in the string
     * @param Base: Base of the value to parse, set to zero if the base should be determined
     * @return: Returns the parsed value on success, zero otherwise
     */
    static FORCEINLINE uint16 ParseUint16(const CharType* String, CharType** End, int32 Base)
    {
        return static_cast<uint16>(strtoul(String, End, Base));
    }

    /**
     * Parse a uint32
     *
     * @param String: String to parse from
     * @param End: Pointer that gets set to the character after the parsed value in the string
     * @param Base: Base of the value to parse, set to zero if the base should be determined
     * @return: Returns the parsed value on success, zero otherwise
     */
    static FORCEINLINE uint32 ParseUint32(const CharType* String, CharType** End, int32 Base)
    {
        return static_cast<uint32>(strtoul(String, End, Base));
    }

    /**
     * Parse a uint64
     *
     * @param String: String to parse from
     * @param End: Pointer that gets set to the character after the parsed value in the string
     * @param Base: Base of the value to parse, set to zero if the base should be determined
     * @return: Returns the parsed value on success, zero otherwise
     */
    static FORCEINLINE uint64 ParseUint64(const CharType* String, CharType** End, int32 Base)
    {
        return static_cast<uint64>(strtoull(String, End, Base));
    }

    /**
     * Parse a float
     *
     * @param String: String to parse from
     * @param End: Pointer that gets set to the character after the parsed value in the string
     * @return: Returns the parsed value on success, zero otherwise
     */
    static FORCEINLINE float ParseFloat(const CharType* String, CharType** End)
    {
        return static_cast<float>(strtold(String, End));
    }

    /**
     * Parse a double
     *
     * @param String: String to parse from
     * @param End: Pointer that gets set to the character after the parsed value in the string
     * @return: Returns the parsed value on success, zero otherwise
     */
    static FORCEINLINE double ParseDouble(const CharType* String, CharType** End)
    {
        return static_cast<double>(strtold(String, End));
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Implementation

template<>
FORCEINLINE int8 TStringParse<char>::ParseInt<int8>(const CharType* String, CharType** End, int32 Base)
{
    return ParseInt8(String, End, Base);
}

template<>
FORCEINLINE int16 TStringParse<char>::ParseInt<int16>(const CharType* String, CharType** End, int32 Base)
{
    return ParseInt16(String, End, Base);
}

template<>
FORCEINLINE int32 TStringParse<char>::ParseInt<int32>(const CharType* String, CharType** End, int32 Base)
{
    return ParseInt32(String, End, Base);
}

template<>
FORCEINLINE int64 TStringParse<char>::ParseInt<int64>(const CharType* String, CharType** End, int32 Base)
{
    return ParseInt64(String, End, Base);
}

template<>
FORCEINLINE uint8 TStringParse<char>::ParseInt<uint8>(const CharType* String, CharType** End, int32 Base)
{
    return ParseUint8(String, End, Base);
}

template<>
FORCEINLINE uint16 TStringParse<char>::ParseInt<uint16>(const CharType* String, CharType** End, int32 Base)
{
    return ParseUint16(String, End, Base);
}

template<>
FORCEINLINE uint32 TStringParse<char>::ParseInt<uint32>(const CharType* String, CharType** End, int32 Base)
{
    return ParseUint32(String, End, Base);
}

template<>
FORCEINLINE uint64 TStringParse<char>::ParseInt<uint64>(const CharType* String, CharType** End, int32 Base)
{
    return ParseUint64(String, End, Base);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Predefined types

using CStringParse = TStringParse<char>;
using WStringParse = TStringParse<wchar_t>;

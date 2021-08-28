#pragma once
#if 0

#include "Array.h"
#include "StringView.h"

#include "Core/Templates/EnableIf.h"
#include "Core/Templates/Identity.h"
#include "Core/Templates/AddReference.h"


#define STRING_USE_INLINE_ALLOCATOR (0)

#if STRING_USE_INLINE_ALLOCATOR 
#define STRING_ALLOCATOR_INLINE_BYTES (24)

template<typename CharType>
using TStringAllocator = TInlineArrayAllocator<CharType, STRING_ALLOCATOR_INLINE_BYTES>;
#else
template<typename CharType>
using TStringAllocator = TDefaultArrayAllocator<CharType>;
#endif

/* Storage class with a fixed allocated number of characters */
template<typename CharType>
class TString
{
public:

    static_assert(TIsSame<CharType, char>::Value || TIsSame<CharType, wchar_t>::Value, "Only char and wchar_t is supported for strings");

    /* Types */
    using ElementType = CharType;
    using SizeType = int32;
    using StringTraits = TStringTraits<CharType>;

    /* Constants */
    enum
    {
        InvalidPosition = SizeType( -1 )
    };

    /* Iterators */
    typedef TArrayIterator<TString, CharType>                    IteratorType;
    typedef TArrayIterator<const TString, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<TString, CharType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TString, const CharType> ReverseConstIteratorType;

    /* Empty constructor */
    FORCEINLINE TString() noexcept
        : Storage()
    {
    }

    /* Create a fixed string from a raw array. If the string is longer than
       the allocators the string will be shortened to fit. */
    FORCEINLINE TString( const CharType* InString ) noexcept
        : Storage()
    {
        if ( InString )
        {
            CopyFrom( InString, StringTraits::Length( InString ) );
        }
    }

    /* Create a fixed string from a raw array and the length. If the string is
       longer than the allocators the string will be shortened to fit. */
    FORCEINLINE explicit TString( const CharType* InString, uint32 InLength ) noexcept
        : Storage()
    {
        if ( InString )
        {
            CopyFrom( InString, InLength );
        }
    }

    /* Create a new string from another string type with similar interface. If the
       string is longer than the allocators the string will be shortened to fit. */
    template<typename StringType>
    FORCEINLINE explicit TString( const StringType& InString ) noexcept
        : Storage()
    {
        CopyFrom( InString.CStr(), InString.Length() );
    }

    /* Copy Constructor */
    FORCEINLINE TString( const TString& Other ) noexcept
        : Storage()
    {
        CopyFrom( Other.CStr(), Other.Length() );
    }

    /* Move Constructor */
    FORCEINLINE TString( TString&& Other ) noexcept
        : Storage()
    {
        MoveFrom( Forward<TString>( Other ) );
    }

    /* Move Constructor */
    template<uint32 OtherLength>
    FORCEINLINE TString( TString<CharType, OtherLength>&& Other ) noexcept
        : Storage()
    {
        MoveFrom( Forward<TString<CharType, OtherLength>>( Other ) );
    }

    /* Retrive the character at Index */
    FORCEINLINE CharType& At( SizeType Index ) noexcept
    {
        Assert( Index < Length() );
        return Data()[Index];
    }

    /* Retrive the character at Index */
    FORCEINLINE const CharType& At( SizeType Index ) const noexcept
    {
        Assert( Index < Length() );
        return Data()[Index];
    }

    /* Appends a string to this string */
    FORCEINLINE void Append( CharType Char ) noexcept
    {
        Assert( StrLength + 1 < Capacity() );

        Storage.LastElement() = Char;
        Storage.PushBack( static_cast<CharType>(StringTraits::Terminator) );
    }

    /* Appends a string to this string */
    FORCEINLINE void Append( const CharType* InString ) noexcept
    {
        Append( InString, StringTraits::Length( InString ) );
    }

    /* Appends a string to this string */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value>::Type Append( const StringType& InString ) noexcept
    {
        Append( InString.CStr(), InString.Length() );
    }

    /* Appends a string to this string */
    FORCEINLINE void Append( const CharType* InString, SizeType InLength ) noexcept
    {
        Assert( InString != nullptr );
        Assert( StrLength + InLength < Capacity() );

        for ( SizeType Index = 0; Index < InLength; Index++ )
        {
            Storage[StrLength + Index] = InString[Index];
        }

        StrLength = StrLength + InLength;
        Storage[StrLength] = StringTraits::Terminator;
    }

    /* Format string (similar to snprintf) */
    FORCEINLINE void Format( const CharType* Format, ... ) noexcept
    {
        va_list ArgList;
        va_start( ArgList, Format );
        FormatV( Format, ArgList );
        va_end( ArgList );
    }

    /* Format string with a va_list (similar to snprintf) */
    FORCEINLINE void FormatV( const CharType* Format, va_list ArgList ) noexcept
    {
        const SizeType WrittenChars = StringTraits::PrintVA( Storage, NumChars, Format, ArgList );
        if ( WrittenChars < NumChars )
        {
            StrLength = WrittenChars;
        }
        else
        {
            StrLength = NumChars - 1;
        }

        Storage[StrLength] = StringTraits::Terminator;
    }

    /* Same as Format, but appends the result to the current string */
    FORCEINLINE void AppendFormat( const CharType* Format, ... ) noexcept
    {
        va_list ArgList;
        va_start( ArgList, Format );
        AppendFormatV( Format, ArgList );
        va_end( ArgList );
    }

    /* Same as FormatV, but appends the result to the current string */
    FORCEINLINE void AppendFormatV( const CharType* Format, va_list ArgList ) noexcept
    {
        const SizeType WrittenChars = StringTraits::PrintVA( Storage + StrLength, NumChars, Format, ArgList );
        const SizeType NewLength = StrLength + WrittenChars;
        if ( NewLength < NumChars )
        {
            StrLength = NewLength;
        }
        else
        {
            StrLength = NumChars - 1;
        }

        Storage[StrLength] = StringTraits::Terminator;
    }

    /* Clears the string */
    FORCEINLINE void Clear() noexcept
    {
        StrLength = 0;
        Storage[StrLength] = StringTraits::Terminator;
    }

    /* Returns this string in lowercase */
    FORCEINLINE void ToLowerInline() noexcept
    {
        for ( SizeType Index = 0; Index < StrLength; Index++ )
        {
            Storage[Index] = StringTraits::ToLower( Storage[Index] );
        }
    }

    /* Returns this string in lowercase */
    FORCEINLINE TString ToLower() const noexcept
    {
        TString Result = *this;
        Result.ToLowerInline();
        return Result;
    }

    /* Converts this string in uppercase */
    FORCEINLINE void ToUpperInline() noexcept
    {
        for ( SizeType Index = 0; Index < StrLength; Index++ )
        {
            Storage[Index] = StringTraits::ToUpper( Storage[Index] );
        }
    }

    /* Returns this string in uppercase */
    FORCEINLINE TString ToUpper() const noexcept
    {
        TString Result = *this;
        Result.ToUpperInline();
        return Result;
    }

    /* Removes whitespace from the beginning and end of the string */
    FORCEINLINE TString Trim() noexcept
    {
        TString Result = *this;
        Result.TrimInline();
        return Result;
    }

    /* Removes whitespace from the beginning and end of the string */
    FORCEINLINE void TrimInline() noexcept
    {
        TrimStartInline();
        TrimEndInline();
    }

    /* Removes whitespace from the beginning of the string */
    FORCEINLINE TString TrimStart() noexcept
    {
        TString Result = *this;
        Result.TrimStartInline();
        return Result;
    }

    /* Removes whitespace from the beginning of the string */
    FORCEINLINE void TrimStartInline() noexcept
    {
        SizeType Index = 0;
        for ( ; Index < StrLength; Index++ )
        {
            if ( StringTraits::IsWhiteSpace( Storage[Index] ) )
            {
                break;
            }
        }

        if ( Index > 0 )
        {
            StrLength = StrLength - Index;
            Memory::Memmove( Storage, Storage + Index, SizeInBytes() );
        }
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE TString TrimEnd() noexcept
    {
        TString Result = *this;
        Result.TrimEndInline();
        return Result;
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE void TrimEndInline() noexcept
    {
        SizeType Index = StrLength - 1;
        for ( ; Index >= 0; Index-- )
        {
            if ( Storage[Index] == ' ' )
            {
                StrLength--;
            }
            else
            {
                break;
            }
        }

        Storage[StrLength] = StringTraits::Terminator;
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE TString Reverse() noexcept
    {
        TString Result = *this;
        Result.ReverseInline();
        return Result;
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE void ReverseInline() noexcept
    {
        SizeType ReverseIndex = StrLength - 1;
        for ( SizeType Index = 0; Index < ReverseIndex; Index++ )
        {
            ::Swap<CharType>( Storage[Index], Storage[ReverseIndex] );
            ReverseIndex--;
        }
    }

    /* Compares two strings and checks if they are equal */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type Compare( const StringType& InString ) const noexcept
    {
        Compare( InString.CStr(), InString.Length() );
    }

    /* Compares two strings and checks if they are equal */
    FORCEINLINE bool Compare( const CharType* InString ) const noexcept
    {
        Compare( InString, StrLen( InString ) );
    }

    /* Compares two strings and checks if they are equal */
    FORCEINLINE bool Compare( const CharType* InString, SizeType InLength )
    {
        if ( StrLength != InLength )
        {
            return false;
        }

        for ( SizeType Index = 0; Index < StrLength; Index++ )
        {
            if ( Storage[Index] != InString[Index] )
            {
                return false;
            }
        }

        return true;
    }

    /* Compares two strings and checks if they are equal, without taking casing into account */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type CompareNoCase( const StringType& InString ) const noexcept
    {
        CompareNoCase( InString.CStr(), InString.Length() );
    }

    /* Compares two strings and checks if they are equal, without taking casing into account */
    FORCEINLINE bool CompareNoCase( const CharType* InString ) const noexcept
    {
        CompareNoCase( InString, StringTraits::Length( InString ) );
    }

    /* Compares two strings and checks if they are equal, without taking casing into account */
    FORCEINLINE bool CompareNoCase( const CharType* InString, SizeType InLength )
    {
        if ( StrLength != InLength )
        {
            return false;
        }

        for ( SizeType Index = 0; Index < StrLength; Index++ )
        {
            if ( StringTraits::ToLower( Storage[Index] ) != StringTraits::ToLower( InString[Index] ) )
            {
                return false;
            }
        }

        return true;
    }

    /* Returns the position of the first occurance of the start of the searchstring */
    FORCEINLINE SizeType Find( const CharType* InString, SizeType InOffset = 0 ) const noexcept
    {
        return Find( InString, StringTraits::Length( InString ), InOffset );
    }

    /* Returns the position of the first occurance of the start of the searchstring */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type Find( const StringType& InString, SizeType InOffset = 0 ) const noexcept
    {
        return Find( InString, InString.Length(), InOffset );
    }

    /* Returns the position of the first occurance of the start of the searchstring */
    FORCEINLINE SizeType Find( const CharType* InString, SizeType InLength, SizeType InOffset = 0 ) const noexcept
    {
        Assert( InOffset < StrLength );

        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (StrLength == 0) )
        {
            return 0;
        }

        const CharType* Start = CStr() + InOffset;
        const CharType* Result = StringTraits::Find( Start, InString );
        if ( !Result )
        {
            return InvalidPosition;
        }

        return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
    }

    /* Returns the position of the first occurance of char */
    FORCEINLINE SizeType Find( CharType InChar, SizeType InOffset = 0 ) const noexcept
    {
        Assert( InOffset < StrLength );

        if ( StringTraits::IsTerminator( InChar ) || (StrLength == 0) )
        {
            return 0;
        }

        const CharType* Start = CStr() + InOffset;
        const CharType* Result = StringTraits::FindChar( Start, InChar );
        if ( !Result )
        {
            return InvalidPosition;
        }

        return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
    }

    /* Returns the position of the first occurance of the start of the searchstring */
    FORCEINLINE SizeType ReverseFind( const CharType* InString, SizeType InOffset = 0 ) const noexcept
    {
        return ReverseFind( InString, StringTraits::Length( InString ), InOffset );
    }

    /* Returns the position of the first occurance of the start of the searchstring */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type ReverseFind( const StringType& InString, SizeType InOffset = 0 ) const noexcept
    {
        return ReverseFind( InString, InString.Length(), InOffset );
    }

    /* Returns the position of the first occurance of the start of the searchstring by searching in reverse. Offset is the end, instead of the start as with normal Find*/
    FORCEINLINE SizeType ReverseFind( const CharType* InString, SizeType InLength, SizeType InOffset = 0 ) const noexcept
    {
        Assert( InOffset < StrLength );

        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (StrLength == 0) )
        {
            return StrLength;
        }

        // Calculate the offset to the end
        const CharType* Start = CStr();
        SizeType Length = (InOffset == 0) ? StringTraits::Length( Start ) : InOffset;

        const CharType* End = Start + Length;
        while ( End != Start )
        {
            End--;

            // Loop each character in substring
            for ( const CharType* EndIt = End, const CharType* SubstringIt = InString; ; )
            {
                // If not found we end loop and start over
                if ( *(EndIt++) != *(SubstringIt++) )
                {
                    break;
                }
                else if ( StringTraits::IsTerminator( *SubstringIt ) )
                {
                    // If terminator is reached we have found the full substring in out string
                    return static_cast<SizeType>(static_cast<intptr_t>(End - Start));
                }
            }
        }

        return InvalidPosition;
    }

    /* Returns the position of the first occurance of char by searching from the end */
    FORCEINLINE SizeType ReverseFind( CharType InChar, SizeType InOffset = 0 ) const noexcept
    {
        Assert( InOffset < StrLength );

        if ( StringTraits::IsTerminator( InChar ) || (StrLength == 0) )
        {
            return StrLength;
        }

        const CharType* Result = nullptr;
        const CharType* Start = CStr();
        if ( InOffset == 0 )
        {
            Result = StringTraits::ReverseFindChar( Start, InChar );
        }
        else
        {
            CharType TempChar = Storage[InOffset + 1];
            Storage[InOffset + 1] = StringTraits::Terminator;

            Result = StringTraits::ReverseFindChar( Start, InChar );

            Storage[InOffset + 1] = TempChar;
        }

        if ( !Result )
        {
            return InvalidPosition;
        }

        return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
    }

    /* Returns the position of the the first found character in the searchstring */
    FORCEINLINE SizeType FindOneOf( const CharType* InString, SizeType InOffset = 0 ) const noexcept
    {
        return FindOneOf( InString, StringTraits::Length( InString ), InOffset );
    }

    /* Returns the position of the the first found character in the searchstring */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type FindOneOf( const StringType& InString, SizeType InOffset = 0 ) const noexcept
    {
        return FindOneOf( InString.CStr(), InString.Length(), InOffset );
    }

    /* Returns the position of the the first found character in the searchstring */
    FORCEINLINE SizeType FindOneOf( const CharType* InString, SizeType InLength, SizeType InOffset = 0 ) const noexcept
    {
        Assert( InOffset < StrLength );

        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (StrLength == 0) )
        {
            return 0;
        }

        const CharType* Start = CStr() + InOffset;
        const CharType* Result = StringTraits::FindOneOf( Start, InString );
        if ( !Result )
        {
            return InvalidPosition;
        }

        return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
    }

    /* Returns the position of the the first character not a part of the searchstring */
    FORCEINLINE SizeType FindOneNotOf( const CharType* InString, SizeType InOffset = 0 ) const noexcept
    {
        return FindOneNotOf( InString, StringTraits::Length( InString ), InOffset );
    }

    /* Returns the position of the the first character not a part of the searchstring */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type FindOneNotOf( const StringType& InString, SizeType InOffset = 0 ) const noexcept
    {
        return FindOneNotOf( InString.CStr(), InString.Length(), InOffset );
    }

    /* Returns the position of the the first character not a part of the searchstring */
    FORCEINLINE SizeType FindOneNotOf( const CharType* InString, SizeType InLength, SizeType InOffset = 0 ) const noexcept
    {
        Assert( InOffset < StrLength );

        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (StrLength == 0) )
        {
            return 0;
        }

        SizeType Pos = static_cast<SizeType>(StringTraits::Span( CStr() + InOffset, InString ));
        SizeType Ret = Position + InOffset;
        if ( Ret >= StrLength )
        {
            return InvalidPosition;
        }

        return Ret;
    }

    /* Returns true if the searchstring exists withing the string */
    FORCEINLINE bool Contains( const CharType* InString, SizeType InOffset = 0 ) const noexcept
    {
        return (Find( InString, InOffset ) != InvalidPosition);
    }

    /* Returns true if the searchstring exists withing the string */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type Contains( const StringType& InString, SizeType InOffset = 0 ) const noexcept
    {
        return (Find( InString, InOffset ) != InvalidPosition);
    }

    /* Returns true if the searchstring exists withing the string */
    FORCEINLINE bool Contains( const CharType* InString, SizeType InLength, SizeType InOffset = 0 ) const noexcept
    {
        return (Find( InString, InLength, InOffset ) != InvalidPosition);
    }

    /* Returns true if the searchstring exists withing the string */
    FORCEINLINE bool Contains( CharType InChar, SizeType InOffset = 0 ) const noexcept
    {
        return (Find( InChar, InOffset ) != InvalidPosition);
    }

    /* Returns the position of the the first found character in the searchstring */
    FORCEINLINE bool ContainsOneOf( const CharType* InString, SizeType InOffset = 0 ) const noexcept
    {
        return (FindOneOf( InString, InOffset ) != InvalidPosition);
    }

    /* Returns true if the searchstring exists withing the string */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type ContainsOneOf( const StringType& InString, SizeType InLength, SizeType InOffset = 0 ) const noexcept
    {
        return (FindOneOf<StringType>( InString, InLength, InOffset ) != InvalidPosition);
    }

    /* Returns true if the searchstring exists withing the string */
    FORCEINLINE bool ContainsOneOf( const CharType* InString, SizeType InLength, SizeType InOffset = 0 ) const noexcept
    {
        return (FindOneOf( InString, InLength, InOffset ) != InvalidPosition);
    }

    /* Erases count characters from position and forward */
    FORCEINLINE void Erase( SizeType Position, SizeType Count ) noexcept
    {
        Assert( (Position < StrLength) && (Position + Count < StrLength) );

        CharType* Dst = Data() + Position;
        CharType* Src = Dst + Count;

        SizeType Num = StrLength - (Position + Count);
        Memory::Memmove( Dst, Src, Num * sizeof( CharType ) );
    }

    /* Insert a string at position */
    FORCEINLINE void Insert( const CharType* InString, SizeType Position ) noexcept
    {
        Insert( InString, StringTraits::Length( InString ), Position );
    }

    /* Insert a string at position */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value>::Type Insert( const StringType& InString, SizeType Position ) noexcept
    {
        Insert( InString.CStr(), InString.Length(), Position );
    }

    /* Insert a string at position */
    FORCEINLINE void Insert( const CharType* InString, SizeType InLength, SizeType Position ) noexcept
    {
        Assert( (Position < StrLength) && (StrLength + InLength < NumChars) );

        const uint64 Size = InLength * sizeof( CharType );

        // Make room for string
        CharType* Src = Data() + Position;
        CharType* Dst = Src + InLength;
        Memory::Memmove( Dst, Src, Size );

        // Copy String
        Memory::Memcpy( Src, InString, Size );

        StrLength += InLength;
        Storage[StrLength] = StringTraits::Terminator;
    }

    /* Insert a character at position */
    FORCEINLINE void Insert( CharType InChar, SizeType Position ) noexcept
    {
        Insert( &InChar, 1, Position );
    }

    /* Replace a part of the string */
    FORCEINLINE void Replace( const CharType* InString, SizeType Position ) noexcept
    {
        Replace( InString, StringTraits::Length( InString ), Position );
    }

    /* Replace a part of the string */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value>::Type Replace( const StringType& InString, SizeType Position ) noexcept
    {
        Replace( InString.CStr(), InString.Length(), Position );
    }

    /* Replace a part of the string */
    FORCEINLINE void Replace( const CharType* InString, SizeType InLength, SizeType Position ) noexcept
    {
        Assert( (Position < StrLength) && (Position + InLength < StrLength) );
        Memory::Memcpy( Data() + Position, InString, InLength * sizeof( CharType ) );
    }

    /* Replace a character */
    FORCEINLINE void Replace( CharType InChar, SizeType Position ) noexcept
    {
        Replace( &InChar, 1, Position );
    }

    /* Replace push a character to the end of the string */
    FORCEINLINE void Push( CharType InChar ) noexcept
    {
        Append( InChar );
    }

    /* Pop a character from the end of the string */
    FORCEINLINE void Pop() noexcept
    {
        Storage[--StrLength] = StringTraits::Terminator;
    }

    /* Swaps two fixed strings with eachother */
    FORCEINLINE void Swap( TString& Other )
    {
        TString Temp( Move( *this ) );
        MoveFrom( Move( Other ) );
        Other.MoveFrom( Move( Temp ) );
    }

    /* Return a null terminated string */
    FORCEINLINE CharType* Data() noexcept
    {
        return Storage;
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* Data() const noexcept
    {
        return Storage;
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* CStr() const noexcept
    {
        return Storage;
    }

    /* Return the length of the string */
    FORCEINLINE SizeType Size() const noexcept
    {
        return StrLength;
    }

    /* Returns a sub-string of this string */
    FORCEINLINE TString SubString( SizeType Offset, SizeType Count ) const noexcept
    {
        Assert( (Offset < StrLength) && (Offset + Count < StrLength) );
        return TString( Storage + Offset, Count );
    }

    /* Returns a sub-stringview of this string */
    FORCEINLINE TStringView<CharType> SubStringView( SizeType Offset, SizeType Count ) const noexcept
    {
        Assert( (Offset < StrLength) && (Offset + Count < StrLength) );
        return TStringView<CharType>( Storage + Offset, Count );
    }

    /* Return the length of the string */
    FORCEINLINE SizeType Length() const noexcept
    {
        return StrLength;
    }

    constexpr SizeType Capacity() const noexcept
    {
        return NumChars;
    }

    constexpr SizeType CapacityInBytes() const noexcept
    {
        return NumChars * sizeof( CharType );
    }

    /* Retrive the size of the string in bytes */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return StrLength * sizeof( CharType );
    }

    /* Checks if the string is empty */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (StrLength == 0);
    }

    /* Compares two containers by comparing each element, returns true if all is equal */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type operator==( const StringType& Other ) const noexcept
    {
        return Compare<StringType>( Other );
    }

    /* Compares two containers by comparing each element, returns false if all elements are equal */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type operator!=( const StringType& Other ) const noexcept
    {
        return !(*this == Other);
    }

    /* Appends a string to this string */
    FORCEINLINE TString& operator+=( CharType InChar ) const noexcept
    {
        Append( InChar );
        return *this;
    }

    /* Appends a string to this string */
    FORCEINLINE TString& operator+=( const CharType* InString ) const noexcept
    {
        Append( InString );
        return *this;
    }

    /* Appends a string to this string */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, typename TAddReference<TString>::LValue>::Type operator+=( const StringType& InString ) const noexcept
    {
        Append<StringType>( InString );
        return *this;
    }

    /* Retrive an element at a certain position */
    FORCEINLINE CharType& operator[]( SizeType Index ) noexcept
    {
        return At( Index );
    }

    /* Retrive an element at a certain position */
    FORCEINLINE const CharType& operator[]( SizeType Index ) const noexcept
    {
        return At( Index );
    }

    /* Assign from another view */
    FORCEINLINE TString& operator=( const CharType* InString ) noexcept
    {
        TString( InString ).Swap( *this );
        return *this;
    }

    /* Assign from another view */
    FORCEINLINE TString& operator=( const TString& Other ) noexcept
    {
        TString( Other ).Swap( *this );
        return *this;
    }

    /* Assign from another view */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, typename TAddReference<TString>::LValue>::Type operator=( const StringType& Other ) noexcept
    {
        TString<StringType>( Other ).Swap( *this );
        return *this;
    }

    /* Move-assign from another view */
    FORCEINLINE TString& operator=( TString&& Other ) noexcept
    {
        TString( Move( Other ) ).Swap( *this );
        return *this;
    }

public:

    /* Returns an iterator to the beginning of the container */
    FORCEINLINE IteratorType StartIterator() noexcept
    {
        return IteratorType( *this, 0 );
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE IteratorType EndIterator() noexcept
    {
        return IteratorType( *this, Size() );
    }

    /* Returns an iterator to the beginning of the container */
    FORCEINLINE ConstIteratorType StartIterator() const noexcept
    {
        return ConstIteratorType( *this, 0 );
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE ConstIteratorType EndIterator() const noexcept
    {
        return ConstIteratorType( *this, Size() );
    }

    /* Returns an reverse iterator to the end of the container */
    FORCEINLINE ReverseIteratorType ReverseStartIterator() noexcept
    {
        return ReverseIteratorType( *this, Size() );
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ReverseIteratorType ReverseEndIterator() noexcept
    {
        return ReverseIteratorType( *this, 0 );
    }

    /* Returns an reverse iterator to the end of the container */
    FORCEINLINE ReverseConstIteratorType ReverseStartIterator() const noexcept
    {
        return ReverseConstIteratorType( *this, Size() );
    }

    /* Returns an reverse iterator to the beginning of the container */
    FORCEINLINE ReverseConstIteratorType ReverseEndIterator() const noexcept
    {
        return ReverseConstIteratorType( *this, 0 );
    }

public:

    /* STL iterator functions - Enables Range-based for-loops */
    FORCEINLINE IteratorType begin() noexcept
    {
        return StartIterator();
    }

    FORCEINLINE IteratorType end() noexcept
    {
        return EndIterator();
    }

    FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    FORCEINLINE ConstIteratorType end() const noexcept
    {
        return EndIterator();
    }

private:

    /* Initializing this string by copying from InString */
    FORCEINLINE void CopyFrom( const CharType* InString, SizeType InLength ) noexcept
    {
        Assert( InLength < Capacity() );

        Memory::Memcpy( Storage, InString, InLength * sizeof( CharType ) );
        StrLength = InLength;
        Storage[StrLength] = StringTraits::Terminator;
    }

    /* Initializing this string by moving from InString */
    FORCEINLINE void MoveFrom( TString&& Other ) noexcept
    {
        StrLength = Other.StrLength;
        Other.StrLength = 0;

        Memory::Memexchange( Storage, Other.Storage, SizeInBytes() );
        Storage[StrLength] = StringTraits::Terminator;
    }

    /* Storage for characters */
    TArray<CharType, TStringAllocator<CharType>> Storage;
};

/* Predefined types */
using CString = TString<char>;
using WString = TString<wchar_t>;

/* Operators */
template<typename CharType>
inline TString<CharType> operator+( const TString<CharType>& LHS, const TString<CharType>& RHS )
{
    TString<CharType> Result = LHS;
    Result.Append( RHS );
    return Result;
}

/* Add TString to be a string-type */
template<typename CharType>
struct TIsTStringType<TString<CharType>>
{
    enum
    {
        Value = true
    };
};

#endif
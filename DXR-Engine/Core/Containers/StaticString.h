#pragma once
#include "StringView.h"

#include "Core/Templates/EnableIf.h"
#include "Core/Templates/Identity.h"
#include "Core/Templates/AddReference.h"

/* Characters class with a fixed allocated number of characters */
template<typename CharType, uint32 CharCount>
class TStaticString
{
public:

    static_assert(TIsSame<CharType, char>::Value || TIsSame<CharType, wchar_t>::Value, "Only char and wchar_t is supported for strings");
    static_assert(CharCount > 0, "The number of chars has to be more than zero");

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
    typedef TArrayIterator<TStaticString, CharType>                    IteratorType;
    typedef TArrayIterator<const TStaticString, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<TStaticString, CharType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TStaticString, const CharType> ReverseConstIteratorType;

    /* Empty constructor */
    FORCEINLINE TStaticString() noexcept
        : Characters()
        , Len( 0 )
    {
        Characters[Len] = StringTraits::Terminator;
    }

    /* Create a fixed string from a raw array. If the string is longer than
       the allocators the string will be shortened to fit. */
    FORCEINLINE TStaticString( const CharType* InString ) noexcept
        : Characters()
        , Len( 0 )
    {
        if ( InString )
        {
            CopyFrom( InString, StringTraits::Length( InString ) );
        }
    }

    /* Create a fixed string from a raw array and the length. If the string is
       longer than the allocators the string will be shortened to fit. */
    FORCEINLINE explicit TStaticString( const CharType* InString, uint32 InLength ) noexcept
        : Characters()
        , Len( 0 )
    {
        if ( InString )
        {
            CopyFrom( InString, InLength );
        }
    }

    /* Create a new string from another string type with similar interface. If the
       string is longer than the allocators the string will be shortened to fit. */
    template<typename StringType>
    FORCEINLINE explicit TStaticString( const StringType& InString ) noexcept
        : Characters()
        , Len( 0 )
    {
        CopyFrom( InString.CStr(), InString.Length() );
    }

    /* Copy Constructor */
    FORCEINLINE TStaticString( const TStaticString& Other ) noexcept
        : Characters()
        , Len( 0 )
    {
        CopyFrom( Other.CStr(), Other.Length() );
    }

    /* Move Constructor */
    FORCEINLINE TStaticString( TStaticString&& Other ) noexcept
        : Characters()
        , Len( 0 )
    {
        MoveFrom( Forward<TStaticString>( Other ) );
    }

    /* Clears the string */
    FORCEINLINE void Clear() noexcept
    {
        Len = 0;
        Characters[Len] = StringTraits::Terminator;
    }

    /* Appends a string to this string */
    FORCEINLINE void Append( CharType Char ) noexcept
    {
        Assert( Len + 1 < Capacity() );

        Characters[Len] = Char;
        Characters[++Len] = StringTraits::Terminator;
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
        Assert( Len + InLength < Capacity() );

        StringTraits::Copy( Characters + Len, InString, InLength );

        Len = Len + InLength;
        Characters[Len] = StringTraits::Terminator;
    }

    /* Resize the string */
    FORCEINLINE void Resize( SizeType NewSize ) noexcept
    {
        Resize( NewSize, CharType() );
    }

    /* Resize the string and fill with Char */
    FORCEINLINE void Resize( SizeType NewSize, CharType FillElement ) noexcept
    {
        Assert( NewSize < CharCount );

        const CharType* It = Characters + Len;
        const CharType* End = Characters + NewSize;
        while ( It != End )
        {
            *(It++) = FillElement;
        }

        Len = NewSize;
        Characters[Len] = StringTraits::Terminator;
    }

    /* Copy this string into buffer */
    FORCEINLINE void Copy( CharType* Buffer, SizeType BufferSize, SizeType Position = 0 ) const noexcept
    {
        Assert( Buffer != nullptr );
        Assert( (Position < Len) || (Position == 0) );

        SizeType CopySize = NMath::Min( BufferSize, Len - Position );
        StringTraits::Copy( Buffer, Characters + Position, CopySize );
    }

    /* Format string (similar to snprintf) */
    void Format( const CharType* Format, ... ) noexcept
    {
        va_list ArgList;
        va_start( ArgList, Format );
        FormatV( Format, ArgList );
        va_end( ArgList );
    }

    /* Format string with a va_list (similar to snprintf) */
    FORCEINLINE void FormatV( const CharType* Format, va_list ArgList ) noexcept
    {
        SizeType WrittenChars = StringTraits::PrintVA( Characters, CharCount, Format, ArgList );
        if ( WrittenChars < CharCount )
        {
            Len = WrittenChars;
        }
        else
        {
            Len = CharCount - 1;
        }

        Characters[Len] = StringTraits::Terminator;
    }

    /* Same as Format, but appends the result to the current string */
    void AppendFormat( const CharType* Format, ... ) noexcept
    {
        va_list ArgList;
        va_start( ArgList, Format );
        AppendFormatV( Format, ArgList );
        va_end( ArgList );
    }

    /* Same as FormatV, but appends the result to the current string */
    FORCEINLINE void AppendFormatV( const CharType* Format, va_list ArgList ) noexcept
    {
        const SizeType WrittenChars = StringTraits::PrintVA( Characters + Len, CharCount, Format, ArgList );
        const SizeType NewLength = Len + WrittenChars;
        if ( NewLength < CharCount )
        {
            Len = NewLength;
        }
        else
        {
            Len = CharCount - 1;
        }

        Characters[Len] = StringTraits::Terminator;
    }

    /* Returns this string in lowercase */
    FORCEINLINE void ToLowerInline() noexcept
    {
        CharType* It = Characters;
        CharType* End = Characters + Len;
        while ( It != End )
        {
            *It = StringTraits::ToLower( *It );
            It++;
        }
    }

    /* Returns this string in lowercase */
    FORCEINLINE TStaticString ToLower() const noexcept
    {
        TStaticString Result = *this;
        Result.ToLowerInline();
        return Result;
    }

    /* Converts this string in uppercase */
    FORCEINLINE void ToUpperInline() noexcept
    {
        CharType* It = Characters;
        CharType* End = Characters + Len;
        while ( It != End )
        {
            *It = StringTraits::ToUpper( *It );
            It++;
        }
    }

    /* Returns this string in uppercase */
    FORCEINLINE TStaticString ToUpper() const noexcept
    {
        TStaticString Result = *this;
        Result.ToUpperInline();
        return Result;
    }

    /* Removes whitespace from the beginning and end of the string */
    FORCEINLINE TStaticString Trim() noexcept
    {
        TStaticString Result = *this;
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
    FORCEINLINE TStaticString TrimStart() noexcept
    {
        TStaticString Result = *this;
        Result.TrimStartInline();
        return Result;
    }

    /* Removes whitespace from the beginning of the string */
    FORCEINLINE void TrimStartInline() noexcept
    {
        SizeType Index = 0;
        for ( ; Index < Len; Index++ )
        {
            if ( !StringTraits::IsWhiteSpace( Characters[Index] ) )
            {
                break;
            }
        }

        if ( Index > 0 )
        {
            Len -= Index;
            Memory::Memmove( Characters, Characters + Index, SizeInBytes() );
        }
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE TStaticString TrimEnd() noexcept
    {
        TStaticString Result = *this;
        Result.TrimEndInline();
        return Result;
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE void TrimEndInline() noexcept
    {
        for ( SizeType Index = Len - 1; Index >= 0; Index-- )
        {
            if ( StringTraits::IsWhiteSpace( Characters[Index] ) )
            {
                Len--;
            }
            else
            {
                break;
            }
        }

        Characters[Len] = StringTraits::Terminator;
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE TStaticString Reverse() noexcept
    {
        TStaticString Result = *this;
        Result.ReverseInline();
        return Result;
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE void ReverseInline() noexcept
    {
        SizeType ReverseIndex = Len - 1;
        for ( SizeType Index = 0; Index < ReverseIndex; Index++ )
        {
            ::Swap<CharType>( Characters[Index], Characters[ReverseIndex] );
            ReverseIndex--;
        }
    }

    /* Compares two strings and checks if they are equal */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, int32>::Type Compare( const StringType& InString ) const noexcept
    {
        return Compare( InString.CStr(), InString.Length() );
    }

    /* Compares two strings and checks if they are equal */
    FORCEINLINE int32 Compare( const CharType* InString ) const noexcept
    {
        return Compare( InString, StringTraits::Length( InString ) );
    }

    /* Compares two strings and checks if they are equal */
    FORCEINLINE int32 Compare( const CharType* InString, SizeType InLength )
    {
        return StringTraits::Compare( Characters, InString );
    }

    /* Compares two strings and checks if they are equal, without taking casing into account */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, int32>::Type CompareNoCase( const StringType& InString ) const noexcept
    {
        return CompareNoCase( InString.CStr(), InString.Length() );
    }

    /* Compares two strings and checks if they are equal, without taking casing into account */
    FORCEINLINE int32 CompareNoCase( const CharType* InString ) const noexcept
    {
        return CompareNoCase( InString, StringTraits::Length( InString ) );
    }

    /* Compares two strings and checks if they are equal, without taking casing into account */
    FORCEINLINE int32 CompareNoCase( const CharType* InString, SizeType InLength )
    {
        if ( Len != InLength )
        {
            return -1;
        }

        for ( SizeType Index = 0; Index < Len; Index++ )
        {
            const CharType TempChar0 = StringTraits::ToLower( Characters[Index] );
            const CharType TempChar1 = StringTraits::ToLower( InString[Index] );

            if ( TempChar0 != TempChar1 )
            {
                return TempChar0 - TempChar1;
            }
        }

        return 0;
    }

    /* Returns the position of the first occurance of the start of the searchstring */
    FORCEINLINE SizeType Find( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return Find( InString, StringTraits::Length( InString ), Position );
    }

    /* Returns the position of the first occurance of the start of the searchstring */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type Find( const StringType& InString, SizeType Position = 0 ) const noexcept
    {
        return Find( InString, InString.Length(), Position );
    }

    /* Returns the position of the first occurance of the start of the searchstring */
    FORCEINLINE SizeType Find( const CharType* InString, SizeType InLength, SizeType Position = 0 ) const noexcept
    {
        Assert( (Position < Len) || (Position == 0) );

        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (Len == 0) )
        {
            return 0;
        }

        const CharType* Start = CStr() + Position;
        const CharType* Result = StringTraits::Find( Start, InString );
        if ( !Result )
        {
            return InvalidPosition;
        }
        else
        {
            return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
        }
    }

    /* Returns the position of the first occurance of char */
    FORCEINLINE SizeType Find( CharType Char, SizeType Position = 0 ) const noexcept
    {
        Assert( (Position < Len) || (Position == 0) );

        if ( StringTraits::IsTerminator( Char ) || (Len == 0) )
        {
            return 0;
        }

        const CharType* Start = CStr() + Position;
        const CharType* Result = StringTraits::FindChar( Start, Char );
        if ( !Result )
        {
            return InvalidPosition;
        }
        else
        {
            return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
        }
    }

    /* Returns the position of the first occurance of the start of the searchstring */
    FORCEINLINE SizeType ReverseFind( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return ReverseFind( InString, StringTraits::Length( InString ), Position );
    }

    /* Returns the position of the first occurance of the start of the searchstring */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type ReverseFind( const StringType& InString, SizeType Position = 0 ) const noexcept
    {
        return ReverseFind( InString, InString.Length(), Position );
    }

    /* Returns the position of the first occurance of the start of the searchstring by searching in reverse. Offset is the end, instead of the start as with normal Find*/
    FORCEINLINE SizeType ReverseFind( const CharType* InString, SizeType InLength, SizeType Position = 0 ) const noexcept
    {
        Assert( (Position < Len) || (Position == 0) );

        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (Len == 0) )
        {
            return Len;
        }

        // Calculate the offset to the end
        SizeType Length = (Position == 0) ? Len : NMath::Min( Position, Len );
        
        const CharType* Start = CStr();
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
    FORCEINLINE SizeType ReverseFind( CharType Char, SizeType Position = 0 ) const noexcept
    {
        Assert( (Position < Len) || (Position == 0) );
        
        if ( StringTraits::IsTerminator( Char ) || (Len == 0) )
        {
            return Len;
        }

        const CharType* Result = nullptr;
        const CharType* Start = CStr();
        if ( InOffset == 0 )
        {
            Result = StringTraits::ReverseFindChar( Start, Char );
        }
        else
        {
            CharType TempChar = Characters[Position + 1];
            Characters[Position + 1] = StringTraits::Terminator;

            Result = StringTraits::ReverseFindChar( Start, Char );

            Characters[Position + 1] = TempChar;
        }

        if ( !Result )
        {
            return InvalidPosition;
        }
        else
        {
            return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
        }
    }

    /* Returns the position of the the first found character in the searchstring */
    FORCEINLINE SizeType FindOneOf( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return FindOneOf( InString, StringTraits::Length( InString ), Position );
    }

    /* Returns the position of the the first found character in the searchstring */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type FindOneOf( const StringType& InString, SizeType Position = 0 ) const noexcept
    {
        return FindOneOf( InString.CStr(), InString.Length(), Position );
    }

    /* Returns the position of the the first found character in the searchstring */
    FORCEINLINE SizeType FindOneOf( const CharType* InString, SizeType InLength, SizeType Position = 0 ) const noexcept
    {
        Assert( (Position < Len) || (Position == 0) );

        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (Len == 0) )
        {
            return 0;
        }

        const CharType* Start = CStr() + Position;
        const CharType* Result = StringTraits::FindOneOf( Start, InString );
        if ( !Result )
        {
            return InvalidPosition;
        }
        else
        {
            return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
        }
    }

    /* Returns the position of the the first character not a part of the searchstring */
    FORCEINLINE SizeType FindOneNotOf( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return FindOneNotOf( InString, StringTraits::Length( InString ), Position );
    }

    /* Returns the position of the the first character not a part of the searchstring */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type FindOneNotOf( const StringType& InString, SizeType Position = 0 ) const noexcept
    {
        return FindOneNotOf( InString.CStr(), InString.Length(), Position );
    }

    /* Returns the position of the the first character not a part of the searchstring */
    FORCEINLINE SizeType FindOneNotOf( const CharType* InString, SizeType InLength, SizeType Position = 0 ) const noexcept
    {
        Assert( (Position < Len) || (Position == 0) );

        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (Len == 0) )
        {
            return 0;
        }

        SizeType Pos = static_cast<SizeType>(StringTraits::Span( CStr() + Position, InString ));
        SizeType Ret = Position + InOffset;
        if ( Ret >= Len )
        {
            return InvalidPosition;
        }
        else
        {
            return Ret;
        }
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
    FORCEINLINE bool Contains( CharType Char, SizeType InOffset = 0 ) const noexcept
    {
        return (Find( Char, InOffset ) != InvalidPosition);
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

    /* Removes count characters from position and forward */
    FORCEINLINE void Remove( SizeType Position, SizeType Count ) noexcept
    {
        Assert( (Position < Len) && (Position + Count < Len) );

        CharType* Dst = Data() + Position;
        CharType* Src = Dst + Count;

        SizeType Num = Len - (Position + Count);
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
        Assert( (Position < Len) && (Len + InLength < CharCount) );

        const uint64 Size = InLength * sizeof( CharType );

        // Make room for string
        CharType* Src = Data() + Position;
        CharType* Dst = Src + InLength;
        Memory::Memmove( Dst, Src, Size );

        // Copy String
        Memory::Memcpy( Src, InString, Size );

        Len += InLength;
        Characters[Len] = StringTraits::Terminator;
    }

    /* Insert a character at position */
    FORCEINLINE void Insert( CharType Char, SizeType Position ) noexcept
    {
        Insert( &Char, 1, Position );
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
        Assert( (Position < Len) && (Position + InLength < Len) );
        StringTraits::Copy( Data() + Position, InString, InLength );
    }

    /* Replace a character */
    FORCEINLINE void Replace( CharType Char, SizeType Position ) noexcept
    {
        Replace( &Char, 1, Position );
    }

    /* Replace push a character to the end of the string */
    FORCEINLINE void Push( CharType Char ) noexcept
    {
        Append( Char );
    }

    /* Pop a character from the end of the string */
    FORCEINLINE void Pop() noexcept
    {
        Characters[--Len] = StringTraits::Terminator;
    }

    /* Swaps two fixed strings with eachother */
    FORCEINLINE void Swap( TStaticString& Other )
    {
        TStaticString TempString( Move( *this ) );
        MoveFrom( Move( Other ) );
        Other.MoveFrom( Move( TempString ) );
    }

    /* Returns a sub-string of this string */
    FORCEINLINE TStaticString SubString( SizeType Offset, SizeType Count ) const noexcept
    {
        Assert( (Offset < Len) && (Offset + Count < Len) );
        return TStaticString( Characters + Offset, Count );
    }

    /* Returns a sub-stringview of this string */
    FORCEINLINE TStringView<CharType> SubStringView( SizeType Offset, SizeType Count ) const noexcept
    {
        Assert( (Offset < Len) && (Offset + Count < Len) );
        return TStringView<CharType>( Characters + Offset, Count );
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

    /* Return a null terminated string */
    FORCEINLINE CharType* Data() noexcept
    {
        return Characters;
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* Data() const noexcept
    {
        return Characters;
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* CStr() const noexcept
    {
        return Characters;
    }

    /* Return the length of the string */
    FORCEINLINE SizeType Size() const noexcept
    {
        return Len;
    }

    /* Return the length of the string */
    FORCEINLINE SizeType Length() const noexcept
    {
        return Len;
    }

    constexpr SizeType Capacity() const noexcept
    {
        return CharCount;
    }

    constexpr SizeType CapacityInBytes() const noexcept
    {
        return CharCount * sizeof( CharType );
    }

    /* Retrive the size of the string in bytes */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Len * sizeof( CharType );
    }

    /* Checks if the string is empty */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (Len == 0);
    }

    /* Appends a string to this string */
    FORCEINLINE TStaticString& operator+=( CharType Char ) const noexcept
    {
        Append( Char );
        return *this;
    }

    /* Appends a string to this string */
    FORCEINLINE TStaticString& operator+=( const CharType* InString ) const noexcept
    {
        Append( InString );
        return *this;
    }

    /* Appends a string to this string */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, typename TAddReference<TStaticString>::LValue>::Type operator+=( const StringType& InString ) const noexcept
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
    FORCEINLINE TStaticString& operator=( const CharType* InString ) noexcept
    {
        TStaticString( InString ).Swap( *this );
        return *this;
    }

    /* Assign from another view */
    FORCEINLINE TStaticString& operator=( const TStaticString& Other ) noexcept
    {
        TStaticString( Other ).Swap( *this );
        return *this;
    }

    /* Assign from another view */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, typename TAddReference<TStaticString>::LValue>::Type operator=( const StringType& Other ) noexcept
    {
        TStaticString<StringType>( Other ).Swap( *this );
        return *this;
    }

    /* Move-assign from another view */
    FORCEINLINE TStaticString& operator=( TStaticString&& Other ) noexcept
    {
        TStaticString( Move( Other ) ).Swap( *this );
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

        StringTraits::Copy( Characters, InString, InLength );
        Len = InLength;
        Characters[Len] = StringTraits::Terminator;
    }

    /* Initializing this string by moving from InString */
    FORCEINLINE void MoveFrom( TStaticString&& Other ) noexcept
    {
        Len = Other.Len;
        Other.Len = 0;

        Memory::Memexchange( Characters, Other.Characters, SizeInBytes() );
        Characters[Len] = StringTraits::Terminator;
    }

    /* Characters for characters */
    CharType Characters[CharCount];
    SizeType Len;
};

/* Predefined types */
template<uint32 CharCount>
using CFixedString = TStaticString<char, CharCount>;

template<uint32 CharCount>
using WFixedString = TStaticString<wchar_t, CharCount>;

/* Operators */
template<typename CharType, int32 CharCount>
inline TStaticString<CharType, CharCount> operator+( const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS )
{
    TStaticString<CharType, CharCount> NewString = LHS;
    NewString.Append( RHS );
    return NewString;
}

template<typename CharType, int32 CharCount>
inline TStaticString<CharType, CharCount> operator+( const CharType* LHS, const TStaticString<CharType, CharCount>& RHS )
{
    TStaticString<CharType, CharCount> NewString = LHS;
    NewString.Append( RHS );
    return NewString;
}

template<typename CharType, int32 CharCount>
inline TStaticString<CharType, CharCount> operator+( const TStaticString<CharType, CharCount>& LHS, const CharType* RHS )
{
    TStaticString<CharType, CharCount> NewString = LHS;
    NewString.Append( RHS );
    return NewString;
}

template<typename CharType, int32 CharCount>
inline TStaticString<CharType, CharCount> operator+( CharType LHS, const TStaticString<CharType, CharCount>& RHS )
{
    TStaticString<CharType, CharCount> NewString = LHS;
    NewString.Append( RHS );
    return NewString;
}

template<typename CharType, int32 CharCount>
inline TStaticString<CharType, CharCount> operator+( const TStaticString<CharType, CharCount>& LHS, CharType RHS )
{
    TStaticString<CharType, CharCount> NewString = LHS;
    NewString.Append( RHS );
    return NewString;
}

/* Compares with a raw string */
template<typename CharType, int32 CharCount>
inline bool operator==( const TStaticString<CharType, CharCount>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) == 0);
}

/* Compares with a raw string */
template<typename CharType, int32 CharCount>
inline bool operator==( const CharType* LHS, const TStaticString<CharType, CharCount>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) == 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType, int32 CharCount>
inline bool operator==( const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS ) noexcept
{
    return (LHS.Compare<StringType>( RHS ) == 0);
}

/* Compares with a raw string */
template<typename CharType, int32 CharCount>
inline bool operator!=( const TStaticString<CharType, CharCount>& LHS, const CharType* RHS ) noexcept
{
    return !(LHS == RHS);
}

/* Compares with a raw string */
template<typename CharType, int32 CharCount>
inline bool operator!=( const CharType* LHS, const TStaticString<CharType, CharCount>& RHS ) noexcept
{
    return !(LHS == RHS);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType, int32 CharCount>
inline bool operator==( const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS ) noexcept
{
    return !(LHS == RHS);
}

/* Compares with a raw string */
template<typename CharType, int32 CharCount>
inline bool operator<( const TStaticString<CharType, CharCount>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) < 0);
}

/* Compares with a raw string */
template<typename CharType, int32 CharCount>
inline bool operator<( const CharType* LHS, const TStaticString<CharType, CharCount>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) < 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType, int32 CharCount>
inline bool operator<( const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS ) noexcept
{
    return (LHS.Compare<StringType>( RHS ) < 0);
}

template<typename CharType, int32 CharCount>
inline bool operator<=( const TStaticString<CharType, CharCount>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) <= 0);
}

/* Compares with a raw string */
template<typename CharType, int32 CharCount>
inline bool operator<=( const CharType* LHS, const TStaticString<CharType, CharCount>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) <= 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType, int32 CharCount>
inline bool operator<=( const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS ) noexcept
{
    return (LHS.Compare<StringType>( RHS ) <= 0);
}

/* Compares with a raw string */
template<typename CharType, int32 CharCount>
inline bool operator>( const TStaticString<CharType, CharCount>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) > 0);
}

/* Compares with a raw string */
template<typename CharType, int32 CharCount>
inline bool operator>( const CharType* LHS, const TStaticString<CharType, CharCount>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) > 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType, int32 CharCount>
inline bool operator>( const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS ) noexcept
{
    return (LHS.Compare<StringType>( RHS ) > 0);
}

/* Compares with a raw string */
template<typename CharType, int32 CharCount>
inline bool operator>=( const TStaticString<CharType, CharCount>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) >= 0);
}

/* Compares with a raw string */
template<typename CharType, int32 CharCount>
inline bool operator>=( const CharType* LHS, const TStaticString<CharType, CharCount>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) >= 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType, int32 CharCount>
inline bool operator>=( const TStaticString<CharType, CharCount>& LHS, const TStaticString<CharType, CharCount>& RHS ) noexcept
{
    return (LHS.Compare<StringType>( RHS ) >= 0);
}

/* Add TStaticString to be a string-type */
template<typename CharType, int32 CharCount>
struct TIsTStringType<TStaticString<CharType, CharCount>>
{
    enum
    {
        Value = true
    };
};

/* Convert between char and wide */
template<int32 CharCount>
inline WFixedString<CharCount> CharToWide( const CFixedString<CharCount>& CharString ) noexcept
{
    WFixedString<CharCount> NewString;
    NewString.Resize( CharString.Length() );

    mbstowcs( NewString.Data(), CharString.CStr(), CharString.Length() );

    return NewString;
}

template<int32 CharCount>
inline CFixedString<CharCount> CharToWide( const WFixedString<CharCount>& WideString ) noexcept
{
    CFixedString<CharCount> NewString;
    NewString.Resize( WideString.Length() );

    wcstombs( NewString.Data(), WideString.CStr(), WideString.Length() );

    return NewString;
}

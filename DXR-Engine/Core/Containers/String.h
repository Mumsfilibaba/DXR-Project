#pragma once
#include "Array.h"
#include "StringView.h"

#include "Core/Templates/Identity.h"
#include "Core/Templates/AddReference.h"

#define STRING_USE_INLINE_ALLOCATOR (1)
#define STRING_FORMAT_BUFFER_SIZE   (128)

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

/* Characters class with a fixed allocated number of characters */
template<typename CharType>
class TString
{
public:

    static_assert(TIsSame<CharType, char>::Value || TIsSame<CharType, wchar_t>::Value, "Only char and wchar_t is supported for strings");

    /* Types */
    using ElementType = CharType;
    using SizeType = int32;
    using StringTraits = TStringTraits<ElementType>;
    using ViewType = TStringView<ElementType>;
    using StorageType = TArray<CharType, TStringAllocator<CharType>>;

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
        : Characters()
    {
    }

    /* Create a fixed string from a raw array. If the string is longer than
       the allocators the string will be shortened to fit. */
    FORCEINLINE TString( const CharType* InString ) noexcept
        : Characters()
    {
        if ( InString )
        {
            CopyFrom( InString, StringTraits::Length( InString ) );
        }
    }

    /* Create a fixed string from a raw array and the length. If the string is
       longer than the allocators the string will be shortened to fit. */
    FORCEINLINE explicit TString( const CharType* InString, uint32 InLength ) noexcept
        : Characters()
    {
        if ( InString )
        {
            CopyFrom( InString, InLength );
        }
    }

    /* Create a new string from another string type with similar interface. If the
       string is longer than the allocators the string will be shortened to fit. */
    template<typename StringType, typename = typename TEnableIf<TIsTStringType<StringType>::Value>::Type>
    FORCEINLINE explicit TString( const StringType& InString ) noexcept
        : Characters()
    {
        CopyFrom( InString.CStr(), InString.Length() );
    }

    /* Copy Constructor */
    FORCEINLINE TString( const TString& Other ) noexcept
        : Characters()
    {
        CopyFrom( Other.CStr(), Other.Length() );
    }

    /* Move Constructor */
    FORCEINLINE TString( TString&& Other ) noexcept
        : Characters()
    {
        MoveFrom( Forward<TString>( Other ) );
    }

    /* Empties the storage */
    FORCEINLINE void Empty() noexcept
    {
        Characters.Empty();
    }

    /* Clears the string */
    FORCEINLINE void Clear() noexcept
    {
        if ( !Characters.IsEmpty() )
        {
            Characters.Resize( 1 );
            Characters[0] = StringTraits::Null;
        }
    }

    /* Appends a string to this string */
    FORCEINLINE void Append( CharType Char ) noexcept
    {
        if ( !Characters.IsEmpty() )
        {
            Characters.Pop();
        }

        Characters.Emplace( Char );
        Characters.Emplace( StringTraits::Null );
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

        // Remove null terminator if there are any
        if ( !Characters.IsEmpty() )
        {
            Characters.Pop();
        }

        // Append string to array
        Characters.Append( InString, InLength );
        Characters.Emplace( StringTraits::Null );
    }

    /* Resize the string */
    FORCEINLINE void Resize( SizeType NewSize ) noexcept
    {
        Resize( NewSize, CharType() );
    }

    /* Resize the string and fill with Char */
    FORCEINLINE void Resize( SizeType NewSize, CharType FillElement ) noexcept
    {
        // Remove null terminator if there are any
        if ( !Characters.IsEmpty() )
        {
            Characters.Pop();
        }

        Characters.Resize( NewSize, FillElement );
        Characters.Emplace( StringTraits::Null );
    }

    /* Reserve storage space */
    FORCEINLINE void Reserve( SizeType NewSize ) noexcept
    {
        Characters.Reserve( NewSize );
    }

    /* Copy this string into buffer */
    FORCEINLINE void Copy( CharType* Buffer, SizeType BufferSize, SizeType Position = 0 ) const noexcept
    {
        Assert( (Position < Length()) || (Position == 0) );

        if ( Buffer && (BufferSize > 0) )
        {
            SizeType CopySize = NMath::Min( BufferSize, Length() - Position );
            StringTraits::Copy( Buffer, Characters.Data() + Position, CopySize );
        }
    }

    /* Format string (similar to snprintf) */
    NOINLINE void Format( const CharType* Format, ... ) noexcept
    {
        va_list ArgList;
        va_start( ArgList, Format );
        FormatV( Format, ArgList );
        va_end( ArgList );
    }

    /* Format string with a va_list (similar to snprintf) */
    FORCEINLINE void FormatV( const CharType* Format, va_list ArgsList ) noexcept
    {
        CharType Buffer[STRING_FORMAT_BUFFER_SIZE];
        SizeType BufferSize = STRING_FORMAT_BUFFER_SIZE;

        CharType* DynamicBuffer = nullptr;
        CharType* WrittenString = Buffer;

        // Try print to buffer
        SizeType WrittenChars = StringTraits::PrintVA( WrittenString, BufferSize, Format, ArgsList );
        while ( (WrittenChars > BufferSize) || (WrittenChars == -1) )
        {
            // Double size of buffer
            BufferSize += BufferSize;

            // Allocate more memory
            DynamicBuffer = reinterpret_cast<CharType*>(Memory::Realloc( DynamicBuffer, BufferSize * sizeof( CharType ) ));
            WrittenString = DynamicBuffer;

            // Try print again
            WrittenChars = StringTraits::PrintVA( WrittenString, BufferSize, Format, ArgsList );
        }

        // Resize to make sure there is enough room in the string
        int32 WrittenLength = StringTraits::Length( WrittenString );
        Characters.Reset( WrittenString, WrittenLength );

        if ( DynamicBuffer )
        {
            Memory::Free( DynamicBuffer );
        }

        Characters.Emplace( StringTraits::Null );
    }

    /* Same as Format, but appends the result to the current string */
    NOINLINE void AppendFormat( const CharType* Format, ... ) noexcept
    {
        va_list ArgList;
        va_start( ArgList, Format );
        AppendFormatV( Format, ArgList );
        va_end( ArgList );
    }

    /* Same as FormatV, but appends the result to the current string */
    FORCEINLINE void AppendFormatV( const CharType* Format, va_list ArgsList ) noexcept
    {
        CharType Buffer[STRING_FORMAT_BUFFER_SIZE];
        SizeType BufferSize = STRING_FORMAT_BUFFER_SIZE;

        CharType* DynamicBuffer = nullptr;
        CharType* WrittenString = Buffer;

        // Try print to buffer
        SizeType WrittenChars = StringTraits::PrintVA( WrittenString, BufferSize, Format, ArgsList );
        while ( (WrittenChars > BufferSize) || (WrittenChars == -1) )
        {
            // Double size of buffer
            BufferSize += BufferSize;

            // Allocate more memory
            DynamicBuffer = reinterpret_cast<CharType*>(Memory::Realloc( DynamicBuffer, BufferSize * sizeof( CharType ) ));
            WrittenString = DynamicBuffer;

            // Try print again
            WrittenChars = StringTraits::PrintVA( WrittenString, BufferSize, Format, ArgsList );
        }

        // Remove nullterminator
        Characters.Pop();

        // Append new string
        Characters.Append( WrittenString, WrittenChars );

        if ( DynamicBuffer )
        {
            Memory::Free( DynamicBuffer );
        }

        Characters.Emplace( StringTraits::Null );
    }

    /* Returns this string in lowercase */
    FORCEINLINE void ToLowerInline() noexcept
    {
        CharType* It = Characters.Data();
        CharType* End = It + Characters.Size();
        while ( It != End )
        {
            *It = StringTraits::ToLower( *It );
            It++;
        }
    }

    /* Returns this string in lowercase */
    FORCEINLINE TString ToLower() const noexcept
    {
        TString NewString( *this );
        NewString.ToLowerInline();
        return NewString;
    }

    /* Converts this string in uppercase */
    FORCEINLINE void ToUpperInline() noexcept
    {
        CharType* It = Characters.Data();
        CharType* End = It + Characters.Size();
        while ( It != End )
        {
            *It = StringTraits::ToUpper( *It );
            It++;
        }
    }

    /* Returns this string in uppercase */
    FORCEINLINE TString ToUpper() const noexcept
    {
        TString NewString( *this );
        NewString.ToUpperInline();
        return NewString;
    }

    /* Removes whitespace from the beginning and end of the string */
    FORCEINLINE TString Trim() noexcept
    {
        TString NewString( *this );
        NewString.TrimInline();
        return NewString;
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
        TString NewString( *this );
        NewString.TrimStartInline();
        return NewString;
    }

    /* Removes whitespace from the beginning of the string */
    FORCEINLINE void TrimStartInline() noexcept
    {
        SizeType Count = 0;
        SizeType ThisLength = Length();

        CharType* Start = Characters.Data();
        CharType* End = Start + ThisLength;
        while ( Start != End )
        {
            if ( StringTraits::IsWhiteSpace( *(Start++) ) )
            {
                Count++;
            }
            else
            {
                break;
            }
        }

        if ( Count > 0 )
        {
            Characters.RemoveRangeAt( 0, Count );
        }
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE TString TrimEnd() noexcept
    {
        TString NewString( *this );
        NewString.TrimEndInline();
        return NewString;
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE void TrimEndInline() noexcept
    {
        SizeType LastIndex = LastElementIndex();
        SizeType Index = LastIndex;
        for ( ; Index >= 0; Index-- )
        {
            if ( !StringTraits::IsWhiteSpace( Characters[Index] ) )
            {
                break;
            }
        }

        SizeType Count = (LastIndex > Index) ? (LastIndex - Index) : 0;
        if ( Count > 0 )
        {
            // Add one to remove the nullterminator as well
            Characters.PopRange( Count + 1 );
            Characters.Emplace( StringTraits::Null );
        }
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE TString Reverse() noexcept
    {
        TString NewString( *this );
        NewString.ReverseInline();
        return NewString;
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE void ReverseInline() noexcept
    {
        CharType* Start = Characters.Data();
        CharType* End = Start + Length();
        while ( Start < End )
        {
            End--;
            ::Swap<CharType>( *Start, *End );
            Start++;
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
        return StringTraits::Compare( Characters.Data(), InString );
    }

    /* Compares two strings and checks if they are equal */
    FORCEINLINE int32 Compare( const CharType* InString, SizeType InLength ) const noexcept
    {
        return StringTraits::Compare( Characters.Data(), InString, InLength );
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
    FORCEINLINE int32 CompareNoCase( const CharType* InString, SizeType InLength ) const noexcept
    {
        Assert( InString != nullptr );

        SizeType Len = Length();
        if ( Len != InLength )
        {
            return -1;
        }

        const CharType* Start = Characters.Data();
        const CharType* End = Start + Len;
        while ( Start != End )
        {
            const CharType TempChar0 = StringTraits::ToLower( *(Start++) );
            const CharType TempChar1 = StringTraits::ToLower( *(InString++) );

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
        Assert( (Position < Length()) || (Position == 0) );

        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (Length() == 0) )
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
        Assert( (Position < Length()) || (Position == 0) );

        if ( StringTraits::IsTerminator( Char ) || (Length() == 0) )
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
        Assert( (Position < Length()) || (Position == 0) );
        Assert( InString != nullptr );

        SizeType Len = Length();
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
            const CharType* SubstringIt = InString;
            for ( const CharType* EndIt = End; ; )
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
        Assert( (Position < Length()) || (Position == 0) );

        SizeType Len = Length();
        if ( StringTraits::IsTerminator( Char ) || (Len == 0) )
        {
            return Len;
        }

        const CharType* Result = nullptr;
        const CharType* Start = CStr();
        if ( Position == 0 )
        {
            Result = StringTraits::ReverseFindChar( Start, Char );
        }
        else
        {
            // TODO: Get rid of const_cast
            CharType* TempCharacters = const_cast<CharType*>(Characters.Data());
            CharType TempChar = TempCharacters[Position + 1];
            TempCharacters[Position + 1] = StringTraits::Null;

            Result = StringTraits::ReverseFindChar( TempCharacters, Char );

            TempCharacters[Position + 1] = TempChar;
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
        Assert( (Position < Length()) || (Position == 0) );

        SizeType Len = Length();
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

    /* Returns the position of the last occurance of one of the characters in the searchstring */
    FORCEINLINE SizeType ReverseFindOneOf( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return ReverseFindOneOf( InString, StringTraits::Length( InString ), Position );
    }

    /* Returns the position of the last occurance of one of the characters in the searchstring */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type ReverseFindOneOf( const StringType& InString, SizeType Position = 0 ) const noexcept
    {
        return ReverseFindOneOf( InString, InString.Length(), Position );
    }

    /* Returns the position of the last occurance of one of the characters in the searchstring */
    FORCEINLINE SizeType ReverseFindOneOf( const CharType* InString, SizeType InLength, SizeType Position = 0 ) const noexcept
    {
        Assert( (Position < Length()) || (Position == 0) );

        SizeType ThisLength = Length();
        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (ThisLength == 0) )
        {
            return ThisLength;
        }

        // Calculate the offset to the end
        if ( Position != 0 )
        {
            ThisLength = NMath::Min( Position, ThisLength );
        }

        // Store the length of the substring outside the loop
        SizeType SubstringLength = StringTraits::Length( InString );

        const CharType* Start = CStr();
        const CharType* End = Start + ThisLength;
        while ( End != Start )
        {
            End--;

            // Loop each character in substring
            const CharType* SubstringStart = InString;
            const CharType* SubstringEnd = SubstringStart + SubstringLength;
            while ( SubstringStart != SubstringEnd )
            {
                // If character is found then return the position
                if ( *End == *(SubstringStart++) )
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(End - Start));
                }
            }
        }

        return InvalidPosition;
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
        Assert( (Position < Length()) || (Position == 0) );

        SizeType Len = Length();
        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (Len == 0) )
        {
            return 0;
        }

        SizeType Pos = static_cast<SizeType>(StringTraits::Span( CStr() + Position, InString ));
        SizeType Ret = Pos + Position;
        if ( Ret >= Len )
        {
            return InvalidPosition;
        }
        else
        {
            return Ret;
        }
    }

    /* Returns the position of the last occurance of one of the characters in the searchstring */
    FORCEINLINE SizeType ReverseFindOneNotOf( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return ReverseFindOneNotOf( InString, StringTraits::Length( InString ), Position );
    }

    /* Returns the position of the last occurance of one of the characters in the searchstring */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, SizeType>::Type ReverseFindOneNotOf( const StringType& InString, SizeType Position = 0 ) const noexcept
    {
        return ReverseFindOneNotOf( InString, InString.Length(), Position );
    }

    /* Returns the position of the last occurance of one of the characters in the searchstring */
    FORCEINLINE SizeType ReverseFindOneNotOf( const CharType* InString, SizeType InLength, SizeType Position = 0 ) const noexcept
    {
        Assert( (Position < Length()) || (Position == 0) );

        SizeType ThisLength = Length();
        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (ThisLength == 0) )
        {
            return ThisLength;
        }

        // Calculate the offset to the end
        if ( Position != 0 )
        {
            ThisLength = NMath::Min( Position, ThisLength );
        }

        // Store the length of the substring outside the loop
        SizeType SubstringLength = StringTraits::Length( InString );

        const CharType* Start = CStr();
        const CharType* End = Start + ThisLength;
        while ( End != Start )
        {
            End--;

            // Loop each character in substring
            const CharType* SubstringStart = InString;
            const CharType* SubstringEnd = SubstringStart + SubstringLength;
            while ( SubstringStart != SubstringEnd )
            {
                // If character is found then return the position
                if ( *End == *(SubstringStart++) )
                {
                    break;
                }
                else if ( StringTraits::IsTerminator( *SubstringStart ) )
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(End - Start));
                }
            }
        }

        return InvalidPosition;
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
        Assert( (Position < Length()) && (Position + Count < Length()) );
        Characters.RemoveRangeAt( Position, Count );
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
        Assert( (Position <= Length()) );

        SizeType ThisLength = Length();
        if ( Position == ThisLength || Characters.IsEmpty() )
        {
            Append( InString, InLength );
        }
        else
        {
            Characters.Insert( Position, InString, InLength );
        }
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
        Assert( (Position < Length()) && (Position + InLength < Length()) );
        StringTraits::Copy( Data() + Position, InString, InLength );
    }

    /* Replace a character */
    FORCEINLINE void Replace( CharType Char, SizeType Position = 0 ) noexcept
    {
        Assert( (Position < Length()) );

        CharType* PositionPtr = Characters.Data() + Position;
        *PositionPtr = Char;
    }

    /* Replace push a character to the end of the string */
    FORCEINLINE void Push( CharType Char ) noexcept
    {
        Append( Char );
    }

    /* Pop a character from the end of the string */
    FORCEINLINE void Pop() noexcept
    {
        SizeType Len = Length();
        Characters.Pop();
        Characters[Len] = StringTraits::Null;
    }

    /* Swaps two fixed strings with eachother */
    FORCEINLINE void Swap( TString& Other )
    {
        TString TempString( Move( *this ) );
        MoveFrom( Move( Other ) );
        Other.MoveFrom( Move( TempString ) );
    }

    /* Returns a sub-string of this string */
    FORCEINLINE TString SubString( SizeType Offset, SizeType Count ) const noexcept
    {
        Assert( (Offset < Length()) && (Offset + Count < Length()) );
        return TString( Characters.Data() + Offset, Count );
    }

    /* Returns a sub-stringview of this string */
    FORCEINLINE ViewType SubStringView( SizeType Offset, SizeType Count ) const noexcept
    {
        Assert( (Offset < Length()) && (Offset + Count < Length()) );
        return TStringView<CharType>( Characters.Data() + Offset, Count );
    }

    /* Retrieve the character at Index */
    FORCEINLINE CharType& At( SizeType Index ) noexcept
    {
        Assert( Index < Length() );
        return Data()[Index];
    }

    /* Retrieve the character at Index */
    FORCEINLINE const CharType& At( SizeType Index ) const noexcept
    {
        Assert( Index < Length() );
        return Data()[Index];
    }

    /* Return a null terminated string */
    FORCEINLINE CharType* Data() noexcept
    {
        return Characters.Data();
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* Data() const noexcept
    {
        return Characters.Data();
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* CStr() const noexcept
    {
        return (!Characters.IsEmpty()) ? Characters.Data() : StringTraits::Empty();
    }

    /* Return the length of the string */
    FORCEINLINE SizeType Size() const noexcept
    {
        SizeType Len = Characters.Size();
        return (Len > 0) ? (Len - 1) : 0; // Characters contains null-terminator
    }

    /* Return the last usable index of the string */
    FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        SizeType Len = Size();
        return (Len > 0) ? (Len - 1) : 0;
    }

    /* Return the length of the string */
    FORCEINLINE SizeType Length() const noexcept
    {
        return Size();
    }

    FORCEINLINE SizeType Capacity() const noexcept
    {
        return Characters.Capacity();
    }

    FORCEINLINE SizeType CapacityInBytes() const noexcept
    {
        return Characters.Capacity() * sizeof( CharType );
    }

    /* Retrieve the size of the string in bytes */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Length() * sizeof( CharType );
    }

    /* Checks if the string is empty */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (Length() == 0);
    }

    /* Appends a string to this string */
    FORCEINLINE TString& operator+=( CharType Char ) const noexcept
    {
        Append( Char );
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

    /* Retrieve an element at a certain position */
    FORCEINLINE CharType& operator[]( SizeType Index ) noexcept
    {
        return At( Index );
    }

    /* Retrieve an element at a certain position */
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
        Characters.Resize( InLength );
        StringTraits::Copy( Characters.Data(), InString, InLength );
        Characters.Emplace( StringTraits::Null );
    }

    /* Initializing this string by moving from InString */
    FORCEINLINE void MoveFrom( TString&& Other ) noexcept
    {
        Characters = Move( Other.Characters );
    }

    /* Using an TArray for storing the characters */
    StorageType Characters;
};

/* Predefined types */
using CString = TString<char>;
using WString = TString<wchar_t>;

/* Operators */
template<typename CharType>
inline TString<CharType> operator+( const TString<CharType>& LHS, const TString<CharType>& RHS ) noexcept
{
    const typename TString<CharType>::SizeType CombinedSize = LHS.Length() + RHS.Length();

    TString<CharType> NewString;
    NewString.Reserve( CombinedSize );
    NewString.Append( LHS );
    NewString.Append( RHS );
    return NewString;
}

template<typename CharType>
inline TString<CharType> operator+( const CharType* LHS, const TString<CharType>& RHS ) noexcept
{
    const typename TString<CharType>::SizeType CombinedSize = TStringTraits<CharType>::Length( LHS ) + RHS.Length();

    TString<CharType> NewString;
    NewString.Reserve( CombinedSize );
    NewString.Append( LHS );
    NewString.Append( RHS );
    return NewString;
}

template<typename CharType>
inline TString<CharType> operator+( const TString<CharType>& LHS, const CharType* RHS ) noexcept
{
    const typename TString<CharType>::SizeType CombinedSize = LHS.Length() + TStringTraits<CharType>::Length( RHS );

    TString<CharType> NewString;
    NewString.Reserve( CombinedSize );
    NewString.Append( LHS );
    NewString.Append( RHS );
    return NewString;
}

template<typename CharType>
inline TString<CharType> operator+( CharType LHS, const TString<CharType>& RHS ) noexcept
{
    const typename TString<CharType>::SizeType CombinedSize = RHS.Length() + 1;

    TString<CharType> NewString;
    NewString.Reserve( CombinedSize );
    NewString.Append( LHS );
    NewString.Append( RHS );
    return NewString;
}

template<typename CharType>
inline TString<CharType> operator+( const TString<CharType>& LHS, CharType RHS ) noexcept
{
    const typename TString<CharType>::SizeType CombinedSize = LHS.Length() + 1;

    TString<CharType> NewString;
    NewString.Reserve( CombinedSize );
    NewString.Append( LHS );
    NewString.Append( RHS );
    return NewString;
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator==( const TString<CharType>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) == 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator==( const CharType* LHS, const TString<CharType>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) == 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType>
inline bool operator==( const TString<CharType>& LHS, const TString<CharType>& RHS ) noexcept
{
    return (LHS.Compare( RHS ) == 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator!=( const TString<CharType>& LHS, const CharType* RHS ) noexcept
{
    return !(LHS == RHS);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator!=( const CharType* LHS, const TString<CharType>& RHS ) noexcept
{
    return !(LHS == RHS);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType>
inline bool operator!=( const TString<CharType>& LHS, const TString<CharType>& RHS ) noexcept
{
    return !(LHS == RHS);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator<( const TString<CharType>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) < 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator<( const CharType* LHS, const TString<CharType>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) < 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType>
inline bool operator<( const TString<CharType>& LHS, const TString<CharType>& RHS ) noexcept
{
    return (LHS.Compare( RHS ) < 0);
}

template<typename CharType>
inline bool operator<=( const TString<CharType>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) <= 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator<=( const CharType* LHS, const TString<CharType>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) <= 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType>
inline bool operator<=( const TString<CharType>& LHS, const TString<CharType>& RHS ) noexcept
{
    return (LHS.Compare( RHS ) <= 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator>( const TString<CharType>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) > 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator>( const CharType* LHS, const TString<CharType>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) > 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType>
inline bool operator>( const TString<CharType>& LHS, const TString<CharType>& RHS ) noexcept
{
    return (LHS.Compare( RHS ) > 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator>=( const TString<CharType>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) >= 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator>=( const CharType* LHS, const TString<CharType>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) >= 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType>
inline bool operator>=( const TString<CharType>& LHS, const TString<CharType>& RHS ) noexcept
{
    return (LHS.Compare( RHS ) >= 0);
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

/* Convert between char to wide */
inline WString CharToWide( const CString& CharString ) noexcept
{
    WString NewString;
    NewString.Resize( CharString.Length() );

    mbstowcs( NewString.Data(), CharString.CStr(), CharString.Length() );

    return NewString;
}

/* Convert between wide to char */
inline CString WideToChar( const WString& WideString ) noexcept
{
    CString NewString;
    NewString.Resize( WideString.Length() );

    wcstombs( NewString.Data(), WideString.CStr(), WideString.Length() );

    return NewString;
}

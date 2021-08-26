#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"
#include "Iterator.h"

#include "Core/Memory/Memory.h"
#include "Core/Math/Math.h"
#include "Core/Templates/IsSame.h"
#include "Core/Templates/Move.h"
#include "Core/Templates/IsTStringType.h"
#include "Core/Templates/StringTraits.h"

/* Class containing a view of a string */
template<typename CharType>
class TStringView
{
public:

    static_assert(TIsSame<CharType, char>::Value || TIsSame<CharType, wchar_t>::Value, "Only char and wchar_t is supported for strings");
    
    /* Types */
    using ElementType  = CharType;
    using SizeType     = int32;
    using StringTraits = TStringTraits<CharType>;

    /* Constants */
    enum
    {
        InvalidPosition = SizeType( -1 )
    };

    /* Iterators */
    typedef TArrayIterator<const TStringView, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<const TStringView, const CharType> ReverseConstIteratorType;

    /* Default construct an empty view */
    FORCEINLINE TStringView() noexcept
        : ViewStart( nullptr )
        , ViewEnd( nullptr )
    {
    }

    /* Create a view from a pointer and count */
    FORCEINLINE TStringView( const CharType* InString ) noexcept
        : ViewStart( InString )
        , ViewEnd( InString + StringTraits::Length(InString) )
    {
    }

    /* Create a view from a pointer and count */
    FORCEINLINE explicit TStringView( const CharType* InString, SizeType Count ) noexcept
        : ViewStart( InString )
        , ViewEnd( InString + Count )
    {
    }

    /* Create a view from a templated string type */
    template<typename StringType>
    FORCEINLINE explicit TStringView( const StringType& InString ) noexcept
        : ViewStart( InString.CStr() )
        , ViewEnd( InString.CStr() + InString.Length() )
    {
    }

    /* Create a view from another view */
    FORCEINLINE TStringView( const TStringView& Other ) noexcept
        : ViewStart( Other.ViewStart )
        , ViewEnd( Other.ViewEnd )
    {
    }

    /* Move anther view into this one */
    FORCEINLINE TStringView( TStringView&& Other ) noexcept
        : ViewStart( Other.ViewStart )
        , ViewEnd( Other.ViewEnd )
    {
        Other.ViewStart = nullptr;
        Other.ViewEnd = nullptr;
    }

    /* Copy this string into buffer */
    FORCEINLINE void Copy( CharType* Buffer, SizeType BufferSize, SizeType Position = 0) const noexcept
    {
        Assert( Buffer != nullptr);
        Assert( Position < Length() );

        SizeType CopySize = NMath::Min(BufferSize, Length() - Position);
        StringTraits::Copy( Buffer, ViewStart + Position, CopySize);
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
        Compare( InString, StringTraits::Length(InString) );
    }

    /* Compares two strings and checks if they are equal */
    FORCEINLINE bool Compare( const CharType* InString, SizeType InLength )
    {
        SizeType ThisLength = Length();
        if ( ThisLength != InLength)
        {
            // Lengths are not equal so the strings cannot be equal
            return false;
        }
        else if ( (ThisLength == 0) )
        {
            // Lengths are equal, length of view is zero, so they must be equal
            return true;
        }

        const CharType* Start = ViewStart;
        while (Start != ViewEnd)
        {
            if (*(Start++) != *(InString++))
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
        CompareNoCase( InString, StringTraits::Length(InString) );
    }

    /* Compares two strings and checks if they are equal, without taking casing into account */
    FORCEINLINE bool CompareNoCase( const CharType* InString, SizeType InLength )
    {
        SizeType ThisLength = Length();
        if ( ThisLength != InLength )
        {
            // Lengths are not equal so the strings cannot be equal
            return false;
        }
        else if ( (ThisLength == 0) )
        {
            // Lengths are equal, length of view is zero, so they must be equal
            return true;
        }

        const CharType* Start = ViewStart;
        while ( Start != ViewEnd )
        {
            if ( StringTraits::ToLower(*(Start++)) != StringTraits::ToLower(*(InString++)) )
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
        Assert( InOffset < ViewLength );

        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (ViewLength == 0) )
        {
            return 0;
        }

        const CharType* Start  = CStr() + InOffset;
        const CharType* Result = StringTraits::Find( Start, InString );
        if (!Result)
        {
            return InvalidPosition;
        }

        return static_cast<SizeType>(static_cast<intptr_t>(Result - Start));
    }

    /* Returns the position of the first occurance of char */
    FORCEINLINE SizeType Find( CharType InChar, SizeType InOffset = 0 ) const noexcept
    {
        Assert( InOffset < ViewLength );

        if ( StringTraits::IsTerminator( InChar ) || (ViewLength == 0) )
        {
            return 0;
        }

        const CharType* Start  = CStr() + InOffset;
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
        Assert( InOffset < Length() );

        SizeType ThisLength = Length();
        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (ThisLength == 0) )
        {
            return ThisLength;
        }

        // Calculate the offset to the end
        if ( InOffset != 0 )
        {
            ThisLength = NMath::Min(InOffset, ThisLength);
        }

        const CharType* End = ViewStart + Length;
        while ( End != ViewStart )
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
                else if ( StringTraits::IsTerminator(*SubstringIt) )
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
        Assert( InOffset < Length() );

        SizeType ThisLength = Length();
        if ( StringTraits::IsTerminator( InChar ) || (ThisLength == 0) )
        {
            return ThisLength;
        }

        // Calculate the offset to the end
        if ( InOffset != 0 )
        {
            ThisLength = NMath::Min(InOffset, ThisLength);
        }

        const CharType* End = ViewStart + ThisLength;
        while ( End != ViewStart )
        {
            End--;
            if ( *End == InChar )
            {
                return static_cast<SizeType>(static_cast<intptr_t>(End - Start));
            }
        }

        return InvalidPosition;
    }

    /* Returns the position of the the first found character in the searchstring */
    FORCEINLINE SizeType FindOneOf( const CharType* InString, SizeType InOffset = 0 ) const noexcept
    {
        return FindOneOf( InString, StringTraits::Length( InString ), InOffset);
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
        Assert( InOffset < Length() );

        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (Length() == 0) )
        {
            return 0;
        }

        const CharType* Start  = CStr() + InOffset;
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
        Assert( InOffset < Length() );

        if ( (InLength == 0) || StringTraits::IsTerminator( *InString ) || (Length() == 0) )
        {
            return 0;
        }

        SizeType Pos = static_cast<SizeType>(StringTraits::Span( CStr() + InOffset, InString ));
        SizeType Ret = Position + InOffset;
        if ( Ret >= ViewLength )
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

    /* Check if the size is zero or not */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (ViewLength == 0);
    }

    /* Retrive the first element */
    FORCEINLINE const CharType& FirstElement() const noexcept
    {
        Assert( IsEmpty() );
        return Data()[0];
    }

    /* Retrive the last element */
    FORCEINLINE const CharType& LastElement() const noexcept
    {
        Assert( IsEmpty() );
        return Data()[ViewLength - 1];
    }

    /* Retrive an element at a certain position */
    FORCEINLINE const CharType& At( SizeType Index ) const noexcept
    {
        Assert( Index < ViewLength );
        return Data()[Index];
    }

    /* Swap two views */
    FORCEINLINE void Swap( TStringView& Other ) noexcept
    {
        Swap<const CharType*>( ViewStart, Other.ViewStart );
        Swap<SizeType>( ViewLength, Other.ViewLength );
    }

    /* Retrive the last valid index for the view */
    FORCEINLINE SizeType LastIndex() const noexcept
    {
        return ViewLength > 0 ? ViewLength - 1 : 0;
    }

    /* Retrive the size of the view */
    FORCEINLINE SizeType Size() const noexcept
    {
        return ViewLength;
    }

    /* Retrive the length of the view */
    FORCEINLINE SizeType Length() const noexcept
    {
        return ViewLength;
    }

    /* Retrive the size of the view in bytes */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Size() * sizeof( CharType );
    }

    /* Retrive the data of the view */
    FORCEINLINE const CharType* Data() const noexcept
    {
        return ViewStart;
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* CStr() const noexcept
    {
        return ViewStart;
    }

    /* Create a sub-stringview */
    FORCEINLINE TStringView SubStringView( SizeType Offset, SizeType Count ) const noexcept
    {
        Assert((Count < ViewLength) && (Offset + Count < ViewLength) );
        return TStringView( Data() + Offset, Count );
    }

    /* Compares two containers by comparing each element, returns true if all is equal */
    template<typename StringType>
    FORCEINLINE bool operator==( const StringType& Other ) const noexcept
    {
        if ( Length() != Other.Length() )
        {
            return false;
        }

        return CompareRange<CharType>( CStr(), Other.CStr(), Length() );
    }

    /* Compares two containers by comparing each element, returns false if all elements are equal */
    template<typename StringType>
    FORCEINLINE bool operator!=( const StringType& Other ) const noexcept
    {
        return !(*this == Other);
    }

    /* Retrive an element at a certain position */
    FORCEINLINE const CharType& operator[]( SizeType Index ) const noexcept
    {
        return At( Index );
    }

    /* Assign from another view */
    FORCEINLINE TStringView& operator=( const TStringView& Other ) noexcept
    {
        TStringView( Other ).Swap( *this );
        return *this;
    }

    /* Move-assign from another view */
    FORCEINLINE TStringView& operator=( TStringView&& Other ) noexcept
    {
        TStringView( Move( Other ) ).Swap( *this );
        return *this;
    }

public:

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
    FORCEINLINE ConstIteratorType begin() const noexcept
    {
        return StartIterator();
    }

    FORCEINLINE ConstIteratorType end() const noexcept
    {
        return EndIterator();
    }

private:
    const CharType* ViewStart = nullptr;
    const CharType* ViewEnd = nullptr;
};

/* Predefined types */
using CStringView = TStringView<char>;
using WStringView = TStringView<wchar_t>;
#pragma once
#include "Iterator.h"

#include "Core/Math/Math.h"
#include "Core/Templates/IsSame.h"
#include "Core/Templates/Move.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/IsTStringType.h"
#include "Core/Templates/StringUtils.h"

/* Class containing a view of a string */
template<typename CharType>
class TStringView
{
public:

    static_assert(TIsSame<CharType, char>::Value || TIsSame<CharType, wchar_t>::Value, "Only char and wchar_t is supported for strings");

    /* Types */
    using ElementType = CharType;
    using SizeType = int32;
    using StringUtils = TStringUtils<CharType>;

    /* Constants */
    enum
    {
        NPos = SizeType( -1 )
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
        , ViewEnd( InString + StringUtils::Length( InString ) )
    {
    }

    /* Create a view from a pointer and count */
    FORCEINLINE explicit TStringView( const CharType* InString, SizeType Count ) noexcept
        : ViewStart( InString )
        , ViewEnd( InString + Count )
    {
    }

    /* Create a view from a templated string type */
    template<typename StringType, typename = typename TEnableIf<TIsTStringType<StringType>::Value>::Type>
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

    /* Clears the view by resetting the pointers */
    FORCEINLINE void Clear() noexcept
    {
        ViewStart = nullptr;
        ViewEnd = nullptr;
    }

    /* Copy this string into buffer */
    FORCEINLINE void Copy( CharType* Buffer, SizeType BufferSize, SizeType Position = 0 ) const noexcept
    {
        Assert( Buffer != nullptr );
        Assert( (Position < Length()) || (Position == 0) );

        SizeType CopySize = NMath::Min( BufferSize, Length() - Position );
        StringUtils::Copy( Buffer, ViewStart + Position, CopySize );
    }

    /* Removes whitespace from the beginning and end of the string */
    FORCEINLINE TStringView Trim() noexcept
    {
        TStringView NewStringView( *this );
        NewStringView.TrimInline();
        return NewStringView;
    }

    /* Removes whitespace from the beginning and end of the string */
    FORCEINLINE void TrimInline() noexcept
    {
        TrimStartInline();
        TrimEndInline();
    }

    /* Removes whitespace from the beginning of the string */
    FORCEINLINE TStringView TrimStart() noexcept
    {
        TStringView NewStringView( *this );
        NewStringView.TrimStartInline();
        return NewStringView;
    }

    /* Removes whitespace from the beginning of the string */
    FORCEINLINE void TrimStartInline() noexcept
    {
        const CharType* ViewIterator = ViewStart;
        while ( ViewIterator != ViewEnd )
        {
            if ( !StringUtils::IsWhiteSpace( *(ViewIterator++) ) )
            {
                break;
            }
            else
            {
                ViewStart++;
            }
        }
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE TStringView TrimEnd() noexcept
    {
        TStringView NewStringView( *this );
        NewStringView.TrimEndInline();
        return NewStringView;
    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE void TrimEndInline() noexcept
    {
        const CharType* ViewIterator = ViewEnd;
        while ( ViewIterator != ViewStart )
        {
            ViewIterator--;
            if ( !StringUtils::IsWhiteSpace( *ViewIterator ) )
            {
                break;
            }
            else
            {
                ViewEnd--;
            }
        }
    }

	/* Removes whitespace from the beginning of the string */
	FORCEINLINE TStringView ShrinkLeft( int32 Num = 1 ) noexcept
	{
		TStringView NewStringView( *this );
		NewStringView.ShrinkLeftInline( Num );
		return NewStringView;
	}

	/* Removes whitespace from the beginning of the string */
	FORCEINLINE void ShrinkLeftInline( int32 Num = 1 ) noexcept
	{
		if ( Length() <= Num )
		{
			ViewStart = ViewEnd;
		}
		else
		{
			ViewStart += Num;
		}
	}
	
	/* Removes whitespace from the beginning of the string */
	FORCEINLINE TStringView ShrinkRight( int32 Num = 1 ) noexcept
	{
		TStringView NewStringView( *this );
		NewStringView.ShrinkRightInline( Num );
		return NewStringView;
	}

	/* Removes whitespace from the beginning of the string */
	FORCEINLINE void ShrinkRightInline( int32 Num = 1 ) noexcept
	{
		if ( Length() <= Num )
		{
			ViewEnd = ViewStart;
		}
		else
		{
			ViewEnd -= Num;
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
        return Compare( InString, StringUtils::Length( InString ) );
    }

    /* Compares two strings and checks if they are equal */
    FORCEINLINE int32 Compare( const CharType* InString, SizeType InLength ) const noexcept
    {
        SizeType ThisLength = Length();
        if ( ThisLength != InLength )
        {
            // Lengths are not equal so the strings cannot be equal
            return -1;
        }
        else if ( ThisLength == 0 )
        {
            // Lengths are equal, length of view is zero, so they must be equal
            return 0;
        }

        const CharType* Start = ViewStart;
        while ( Start != ViewEnd )
        {
            if ( *Start != *InString )
            {
                return *Start - *InString;
            }

            Start++;
            InString++;
        }

        return 0;
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
        return CompareNoCase( InString, StringUtils::Length( InString ) );
    }

    /* Compares two strings and checks if they are equal, without taking casing into account */
    FORCEINLINE int32 CompareNoCase( const CharType* InString, SizeType InLength ) const noexcept
    {
        SizeType ThisLength = Length();
        if ( ThisLength != InLength )
        {
            // Lengths are not equal so the strings cannot be equal
            return -1;
        }
        else if ( ThisLength == 0 )
        {
            // Lengths are equal, length of view is zero, so they must be equal
            return 0;
        }

        const CharType* Start = ViewStart;
        while ( Start != ViewEnd )
        {
            const CharType TempChar0 = StringUtils::ToLower( *Start );
            const CharType TempChar1 = StringUtils::ToLower( *InString );

            if ( TempChar0 != TempChar1 )
            {
                return TempChar0 - TempChar1;
            }

            Start++;
            InString++;
        }

        return 0;
    }

    /* Returns the position of the first occurance of the start of the searchstring */
    FORCEINLINE SizeType Find( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return Find( InString, StringUtils::Length( InString ), Position );
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

        if ( (InLength == 0) || StringUtils::IsTerminator( *InString ) || (Length() == 0) )
        {
            return 0;
        }

        const CharType* Start = ViewStart + Position;
        while ( Start != ViewEnd )
        {
            // Loop each character in substring
            const CharType* SubstringIt = InString;
            for ( const CharType* It = Start; ; )
            {
                // If not found we end loop and start over
                if ( *(It++) != *(SubstringIt++) )
                {
                    break;
                }
                else if ( StringUtils::IsTerminator( *SubstringIt ) )
                {
                    // If terminator is reached we have found the full substring in out string
                    return static_cast<SizeType>(static_cast<intptr_t>(Start - ViewStart));
                }
            }

            Start++;
        }

        return NPos;
    }

    /* Returns the position of the first occurance of char */
    FORCEINLINE SizeType Find( CharType Char, SizeType Position = 0 ) const noexcept
    {
        Assert( (Position < Length()) || (Position == 0) );

        if ( StringUtils::IsTerminator( Char ) || (Length() == 0) )
        {
            return 0;
        }

        const CharType* Start = ViewStart + Position;
        while ( Start != ViewEnd )
        {
            if ( *Start == Char )
            {
                // If terminator is reached we have found the full substring in out string
                return static_cast<SizeType>(static_cast<intptr_t>(Start - ViewStart));
            }

            Start++;
        }

        return NPos;
    }

    /* Returns the position of the first occurance of the start of the searchstring */
    FORCEINLINE SizeType ReverseFind( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return ReverseFind( InString, StringUtils::Length( InString ), Position );
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

        SizeType ThisLength = Length();
        if ( (InLength == 0) || StringUtils::IsTerminator( *InString ) || (ThisLength == 0) )
        {
            return ThisLength;
        }

        // Calculate the offset to the end
        if ( Position != 0 )
        {
            ThisLength = NMath::Min( Position, ThisLength );
        }

        const CharType* End = ViewStart + ThisLength;
        while ( End != ViewStart )
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
                else if ( StringUtils::IsTerminator( *SubstringIt ) )
                {
                    // If terminator is reached we have found the full substring in out string
                    return static_cast<SizeType>(static_cast<intptr_t>(End - ViewStart));
                }
            }
        }

        return NPos;
    }

    /* Returns the position of the first occurance of char by searching from the end */
    FORCEINLINE SizeType ReverseFind( CharType Char, SizeType Position = 0 ) const noexcept
    {
        Assert( (Position < Length()) || (Position == 0) );

        SizeType ThisLength = Length();
        if ( StringUtils::IsTerminator( Char ) || (ThisLength == 0) )
        {
            return ThisLength;
        }

        // Calculate the offset to the end
        if ( Position != 0 )
        {
            ThisLength = NMath::Min( Position, ThisLength );
        }

        const CharType* End = ViewStart + ThisLength;
        while ( End != ViewStart )
        {
            if ( *(--End) == Char )
            {
                return static_cast<SizeType>(static_cast<intptr_t>(End - ViewStart));
            }
        }

        return NPos;
    }

    /* Returns the position of the the first found character in the searchstring */
    FORCEINLINE SizeType FindOneOf( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return FindOneOf( InString, StringUtils::Length( InString ), Position );
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

        if ( (InLength == 0) || StringUtils::IsTerminator( *InString ) || (Length() == 0) )
        {
            return 0;
        }

        const CharType* Start = ViewStart + Position;
        while ( Start != ViewEnd )
        {
            // Loop each character in substring
            const CharType* SubstringStart = InString;
            const CharType* SubstringEnd = SubstringStart + InLength;
            while ( SubstringStart != SubstringEnd )
            {
                // If not found we end loop and start over
                if ( *(SubstringStart++) == *Start )
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(Start - ViewStart));
                }
            }

            Start++;
        }

        return NPos;
    }

    /* Returns the position of the last occurance of one of the characters in the searchstring */
    FORCEINLINE SizeType ReverseFindOneOf( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return ReverseFindOneOf( InString, StringUtils::Length( InString ), Position );
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
        if ( (InLength == 0) || StringUtils::IsTerminator( *InString ) || (ThisLength == 0) )
        {
            return ThisLength;
        }

        // Calculate the offset to the end
        if ( Position != 0 )
        {
            ThisLength = NMath::Min( Position, ThisLength );
        }

        // Store the length of the substring outside the loop
        SizeType SubstringLength = StringUtils::Length( InString );

        const CharType* ViewIterator = ViewStart + ThisLength;
        while ( ViewIterator != ViewStart )
        {
            ViewIterator--;

            // Loop each character in substring
            const CharType* SubstringStart = InString;
            const CharType* SubstringEnd = SubstringStart + SubstringLength;
            while ( SubstringStart != SubstringEnd )
            {
                // If character is found then return the position
                if ( *ViewIterator == *(SubstringStart++) )
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(ViewIterator - ViewStart));
                }
            }
        }

        return NPos;
    }

    /* Returns the position of the the first character not a part of the searchstring */
    FORCEINLINE SizeType FindOneNotOf( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return FindOneNotOf( InString, StringUtils::Length( InString ), Position );
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

        if ( (InLength == 0) || StringUtils::IsTerminator( *InString ) || (Length() == 0) )
        {
            return 0;
        }

        const CharType* Start = ViewStart + Position;
        while ( Start != ViewEnd )
        {
            // Loop each character in substring
            const CharType* SubstringStart = InString;
            const CharType* SubstringEnd = SubstringStart + InLength;
            while ( SubstringStart != SubstringEnd )
            {
                // If not found we end loop and start over
                if ( *(SubstringStart++) == *Start )
                {
                    break;
                }
                else if ( StringUtils::IsTerminator( *SubstringStart ) )
                {
                    // If terminator is reached we have found the full substring in out string
                    return static_cast<SizeType>(static_cast<intptr_t>(Start - ViewStart));
                }
            }

            Start++;
        }

        return NPos;
    }

    /* Returns the position of the last occurance of one of the characters in the searchstring */
    FORCEINLINE SizeType ReverseFindOneNotOf( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return ReverseFindOneNotOf( InString, StringUtils::Length( InString ), Position );
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
        if ( (InLength == 0) || StringUtils::IsTerminator( *InString ) || (ThisLength == 0) )
        {
            return ThisLength;
        }

        // Calculate the offset to the end
        if ( Position != 0 )
        {
            ThisLength = NMath::Min( Position, ThisLength );
        }

        // Store the length of the substring outside the loop
        SizeType SubstringLength = StringUtils::Length( InString );

        const CharType* ViewIterator = ViewStart + ThisLength;
        while ( ViewIterator != ViewStart )
        {
            ViewIterator--;

            // Loop each character in substring
            const CharType* SubstringStart = InString;
            const CharType* SubstringEnd = SubstringStart + SubstringLength;
            while ( SubstringStart != SubstringEnd )
            {
                // If character is found then return the position
                if ( *ViewIterator == *(SubstringStart++) )
                {
                    break;
                }
                else if ( StringUtils::IsTerminator( *SubstringStart ) )
                {
                    return static_cast<SizeType>(static_cast<intptr_t>(ViewIterator - ViewStart));
                }
            }
        }

        return NPos;
    }

    /* Returns true if the searchstring exists withing the string */
    FORCEINLINE bool Contains( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return (Find( InString, Position ) != NPos);
    }

    /* Returns true if the searchstring exists withing the string */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type Contains( const StringType& InString, SizeType Position = 0 ) const noexcept
    {
        return (Find( InString, Position ) != NPos);
    }

    /* Returns true if the searchstring exists withing the string */
    FORCEINLINE bool Contains( const CharType* InString, SizeType InLength, SizeType Position = 0 ) const noexcept
    {
        return (Find( InString, InLength, Position ) != NPos);
    }

    /* Returns true if the searchstring exists withing the string */
    FORCEINLINE bool Contains( CharType Char, SizeType Position = 0 ) const noexcept
    {
        return (Find( Char, Position ) != NPos);
    }

    /* Returns the position of the the first found character in the searchstring */
    FORCEINLINE bool ContainsOneOf( const CharType* InString, SizeType Position = 0 ) const noexcept
    {
        return (FindOneOf( InString, Position ) != NPos);
    }

    /* Returns true if the searchstring exists withing the string */
    template<typename StringType>
    FORCEINLINE typename TEnableIf<TIsTStringType<StringType>::Value, bool>::Type ContainsOneOf( const StringType& InString, SizeType InLength, SizeType Position = 0 ) const noexcept
    {
        return (FindOneOf<StringType>( InString, InLength, Position ) != NPos);
    }

    /* Returns true if the searchstring exists withing the string */
    FORCEINLINE bool ContainsOneOf( const CharType* InString, SizeType InLength, SizeType Position = 0 ) const noexcept
    {
        return (FindOneOf( InString, InLength, Position ) != NPos);
    }

    /* Check if the size is zero or not */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (Length() == 0);
    }

    /* Retrieve the first element */
    FORCEINLINE const CharType& FirstElement() const noexcept
    {
        Assert( IsEmpty() );
        return Data()[0];
    }

    /* Retrieve the last element */
    FORCEINLINE const CharType& LastElement() const noexcept
    {
        Assert( IsEmpty() );
        return Data()[LastElementIndex()];
    }

    /* Retrieve an element at a certain position */
    FORCEINLINE const CharType& At( SizeType Index ) const noexcept
    {
        Assert( Index < Length() );
        return Data()[Index];
    }

    /* Swap two views */
    FORCEINLINE void Swap( TStringView& Other ) noexcept
    {
        ::Swap<const CharType*>( ViewStart, Other.ViewStart );
        ::Swap<const CharType*>( ViewEnd, Other.ViewEnd );
    }

    /* Retrieve the last valid index for the view */
    FORCEINLINE SizeType LastElementIndex() const noexcept
    {
        SizeType Len = Length();
        return (Len > 0) ? (Len - 1) : 0;
    }

    /* Retrieve the size of the view */
    FORCEINLINE SizeType Size() const noexcept
    {
        return Length();
    }

    /* Retrieve the length of the view */
    FORCEINLINE SizeType Length() const noexcept
    {
        return static_cast<SizeType>(static_cast<intptr_t>(ViewEnd - ViewStart));
    }

    /* Retrieve the size of the view in bytes */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Size() * sizeof( CharType );
    }

    /* Retrieve the data of the view */
    FORCEINLINE const CharType* Data() const noexcept
    {
        return ViewStart;
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* CStr() const noexcept
    {
        return (ViewStart == nullptr) ? StringUtils::Empty() : ViewStart;
    }

    /* Create a sub-stringview */
    FORCEINLINE TStringView SubStringView( SizeType Offset, SizeType Count ) const noexcept
    {
        Assert( (Count < Length()) && (Offset + Count < Length()) );
        return TStringView( Data() + Offset, Count );
    }

    /* Retrieve an element at a certain position */
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

/* Compares with a raw string */
template<typename CharType>
inline bool operator==( const TStringView<CharType>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) == 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator==( const CharType* LHS, const TStringView<CharType>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) == 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType>
inline bool operator==( const TStringView<CharType>& LHS, const TStringView<CharType>& RHS ) noexcept
{
    return (LHS.Compare( RHS ) == 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator!=( const TStringView<CharType>& LHS, const CharType* RHS ) noexcept
{
    return !(LHS == RHS);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator!=( const CharType* LHS, const TStringView<CharType>& RHS ) noexcept
{
    return !(LHS == RHS);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType>
inline bool operator!=( const TStringView<CharType>& LHS, const TStringView<CharType>& RHS ) noexcept
{
    return !(LHS == RHS);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator<( const TStringView<CharType>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) < 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator<( const CharType* LHS, const TStringView<CharType>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) < 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType>
inline bool operator<( const TStringView<CharType>& LHS, const TStringView<CharType>& RHS ) noexcept
{
    return (LHS.Compare( RHS ) < 0);
}

template<typename CharType>
inline bool operator<=( const TStringView<CharType>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) <= 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator<=( const CharType* LHS, const TStringView<CharType>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) <= 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType>
inline bool operator<=( const TStringView<CharType>& LHS, const TStringView<CharType>& RHS ) noexcept
{
    return (LHS.Compare( RHS ) <= 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator>( const TStringView<CharType>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) > 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator>( const CharType* LHS, const TStringView<CharType>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) > 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType>
inline bool operator>( const TStringView<CharType>& LHS, const TStringView<CharType>& RHS ) noexcept
{
    return (LHS.Compare( RHS ) > 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator>=( const TStringView<CharType>& LHS, const CharType* RHS ) noexcept
{
    return (LHS.Compare( RHS ) >= 0);
}

/* Compares with a raw string */
template<typename CharType>
inline bool operator>=( const CharType* LHS, const TStringView<CharType>& RHS ) noexcept
{
    return (RHS.Compare( LHS ) >= 0);
}

/* Compares two containers by comparing each element, returns true if all is equal */
template<typename CharType>
inline bool operator>=( const TStringView<CharType>& LHS, const TStringView<CharType>& RHS ) noexcept
{
    return (LHS.Compare( RHS ) >= 0);
}

/* Add TStringView to be a string-type */
template<typename CharType>
struct TIsTStringType<TStringView<CharType>>
{
    enum
    {
        Value = true
    };
};

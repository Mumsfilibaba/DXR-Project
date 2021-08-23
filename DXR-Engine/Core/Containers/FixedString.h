#pragma once
#include "StringView.h"

#include "Core/Templates/EnableIf.h"
#include "Core/Templates/Identity.h"

/* String class with a fixed allocated number of characters */
template<typename CharType, uint32 NumChars>
class TFixedString
{
public:

    using ElementType = CharType;
    using SizeType    = int32;

    enum
    {
        InvalidPosition = SizeType(-1)
    }

    /* Iterators */
    typedef TArrayIterator<TFixedString, CharType>                    IteratorType;
    typedef TArrayIterator<const TFixedString, const CharType>        ConstIteratorType;
    typedef TReverseArrayIterator<TFixedString, CharType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TFixedString, const CharType> ReverseConstIteratorType;

    static_assert(TIsSame<CharType, char>::Value || TIsSame<CharType, wchar_t>::Value, "Only char and wchar_t is supported for strings");
    static_assert(NumChars > 0, "The number of chars has to be more than zero");

    /* Empty constructor */
    FORCEINLINE TFixedString() noexcept
        : String()
        , Length( 0 )
    {
        Memory::Memzero( String, CapacityInBytes() );
    }

    /* Create a fixed string from a raw array. If the string is longer than 
       the allocators the string will be shortened to fit. */ 
    FORCEINLINE TFixedString( const CharType* InString ) noexcept
        : String()
        , Length( 0 )
    {
        if (InString)
        {
            CopyFrom( InString, StrLen(InString) );
        }
    }

    /* Create a fixed string from a raw array and the length. If the string is 
       longer than the allocators the string will be shortened to fit. */ 
    FORCEINLINE explicit TFixedString( const CharType* InString, uint32 InLength ) noexcept
        : String()
        , Length( 0 )
    {
        if (InString)
        {
            CopyFrom( InString, InLength );
        }
    }

    /* Create a string from a bounded array */
    template<SizeType N, typename = typename TEnableIf<TValue<(N < NumChars)>::Value>::Type>
    FORCEINLINE explicit TFixedString( CharType( &InString )[N] ) noexcept
        : String( InArray )
        , Length( 0 )
    {
        CopyFrom( InString, N );
    }

    /* Create a new string from another string type with similar interface. If the
       string is longer than the allocators the string will be shortened to fit. */ 
    template<typename StringType>
    FORCEINLINE explicit TFixedString( const StringType& InString ) noexcept
        : String()
        , Length( 0 )
    {
        CopyFrom( InString.CStr(), InString.Length() );
    }

    /* Retrive the character at Index */
    FORCEINLINE CharType& At( SizeType Index ) noexcept
    {
        return String[Index];
    }

    /* Retrive the character at Index */
    FORCEINLINE const CharType& At( SizeType Index ) const noexcept
    {
        return String[Index];
    }

    /* Appends a string to this string */
    FORCEINLINE void Append( const CharType* InString ) noexcept
    {
        Append( InString, StrLen(InString) );
    }

    /* Appends a string to this string */
    template<typename StringType>
    FORCEINLINE void Append( const StringType& InString ) noexcept
    {
        Append( InString.CStr(), InString.Length() );
    }

    /* Appends a string to this string */
    FORCEINLINE void Append( const CharType* InString, SizeType InLength ) noexcept
    {
        Assert( InString != nullptr );
        Assert( Length + InLength < Capacity() );

        for (SizeType Index = 0; Index < InLength; Index++)
        {
            String[Length + Index] = InString[Index]; 
        }

        Length = Length + InLength;
        String[Length] = '\0';
    }

    /* Format string (similar to snprintf) */
    FORCEINLINE void Format(const CharType* Format, ...) noexcept
    {
        va_list ArgList;
        va_start(ArgList, Format);
        FormatV(Format, ArgList);
        va_end(ArgList);
    }

    /* Format string (similar to snprintf) */
    template<typename StringType>
    FORCEINLINE void Format(const StringType& Format, ...) noexcept
    {
        va_list ArgList;
        va_start(ArgList, Format);
        FormatV(Format.CStr(), ArgList);
        va_end(ArgList);
    }

    /* Format string with a va_list (similar to snprintf) */
    FORCEINLINE void FormatV(const CharType* Format, va_list ArgList) noexcept
    {
        const int32 WrittenChars = VSNPrintF(String, NumChars, Format, ArgList);
        if ( Length + WrittenChars < NumChars )
        {
            Length = WrittenChars;
        }
        else
        {
            Length = NumChars - 1;
        }

        String[Length] = '\0';
    }

    /* Format string with a va_list (similar to snprintf) */    
    template<typename StringType>
    FORCEINLINE void FormatV(const StringType& Format, va_list ArgList) noexcept
    {
        FormatV(Format.CStr(), ArgList);
    }

    /* Same as Format, but appends the result to the current string */
    FORCEINLINE AppendFormat( const CharType* Format, ... ) noexcept
    {
        va_list ArgList;
        va_start(ArgList, Format);
        AppendFormatV( Format, ArgList );
        va_end(ArgList);
    }

    /* Same as Format, but appends the result to the current string */
        template<typename StringType>
    FORCEINLINE AppendFormat( const StringType& Format, ... ) noexcept
    {
        va_list ArgList;
        va_start(ArgList, Format);
        AppendFormatV( Format.CStr(), ArgList );
        va_end(ArgList);
    }

    /* Same as FormatV, but appends the result to the current string */
    FORCEINLINE AppendFormatV( const CharType* Format, va_list ArgList ) noexcept
    {
        const int32 WrittenChars = VSNPrintF( String + Length, NumChars, Format, ArgList );
        if ( Length + WrittenChars < NumChars )
        {
            Length = WrittenChars;
        }
        else
        {
            Length = NumChars - 1;
        }
        
        String[Length] = '\0';
    }

    /* Same as FormatV, but appends the result to the current string */
    template<typename StringType>
    FORCEINLINE AppendFormatV( const StringType& Format, va_list ArgList ) noexcept
    {
        AppendFormatV( Format.CStr(), ArgList );
    }

    /* Clears the string */
    FORCEINLINE void Clear() noexcept
    {
        Length = 0;
        String[Length] = '\0';
    }

    /* Returns this string in lowercase */
    FORCEINLINE void ToLowerInline() noexcept
    {
        for (SizeType Index = 0; Index < NumChars; Index++)
        {
            Elements[Index] = ToLowerSingle(Elements[Index]);
        }
    }

    /* Returns this string in lowercase */
    FORCEINLINE TFixedString ToLower() const noexcept
    {
        TFixedString Result = *this;
        Result.ToLowerInline();
        return Result;
    }

    /* Converts this string in uppercase */
    FORCEINLINE void ToUpperInline() noexcept
    {
        for (SizeType Index = 0; Index < NumChars; Index++)
        {
            Elements[Index] = ToUpperSingle(Elements[Index]);
        }
    }

    /* Returns this string in uppercase */
    FORCEINLINE TFixedString ToUpper() const noexcept
    {
        TFixedString Result = *this;
        Result.ToUpperInline();
        return Result;
    }

    /* Removes whitespace from the beginning and end of the string */
    FORCEINLINE TFixedString Trim() noexcept
    {
        TFixedString Result = *this;
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
    FORCEINLINE TFixedString TrimStart() noexcept
    {

    }

    /* Removes whitespace from the beginning of the string */
    FORCEINLINE void TrimStartInline() noexcept
    {

    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE TFixedString TrimEnd() noexcept
    {

    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE void TrimEndInline() noexcept
    {

    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE TFixedString Reverse() noexcept
    {

    }

    /* Removes whitespace from the end of the string */
    FORCEINLINE TFixedString ReverseInline() noexcept
    {

    }

    /* Compares two strings and checks if they are equal */
    template<typename StringType>
    FORCEINLINE bool Compare( const StringType& InString ) const noexcept
    {
        Compare( InString.CStr(), InString.Length() );
    }

    /* Compares two strings and checks if they are equal */
    FORCEINLINE bool Compare( const CharType* InString ) const noexcept
    {
        Compare( InString, StrLen(InString) );
    }

    /* Compares two strings and checks if they are equal */
    FORCEINLINE bool Compare( const CharType* InString, SizeType InLength )
    {
        if (Length != InLength)
        {
            return false;
        }

        for (SizeType Index = 0; Index < Length; Index++)
        {
            if (String[Index] != InString[Index])
            {
                return false;
            }
        }

        return true;
    }

    /* Compares two strings and checks if they are equal, without taking casing into account */
    template<typename StringType>
    FORCEINLINE bool CompareNoCase( const StringType& InString ) const noexcept
    {
        CompareNoCase( InString.CStr(), InString.Length() );
    }

    /* Compares two strings and checks if they are equal, without taking casing into account */
    FORCEINLINE bool CompareNoCase( const CharType* InString ) const noexcept
    {
        CompareNoCase( InString, StrLen(InString) );
    }

    /* Compares two strings and checks if they are equal, without taking casing into account */
    FORCEINLINE bool CompareNoCase( const CharType* InString, SizeType InLength )
    {
        if (Length != InLength)
        {
            return false;
        }

        for (SizeType Index = 0; Index < Length; Index++)
        {
            if (ToLowerSingle(String[Index]) != ToLowerSingle(InString[Index]))
            {
                return false;
            }
        }

        return true;
    }

    /* Returns the position of the start of the searchstring */
    FORCEINLINE SizeType Find( const CharType* SearchString, SizeType Offset = 0 ) const noexcept
    {
    }

    /* Returns the position of the the first found character in the searchstring */
    FORCEINLINE SizeType FindOneOf( const CharType* SearchString, SizeType Offset = 0 ) const noexcept
    {
    }

     /* Returns true if the searchstring exists withing the string */
    FORCEINLINE bool Contains( const CharType* SearchString, SizeType Offset = 0 ) const noexcept
    {
        return (Find( SearchString, Offset ) != InvalidPosition);
    }

    /* Returns the position of the the first found character in the searchstring */
    FORCEINLINE bool Contains( const CharType* SearchString, SizeType Offset = 0 ) const noexcept
    {
        return (FindOneOf( SearchString, Offset ) != InvalidPosition);
    }

    /* Return a null terminated string */
    FORCEINLINE CharType* Data() noexcept
    {
        return Elements;
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* Data() const noexcept
    {
        return Elements;
    }

    /* Return a null terminated string */
    FORCEINLINE const CharType* CStr() const noexcept
    {
        return Elements;
    }

    /* Return the length of the string */
    FORCEINLINE SizeType Size() const noexcept
    {
        return Length;
    }

    /* Returns a sub-string of this string */
    FORCEINLINE TFixedString SubString( SizeType Offset, SizeType Count ) const noexcept
    {
        Assert((Offset < Length) && (Offset + Count < Length));
        return TFixedString(Elements + Offset, Count);
    }

    /* Returns a sub-stringview of this string */
    FORCEINLINE TStringView<CharType> SubStringView( SizeType Offset, SizeType Count ) const noexcept
    {
        Assert((Offset < Length) && (Offset + Count < Length));
        return TStringView<CharType>(Elements + Offset, Count);
    }

    /* Return the length of the string */
    FORCEINLINE SizeType Length() const noexcept
    {
        return Length;
    }

    constexpr SizeType Capacity() const noexcept
    {
        return NumChars;
    }

    constexpr SizeType CapacityInBytes() const noexcept
    {
        return NumChars * sizeof(CharType);
    }

    /* Retrive the size of the string in bytes */
    FORCEINLINE SizeType SizeInBytes() const noexcept
    {
        return Length * sizeof(CharType);
    }

    /* Checks if the string is empty */
    FORCEINLINE bool IsEmpty() const noexcept
    {
        return (Length == 0);
    }

    /* Compares two containers by comparing each element, returns true if all is equal */
    template<typename StringType>
    FORCEINLINE bool operator==( const StringType& Other ) const noexcept
    {
        return Compare<StringType>(Other);
    }

    /* Compares two containers by comparing each element, returns false if all elements are equal */
    template<typename StringType>
    FORCEINLINE bool operator!=( const StringType& Other ) const noexcept
    {
        return !(*this == Other);
    }

    /* Appends a string to this string */
    FORCEINLINE TFixedString& operator+=( const CharType* InString ) const noexcept
    {
        Append<StringType>( InString );
        return *this;
    }

    /* Appends a string to this string */
    template<typename StringType>
    FORCEINLINE TFixedString& operator+=( const StringType& InString ) const noexcept
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
    FORCEINLINE IteratorType StartIterator() noexcept noexcept
    {
        return IteratorType( *this, 0 );
    }

    /* Returns an iterator to the end of the container */
    FORCEINLINE IteratorType EndIterator() noexcept noexcept
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

        Memory::Memcpy( String, InString, InLength * sizeof(CharType) );
        Length = InLength;
        String[Length] = '\0';
    }

    CharType String[NumChars];
    SizeType Length;
};

/* Predefined types */
template<uint32 NumChars>
using FixedString = TFixedString<char, NumChars>;

template<uint32 NumChars>
using WFixedString = TFixedString<wchar_t, NumChars>;

/* Operators */
template<typename CharType, int32 FirstNumChars, int32 SecondNumChars>
inline TFixedString<CharType, FirstNumChars> operator+( const TFixedString<CharType, FirstNumChars>& LHS, const TFixedString<CharType, FirstNumChars>& RHS )
{
    TFixedString<CharType, FirstNumChars> Result = LHS;
    Result.Append(RHS);
    return Result;
}

// Add TFixedString to be a string-type
template<typename CharType, int32 NumChars>
struct TIsTStringType<TFixedString<CharType, NumChars>>
{
    enum
    {
        Value = true;
    };
};

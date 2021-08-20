#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"

#include "Core/Memory/Memory.h"

template<typename CharType, uint32 NumChars>
class TFixedString
{
public:
    using ElementType = CharType;
    using SizeType    = int32;

    /* Iterators */
    typedef TArrayIterator<TFixedString, ElementType>                    IteratorType;
    typedef TArrayIterator<const TFixedString, const ElementType>        ConstIteratorType;
    typedef TReverseArrayIterator<TFixedString, ElementType>             ReverseIteratorType;
    typedef TReverseArrayIterator<const TFixedString, const ElementType> ReverseConstIteratorType;

    static_assert(NumChars > 0, "The number of chars has to be more than zero");

    /* Empty constructor */
    FORCEINLINE TFixedString()
        : Elements()
        , Length(0)
    {
        Memory::Memzero( Elements, sizeof(CharType) * NumChars );
    }

    /* Return the length of the string */
    FORCEINLINE SizeType Length() const
    {
        return Length;
    }

private:
    CharType Elements[NumChars];
    SizeType Length;
};
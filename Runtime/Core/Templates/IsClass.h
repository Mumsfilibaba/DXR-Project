#pragma once
#include "CoreTypes.h"

#include "IsUnion.h"

/* Determines if a type is of class or struct type*/
template<typename T>
struct TIsClass
{
private:

    /* Has to be declared before usage */
    template<typename U>
    static int8 Test( int U::* );

    template<typename U>
    static int16 Test( ... );

public:
    enum
    {
        Value = !TIsUnion<T>::Value && sizeof( Test<T>( 0 ) ) == 1
    };
};
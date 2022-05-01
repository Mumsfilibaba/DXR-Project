#pragma once
#include "Core/CoreTypes.h"

#include "IsUnion.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsClass

template<typename T>
struct TIsClass
{
private:

    /* Has to be declared before usage */
    template<typename U>
    static int8 Test(int U::*);

    template<typename U>
    static int16 Test(...);

public:

    enum
    {
        Value = (!(TIsUnion<T>::Value) && (sizeof(Test<T>(0)) == 1))
    };
};
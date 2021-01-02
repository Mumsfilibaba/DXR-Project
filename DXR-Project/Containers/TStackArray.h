#pragma once
#include "Defines.h"
#include "Types.h"

#include "Utilities/TUtilities.h"

/*
* TStaticArray - Static array similar to std::array
*/

template<typename T, Int32 N>
class TStaticArray
{
public:
    FORCEINLINE TStaticArray() noexcept
        : Array()
    {
    }

    FORCEINLINE T& At(Int32 Index)
    {
        VALIDATE(Index < N);
        return Array[Index];
    }

    FORCEINLINE const T& At(Int32 Index) const
    {
        VALIDATE(Index < N);
        return Array[Index];
    }

    FORCEINLINE T& Front()
    {
        return Array[0];
    }

    FORCEINLINE const T& Front() const
    {
        return Array[0];
    }

    FORCEINLINE T& Back()
    {
        return Array[N-1];
    }

    FORCEINLINE const T& Back() const
    {
        return Array[N-1];
    }

    FORCEINLINE T* Data()
    {
        return Array;
    }

    FORCEINLINE const T* Data() const
    {
        return Array;
    }

    FORCEINLINE Int32 Size() const
    {
        return N;
    }

    FORCEINLINE T& operator[](Int32 Index)
    {
        return At(Index);
    }

    FORCEINLINE const T& operator[](Int32 Index) const
    {
        return At(Index);
    }

private:
    T Array[N];
};

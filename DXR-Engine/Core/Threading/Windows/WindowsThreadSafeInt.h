#pragma once
#include "Core.h"

#include <Windows.h>

template<typename T>
class TWindowsThreadSafeInt
{
public:
    TWindowsThreadSafeInt() noexcept
        : Value(0)
    {
    }

    TWindowsThreadSafeInt(T InValue)
        : Value(InValue)
    {
    }

    TWindowsThreadSafeInt(const TWindowsThreadSafeInt&) = delete;

    T Increment();

    T Decrement();

    T Add(T RHS);

    T Sub(T RHS);

    T Load() const;

    void Store(T InValue);

    TWindowsThreadSafeInt& operator=(const TWindowsThreadSafeInt&) = delete;

    T operator=(T RHS)
    {
        return Store(RHS);
    }

    T operator++()
    {
        return Increment();
    }

    T operator--()
    {
        return Decrement();
    }

    T operator+=(T RHS)
    {
        return Add(RHS);
    }

    T operator-=(T RHS)
    {
        return Sub(RHS);
    }

    T operator*=(T RHS)
    {
        return Mul(RHS);
    }

    T operator/=(T RHS)
    {
        return Div(RHS);
    }

private:
    T Value;
};

typedef TWindowsThreadSafeInt<int16> ThreadSafeInt16;
typedef TWindowsThreadSafeInt<int32> ThreadSafeInt32;
typedef TWindowsThreadSafeInt<int64> ThreadSafeInt64;

typedef TWindowsThreadSafeInt<uint16> ThreadSafeUInt16;
typedef TWindowsThreadSafeInt<uint32> ThreadSafeUInt32;
typedef TWindowsThreadSafeInt<uint64> ThreadSafeUUnt64;

// Int32
template<>
inline int32 TWindowsThreadSafeInt<int32>::Increment()
{
    return InterlockedIncrement((LONG*)&Value);
}

template<>
inline int32 TWindowsThreadSafeInt<int32>::Decrement()
{
    return InterlockedDecrement((LONG*)&Value);
}

template<>
inline int32 TWindowsThreadSafeInt<int32>::Add(int32 RHS)
{
    InterlockedExchangeAdd((LONG*)&Value, RHS);
    return Value;
}

template<>
inline int32 TWindowsThreadSafeInt<int32>::Sub(int32 RHS)
{
    InterlockedExchangeAdd((LONG*)&Value, -RHS);
    return Value;
}

template<>
inline int32 TWindowsThreadSafeInt<int32>::Load() const
{
    // Makes sure that all prior accesses has completed
    InterlockedCompareExchange((LONG*)&Value, 0, 0);
    return Value;
}

template<>
inline void TWindowsThreadSafeInt<int32>::Store(int32 RHS)
{
    InterlockedExchange((LONG*)&Value, RHS);
}
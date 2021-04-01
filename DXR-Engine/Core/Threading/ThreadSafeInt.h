#pragma once
#include "Core.h"

#include "Core/Threading/Platform/PlatformAtomic.h"

// TODO: Maybe have an unsigned version?
template<typename T>
class TThreadSafeInt
{
public:
    TThreadSafeInt() noexcept
        : Value(0)
    {
    }

    TThreadSafeInt(T InValue) noexcept
        : Value(InValue)
    {
    }

    TThreadSafeInt(const TThreadSafeInt&) = delete;

    T Increment() noexcept;

    T Decrement() noexcept;

    T Add(T RHS) noexcept;

    T Sub(T RHS) noexcept;

    T Load() noexcept;

    void Store(T InValue) noexcept;

    TThreadSafeInt& operator=(const TThreadSafeInt&) = delete;

    T operator=(T RHS) noexcept
    {
        return Store(RHS);
    }

    T operator++(int32) noexcept
    {
        T OldValue = Value;
        Increment();
        return OldValue;
    }

    T operator++() noexcept
    {
        return Increment();
    }

    T operator--(int32) noexcept
    {
        T OldValue = Value;
        Decrement();
        return OldValue;
    }

    T operator--() noexcept
    {
        return Decrement();
    }

    T operator+=(T RHS) noexcept
    {
        return Add(RHS);
    }

    T operator-=(T RHS) noexcept
    {
        return Sub(RHS);
    }

    T operator*=(T RHS) noexcept
    {
        return Mul(RHS);
    }

    T operator/=(T RHS) noexcept
    {
        return Div(RHS);
    }

private:
    volatile T Value;
};

typedef TThreadSafeInt<int32> ThreadSafeInt32;
typedef TThreadSafeInt<int64> ThreadSafeInt64;

// Int32
template<>
inline int32 TThreadSafeInt<int32>::Increment() noexcept
{
    return PlatformAtomic::InterlockedIncrement(&Value);
}

template<>
inline int32 TThreadSafeInt<int32>::Decrement() noexcept
{
    return PlatformAtomic::InterlockedDecrement(&Value);
}

template<>
inline int32 TThreadSafeInt<int32>::Add(int32 RHS) noexcept
{
    return PlatformAtomic::InterlockedAdd(&Value, RHS);
}

template<>
inline int32 TThreadSafeInt<int32>::Sub(int32 RHS) noexcept
{
    return PlatformAtomic::InterlockedSub(&Value, RHS);
}

template<>
inline int32 TThreadSafeInt<int32>::Load() noexcept
{
    // Makes sure that all prior accesses has completed
    PlatformAtomic::InterlockedCompareExchange(&Value, 0, 0);
    return Value;
}

template<>
inline void TThreadSafeInt<int32>::Store(int32 RHS) noexcept
{
    PlatformAtomic::InterlockedExchange(&Value, RHS);
}

// Int64
template<>
inline int64 TThreadSafeInt<int64>::Increment() noexcept
{
    return PlatformAtomic::InterlockedIncrement(&Value);
}

template<>
inline int64 TThreadSafeInt<int64>::Decrement() noexcept
{
    return PlatformAtomic::InterlockedDecrement(&Value);
}

template<>
inline int64 TThreadSafeInt<int64>::Add(int64 RHS) noexcept
{
    return PlatformAtomic::InterlockedAdd(&Value, RHS);
}

template<>
inline int64 TThreadSafeInt<int64>::Sub(int64 RHS) noexcept
{
    return PlatformAtomic::InterlockedSub(&Value, RHS);
}

template<>
inline int64 TThreadSafeInt<int64>::Load() noexcept
{
    // Makes sure that all prior accesses has completed
    PlatformAtomic::InterlockedCompareExchange(&Value, 0, 0);
    return Value;
}

template<>
inline void TThreadSafeInt<int64>::Store(int64 RHS) noexcept
{
    PlatformAtomic::InterlockedExchange(&Value, RHS);
}
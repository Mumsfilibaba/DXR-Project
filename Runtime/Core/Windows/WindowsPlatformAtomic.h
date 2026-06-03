#pragma once
#include "Core/Generic/GenericPlatformAtomic.h"
#include "Core/Windows/Windows.h"
#include <intrin.h>

struct FWindowsPlatformAtomic final : public FGenericPlatformAtomic
{
    // See: https://docs.microsoft.com/en-us/cpp/intrinsics/interlockedexchangeadd-intrinsic-functions?view=msvc-160
    //      https://docs.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-interlockedincrement16
    static_assert(sizeof(int32) == sizeof(long) && alignof(int32) == alignof(long), "int32 must have the same size and alignment as long");

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 Read(volatile const int8* Source) requires(IsLoadOrderingValid<Order>())
    {
        if constexpr (Order == EMemoryOrder::Relaxed)
        {
            return *Source;
        }
        else if constexpr (Order == EMemoryOrder::Acquire)
        {
            const int8 Result = static_cast<int8>(__iso_volatile_load8(reinterpret_cast<const volatile char*>(Source)));
            _ReadWriteBarrier();
            return Result;
        }
        else
        {
            return static_cast<int8>(::_InterlockedCompareExchange8(reinterpret_cast<volatile char*>(const_cast<int8*>(Source)), 0, 0));
        }
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 Read(volatile const int16* Source) requires(IsLoadOrderingValid<Order>())
    {
        if constexpr (Order == EMemoryOrder::Relaxed)
        {
            return *Source;
        }
        else if constexpr (Order == EMemoryOrder::Acquire)
        {
            const int16 Result = static_cast<int16>(__iso_volatile_load16(reinterpret_cast<const volatile short*>(Source)));
            _ReadWriteBarrier();
            return Result;
        }
        else
        {
            return static_cast<int16>(::_InterlockedCompareExchange16(reinterpret_cast<volatile short*>(const_cast<int16*>(Source)), 0, 0));
        }
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 Read(volatile const int32* Source) requires(IsLoadOrderingValid<Order>())
    {
        if constexpr (Order == EMemoryOrder::Relaxed)
        {
            return *Source;
        }
        else if constexpr (Order == EMemoryOrder::Acquire)
        {
            const int32 Result = static_cast<int32>(__iso_volatile_load32(reinterpret_cast<const volatile int*>(Source)));
            _ReadWriteBarrier();
            return Result;
        }
        else
        {
            return static_cast<int32>(::_InterlockedCompareExchange(reinterpret_cast<volatile long*>(const_cast<int32*>(Source)), 0, 0));
        }
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 Read(volatile const int64* Source) requires(IsLoadOrderingValid<Order>())
    {
        if constexpr (Order == EMemoryOrder::Relaxed)
        {
            return *Source;
        }
        else if constexpr (Order == EMemoryOrder::Acquire)
        {
            const int64 Result = static_cast<int64>(__iso_volatile_load64(reinterpret_cast<const volatile long long*>(Source)));
            _ReadWriteBarrier();
            return Result;
        }
        else
        {
            return static_cast<int64>(::_InterlockedCompareExchange64(reinterpret_cast<volatile long long*>(const_cast<int64*>(Source)), 0, 0));
        }
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void Store(volatile int8* Dest, int8 Value) requires(IsStoreOrderingValid<Order>())
    {
        if constexpr (Order == EMemoryOrder::Relaxed)
        {
            *Dest = Value;
        }
        else if constexpr (Order == EMemoryOrder::Release)
        {
            _ReadWriteBarrier();
            __iso_volatile_store8(reinterpret_cast<volatile char*>(Dest), static_cast<char>(Value));
        }
        else
        {
            ::_InterlockedExchange8(reinterpret_cast<volatile char*>(Dest), static_cast<char>(Value));
        }
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void Store(volatile int16* Dest, int16 Value) requires(IsStoreOrderingValid<Order>())
    {
        if constexpr (Order == EMemoryOrder::Relaxed)
        {
            *Dest = Value;
        }
        else if constexpr (Order == EMemoryOrder::Release)
        {
            _ReadWriteBarrier();
            __iso_volatile_store16(reinterpret_cast<volatile short*>(Dest), static_cast<short>(Value));
        }
        else
        {
            ::_InterlockedExchange16(reinterpret_cast<volatile short*>(Dest), static_cast<short>(Value));
        }
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void Store(volatile int32* Dest, int32 Value) requires(IsStoreOrderingValid<Order>())
    {
        if constexpr (Order == EMemoryOrder::Relaxed)
        {
            *Dest = Value;
        }
        else if constexpr (Order == EMemoryOrder::Release)
        {
            _ReadWriteBarrier();
            __iso_volatile_store32(reinterpret_cast<volatile int*>(Dest), static_cast<int>(Value));
        }
        else
        {
            ::_InterlockedExchange(reinterpret_cast<volatile long*>(Dest), static_cast<long>(Value));
        }
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void Store(volatile int64* Dest, int64 Value) requires(IsStoreOrderingValid<Order>())
    {
        if constexpr (Order == EMemoryOrder::Relaxed)
        {
            *Dest = Value;
        }
        else if constexpr (Order == EMemoryOrder::Release)
        {
            _ReadWriteBarrier();
            __iso_volatile_store64(reinterpret_cast<volatile long long*>(Dest), static_cast<long long>(Value));
        }
        else
        {
            ::_InterlockedExchange64(reinterpret_cast<volatile long long*>(Dest), static_cast<long long>(Value));
        }
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedAdd(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int8>(::_InterlockedExchangeAdd8(reinterpret_cast<volatile char*>(Target), static_cast<char>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedAdd(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int16>(::_InterlockedExchangeAdd16(static_cast<volatile short*>(Target), static_cast<short>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedAdd(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int32>(::_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(Target), static_cast<long>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedAdd(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int64>(::_InterlockedExchangeAdd64(static_cast<volatile long long*>(Target), static_cast<long long>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedAnd(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int8>(::_InterlockedAnd8(reinterpret_cast<volatile char*>(Target), static_cast<char>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedAnd(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int16>(::_InterlockedAnd16(static_cast<volatile short*>(Target), static_cast<short>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedAnd(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int32>(::_InterlockedAnd(reinterpret_cast<volatile long*>(Target), static_cast<long>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedAnd(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int64>(::_InterlockedAnd64(static_cast<volatile long long*>(Target), static_cast<long long>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedOr(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int8>(::_InterlockedOr8(reinterpret_cast<volatile char*>(Target), static_cast<char>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedOr(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int16>(::_InterlockedOr16(static_cast<volatile short*>(Target), static_cast<short>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedOr(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int32>(::_InterlockedOr(reinterpret_cast<volatile long*>(Target), static_cast<long>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedOr(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int64>(::_InterlockedOr64(static_cast<volatile long long*>(Target), static_cast<long long>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedXor(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int8>(::_InterlockedXor8(reinterpret_cast<volatile char*>(Target), static_cast<char>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedXor(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int16>(::_InterlockedXor16(static_cast<volatile short*>(Target), static_cast<short>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedXor(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int32>(::_InterlockedXor(reinterpret_cast<volatile long*>(Target), static_cast<long>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedXor(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int64>(::_InterlockedXor64(static_cast<volatile long long*>(Target), static_cast<long long>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedIncrement(volatile int8* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int8>(::_InterlockedExchangeAdd8(reinterpret_cast<volatile char*>(Target), 1)) + 1;
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedIncrement(volatile int16* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int16>(::_InterlockedIncrement16(static_cast<volatile short*>(Target)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedIncrement(volatile int32* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int32>(::_InterlockedIncrement(reinterpret_cast<volatile long*>(Target)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedIncrement(volatile int64* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int64>(::_InterlockedIncrement64(static_cast<volatile long long*>(Target)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedDecrement(volatile int8* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int8>(::_InterlockedExchangeAdd8(reinterpret_cast<volatile char*>(Target), -1)) - 1;
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedDecrement(volatile int16* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int16>(::_InterlockedDecrement16(static_cast<volatile short*>(Target)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedDecrement(volatile int32* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int32>(::_InterlockedDecrement(reinterpret_cast<volatile long*>(Target)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedDecrement(volatile int64* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int64>(::_InterlockedDecrement64(static_cast<volatile long long*>(Target)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedCompareExchange(volatile int8* Target, int8 Exchange, int8 Comparand)
        requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int8>(::_InterlockedCompareExchange8(reinterpret_cast<volatile char*>(Target), static_cast<char>(Exchange), static_cast<char>(Comparand)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedCompareExchange(volatile int16* Target, int16 Exchange, int16 Comparand)
        requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int16>(::_InterlockedCompareExchange16(static_cast<volatile short*>(Target), static_cast<short>(Exchange), static_cast<short>(Comparand)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedCompareExchange(volatile int32* Target, int32 Exchange, int32 Comparand)
        requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int32>(::_InterlockedCompareExchange(reinterpret_cast<volatile long*>(Target), static_cast<long>(Exchange), static_cast<long>(Comparand)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedCompareExchange(volatile int64* Target, int64 Exchange, int64 Comparand)
        requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int64>(::_InterlockedCompareExchange64(static_cast<volatile long long*>(Target), static_cast<long long>(Exchange), static_cast<long long>(Comparand)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedExchange(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int8>(::_InterlockedExchange8(reinterpret_cast<volatile char*>(Target), static_cast<char>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedExchange(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int16>(::_InterlockedExchange16(static_cast<volatile short*>(Target), static_cast<short>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedExchange(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int32>(::_InterlockedExchange(reinterpret_cast<volatile long*>(Target), static_cast<long>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedExchange(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<int64>(::_InterlockedExchange64(static_cast<volatile long long*>(Target), static_cast<long long>(Value)));
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void* InterlockedExchangePointer(void* volatile* Target, void* Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return static_cast<void*>(::_InterlockedExchangePointer(static_cast<volatile PVOID*>(Target), Value));
    }
};

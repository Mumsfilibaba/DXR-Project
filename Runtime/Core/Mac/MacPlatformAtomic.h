#pragma once
#include "Core/Generic/GenericPlatformAtomic.h"

template<EMemoryOrder Order>
static constexpr auto GetGCCMemoryOrder()
{
    if constexpr (Order == EMemoryOrder::Relaxed)
    {
        return __ATOMIC_RELAXED;
    }
    else if constexpr (Order == EMemoryOrder::Acquire)
    {
        return __ATOMIC_ACQUIRE;
    }
    else if constexpr (Order == EMemoryOrder::Release)
    {
        return __ATOMIC_RELEASE;
    }
    else if constexpr (Order == EMemoryOrder::AcquireRelease)
    {
        return __ATOMIC_ACQ_REL;
    }
    else
    {
        return __ATOMIC_SEQ_CST;
    }
}

template<EMemoryOrder Order>
static constexpr auto GetGCCFailureMemoryOrder()
{
    if constexpr (Order == EMemoryOrder::Relaxed)
    {
        return __ATOMIC_RELAXED;
    }
    else if constexpr (Order == EMemoryOrder::Acquire)
    {
        return __ATOMIC_ACQUIRE;
    }
    else if constexpr (Order == EMemoryOrder::Release)
    {
        return __ATOMIC_RELAXED;
    }
    else if constexpr (Order == EMemoryOrder::AcquireRelease)
    {
        return __ATOMIC_ACQUIRE;
    }
    else
    {
        return __ATOMIC_SEQ_CST;
    }
}

struct CORE_API FMacPlatformAtomic final : public FGenericPlatformAtomic
{
    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 Read(volatile const int8* Source) requires(IsLoadOrderingValid<Order>())
    {
        return __atomic_load_n(const_cast<volatile int8*>(Source), GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 Read(volatile const int16* Source) requires(IsLoadOrderingValid<Order>())
    {
        return __atomic_load_n(const_cast<volatile int16*>(Source), GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 Read(volatile const int32* Source) requires(IsLoadOrderingValid<Order>())
    {
        return __atomic_load_n(const_cast<volatile int32*>(Source), GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 Read(volatile const int64* Source) requires(IsLoadOrderingValid<Order>())
    {
        return __atomic_load_n(const_cast<volatile int64*>(Source), GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void Store(volatile int8* Dest, int8 Value) requires(IsStoreOrderingValid<Order>())
    {
        __atomic_store_n(Dest, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void Store(volatile int16* Dest, int16 Value) requires(IsStoreOrderingValid<Order>())
    {
        __atomic_store_n(Dest, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void Store(volatile int32* Dest, int32 Value) requires(IsStoreOrderingValid<Order>())
    {
        __atomic_store_n(Dest, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void Store(volatile int64* Dest, int64 Value) requires(IsStoreOrderingValid<Order>())
    {
        __atomic_store_n(Dest, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedAdd(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_add(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedAdd(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_add(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedAdd(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_add(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedAdd(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_add(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedAnd(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_and(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedAnd(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_and(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedAnd(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_and(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedAnd(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_and(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedOr(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_or(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedOr(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_or(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedOr(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_or(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedOr(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_or(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedXor(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_xor(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedXor(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_xor(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedXor(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_xor(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedXor(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_fetch_xor(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedIncrement(volatile int8* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_add_fetch(Target, 1, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedIncrement(volatile int16* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_add_fetch(Target, 1, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedIncrement(volatile int32* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_add_fetch(Target, 1, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedIncrement(volatile int64* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_add_fetch(Target, 1, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedDecrement(volatile int8* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_sub_fetch(Target, 1, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedDecrement(volatile int16* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_sub_fetch(Target, 1, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedDecrement(volatile int32* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_sub_fetch(Target, 1, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedDecrement(volatile int64* Target) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_sub_fetch(Target, 1, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedCompareExchange(volatile int8* Target, int8 Exchange, int8 Comparand)
        requires(IsReadModifyWriteOrderingValid<Order>())
    {
        int8 Expected = Comparand;
        __atomic_compare_exchange_n(Target, &Expected, Exchange, false, GetGCCMemoryOrder<Order>(), GetGCCFailureMemoryOrder<Order>());
        return Expected;
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedCompareExchange(volatile int16* Target, int16 Exchange, int16 Comparand)
        requires(IsReadModifyWriteOrderingValid<Order>())
    {
        int16 Expected = Comparand;
        __atomic_compare_exchange_n(Target, &Expected, Exchange, false, GetGCCMemoryOrder<Order>(), GetGCCFailureMemoryOrder<Order>());
        return Expected;
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedCompareExchange(volatile int32* Target, int32 Exchange, int32 Comparand)
        requires(IsReadModifyWriteOrderingValid<Order>())
    {
        int32 Expected = Comparand;
        __atomic_compare_exchange_n(Target, &Expected, Exchange, false, GetGCCMemoryOrder<Order>(), GetGCCFailureMemoryOrder<Order>());
        return Expected;
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedCompareExchange(volatile int64* Target, int64 Exchange, int64 Comparand)
        requires(IsReadModifyWriteOrderingValid<Order>())
    {
        int64 Expected = Comparand;
        __atomic_compare_exchange_n(Target, &Expected, Exchange, false, GetGCCMemoryOrder<Order>(), GetGCCFailureMemoryOrder<Order>());
        return Expected;
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int8 InterlockedExchange(volatile int8* Target, int8 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_exchange_n(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int16 InterlockedExchange(volatile int16* Target, int16 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_exchange_n(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int32 InterlockedExchange(volatile int32* Target, int32 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_exchange_n(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE int64 InterlockedExchange(volatile int64* Target, int64 Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_exchange_n(Target, Value, GetGCCMemoryOrder<Order>());
    }

    template<EMemoryOrder Order = EMemoryOrder::Default>
    static FORCEINLINE void* InterlockedExchangePointer(void* volatile* Target, void* Value) requires(IsReadModifyWriteOrderingValid<Order>())
    {
        return __atomic_exchange_n(Target, Value, GetGCCMemoryOrder<Order>());
    }
};

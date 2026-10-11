#pragma once
#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformTLS.h"
#include "Core/Threading/Atomic.h"
#include <new>

class FThreadLocalSlot
{
public:
    FThreadLocalSlot(const FThreadLocalSlot&) = delete;
    FThreadLocalSlot& operator=(const FThreadLocalSlot&) = delete;

    /**
     * @brief Creates a slot that is not allocated until it is first used. The slot is never freed, which keeps the
     * type trivially destructible and usable during static destruction.
     *
     * @param InDestructor Called with a thread's value when that thread exits, unless the value is nullptr
     */
    explicit constexpr FThreadLocalSlot(TLSDestructorFunc InDestructor = nullptr) noexcept
        : Destructor(InDestructor)
        , Index(CORE_INVALID_TLS_INDEX)
    {
    }

    /** @return Returns the slot index, allocating it on the first call, or CORE_INVALID_TLS_INDEX when no slot is available */
    FORCEINLINE uint32 GetIndex() noexcept
    {
        const uint32 CurrentIndex = Index.Load(EMemoryOrder::Acquire);
        return CurrentIndex != CORE_INVALID_TLS_INDEX ? CurrentIndex : AllocateIndex();
    }

    /** @return Returns the calling thread's value, or nullptr when none has been set */
    FORCEINLINE void* GetValue() noexcept
    {
        const uint32 SlotIndex = GetIndex();
        return SlotIndex != CORE_INVALID_TLS_INDEX ? FPlatformTLS::GetTLSValue(SlotIndex) : nullptr;
    }

    /**
     * @brief Sets the calling thread's value. Nothing is stored when no slot could be allocated.
     *
     * @param Value Value to store for the calling thread
     */
    FORCEINLINE void SetValue(void* Value) noexcept
    {
        const uint32 SlotIndex = GetIndex();
        if (SlotIndex != CORE_INVALID_TLS_INDEX)
        {
            FPlatformTLS::SetTLSValue(SlotIndex, Value);
        }
    }

private:
    uint32 AllocateIndex() noexcept
    {
        const uint32 NewIndex = FPlatformTLS::AllocTLSSlot(Destructor);
        if (NewIndex == CORE_INVALID_TLS_INDEX)
        {
            return CORE_INVALID_TLS_INDEX;
        }

        if (Index.CompareExchange(NewIndex, CORE_INVALID_TLS_INDEX))
        {
            return NewIndex;
        }

        FPlatformTLS::FreeTLSSlot(NewIndex);
        return Index.Load(EMemoryOrder::Acquire);
    }

    TLSDestructorFunc Destructor;
    AtomicUInt32      Index;
};

struct FThreadLocalDefaultAllocator
{
    /**
     * @brief Allocates the storage for one thread's instance
     *
     * @param Size Number of bytes to allocate
     * @param Alignment Alignment of the instance
     * @return Returns the storage, or nullptr when the allocation failed
     */
    static FORCEINLINE void* Allocate(uint64 Size, uint32 Alignment)
    {
        return Memory::Malloc(Size, Alignment);
    }

    /**
     * @brief Releases the storage of one thread's instance once it has been destroyed
     *
     * @param Block The storage Allocate returned
     * @param Size The size that was passed to Allocate
     */
    static FORCEINLINE void Free(void* Block, MAYBE_UNUSED uint64 Size)
    {
        Memory::Free(Block);
    }
};

template<typename T, typename AllocatorType = FThreadLocalDefaultAllocator>
class TThreadLocal
{
public:
    TThreadLocal(const TThreadLocal&) = delete;
    TThreadLocal& operator=(const TThreadLocal&) = delete;

    constexpr TThreadLocal() noexcept
        : Slot(&DestroyValue)
    {
    }

    /**
     * @brief Returns the calling thread's instance, creating it on the first call from that thread. The main thread's
     * instance is not destroyed when the process exits.
     *
     * @return Returns the calling thread's instance
     */
    T& Get()
    {
        T* Value = static_cast<T*>(Slot.GetValue());
        if (!Value)
        {
            CHECK(Slot.GetIndex() != CORE_INVALID_TLS_INDEX);
            void* Storage = AllocatorType::Allocate(sizeof(T), static_cast<uint32>(alignof(T)));
            CHECK(Storage != nullptr);
            Value = ::new (Storage) T();
            Slot.SetValue(Value);
        }

        return *Value;
    }

    /** @return Returns the calling thread's instance, or nullptr when the thread has not created it yet */
    FORCEINLINE T* TryGet() noexcept
    {
        return static_cast<T*>(Slot.GetValue());
    }

    /** @return Returns a pointer to the calling thread's instance, creating it on the first call from that thread */
    NODISCARD FORCEINLINE T* operator->()
    {
        return &Get();
    }

    /** @return Returns a reference to the calling thread's instance, creating it on the first call from that thread */
    NODISCARD FORCEINLINE T& operator*()
    {
        return Get();
    }

private:
    static void DestroyValue(void* Value)
    {
        T* Instance = static_cast<T*>(Value);
        Instance->~T();
        AllocatorType::Free(Instance, sizeof(T));
    }

    FThreadLocalSlot Slot;
};

#pragma once
#include "Core/RefCountedObject.h"

#include <type_traits>

// TRef - Helper class when using objects with RefCountedObject as a base

template<typename T>
class TRef
{
public:
    template<typename TOther>
    friend class TRef;

    FORCEINLINE TRef() noexcept
        : RefPtr(nullptr)
    {
    }

    FORCEINLINE TRef(const TRef& Other) noexcept
        : RefPtr(Other.RefPtr)
    {
        AddRef();
    }

    template<typename TOther>
    FORCEINLINE TRef(const TRef<TOther>& Other) noexcept
        : RefPtr(Other.RefPtr)
    {
        static_assert(std::is_convertible<TOther*, T*>());
        AddRef();
    }

    FORCEINLINE TRef(TRef&& Other) noexcept
        : RefPtr(Other.RefPtr)
    {
        Other.RefPtr = nullptr;
    }

    template<typename TOther>
    FORCEINLINE TRef(TRef<TOther>&& Other) noexcept
        : RefPtr(Other.RefPtr)
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Other.RefPtr = nullptr;
    }

    FORCEINLINE TRef(T* InPtr) noexcept
        : RefPtr(InPtr)
    {
    }

    template<typename TOther>
    FORCEINLINE TRef(TOther* InPtr) noexcept
        : RefPtr(InPtr)
    {
        static_assert(std::is_convertible<TOther*, T*>());
    }

    FORCEINLINE ~TRef()
    {
        Release();
    }

    FORCEINLINE T* Reset() noexcept
    {
        T* WeakPtr = RefPtr;
        Release();

        return WeakPtr;
    }

    FORCEINLINE T* ReleaseOwnership() noexcept
    {
        T* WeakPtr = RefPtr;
        RefPtr = nullptr;
        return WeakPtr;
    }

    FORCEINLINE void AddRef() noexcept
    {
        if (RefPtr)
        {
            RefPtr->AddRef();
        }
    }

    FORCEINLINE void Swap(T* InPtr) noexcept
    {
        Release();
        RefPtr = InPtr;
    }

    template<typename TOther>
    FORCEINLINE void Swap(TOther* InPtr) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        Release();
        RefPtr = InPtr;
    }

    FORCEINLINE T* Get() const noexcept
    {
        return RefPtr;
    }

    FORCEINLINE T* GetRefCount() const noexcept
    {
        return RefPtr->GetRefCount();
    }

    FORCEINLINE T* GetAndAddRef() noexcept
    {
        AddRef();
        return RefPtr;
    }

    template<typename TCastType>
    FORCEINLINE TEnableIf<std::is_convertible_v<TCastType*, T*>, TCastType*> GetAs() const noexcept
    {
        return static_cast<TCastType*>(RefPtr);
    }

    FORCEINLINE T* const* GetAddressOf() const noexcept
    {
        return &RefPtr;
    }

    FORCEINLINE T& Dereference() const noexcept
    {
        return *RefPtr;
    }

    FORCEINLINE T* operator->() const noexcept
    {
        return Get();
    }

    FORCEINLINE T* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    FORCEINLINE T& operator*() const noexcept
    {
        return Dereference();
    }

    FORCEINLINE Bool operator==(T* InPtr) const noexcept
    {
        return (RefPtr == InPtr);
    }

    FORCEINLINE Bool operator==(const TRef& Other) const noexcept
    {
        return (RefPtr == Other.RefPtr);
    }

    FORCEINLINE Bool operator!=(T* InPtr) const noexcept
    {
        return (RefPtr != InPtr);
    }

    FORCEINLINE Bool operator!=(const TRef& Other) const noexcept
    {
        return (RefPtr != Other.RefPtr);
    }

    FORCEINLINE Bool operator==(std::nullptr_t) const noexcept
    {
        return (RefPtr == nullptr);
    }

    FORCEINLINE Bool operator!=(std::nullptr_t) const noexcept
    {
        return (RefPtr != nullptr);
    }

    template<typename TOther>
    FORCEINLINE TEnableIf<std::is_convertible_v<TOther*, T*>, Bool> operator==(TOther* RHS) const noexcept
    {
        return (RefPtr == RHS);
    }

    template<typename TOther>
    friend FORCEINLINE TEnableIf<std::is_convertible_v<TOther*, T*>, Bool> operator==(TOther* LHS, const TRef& RHS) noexcept
    {
        return (RHS == LHS);
    }

    template<typename TOther>
    FORCEINLINE TEnableIf<std::is_convertible_v<TOther*, T*>, Bool> operator==(const TRef<TOther>& RHS) const noexcept
    {
        return (RefPtr == RHS.RefPtr);
    }

    template<typename TOther>
    FORCEINLINE TEnableIf<std::is_convertible_v<TOther*, T*>, Bool> operator!=(TOther* RHS) const noexcept
    {
        return (RefPtr != RHS);
    }

    template<typename TOther>
    friend FORCEINLINE TEnableIf<std::is_convertible_v<TOther*, T*>, Bool> operator!=(TOther* LHS, const TRef& RHS) noexcept
    {
        return (RHS != LHS);
    }

    template<typename TOther>
    FORCEINLINE TEnableIf<std::is_convertible_v<TOther*, T*>, Bool> operator!=(const TRef<TOther>& RHS) const noexcept
    {
        return (RefPtr != RHS.RefPtr);
    }

    FORCEINLINE operator Bool() const noexcept
    {
        return (RefPtr != nullptr);
    }

    FORCEINLINE TRef& operator=(const TRef& Other) noexcept
    {
        if (this != std::addressof(Other))
        {
            Release();

            RefPtr = Other.RefPtr;
            AddRef();
        }

        return *this;
    }

    template<typename TOther>
    FORCEINLINE TRef& operator=(const TRef<TOther>& Other) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        if (this != std::addressof(Other))
        {
            Release();

            RefPtr = Other.RefPtr;
            AddRef();
        }

        return *this;
    }

    FORCEINLINE TRef& operator=(TRef&& Other) noexcept
    {
        if (this != std::addressof(Other))
        {
            Release();

            RefPtr			= Other.RefPtr;
            Other.RefPtr	= nullptr;
        }

        return *this;
    }

    template<typename TOther>
    FORCEINLINE TRef& operator=(TRef<TOther>&& Other) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        if (this != std::addressof(Other))
        {
            Release();

            RefPtr			= Other.RefPtr;
            Other.RefPtr	= nullptr;
        }

        return *this;
    }

    FORCEINLINE TRef& operator=(T* InPtr) noexcept
    {
        if (RefPtr != InPtr)
        {
            Release();
            RefPtr = InPtr;
        }

        return *this;
    }

    template<typename TOther>
    FORCEINLINE TRef& operator=(TOther* InPtr) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        if (RefPtr != InPtr)
        {
            Release();
            RefPtr = InPtr;
        }

        return *this;
    }

    FORCEINLINE TRef& operator=(std::nullptr_t) noexcept
    {
        Release();
        return *this;
    }

private:
    FORCEINLINE void Release() noexcept
    {
        if (RefPtr)
        {
            RefPtr->Release();
            RefPtr = nullptr;
        }
    }

    T* RefPtr;
};

// static_cast
template<typename T, typename U>
TRef<T> StaticCast(const TRef<U>& Pointer)
{
    T* RawPointer = static_cast<T*>(Pointer.Get());
    RawPointer->AddRef();
    return TRef<T>(RawPointer);
}

template<typename T, typename U>
TRef<T> StaticCast(TRef<U>&& Pointer)
{
    T* RawPointer = static_cast<T*>(Pointer.Get());
    return TRef<T>(RawPointer);
}

// const_cast
template<typename T, typename U>
TRef<T> ConstCast(const TRef<U>& Pointer)
{
    T* RawPointer = const_cast<T*>(Pointer.Get());
    RawPointer->AddRef();
    return TRef<T>(RawPointer);
}

template<typename T, typename U>
TRef<T> ConstCast(TRef<U>&& Pointer)
{
    T* RawPointer = const_cast<T*>(Pointer.Get());
    return TRef<T>(RawPointer);
}

// reinterpret_cast
template<typename T, typename U>
TRef<T> ReinterpretCast(const TRef<U>& Pointer)
{
    T* RawPointer = reinterpret_cast<T*>(Pointer.Get());
    RawPointer->AddRef();
    return TRef<T>(RawPointer);
}

template<typename T, typename U>
TRef<T> ReinterpretCast(TRef<U>&& Pointer)
{
    T* RawPointer = reinterpret_cast<T*>(Pointer.Get());
    return TRef<T>(RawPointer);
}

// dynamic_cast
template<typename T, typename U>
TRef<T> DynamicCast(const TRef<U>& Pointer)
{
    T* RawPointer = dynamic_cast<T*>(Pointer.Get());
    RawPointer->AddRef();
    return TRef<T>(RawPointer);
}

template<typename T, typename U>
TRef<T> DynamicCast(TRef<U>&& Pointer)
{
    T* RawPointer = dynamic_cast<T*>(Pointer.Get());
    return TRef<T>(RawPointer);
}

// Converts a raw pointer into a TRef
template<typename T, typename U>
TRef<T> MakeSharedRef(U* InRefCountedObject)
{
    if (InRefCountedObject)
    {
        InRefCountedObject->AddRef();
        return TRef<T>(static_cast<T*>(InRefCountedObject));
    }

    return TRef<T>();
}
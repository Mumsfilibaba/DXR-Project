#pragma once

#if PLATFORM_WINDOWS
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/IsConvertible.h"
#include "Core/Templates/IsNullptr.h"

#include <Unknwn.h>

/* TComPtr - Helper class when using ref-counted com-objects  */
template<typename T>
class TComPtr
{
public:

    using ElementType = T;

    template<typename OtherType>
    friend class TComPtr;

    /* Default constructor that set the ptr to nullptr */
    FORCEINLINE TComPtr() noexcept
        : Ptr(nullptr)
    {
    }

    /* Copy-constructor that copy another ptr */
    FORCEINLINE TComPtr(const TComPtr& Other) noexcept
        : Ptr(Other.Ptr)
    {
        AddRef();
    }

    /* Copy-constructor that copy another ptr, by another convertible type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TComPtr(const TComPtr<OtherType>& Other) noexcept
        : Ptr(Other.Ptr)
    {
        AddRef();
    }

    /* Move-constructor that move another ptr */
    FORCEINLINE TComPtr(TComPtr&& Other) noexcept
        : Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /* Move-constructor that move another ptr, by another convertible type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TComPtr(TComPtr<OtherType>&& Other) noexcept
        : Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /* Create a shared-ref from a raw pointer */
    FORCEINLINE TComPtr(ElementType* InPtr) noexcept
        : Ptr(InPtr)
    {
    }

    /* Create a shared-ref from a raw pointer of a convertible type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TComPtr(OtherType* InPtr) noexcept
        : Ptr(InPtr)
    {
    }

    FORCEINLINE ~TComPtr()
    {
        Release();
    }

    /* Resets the container and sets to a potential new raw pointer */
    FORCEINLINE void Reset(ElementType* NewPtr = nullptr) noexcept
    {
        TComPtr(NewPtr).Swap(*this);
    }

    /* Resets the container and sets to a potential new raw pointer of another type */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type Reset(OtherType* NewPtr) noexcept
    {
        Reset(static_cast<ElementType*>(NewPtr));
    }

    /* Swaps the pointers in the two containers */
    FORCEINLINE void Swap(TComPtr& Other)
    {
        ::Swap(Ptr, Other.Ptr);
    }

    /* Releases the ownership of the pointer and returns the pointer */
    FORCEINLINE ElementType* ReleaseOwnership() noexcept
    {
        ElementType* OldPtr = Ptr;
        Ptr = nullptr;
        return OldPtr;
    }

    /* Adds a reference to the stored pointer */
    FORCEINLINE void AddRef() noexcept
    {
        if (Ptr)
        {
            Ptr->AddRef();
        }
    }

    /* Retrieve the raw pointer */
    FORCEINLINE ElementType* Get() const noexcept
    {
        return Ptr;
    }

    /* Retrieve the current reference count of the object */
    FORCEINLINE uint64 GetRefCount() const noexcept
    {
        Assert(IsValid());

        // There are no function to retrieve the refcount for COM-Objects, add a ref and then release it
        Ptr->AddRef();
        return Ptr->Release();
    }

    /* Retrieve the raw pointer and add a reference */
    FORCEINLINE ElementType* GetAndAddRef() noexcept
    {
        AddRef();
        return Ptr;
    }

    /* Releases the objects and returns the address of Ptr */
    FORCEINLINE ElementType** ReleaseAndGetAddressOf() noexcept
    {
        Ptr->Release();
        return &Ptr;
    }

    /* Retrieve the pointer as another type that is convertible */
    template<typename CastType>
    FORCEINLINE HRESULT GetAs(CastType** NewPointer) const noexcept
    {
        return Ptr->QueryInterface(IID_PPV_ARGS(NewPointer));
    }

    FORCEINLINE HRESULT GetAs(REFIID Riid, TComPtr<IUnknown>* ComObject) const
    {
        TComPtr<IUnknown> NewPointer;

        HRESULT Result = Ptr->QueryInterface(Riid, &NewPointer);
        if (SUCCEEDED(Result))
        {
            *ComObject = NewPointer;
        }

        return Result;
    }

    /* Get the address of the raw pointer */
    FORCEINLINE ElementType** GetAddressOf() noexcept
    {
        return &Ptr;
    }

    /* Get the address of the raw pointer */
    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    /* Checks weather the pointer is valid or not */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Ptr != nullptr);
    }

    /* Retrieve the raw pointer */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }

    /* Retrieve the address of the pointer */
    FORCEINLINE ElementType** operator&() noexcept
    {
        return GetAddressOf();
    }

    /* Retrieve the address of the pointer */
    FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /* Dereference the pointer */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    /* Check weather the pointer is valid or not */
    FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /* Copy another shared-ref into this */
    FORCEINLINE TComPtr& operator=(const TComPtr& RHS) noexcept
    {
        TComPtr(RHS).Swap(*this);
        return *this;
    }

    /* Copy another shared-ref into this of another type */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TComPtr&>::Type operator=(const TComPtr<OtherType>& RHS) noexcept
    {
        TComPtr(RHS).Swap(*this);
        return *this;
    }

    /* Move another shared-ref into this */
    FORCEINLINE TComPtr& operator=(TComPtr&& RHS) noexcept
    {
        TComPtr(RHS).Swap(*this);
        return *this;
    }

    /* Move another shared-ref into this of another type */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TComPtr&>::Type operator=(TComPtr<OtherType>&& RHS) noexcept
    {
        TComPtr(RHS).Swap(*this);
        return *this;
    }

    /* Assign from a raw pointer */
    FORCEINLINE TComPtr& operator=(ElementType* RHS) noexcept
    {
        TComPtr(RHS).Swap(*this);
        return *this;
    }

    /* Assign from a pointer of another type */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TComPtr&>::Type operator=(OtherType* RHS) noexcept
    {
        TComPtr(RHS).Swap(*this);
        return *this;
    }

    /* Set the pointer to nullptr */
    FORCEINLINE TComPtr& operator=(NullptrType) noexcept
    {
        TComPtr().Swap(*this);
        return *this;
    }

private:

    FORCEINLINE void Release() noexcept
    {
        if (Ptr)
        {
            Ptr->Release();
            Ptr = nullptr;
        }
    }

    ElementType* Ptr;
};

/* Check the equality between shared ref and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==(const TComPtr<T>& LHS, U* RHS) noexcept
{
    return (LHS.Get() == RHS);
}

/* Check the equality between shared ref and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==(T* LHS, const TComPtr<U>& RHS) noexcept
{
    return (LHS == RHS.Get());
}

/* Check the inequality between shared ref and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=(const TComPtr<T>& LHS, U* RHS) noexcept
{
    return (LHS.Get() != RHS);
}

/* Check the inequality between shared-ref and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=(T* LHS, const TComPtr<U>& RHS) noexcept
{
    return (LHS != RHS.Get());
}

/* Check the equality between shared-refs */
template<typename T, typename U>
FORCEINLINE bool operator==(const TComPtr<T>& LHS, const TComPtr<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

/* Check the equality between shared-refs */
template<typename T, typename U>
FORCEINLINE bool operator!=(const TComPtr<T>& LHS, const TComPtr<U>& RHS) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/* Check the equality between shared-refs and nullptr */
template<typename T>
FORCEINLINE bool operator==(const TComPtr<T>& LHS, NullptrType) noexcept
{
    return (LHS.Get() == nullptr);
}

/* Check the equality between shared-ref and nullptr */
template<typename T>
FORCEINLINE bool operator==(NullptrType, const TComPtr<T>& RHS) noexcept
{
    return (nullptr == RHS.Get());
}

/* Check the inequality between shared-ref and nullptr */
template<typename T>
FORCEINLINE bool operator!=(const TComPtr<T>& LHS, NullptrType) noexcept
{
    return (LHS.Get() != nullptr);
}

/* Check the inequality between shared-ref and nullptr */
template<typename T>
FORCEINLINE bool operator!=(NullptrType, const TComPtr<T>& RHS) noexcept
{
    return (nullptr != RHS.Get());
}

/* Converts a raw pointer into a TComPtr */
template<typename T, typename U>
FORCEINLINE TComPtr<T> MakeComPtr(U* InRefCountedObject)
{
    if (InRefCountedObject)
    {
        InRefCountedObject->AddRef();
        return TComPtr<T>(static_cast<T*>(InRefCountedObject));
    }

    return nullptr;
}

#endif
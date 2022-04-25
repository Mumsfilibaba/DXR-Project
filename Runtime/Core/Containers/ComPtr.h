#pragma once

#if PLATFORM_WINDOWS
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/IsConvertible.h"
#include "Core/Templates/IsNullptr.h"

#include <Unknwn.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* TComPtr - Helper class when using ref-counted com-objects  */

template<typename T>
class TComPtr
{
public:

    using ElementType = T;

    template<typename OtherType>
    friend class TComPtr;

    /**
     * @brief:  Default constructor that set the pointer to nullptr 
     */
    FORCEINLINE TComPtr() noexcept
        : Ptr(nullptr)
    { }

    /**
     * @brief: Copy-constructor 
     * 
     * @param Other: ComPtr to copy from
     */
    FORCEINLINE TComPtr(const TComPtr& Other) noexcept
        : Ptr(Other.Ptr)
    {
        AddRef();
    }

    /**
     * @brief: Copy-constructor that copies from a ComPtr of a convertible type
     *
     * @param Other: ComPtr to copy from
     */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TComPtr(const TComPtr<OtherType>& Other) noexcept
        : Ptr(Other.Ptr)
    {
        AddRef();
    }

    /**
     * @brief: Move-constructor
     *
     * @param Other: ComPtr to move from
     */
    FORCEINLINE TComPtr(TComPtr&& Other) noexcept
        : Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /**
     * @brief: Move-constructor that moves from a ComPtr of a convertible type
     *
     * @param Other: ComPtr to move from
     */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TComPtr(TComPtr<OtherType>&& Other) noexcept
        : Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /**
     * @brief: Construct a ComPtr from a raw pointer. The container takes ownership.
     * 
     * @param InPointer: Pointer to reference
     */
    FORCEINLINE TComPtr(ElementType* InPointer) noexcept
        : Ptr(InPointer)
    { }

    /**
     * @brief: Construct a ComPtr from a raw pointer of a convertible type. The container takes ownership.
     *
     * @param InPointer: Pointer to reference
     */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TComPtr(OtherType* InPointer) noexcept
        : Ptr(InPointer)
    { }

    /**
     * @brief: Default destructor
     */
    FORCEINLINE ~TComPtr()
    {
        Release();
    }

    /**
     * @brief: Resets the container and sets to a potential new raw pointer 
     * 
     * @param NewPtr: New pointer to reference
     */
    FORCEINLINE void Reset(ElementType* NewPtr = nullptr) noexcept
    {
        TComPtr(NewPtr).Swap(*this);
    }

    /**
     * @brief: Resets the container and sets to a new raw pointer from a convertible type
     *
     * @param NewPtr: New pointer to reference
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type Reset(OtherType* NewPtr) noexcept
    {
        Reset(static_cast<ElementType*>(NewPtr));
    }

    /**
     * @brief: Swaps the pointers in the two containers 
     * 
     * @param Other: ComPtr to swap with
     */
    FORCEINLINE void Swap(TComPtr& Other)
    {
        ::Swap(Ptr, Other.Ptr);
    }

    /**
     * @brief: Releases the ownership of the pointer and returns the pointer
     * 
     * @return: Returns the pointer that was previously held by the container
     */
    FORCEINLINE ElementType* ReleaseOwnership() noexcept
    {
        ElementType* OldPtr = Ptr;
        Ptr = nullptr;
        return OldPtr;
    }

    /**
     * @brief: Adds a reference to the stored pointer 
     */
    FORCEINLINE void AddRef() noexcept
    {
        if (Ptr)
        {
            Ptr->AddRef();
        }
    }

    /**
     * @brief: Retrieve the raw pointer 
     * 
     * @return: Returns the raw pointer
     */
    FORCEINLINE ElementType* Get() const noexcept
    {
        return Ptr;
    }

    /**
     * @brief: Retrieve the current reference count of the object. The object needs to be valid.
     * 
     * @return: The current reference count of the stored pointer
     */
    FORCEINLINE uint64 GetRefCount() const noexcept
    {
        Assert(IsValid());

        // There are no function to retrieve the refcount for COM-Objects, add a ref and then release it
        Ptr->AddRef();
        return Ptr->Release();
    }

    /**
     * @brief: Retrieve the raw pointer and add a reference 
     * 
     * @return: Returns the raw pointer
     */
    FORCEINLINE ElementType* GetAndAddRef() noexcept
    {
        AddRef();
        return Ptr;
    }

    /**
     * @brief: Releases the objects and returns the address of the stored pointer
     * 
     * @return: Pointer to the stored pointer
     */
    FORCEINLINE ElementType** ReleaseAndGetAddressOf() noexcept
    {
        Ptr->Release();
        return &Ptr;
    }

    /** 
     * Retrieve the pointer as another type that is convertible by querying the interface type
     * 
     * @param NewPointer: A pointer to store the result in
     * @return: The result of the operation
     */
    template<typename CastType>
    FORCEINLINE HRESULT GetAs(CastType** NewPointer) const noexcept
    {
        return Ptr->QueryInterface(IID_PPV_ARGS(NewPointer));
    }

    /**
     * @brief: Retrieve the pointer as another type that is convertible by querying the interface type
     *
     * @param Riid: The IID of the type to query
     * @param NewPointer: A ComPtr of unknown type to store the result in
     * @return: The result of the operation
     */
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

    /**
     * @brief: Get the address of the raw pointer 
     * 
     * @return: The address of the raw pointer
     */
    FORCEINLINE ElementType** GetAddressOf() noexcept
    {
        return &Ptr;
    }

    /**
     * @brief: Get the address of the raw pointer
     *
     * @return: The address of the raw pointer
     */
    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    /**
     * @brief: Checks weather the pointer is valid or not 
     * 
     * @return: True if the pointer is not nullptr otherwise false
     */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Ptr != nullptr);
    }

public:

    /**
     * @brief: Retrieve the raw pointer
     *
     * @return: Returns the raw pointer
     */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }

    /**
     * @brief: Get the address of the raw pointer
     *
     * @return: The address of the raw pointer
     */
    FORCEINLINE ElementType** operator&() noexcept
    {
        return GetAddressOf();
    }

    /**
     * @brief: Get the address of the raw pointer
     *
     * @return: The address of the raw pointer
     */
    FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /**
     * @brief: Dereference the stored pointer
     *
     * @return: A reference to the object pointed to by the pointer
     */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    /**
     * @brief: Checks weather the pointer is valid or not
     *
     * @return: True if the pointer is not nullptr otherwise false
     */
    FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /**
     * @brief: Copy-assignment operator
     * 
     * @param Rhs: Instance to copy from
     * @return: A reference to this object
     */
    FORCEINLINE TComPtr& operator=(const TComPtr& Rhs) noexcept
    {
        TComPtr(Rhs).Swap(*this);
        return *this;
    }

    /**
     * @brief: Copy-assignment operator that takes a convertible type
     *
     * @param Rhs: Instance to copy from
     * @return: A reference to this object
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TComPtr&>::Type operator=(const TComPtr<OtherType>& Rhs) noexcept
    {
        TComPtr(Rhs).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator
     *
     * @param Rhs: Instance to move from
     * @return: A reference to this object
     */
    FORCEINLINE TComPtr& operator=(TComPtr&& Rhs) noexcept
    {
        TComPtr(Rhs).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator that takes a convertible type
     *
     * @param Rhs: Instance to move from
     * @return: A reference to this object
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TComPtr&>::Type operator=(TComPtr<OtherType>&& Rhs) noexcept
    {
        TComPtr(Rhs).Swap(*this);
        return *this;
    }

    /**
     * @brief: Assignment operator that takes a raw pointer
     *
     * @param Rhs: Pointer to store
     * @return: A reference to this object
     */
    FORCEINLINE TComPtr& operator=(ElementType* Rhs) noexcept
    {
        TComPtr(Rhs).Swap(*this);
        return *this;
    }

    /**
     * @brief: Assignment operator that takes a raw pointer of a convertible type
     *
     * @param Rhs: Pointer to store
     * @return: A reference to this object
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TComPtr&>::Type operator=(OtherType* Rhs) noexcept
    {
        TComPtr(Rhs).Swap(*this);
        return *this;
    }

    /**
     * @brief: Set the pointer to nullptr 
     * 
     * @return: A reference to this object
     */
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TComPtr equality operators

template<typename T, typename U>
FORCEINLINE bool operator==(const TComPtr<T>& Lhs, U* Rhs) noexcept
{
    return (Lhs.Get() == Rhs);
}

template<typename T, typename U>
FORCEINLINE bool operator==(T* Lhs, const TComPtr<U>& Rhs) noexcept
{
    return (Lhs == Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator!=(const TComPtr<T>& Lhs, U* Rhs) noexcept
{
    return (Lhs.Get() != Rhs);
}

template<typename T, typename U>
FORCEINLINE bool operator!=(T* Lhs, const TComPtr<U>& Rhs) noexcept
{
    return (Lhs != Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator==(const TComPtr<T>& Lhs, const TComPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() == Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator!=(const TComPtr<T>& Lhs, const TComPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() != Rhs.Get());
}

template<typename T>
FORCEINLINE bool operator==(const TComPtr<T>& Lhs, NullptrType) noexcept
{
    return (Lhs.Get() == nullptr);
}

template<typename T>
FORCEINLINE bool operator==(NullptrType, const TComPtr<T>& Rhs) noexcept
{
    return (nullptr == Rhs.Get());
}

template<typename T>
FORCEINLINE bool operator!=(const TComPtr<T>& Lhs, NullptrType) noexcept
{
    return (Lhs.Get() != nullptr);
}

template<typename T>
FORCEINLINE bool operator!=(NullptrType, const TComPtr<T>& Rhs) noexcept
{
    return (nullptr != Rhs.Get());
}

 /** @brief: Converts a raw pointer into a TComPtr */
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
#pragma once
#include "Core/RefCounted.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"

template<typename T>
class TSharedRef
{
public:
    using ElementType = T;

    template<typename OtherType>
    friend class TSharedRef;

    /**
     * @brief -  Default constructor that set the pointer to nullptr
     */
    FORCEINLINE TSharedRef() noexcept
        : Ptr(nullptr)
    { }

    /**
     * @brief       - Copy-constructor
     * @param Other - SharedRef to copy from
     */
    FORCEINLINE TSharedRef(const TSharedRef& Other) noexcept
        : Ptr(Other.Ptr)
    {
        AddRef();
    }

    /**
     * @brief       - Copy-constructor from a SharedRef of a convertible type
     * @param Other - SharedRef to copy from
     */
    template<
        typename OtherType, 
        typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedRef(const TSharedRef<OtherType>& Other) noexcept
        : Ptr(Other.Ptr)
    {
        AddRef();
    }

    /**
     * @brief       - Move-constructor
     * @param Other - SharedRef to move from
     */
    FORCEINLINE TSharedRef(TSharedRef&& Other) noexcept
        : Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /**
     * @brief       - Move-constructor from a SharedRef of a convertible type
     * @param Other - SharedRef to move from
     */
    template<
        typename OtherType,
        typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedRef(TSharedRef<OtherType>&& Other) noexcept
        : Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /**
     * @brief           - Construct a SharedRef from a raw pointer. The container takes ownership.
     * @param InPointer - Pointer to reference
     */
    FORCEINLINE TSharedRef(ElementType* InPointer) noexcept
        : Ptr(InPointer)
    { }

    /**
     * @brief           - Construct a SharedRef from a raw pointer of a convertible type. The container takes ownership.
     * @param InPointer - Pointer to reference
     */
    template<
        typename OtherType, 
        typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedRef(OtherType* InPtr) noexcept
        : Ptr(InPtr)
    { }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TSharedRef()
    {
        Release();
    }

    /**
     * @brief        - Resets the container and sets to a potential new raw pointer
     * @param NewPtr - New pointer to reference
     */
    FORCEINLINE void Reset(ElementType* NewPtr = nullptr) noexcept
    {
        TSharedRef(NewPtr).Swap(*this);
    }

    /**
     * @brief        - Resets the container and sets to a potential new raw pointer of a convertible type
     * @param NewPtr - New pointer to reference
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type Reset(OtherType* NewPtr) noexcept
    {
        Reset(static_cast<ElementType*>(NewPtr));
    }

    /**
     * @brief       - Swaps the pointers in the two containers
     * @param Other - SharedPtr to swap with
     */
    FORCEINLINE void Swap(TSharedRef& Other)
    {
        ::Swap(Ptr, Other.Ptr);
    }

    /**
     * @brief  - Releases the ownership of the pointer and returns the pointer
     * @return - Returns the pointer that was previously held by the container
     */
    NODISCARD FORCEINLINE ElementType* ReleaseOwnership() noexcept
    {
        ElementType* OldPtr = Ptr;
        Ptr = nullptr;
        return OldPtr;
    }

    /**
     * @brief - Adds a reference to the stored pointer
     */
    FORCEINLINE void AddRef() noexcept
    {
        if (Ptr)
        {
            Ptr->AddRef();
        }
    }

    /**
     * @brief  - Retrieve the raw pointer
     * @return - Returns the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* Get() const noexcept
    {
        return Ptr;
    }

    /**
     * @brief  - Retrieve the current reference count of the object. The object needs to be valid.
     * @return - The current reference count of the stored pointer
     */
    NODISCARD FORCEINLINE uint64 GetRefCount() const noexcept
    {
        CHECK(IsValid());
        return Ptr->GetRefCount();
    }

    /**
     * @brief  - Releases the objects and returns the address of the stored pointer
     * @return - Pointer to the stored pointer
     */
    NODISCARD FORCEINLINE ElementType** ReleaseAndGetAddressOf() noexcept
    {
        CHECK(IsValid());
        Ptr->Release();
        return &Ptr;
    }

    /**
     * @brief  - Retrieve the raw pointer and add a reference
     * @return - Returns the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* GetAndAddRef() noexcept
    {
        AddRef();
        return Ptr;
    }

    /**
     * @brief  - Retrieve the pointer as another type that is convertible
     * @return - A pointer of the casted type
     */
    template<typename CastType>
    NODISCARD FORCEINLINE typename TEnableIf<TIsConvertible<CastType*, ElementType*>::Value, CastType*>::Type GetAs() const noexcept
    {
        return static_cast<CastType*>(Ptr);
    }

    /**
     * @brief  - Get the address of the raw pointer
     * @return - The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType** GetAddressOf() noexcept
    {
        return &Ptr;
    }

    /**
     * @brief  - Get the address of the raw pointer
     * @return - The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    /**
     * @brief  - Dereference the stored pointer
     * @return - A reference to the object pointed to by the pointer
     */
    NODISCARD FORCEINLINE ElementType& Dereference() const noexcept
    {
        CHECK(IsValid());
        return *Ptr;
    }

    /**
     * @brief  - Checks weather the pointer is valid or not
     * @return - True if the pointer is not nullptr otherwise false
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Ptr != nullptr);
    }

public:

    /**
     * @brief  - Retrieve the raw pointer
     * @return - Returns the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }

    /**
     * @brief  - Retrieve the address of the raw pointer
     * @return - The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType** operator&() noexcept
    {
        return GetAddressOf();
    }

    /**
     * @brief  - Retrieve the address of the raw pointer
     * @return - The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /**
     * @brief  - Dereference the stored pointer
     * @return - A reference to the object pointed to by the pointer
     */
    NODISCARD FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    /**
     * @brief  - Checks weather the pointer is valid or not
     * @return - True if the pointer is not nullptr otherwise false
     */
    NODISCARD FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /**
     * @brief       - Copy-assignment operator
     * @param Other - SharedRef to copy from
     * @return      - A reference to this instance
     */
    FORCEINLINE TSharedRef& operator=(const TSharedRef& Other) noexcept
    {
        TSharedRef(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Copy-assignment operator that takes a convertible type
     * @param Other - SharedRef to copy from
     * @return      - A reference to this instance
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TSharedRef&>::Type operator=(const TSharedRef<OtherType>& Other) noexcept
    {
        TSharedRef(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - SharedRef to move from
     * @return      - A reference to this instance
     */
    FORCEINLINE TSharedRef& operator=(TSharedRef&& Other) noexcept
    {
        TSharedRef(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator that takes a convertible type
     * @param Other - SharedRef to move from
     * @return      - A reference to this instance
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TSharedRef&>::Type operator=(TSharedRef<OtherType>&& Other) noexcept
    {
        TSharedRef(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Assignment operator that takes a raw pointer
     * @param Other - Pointer to store
     * @return      - A reference to this object
     */
    FORCEINLINE TSharedRef& operator=(ElementType* Other) noexcept
    {
        TSharedRef(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Assignment operator that takes a raw pointer of a convertible type
     * @param Other - Pointer to store
     * @return      - A reference to this object
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TSharedRef&>::Type operator=(OtherType* Other) noexcept
    {
        TSharedRef(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief  - Set the pointer to nullptr
     * @return - A reference to this object
     */
    FORCEINLINE TSharedRef& operator=(nullptr_type) noexcept
    {
        TSharedRef().Swap(*this);
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

    ElementType* Ptr = nullptr;
};


template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TSharedRef<T>& LHS, U* RHS) noexcept
{
    return (LHS.Get() == RHS);
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(T* LHS, const TSharedRef<U>& RHS) noexcept
{
    return (LHS == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TSharedRef<T>& LHS, U* RHS) noexcept
{
    return (LHS.Get() != RHS);
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(T* LHS, const TSharedRef<U>& RHS) noexcept
{
    return (LHS != RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TSharedRef<T>& LHS, const TSharedRef<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TSharedRef<T>& LHS, const TSharedRef<U>& RHS) noexcept
{
    return (LHS.Get() != RHS.Get());
}

template<typename T>
NODISCARD FORCEINLINE bool operator==(const TSharedRef<T>& LHS, nullptr_type) noexcept
{
    return (LHS.Get() == nullptr);
}

template<typename T>
NODISCARD FORCEINLINE bool operator==(nullptr_type, const TSharedRef<T>& RHS) noexcept
{
    return (nullptr == RHS.Get());
}

template<typename T>
NODISCARD FORCEINLINE bool operator!=(const TSharedRef<T>& LHS, nullptr_type) noexcept
{
    return (LHS.Get() != nullptr);
}

template<typename T>
NODISCARD FORCEINLINE bool operator!=(nullptr_type, const TSharedRef<T>& RHS) noexcept
{
    return (nullptr != RHS.Get());
}


template<typename T, typename U>
NODISCARD FORCEINLINE TSharedRef<T> MakeSharedRef(U* InRefCountedObject)
{
    if (InRefCountedObject)
    {
        InRefCountedObject->AddRef();
        return TSharedRef<T>(static_cast<T*>(InRefCountedObject));
    }

    return nullptr;
}


template<typename T, typename U>
NODISCARD FORCEINLINE TSharedRef<T> StaticCastSharedRef(const TSharedRef<U>& Pointer)
{
    T* RawPointer = static_cast<T*>(Pointer.Get());
    return MakeSharedRef<T>(RawPointer);
}

template<typename T, typename U>
NODISCARD FORCEINLINE TSharedRef<T> StaticCastSharedRef(TSharedRef<U>&& Pointer)
{
    T* RawPointer = static_cast<T*>(Pointer.ReleaseOwnership());
    return TSharedRef<T>(RawPointer);
}

template<typename T, typename U>
NODISCARD FORCEINLINE TSharedRef<T> ConstCastSharedRef(const TSharedRef<U>& Pointer)
{
    T* RawPointer = const_cast<T*>(Pointer.Get());
    return MakeSharedRef<T>(RawPointer);
}

template<typename T, typename U>
NODISCARD FORCEINLINE TSharedRef<T> ConstCastSharedRef(TSharedRef<U>&& Pointer)
{
    T* RawPointer = const_cast<T*>(Pointer.ReleaseOwnership());
    return TSharedRef<T>(RawPointer);
}

template<typename T, typename U>
NODISCARD FORCEINLINE TSharedRef<T> ReinterpretCastSharedRef(const TSharedRef<U>& Pointer)
{
    T* RawPointer = reinterpret_cast<T*>(Pointer.Get());
    return MakeSharedRef<T>(RawPointer);
}

template<typename T, typename U>
NODISCARD FORCEINLINE TSharedRef<T> ReinterpretCastSharedRef(TSharedRef<U>&& Pointer)
{
    T* RawPointer = reinterpret_cast<T*>(Pointer.ReleaseOwnership());
    return TSharedRef<T>(RawPointer);
}

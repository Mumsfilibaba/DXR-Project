#pragma once
#include "Core/RefCounted.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"

template<typename ElementType>
class TSharedRef
{
public:
    template<typename OtherType>
    friend class TSharedRef;

    /** @brief -  Default constructor that set the pointer to nullptr */
    TSharedRef() noexcept = default;

    /**
     * @brief       - Copy-constructor
     * @param Other - SharedRef to copy from
     */
    FORCEINLINE TSharedRef(const TSharedRef& Other) noexcept
        : Object(Other.Object)
    {
        AddRef();
    }

    /**
     * @brief       - Copy-constructor from a SharedRef of a convertible type
     * @param Other - SharedRef to copy from
     */
    template<typename OtherType>
    FORCEINLINE TSharedRef(const TSharedRef<OtherType>& Other) noexcept requires(TIsConvertible<TAddPointer<OtherType>::Type, TAddPointer<ElementType>::Type>::Value)
        : Object(Other.Object)
    {
        AddRef();
    }

    /**
     * @brief       - Move-constructor
     * @param Other - SharedRef to move from
     */
    FORCEINLINE TSharedRef(TSharedRef&& Other) noexcept
        : Object(Other.Object)
    {
        Other.Object = nullptr;
    }

    /**
     * @brief       - Move-constructor from a SharedRef of a convertible type
     * @param Other - SharedRef to move from
     */
    template<typename OtherType>
    FORCEINLINE TSharedRef(TSharedRef<OtherType>&& Other) noexcept requires(TIsConvertible<TAddPointer<OtherType>::Type, TAddPointer<ElementType>::Type>::Value)
        : Object(Other.Object)
    {
        Other.Object = nullptr;
    }

    /**
     * @brief           - Construct a SharedRef from a raw pointer. The container takes ownership.
     * @param InPointer - Pointer to reference
     */
    FORCEINLINE TSharedRef(ElementType* InPointer) noexcept
        : Object(InPointer)
    {
    }

    /**
     * @brief           - Construct a SharedRef from a raw pointer of a convertible type. The container takes ownership.
     * @param InPointer - Pointer to reference
     */
    template<typename OtherType>
    FORCEINLINE TSharedRef(OtherType* InPtr) noexcept requires(TIsConvertible<TAddPointer<OtherType>::Type, TAddPointer<ElementType>::Type>::Value)
        : Object(InPtr)
    {
    }

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
    FORCEINLINE void Reset(OtherType* NewPtr) noexcept requires(TIsConvertible<TAddPointer<OtherType>::Type, TAddPointer<ElementType>::Type>::Value)
    {
        Reset(static_cast<ElementType*>(NewPtr));
    }

    /**
     * @brief       - Swaps the pointers in the two containers
     * @param Other - SharedPtr to swap with
     */
    FORCEINLINE void Swap(TSharedRef& Other)
    {
        ::Swap(Object, Other.Object);
    }

    /**
     * @brief  - Releases the ownership of the pointer and returns the pointer
     * @return - Returns the pointer that was previously held by the container
     */
    NODISCARD FORCEINLINE ElementType* ReleaseOwnership() noexcept
    {
        ElementType* OldPtr = Object;
        Object = nullptr;
        return OldPtr;
    }

    /**
     * @brief - Adds a reference to the stored pointer
     */
    FORCEINLINE void AddRef() noexcept
    {
        if (Object)
        {
            Object->AddRef();
        }
    }

    /**
     * @brief  - Retrieve the raw pointer
     * @return - Returns the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* Get() const noexcept
    {
        return Object;
    }

    /**
     * @brief  - Releases the objects and returns the address of the stored pointer
     * @return - Pointer to the stored pointer
     */
    NODISCARD FORCEINLINE ElementType** ReleaseAndGetAddressOf() noexcept
    {
        CHECK(IsValid());
        Object->Release();
        return &Object;
    }

    /**
     * @brief  - Retrieve the raw pointer and add a reference
     * @return - Returns the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* GetAndAddRef() noexcept
    {
        AddRef();
        return Object;
    }

    /**
     * @brief  - Retrieve the pointer as another type that is convertible
     * @return - A pointer of the casted type
     */
    template<typename CastType>
    NODISCARD FORCEINLINE CastType* GetAs() const noexcept requires(TIsConvertible<TAddPointer<CastType>::Type, TAddPointer<ElementType>::Type>::Value)
    {
        return static_cast<CastType*>(Object);
    }

    /**
     * @brief  - Get the address of the raw pointer
     * @return - The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType** GetAddressOf() noexcept
    {
        return &Object;
    }

    /**
     * @brief  - Get the address of the raw pointer
     * @return - The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Object;
    }

    /**
     * @brief  - Dereference the stored pointer
     * @return - A reference to the object pointed to by the pointer
     */
    NODISCARD FORCEINLINE ElementType& Dereference() const noexcept
    {
        CHECK(IsValid());
        return *Object;
    }

    /**
     * @brief  - Checks weather the pointer is valid or not
     * @return - True if the pointer is not nullptr otherwise false
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Object != nullptr);
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
    FORCEINLINE TSharedRef& operator=(const TSharedRef<OtherType>& Other) noexcept requires(TIsConvertible<TAddPointer<OtherType>::Type, TAddPointer<ElementType>::Type>::Value)
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
        TSharedRef(::Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator that takes a convertible type
     * @param Other - SharedRef to move from
     * @return      - A reference to this instance
     */
    template<typename OtherType>
    FORCEINLINE TSharedRef& operator=(TSharedRef<OtherType>&& Other) noexcept requires(TIsConvertible<TAddPointer<OtherType>::Type, TAddPointer<ElementType>::Type>::Value)
    {
        TSharedRef(::Move(Other)).Swap(*this);
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
    FORCEINLINE TSharedRef& operator=(OtherType* Other) noexcept requires(TIsConvertible<TAddPointer<OtherType>::Type, TAddPointer<ElementType>::Type>::Value)
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
        if (Object)
        {
            Object->Release();
            Object = nullptr;
        }
    }

    ElementType* Object{nullptr};
};


template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator==(const TSharedRef<ElementType>& LHS, U* RHS) noexcept
{
    return (LHS.Get() == RHS);
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator==(ElementType* LHS, const TSharedRef<U>& RHS) noexcept
{
    return (LHS == RHS.Get());
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator!=(const TSharedRef<ElementType>& LHS, U* RHS) noexcept
{
    return (LHS.Get() != RHS);
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator!=(ElementType* LHS, const TSharedRef<U>& RHS) noexcept
{
    return (LHS != RHS.Get());
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator==(const TSharedRef<ElementType>& LHS, const TSharedRef<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator!=(const TSharedRef<ElementType>& LHS, const TSharedRef<U>& RHS) noexcept
{
    return (LHS.Get() != RHS.Get());
}

template<typename ElementType>
NODISCARD FORCEINLINE bool operator==(const TSharedRef<ElementType>& LHS, nullptr_type) noexcept
{
    return (LHS.Get() == nullptr);
}

template<typename ElementType>
NODISCARD FORCEINLINE bool operator==(nullptr_type, const TSharedRef<ElementType>& RHS) noexcept
{
    return (nullptr == RHS.Get());
}

template<typename ElementType>
NODISCARD FORCEINLINE bool operator!=(const TSharedRef<ElementType>& LHS, nullptr_type) noexcept
{
    return (LHS.Get() != nullptr);
}

template<typename ElementType>
NODISCARD FORCEINLINE bool operator!=(nullptr_type, const TSharedRef<ElementType>& RHS) noexcept
{
    return (nullptr != RHS.Get());
}


template<typename ElementType, typename U>
NODISCARD FORCEINLINE TSharedRef<ElementType> MakeSharedRef(U* InRefCountedObject)
{
    if (InRefCountedObject)
    {
        InRefCountedObject->AddRef();
        return TSharedRef<ElementType>(static_cast<ElementType*>(InRefCountedObject));
    }

    return nullptr;
}


template<typename ElementType, typename U>
NODISCARD FORCEINLINE TSharedRef<ElementType> StaticCastSharedRef(const TSharedRef<U>& Pointer)
{
    ElementType* RawPointer = static_cast<ElementType*>(Pointer.Get());
    return MakeSharedRef<ElementType>(RawPointer);
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE TSharedRef<ElementType> StaticCastSharedRef(TSharedRef<U>&& Pointer)
{
    ElementType* RawPointer = static_cast<ElementType*>(Pointer.ReleaseOwnership());
    return TSharedRef<ElementType>(RawPointer);
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE TSharedRef<ElementType> ConstCastSharedRef(const TSharedRef<U>& Pointer)
{
    ElementType* RawPointer = const_cast<ElementType*>(Pointer.Get());
    return MakeSharedRef<ElementType>(RawPointer);
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE TSharedRef<ElementType> ConstCastSharedRef(TSharedRef<U>&& Pointer)
{
    ElementType* RawPointer = const_cast<ElementType*>(Pointer.ReleaseOwnership());
    return TSharedRef<ElementType>(RawPointer);
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE TSharedRef<ElementType> ReinterpretCastSharedRef(const TSharedRef<U>& Pointer)
{
    ElementType* RawPointer = reinterpret_cast<ElementType*>(Pointer.Get());
    return MakeSharedRef<ElementType>(RawPointer);
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE TSharedRef<ElementType> ReinterpretCastSharedRef(TSharedRef<U>&& Pointer)
{
    ElementType* RawPointer = reinterpret_cast<ElementType*>(Pointer.ReleaseOwnership());
    return TSharedRef<ElementType>(RawPointer);
}

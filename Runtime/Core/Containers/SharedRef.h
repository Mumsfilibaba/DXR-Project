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

    /** @brief Default constructor that sets the pointer to nullptr */
    TSharedRef() = default;

    /**
     * @brief Copy constructor
     * @param Other SharedRef to copy from
     */
    FORCEINLINE TSharedRef(const TSharedRef& Other)
        : Object(Other.Object)
    {
        AddRef();
    }

    /**
     * @brief Copy constructor from a SharedRef of a convertible type
     * @param Other SharedRef to copy from
     */
    template<typename OtherType>
    FORCEINLINE TSharedRef(const TSharedRef<OtherType>& Other) requires(TIsPointerConvertible<OtherType, ElementType>::Value)
        : Object(Other.Object)
    {
        AddRef();
    }

    /**
     * @brief Move constructor
     * @param Other SharedRef to move from
     */
    FORCEINLINE TSharedRef(TSharedRef&& Other)
        : Object(Other.Object)
    {
        Other.Object = nullptr;
    }

    /**
     * @brief Move constructor from a SharedRef of a convertible type
     * @param Other SharedRef to move from
     */
    template<typename OtherType>
    FORCEINLINE TSharedRef(TSharedRef<OtherType>&& Other) requires(TIsConvertible<typename TAddPointer<OtherType>::Type, typename TAddPointer<ElementType>::Type>::Value)
        : Object(Other.Object)
    {
        Other.Object = nullptr;
    }

    /**
     * @brief Construct a SharedRef from a raw pointer. The container takes ownership.
     * @param InPointer Pointer to reference
     */
    FORCEINLINE TSharedRef(ElementType* InPointer)
        : Object(InPointer)
    {
    }

    /**
     * @brief Construct a SharedRef from a raw pointer of a convertible type. The container takes ownership.
     * @param InPointer Pointer to reference
     */
    template<typename OtherType>
    FORCEINLINE TSharedRef(OtherType* InPtr) requires(TIsConvertible<typename TAddPointer<OtherType>::Type, typename TAddPointer<ElementType>::Type>::Value)
        : Object(InPtr)
    {
    }

    /**
     * @brief Destructor
     */
    FORCEINLINE ~TSharedRef()
    {
        Release();
    }

    /**
     * @brief Resets the container and sets to a potential new raw pointer
     * @param NewPtr New pointer to reference
     */
    FORCEINLINE void Reset(ElementType* NewPtr = nullptr)
    {
        TSharedRef(NewPtr).Swap(*this);
    }

    /**
     * @brief Resets the container and sets to a potential new raw pointer of a convertible type
     * @param NewPtr New pointer to reference
     */
    template<typename OtherType>
    FORCEINLINE void Reset(OtherType* NewPtr) requires(TIsConvertible<typename TAddPointer<OtherType>::Type, typename TAddPointer<ElementType>::Type>::Value)
    {
        Reset(static_cast<ElementType*>(NewPtr));
    }

    /**
     * @brief Swaps the pointers in the two containers
     * @param Other SharedRef to swap with
     */
    FORCEINLINE void Swap(TSharedRef& Other)
    {
        ::Swap(Object, Other.Object);
    }

    /**
     * @brief Releases the ownership of the pointer and returns the pointer
     * @return Returns the pointer that was previously held by the container
     */
    NODISCARD FORCEINLINE ElementType* ReleaseOwnership()
    {
        ElementType* OldPtr = Object;
        Object = nullptr;
        return OldPtr;
    }

    /**
     * @brief Adds a reference to the stored pointer
     */
    FORCEINLINE void AddRef()
    {
        if (Object)
        {
            Object->AddRef();
        }
    }

    /**
     * @brief Retrieve the raw pointer
     * @return Returns the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* Get() const
    {
        return Object;
    }

    /**
     * @brief Releases the object and returns the address of the stored pointer
     * @return Pointer to the stored pointer
     */
    NODISCARD FORCEINLINE ElementType** ReleaseAndGetAddressOf()
    {
        CHECK(IsValid());
        Object->Release();
        return &Object;
    }

    /**
     * @brief Retrieve the raw pointer and add a reference
     * @return Returns the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* GetAndAddRef()
    {
        AddRef();
        return Object;
    }

    /**
     * @brief Retrieve the pointer as another type that is convertible
     * @return A pointer of the casted type
     */
    template<typename CastType>
    NODISCARD FORCEINLINE CastType* GetAs() const requires(TIsConvertible<typename TAddPointer<CastType>::Type, typename TAddPointer<ElementType>::Type>::Value)
    {
        return static_cast<CastType*>(Object);
    }

    /**
     * @brief Get the address of the raw pointer
     * @return The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType** GetAddressOf()
    {
        return &Object;
    }

    /**
     * @brief Get the address of the raw pointer
     * @return The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* const* GetAddressOf() const
    {
        return &Object;
    }

    /**
     * @brief Dereference the stored pointer
     * @return A reference to the object pointed to by the pointer
     */
    NODISCARD FORCEINLINE ElementType& Dereference() const
    {
        CHECK(IsValid());
        return *Object;
    }

    /**
     * @brief Checks whether the pointer is valid or not
     * @return True if the pointer is not nullptr, otherwise false
     */
    NODISCARD FORCEINLINE bool IsValid() const
    {
        return Object != nullptr;
    }

public:

    /**
     * @brief Retrieve the raw pointer
     * @return Returns the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* operator->() const
    {
        return Get();
    }

    /**
     * @brief Retrieve the address of the raw pointer
     * @return The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType** operator&()
    {
        return GetAddressOf();
    }

    /**
     * @brief Retrieve the address of the raw pointer
     * @return The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* const* operator&() const
    {
        return GetAddressOf();
    }

    /**
     * @brief Dereference the stored pointer
     * @return A reference to the object pointed to by the pointer
     */
    NODISCARD FORCEINLINE ElementType& operator*() const
    {
        return Dereference();
    }

    /**
     * @brief Checks whether the pointer is valid or not
     * @return True if the pointer is not nullptr, otherwise false
     */
    NODISCARD FORCEINLINE operator bool() const
    {
        return IsValid();
    }

    /**
     * @brief Copy-assignment operator
     * @param Other SharedRef to copy from
     * @return A reference to this instance
     */
    FORCEINLINE TSharedRef& operator=(const TSharedRef& Other)
    {
        TSharedRef(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief Copy-assignment operator that takes a convertible type
     * @param Other SharedRef to copy from
     * @return A reference to this instance
     */
    template<typename OtherType>
    FORCEINLINE TSharedRef& operator=(const TSharedRef<OtherType>& Other) requires(TIsConvertible<typename TAddPointer<OtherType>::Type, typename TAddPointer<ElementType>::Type>::Value)
    {
        TSharedRef(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief Move-assignment operator
     * @param Other SharedRef to move from
     * @return A reference to this instance
     */
    FORCEINLINE TSharedRef& operator=(TSharedRef&& Other)
    {
        TSharedRef(Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief Move-assignment operator that takes a convertible type
     * @param Other SharedRef to move from
     * @return A reference to this instance
     */
    template<typename OtherType>
    FORCEINLINE TSharedRef& operator=(TSharedRef<OtherType>&& Other) requires(TIsConvertible<typename TAddPointer<OtherType>::Type, typename TAddPointer<ElementType>::Type>::Value)
    {
        TSharedRef(Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief Assignment operator that takes a raw pointer
     * @param Other Pointer to store
     * @return A reference to this object
     */
    FORCEINLINE TSharedRef& operator=(ElementType* Other)
    {
        TSharedRef(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief Assignment operator that takes a raw pointer of a convertible type
     * @param Other Pointer to store
     * @return A reference to this object
     */
    template<typename OtherType>
    FORCEINLINE TSharedRef& operator=(OtherType* Other) requires(TIsConvertible<typename TAddPointer<OtherType>::Type, typename TAddPointer<ElementType>::Type>::Value)
    {
        TSharedRef(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief Set the pointer to nullptr
     * @return A reference to this object
     */
    FORCEINLINE TSharedRef& operator=(nullptr_type)
    {
        TSharedRef().Swap(*this);
        return *this;
    }

private:
    FORCEINLINE void Release()
    {
        if (Object)
        {
            Object->Release();
            Object = nullptr;
        }
    }

    ElementType* Object{ nullptr };
};

// Comparison operators
template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator==(const TSharedRef<ElementType>& LHS, U* RHS)
{
    return LHS.Get() == RHS;
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator==(ElementType* LHS, const TSharedRef<U>& RHS)
{
    return LHS == RHS.Get();
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator!=(const TSharedRef<ElementType>& LHS, U* RHS)
{
    return LHS.Get() != RHS;
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator!=(ElementType* LHS, const TSharedRef<U>& RHS)
{
    return LHS != RHS.Get();
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator==(const TSharedRef<ElementType>& LHS, const TSharedRef<U>& RHS)
{
    return LHS.Get() == RHS.Get();
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator!=(const TSharedRef<ElementType>& LHS, const TSharedRef<U>& RHS)
{
    return LHS.Get() != RHS.Get();
}

template<typename ElementType>
NODISCARD FORCEINLINE bool operator==(const TSharedRef<ElementType>& LHS, nullptr_type)
{
    return LHS.Get() == nullptr;
}

template<typename ElementType>
NODISCARD FORCEINLINE bool operator==(nullptr_type, const TSharedRef<ElementType>& RHS)
{
    return RHS.Get() == nullptr;
}

template<typename ElementType>
NODISCARD FORCEINLINE bool operator!=(const TSharedRef<ElementType>& LHS, nullptr_type)
{
    return LHS.Get() != nullptr;
}

template<typename ElementType>
NODISCARD FORCEINLINE bool operator!=(nullptr_type, const TSharedRef<ElementType>& RHS)
{
    return RHS.Get() != nullptr;
}

// Utility functions
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

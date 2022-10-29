#pragma once
#include "Delete.h"

#include "Core/Memory/New.h"
#include "Core/Templates/TypeTraits.h"

template<typename T, typename DeleterType = TDefaultDelete<T>>
class TUniquePtr 
    : private DeleterType // Using inheritance instead of composition to avoid extra memory usage
{
    using Super = DeleterType;

public:
    using ElementType = T;

    template<typename OtherType, typename OtherDeleterType>
    friend class TUniquePtr;

    TUniquePtr(const TUniquePtr& Other) = delete;
    TUniquePtr& operator=(const TUniquePtr& Other) noexcept = delete;

    /**
     * @brief - Default constructor
     */
    FORCEINLINE TUniquePtr() noexcept
        : Super()
        , Object(nullptr)
    { }

    /**
     * @brief - Construct from nullptr
     */
    FORCEINLINE TUniquePtr(nullptr_type) noexcept
        : Super()
        , Object(nullptr)
    { }

    /**
     * @brief           - Constructor that takes a raw pointer
     * @param InPointer - Raw pointer to store
     */
    FORCEINLINE explicit TUniquePtr(ElementType* InPointer) noexcept
        : Super()
        , Object(InPointer)
    { }

    /**
     * @brief       - Move-constructor
     * @param Other - UniquePtr to move from
     */
    FORCEINLINE TUniquePtr(TUniquePtr&& Other) noexcept
        : Super(Move(Other))
        , Object(Other.Object)
    {
        Other.Object = nullptr;
    }

    /**
     * @brief       - Move-constructor that takes a convertible type
     * @param Other - UniquePtr to move from
     */
    template<
        typename OtherType,
        typename OtherDeleterType,
        typename = typename TEnableIf<TIsPointerConvertible<OtherType, ElementType>::Value>::Type>
    FORCEINLINE TUniquePtr(TUniquePtr<OtherType, OtherDeleterType>&& Other) noexcept
        : Super(Move(Other))
        , Object(Other.Object)
    {
        Other.Object = nullptr;
    }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TUniquePtr()
    {
        InternalRelease();
    }

    /**
     * @brief  - Reset stored pointer to nullptr and return the old pointer
     * @return - Returns the pointer previously stored
     */
    FORCEINLINE ElementType* Release() noexcept
    {
        ElementType* OldPointer = Object;
        Object = nullptr;
        return OldPointer;
    }

    /**
     * @brief            - Resets the container by setting the pointer to a new value and releases the old one 
     * @param NewPointer - New pointer to store
     */
    FORCEINLINE void Reset(ElementType* NewPointer = nullptr) noexcept
    {
        TUniquePtr(NewPointer).Swap(*this);
    }

    /**
     * @brief            - Resets the container by setting the pointer to a new value of a convertible type and releases the old one
     * @param NewPointer - New pointer to store
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsPointerConvertible<OtherType, ElementType>::Value>::Type Reset(OtherType* NewPointer) noexcept
    {
        Reset(static_cast<ElementType*>(NewPointer));
    }

    /**
     * @brief       - Swaps this UniquePtr with another
     * @param Other - Pointer to swap
     */ 
    FORCEINLINE void Swap(TUniquePtr& Other) noexcept
    {
        ElementType* Temp = Object;
        Object = Other.Object;
        Other.Object = Temp;
    }

    /**
     * @brief  - Retrieve the stored pointer 
     * @return - Returns the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* Get() const noexcept
    {
        return Object;
    }

    /**
     * @brief  - Retrieve the address of the stored pointer
     * @return - Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Object;
    }

    /**
     * @brief  - Dereference the stored pointer
     * @return - A reference to the object pointed to by the stored pointer
     */
    NODISCARD FORCEINLINE ElementType& Dereference() const noexcept
    {
        CHECK(IsValid());
        return *Object;
    }

    /**
     * @brief  - Checks if the stored pointer is valid
     * @return - Returns true if the stored pointer is not nullptr
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Object != nullptr);
    }

public:

    /**
     * @brief  - Retrieve the stored pointer
     * @return - Returns the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* operator->() const noexcept
    {
        return Object;
    }

    /**
     * @brief  - Dereference the stored pointer
     * @return - A reference to the object pointed to by the stored pointer
     */
    NODISCARD FORCEINLINE ElementType& operator*() const noexcept
    {
        return *Object;
    }

    /**
     * @brief  - Retrieve the address of the stored pointer
     * @return - Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return &Object;
    }

    /**
     * @brief       - Assignment operator that takes a raw pointer 
     * @param Other - Pointer to store
     * @return      - A reference to this instance
     */
    FORCEINLINE TUniquePtr& operator=(ElementType* Other) noexcept
    {
        TUniquePtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - UniquePtr to move from
     * @return      - A reference to this instance
     */
    FORCEINLINE TUniquePtr& operator=(TUniquePtr&& Other) noexcept
    {
        TUniquePtr(Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator that takes a convertible type
     * @param Other - UniquePtr to move from
     * @return      - A reference to this instance
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<
            TIsPointerConvertible<OtherType, ElementType>::Value,
            typename TAddLValueReference<TUniquePtr>::Type
        >::Type operator=(TUniquePtr<OtherType, OtherDeleterType>&& Other) noexcept
    {
        TUniquePtr(Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief - Assignment operator that takes a nullptr
     */
    FORCEINLINE TUniquePtr& operator=(nullptr_type) noexcept
    {
        TUniquePtr().Swap(*this);
        return *this;
    }

    /**
     * @brief  - Checks if the stored pointer is valid
     * @return - Returns true if the stored pointer is not nullptr
     */
    NODISCARD FORCEINLINE operator bool() const noexcept
    {
        return (Object != nullptr);
    }

private:
    FORCEINLINE void InternalRelease() noexcept
    {
        if (Object)
        {
            Super::DeleteElement(Object);
            Object = nullptr;
        }
    }

    ElementType* Object;
};


template<typename T, typename DeleterType>
class TUniquePtr<T[], DeleterType> 
    : private DeleterType
{
    using Super = DeleterType;

public:
    using ElementType = T;
    using SizeType    = int32;

    template<typename OtherType, typename OtherDeleterType>
    friend class TUniquePtr;

    TUniquePtr(const TUniquePtr& Other) = delete;
    TUniquePtr& operator=(const TUniquePtr& Other) noexcept = delete;

    /**
     * @brief - Default constructor
     */
    FORCEINLINE TUniquePtr() noexcept
        : Super()
        , Object(nullptr)
    { }

    /**
     * @brief - Construct from nullptr
     */
    FORCEINLINE TUniquePtr(nullptr_type) noexcept
        : Super()
        , Object(nullptr)
    { }

    /**
     * @brief           - Constructor that takes a raw pointer
     * @param InPointer - Raw pointer to store
     */
    FORCEINLINE explicit TUniquePtr(ElementType* InPointer) noexcept
        : Super()
        , Object(InPointer)
    { }

    /**
     * @brief       - Move-constructor
     * @param Other - UniquePtr to move from
     */
    FORCEINLINE TUniquePtr(TUniquePtr&& Other) noexcept
        : Super(Move(Other))
        , Object(Other.Object)
    {
        Other.Object = nullptr;
    }

    /**
     * @brief       - Move-constructor that takes a convertible type
     * @param Other - UniquePtr to move from
     */
    template<
        typename OtherType,
        typename OtherDeleterType,
        typename = typename TEnableIf<TIsPointerConvertible<OtherType, ElementType>::Value>::Type>
    FORCEINLINE TUniquePtr(TUniquePtr<OtherType[], OtherDeleterType>&& Other) noexcept
        : Super(Move(Other))
        , Object(Other.Object)
    {
        Other.Object = nullptr;
    }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TUniquePtr()
    {
        InternalRelease();
    }

    /**
     * @brief  - Reset stored pointer to nullptr and return the old pointer
     * @return - Returns the pointer previously stored
     */
    NODISCARD FORCEINLINE ElementType* Release() noexcept
    {
        ElementType* OldPointer = Object;
        Object = nullptr;
        return OldPointer;
    }

    /**
     * @brief            - Resets the container by setting the pointer to a new value and releases the old one
     * @param NewPointer - New pointer to store
     */
    FORCEINLINE void Reset(ElementType* NewPointer = nullptr) noexcept
    {
        TUniquePtr(NewPointer).Swap(*this);
    }

    /**
     * @brief            - Resets the container by setting the pointer to a new value of a convertible type and releases the old one
     * @param NewPointer - New pointer to store
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsPointerConvertible<OtherType, ElementType>::Value>::Type Reset(OtherType* NewPointer) noexcept
    {
        Reset(static_cast<ElementType*>(NewPointer));
    }

    /**
     * @brief       - Swaps this UniquePtr with another
     * @param Other - Pointer to swap
     */
    FORCEINLINE void Swap(TUniquePtr& Other) noexcept
    {
        ElementType* Temp = Object;
        Object = Other.Object;
        Other.Object = Temp;
    }

    /**
     * @brief  - Retrieve the stored pointer
     * @return - Returns the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* Get() const noexcept
    {
        return Object;
    }

    /**
     * @brief  - Retrieve the address of the stored pointer
     * @return - Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Object;
    }

    /**
     * @brief  - Checks if the stored pointer is valid
     * @return - Returns true if the stored pointer is not nullptr
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Object != nullptr);
    }

    /**
     * @brief       - Retrieve a element at a certain index of the array
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& GetElementAt(SizeType Index) const noexcept
    {
        CHECK(IsValid());
        return Object[Index];
    }

public:

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& operator[](SizeType Index) const noexcept
    {
        return GetElementAt(Index);
    }

    /**
     * @brief  - Retrieve the address of the stored pointer
     * @return - Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return &Object;
    }

    /**
     * @brief       - Assignment operator that takes a raw pointer
     * @param Other - Pointer to store
     * @return      - A reference to this instance
     */
    FORCEINLINE TUniquePtr& operator=(ElementType* Other) noexcept
    {
        TUniquePtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - UniquePtr to move from
     * @return      - A reference to this instance
     */
    FORCEINLINE TUniquePtr& operator=(TUniquePtr&& Other) noexcept
    {
        TUniquePtr(Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator that takes a convertible type
     * @param Other - UniquePtr to move from
     * @return      - A reference to this instance
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<
            TIsPointerConvertible<OtherType, ElementType>::Value,
            typename TAddLValueReference<TUniquePtr>::Type
        >::Type operator=(TUniquePtr<OtherType[], OtherDeleterType>&& Other) noexcept
    {
        TUniquePtr(Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief  - Assignment operator that takes a nullptr
     * @return - A reference to this instance
     */
    FORCEINLINE TUniquePtr& operator=(nullptr_type) noexcept
    {
        TUniquePtr().Swap(*this);
        return *this;
    }

    /**
     * @brief  - Checks if the stored pointer is valid
     * @return - Returns true if the stored pointer is not nullptr
     */
    NODISCARD FORCEINLINE operator bool() const noexcept
    {
        return (Object != nullptr);
    }

private:
    FORCEINLINE void InternalRelease() noexcept
    {
        if (Object)
        {
            Super::DeleteElement(Object);
            Object = nullptr;
        }
    }

    ElementType* Object;
};


template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TUniquePtr<T>& LHS, U* RHS) noexcept
{
    return (LHS.Get() == RHS);
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(T* LHS, const TUniquePtr<U>& RHS) noexcept
{
    return (LHS == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TUniquePtr<T>& LHS, U* RHS) noexcept
{
    return (LHS.Get() != RHS);
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(T* LHS, const TUniquePtr<U>& RHS) noexcept
{
    return (LHS != RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TUniquePtr<T>& LHS, const TUniquePtr<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TUniquePtr<T>& LHS, const TUniquePtr<U>& RHS) noexcept
{
    return (LHS.Get() != RHS.Get());
}

template<typename T>
NODISCARD FORCEINLINE bool operator==(const TUniquePtr<T>& LHS, nullptr_type) noexcept
{
    return (LHS.Get() == nullptr);
}

template<typename T>
NODISCARD FORCEINLINE bool operator==(nullptr_type, const TUniquePtr<T>& RHS) noexcept
{
    return (nullptr == RHS.Get());
}

template<typename T>
NODISCARD FORCEINLINE bool operator!=(const TUniquePtr<T>& LHS, nullptr_type) noexcept
{
    return (LHS.Get() != nullptr);
}

template<typename T>
NODISCARD FORCEINLINE bool operator!=(nullptr_type, const TUniquePtr<T>& RHS) noexcept
{
    return (nullptr != RHS.Get());
}


template<typename T, typename... ArgTypes>
NODISCARD FORCEINLINE typename TEnableIf<TNot<TIsArray<T>>::Value, TUniquePtr<T>>::Type MakeUnique(ArgTypes&&... Args) noexcept
{
    T* UniquePtr = dbg_new T(Forward<ArgTypes>(Args)...);
    return TUniquePtr<T>(UniquePtr);
}

template<typename T>
NODISCARD FORCEINLINE typename TEnableIf<TIsArray<T>::Value, TUniquePtr<T>>::Type MakeUnique(uint32 Size) noexcept
{
    typedef typename TRemoveExtent<T>::Type Type;

    Type* UniquePtr = dbg_new Type[Size];
    return TUniquePtr<T>(UniquePtr);
}

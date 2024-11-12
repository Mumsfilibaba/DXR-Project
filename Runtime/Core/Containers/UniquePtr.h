#pragma once
#include "Delete.h"
#include "Core/Templates/TypeTraits.h"

template<typename ElementType, typename DeleterType = TDefaultDelete<ElementType>>
class TUniquePtr : private DeleterType // Using inheritance instead of composition to avoid extra memory usage
{
    using Super = DeleterType;

public:

    template<typename OtherType, typename OtherDeleterType>
    friend class TUniquePtr;

    TUniquePtr(const TUniquePtr& Other) = delete;
    TUniquePtr& operator=(const TUniquePtr& Other) noexcept = delete;

    /** @brief - Default constructor */
    TUniquePtr() = default;

    /**
     * @brief - Construct from nullptr
     */
    FORCEINLINE TUniquePtr(nullptr_type) noexcept
        : Super()
        , Object(nullptr)
    {
    }

    /**
     * @brief           - Constructor that takes a raw pointer
     * @param InPointer - Raw pointer to store
     */
    FORCEINLINE explicit TUniquePtr(ElementType* InPointer) noexcept
        : Super()
        , Object(InPointer)
    {
    }

    /**
     * @brief           - Constructor that takes a raw pointer
     * @param InPointer - Raw pointer to store
     * @param Deleter   - Deleter
     */
    FORCEINLINE explicit TUniquePtr(ElementType* InPointer, DeleterType&& Deleter) noexcept
        : Super(::Move(Deleter))
        , Object(InPointer)
    {
    }

    /**
     * @brief       - Move-constructor
     * @param Other - UniquePtr to move from
     */
    FORCEINLINE TUniquePtr(TUniquePtr&& Other) noexcept
        : Super(::Move(Other))
        , Object(Other.Object)
    {
        Other.Object = nullptr;
    }

    /**
     * @brief       - Move-constructor that takes a convertible type
     * @param Other - UniquePtr to move from
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE TUniquePtr(TUniquePtr<OtherType, OtherDeleterType>&& Other) noexcept requires(TIsPointerConvertible<OtherType, ElementType>::Value)
        : Super(::Move(Other.GetDeleter()))
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

    /**
     * @return - Returns this as a reference as a DeleterType
     */
    NODISCARD FORCEINLINE DeleterType& GetDeleter()
    {
        return static_cast<DeleterType&>(*this);
    }

    /**
     * @return - Returns this as a reference as a DeleterType
     */
    NODISCARD FORCEINLINE const DeleterType& GetDeleter() const
    {
        return static_cast<const DeleterType&>(*this);
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
        TUniquePtr(::Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator that takes a convertible type
     * @param Other - UniquePtr to move from
     * @return      - A reference to this instance
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE TUniquePtr& operator=(TUniquePtr<OtherType, OtherDeleterType>&& Other) noexcept requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
        TUniquePtr(::Move(Other)).Swap(*this);
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
            Super::Call(Object);
            Object = nullptr;
        }
    }

    ElementType* Object{nullptr};
};

template<typename ElementType, typename DeleterType>
class TUniquePtr<ElementType[], DeleterType> : private DeleterType
{
    using Super = DeleterType;

public:
    using SizeType = int32;

    template<typename OtherType, typename OtherDeleterType>
    friend class TUniquePtr;

    TUniquePtr(const TUniquePtr& Other) = delete;
    TUniquePtr& operator=(const TUniquePtr& Other) noexcept = delete;

    /** @brief - Default constructor */
    TUniquePtr() = default;

    /**
     * @brief - Construct from nullptr
     */
    FORCEINLINE TUniquePtr(nullptr_type) noexcept
        : Super()
        , Object(nullptr)
    {
    }

    /**
     * @brief           - Constructor that takes a raw pointer
     * @param InPointer - Raw pointer to store
     */
    FORCEINLINE explicit TUniquePtr(ElementType* InPointer) noexcept
        : Super()
        , Object(InPointer)
    {
    }

    /**
     * @brief           - Constructor that takes a raw pointer
     * @param InPointer - Raw pointer to store
     * @param Deleter   - Deleter
     */
    FORCEINLINE explicit TUniquePtr(ElementType* InPointer, DeleterType&& Deleter) noexcept
        : Super(::Move(Deleter))
        , Object(InPointer)
    {
    }

    /**
     * @brief       - Move-constructor
     * @param Other - UniquePtr to move from
     */
    FORCEINLINE TUniquePtr(TUniquePtr&& Other) noexcept
        : Super(::Move(Other))
        , Object(Other.Object)
    {
        Other.Object = nullptr;
    }

    /**
     * @brief       - Move-constructor that takes a convertible type
     * @param Other - UniquePtr to move from
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE TUniquePtr(TUniquePtr<OtherType[], OtherDeleterType>&& Other) noexcept requires(TIsPointerConvertible<OtherType, ElementType>::Value)
        : Super(::Move(Other.GetDeleter()))
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
     * @return - Returns this as a reference as a DeleterType
     */
    NODISCARD FORCEINLINE DeleterType& GetDeleter()
    {
        return static_cast<DeleterType&>(*this);
    }

    /**
     * @return - Returns this as a reference as a DeleterType
     */
    NODISCARD FORCEINLINE const DeleterType& GetDeleter() const
    {
        return static_cast<const DeleterType&>(*this);
    }

public:

    /**
     * @brief       - Bracket-operator to retrieve an element at a certain index
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& operator[](SizeType Index) const noexcept
    {
        CHECK(IsValid());
        return Object[Index];
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
        TUniquePtr(::Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator that takes a convertible type
     * @param Other - UniquePtr to move from
     * @return      - A reference to this instance
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE TUniquePtr& operator=(TUniquePtr<OtherType[], OtherDeleterType>&& Other) noexcept requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
        TUniquePtr(::Move(Other)).Swap(*this);
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
            Super::Call(Object);
            Object = nullptr;
        }
    }

    ElementType* Object{nullptr};
};

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator==(const TUniquePtr<ElementType>& LHS, U* RHS) noexcept
{
    return LHS.Get() == RHS;
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator==(ElementType* LHS, const TUniquePtr<U>& RHS) noexcept
{
    return LHS == RHS.Get();
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator!=(const TUniquePtr<ElementType>& LHS, U* RHS) noexcept
{
    return LHS.Get() != RHS;
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator!=(ElementType* LHS, const TUniquePtr<U>& RHS) noexcept
{
    return LHS != RHS.Get();
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator==(const TUniquePtr<ElementType>& LHS, const TUniquePtr<U>& RHS) noexcept
{
    return LHS.Get() == RHS.Get();
}

template<typename ElementType, typename U>
NODISCARD FORCEINLINE bool operator!=(const TUniquePtr<ElementType>& LHS, const TUniquePtr<U>& RHS) noexcept
{
    return LHS.Get() != RHS.Get();
}

template<typename ElementType>
NODISCARD FORCEINLINE bool operator==(const TUniquePtr<ElementType>& LHS, nullptr_type) noexcept
{
    return LHS.Get() == nullptr;
}

template<typename ElementType>
NODISCARD FORCEINLINE bool operator==(nullptr_type, const TUniquePtr<ElementType>& RHS) noexcept
{
    return nullptr == RHS.Get();
}

template<typename ElementType>
NODISCARD FORCEINLINE bool operator!=(const TUniquePtr<ElementType>& LHS, nullptr_type) noexcept
{
    return LHS.Get() != nullptr;
}

template<typename ElementType>
NODISCARD FORCEINLINE bool operator!=(nullptr_type, const TUniquePtr<ElementType>& RHS) noexcept
{
    return nullptr != RHS.Get();
}

template<typename ElementType, typename... ArgTypes>
NODISCARD FORCEINLINE TUniquePtr<ElementType> MakeUniquePtr(ArgTypes&&... Args) noexcept requires(TNot<TIsArray<ElementType>>::Value)
{
    ElementType* UniquePtr = new ElementType(Forward<ArgTypes>(Args)...);
    return TUniquePtr<ElementType>(UniquePtr);
}

template<typename ElementType>
NODISCARD FORCEINLINE TUniquePtr<ElementType> MakeUniquePtr(uint64 Size) noexcept requires(TIsArray<ElementType>::Value)
{
    typedef typename TRemoveExtent<ElementType>::Type Type;

    Type* UniquePtr = new Type[Size];
    return TUniquePtr<ElementType>(UniquePtr);
}

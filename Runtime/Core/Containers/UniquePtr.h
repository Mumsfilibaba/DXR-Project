#pragma once
#include "Delete.h"

#include "Core/Memory/New.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/RemoveExtent.h"
#include "Core/Templates/IsArray.h"
#include "Core/Templates/IsNullptr.h"
#include "Core/Templates/IsConvertible.h"
#include "Core/Templates/AddressOf.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* TUniquePtr - Scalar values. Similar to std::unique_ptr */

template<
    typename T,
    typename DeleterType = TDefaultDelete<T>>
class TUniquePtr 
    : private DeleterType // Using inheritance instead of composition to avoid extra memory usage
{
public:
    using ElementType = T;

    template<
        typename OtherType,
        typename OtherDeleterType>
    friend class TUniquePtr;

    TUniquePtr(const TUniquePtr& Other) = delete;
    TUniquePtr& operator=(const TUniquePtr& Other) noexcept = delete;

    /**
     * @brief: Default constructor
     */
    FORCEINLINE TUniquePtr() noexcept
        : DeleterType()
        , Ptr(nullptr)
    { }

    /**
     * @brief: Construct from nullptr
     */
    FORCEINLINE TUniquePtr(nullptr_type) noexcept
        : DeleterType()
        , Ptr(nullptr)
    { }

    /**
     * @brief: Constructor that takes a raw pointer
     * 
     * @param InPointer: Raw pointer to store
     */
    FORCEINLINE explicit TUniquePtr(ElementType* InPointer) noexcept
        : DeleterType()
        , Ptr(InPointer)
    { }

    /**
     * @brief: Move-constructor
     * 
     * @param Other: UniquePtr to move from
     */
    FORCEINLINE TUniquePtr(TUniquePtr&& Other) noexcept
        : DeleterType(Move(Other))
        , Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /**
     * @brief: Move-constructor that takes a convertible type
     *
     * @param Other: UniquePtr to move from
     */
    template<
        typename OtherType,
        typename OtherDeleterType,
        typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TUniquePtr(TUniquePtr<OtherType, OtherDeleterType>&& Other) noexcept
        : DeleterType(Move(Other))
        , Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /**
     * @brief: Destructor
     */
    FORCEINLINE ~TUniquePtr()
    {
        InternalRelease();
    }

    /**
     * @brief: Reset stored pointer to nullptr and return the old pointer
     * 
     * @return: Returns the pointer previously stored
     */
    FORCEINLINE ElementType* Release() noexcept
    {
        ElementType* OldPointer = Ptr;
        Ptr = nullptr;
        return OldPointer;
    }

    /**
     * @brief: Resets the container by setting the pointer to a new value and releases the old one 
     * 
     * @param NewPointer: New pointer to store
     */
    FORCEINLINE void Reset(ElementType* NewPointer = nullptr) noexcept
    {
        TUniquePtr(NewPointer).Swap(*this);
    }

    /**
     * @brief: Resets the container by setting the pointer to a new value of a convertible type and releases the old one
     *
     * @param NewPointer: New pointer to store
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type Reset(OtherType* NewPointer) noexcept
    {
        Reset(static_cast<ElementType*>(NewPointer));
    }

    /**
     * @brief: Swaps this UniquePtr with another
     * 
     * @param Other: Pointer to swap
     */ 
    FORCEINLINE void Swap(TUniquePtr& Other) noexcept
    {
        ElementType* Temp = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = Temp;
    }

    /**
     * @brief: Retrieve the stored pointer 
     * 
     * @return: Returns the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* Get() const noexcept
    {
        return Ptr;
    }

    /**
     * @brief: Retrieve the address of the stored pointer
     *
     * @return: Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    /**
     * @brief: Dereference the stored pointer
     * 
     * @return: A reference to the object pointed to by the stored pointer
     */
    NODISCARD FORCEINLINE ElementType& Dereference() const noexcept
    {
        Check(IsValid());
        return *Ptr;
    }

    /**
     * @brief: Checks if the stored pointer is valid
     * 
     * @return: Returns true if the stored pointer is not nullptr
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Ptr != nullptr);
    }

public:

    /**
     * @brief: Retrieve the stored pointer
     *
     * @return: Returns the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }

    /**
     * @brief: Dereference the stored pointer
     *
     * @return: A reference to the object pointed to by the stored pointer
     */
    NODISCARD FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    /**
     * @brief: Retrieve the address of the stored pointer
     *
     * @return: Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /**
     * @brief: Assignment operator that takes a raw pointer 
     * 
     * @param RHS: Pointer to store
     * @return: A reference to this instance
     */
    FORCEINLINE TUniquePtr& operator=(ElementType* RHS) noexcept
    {
        TUniquePtr(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator
     * 
     * @param RHS: UniquePtr to move from
     * @return: A reference to this instance
     */
    FORCEINLINE TUniquePtr& operator=(TUniquePtr&& RHS) noexcept
    {
        TUniquePtr(Move(RHS)).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator that takes a convertible type
     *
     * @param RHS: UniquePtr to move from
     * @return: A reference to this instance
     */
    template<
        typename OtherType,
        typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TUniquePtr&>::Type 
        operator=(TUniquePtr<OtherType, OtherDeleterType>&& RHS) noexcept
    {
        TUniquePtr(Move(RHS)).Swap(*this);
        return *this;
    }

    /**
     * @brief: Assignment operator that takes a nullptr
     */
    FORCEINLINE TUniquePtr& operator=(nullptr_type) noexcept
    {
        TUniquePtr().Swap(*this);
        return *this;
    }

    /**
     * @brief: Checks if the stored pointer is valid
     *
     * @return: Returns true if the stored pointer is not nullptr
     */
    NODISCARD FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

private:
    FORCEINLINE void InternalRelease() noexcept
    {
        if (Ptr)
        {
            DeleterType::DeleteElement(Ptr);
            Ptr = nullptr;
        }
    }

    ElementType* Ptr;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* TUniquePtr - Array values. Similar to std::unique_ptr */

template<
    typename T,
    typename DeleterType>
class TUniquePtr<T[], DeleterType> 
    : private DeleterType
{
public:
    using ElementType = T;
    using SizeType    = int32;

    template<
        typename OtherType,
        typename OtherDeleterType>
    friend class TUniquePtr;

    TUniquePtr(const TUniquePtr& Other) = delete;
    TUniquePtr& operator=(const TUniquePtr& Other) noexcept = delete;

    /**
     * @brief: Default constructor
     */
    FORCEINLINE TUniquePtr() noexcept
        : DeleterType()
        , Ptr(nullptr)
    { }

    /**
     * @brief: Construct from nullptr
     */
    FORCEINLINE TUniquePtr(nullptr_type) noexcept
        : DeleterType()
        , Ptr(nullptr)
    { }

    /**
     * @brief: Constructor that takes a raw pointer
     *
     * @param InPointer: Raw pointer to store
     */
    FORCEINLINE explicit TUniquePtr(ElementType* InPointer) noexcept
        : DeleterType()
        , Ptr(InPointer)
    { }

    /**
     * @brief: Move-constructor
     *
     * @param Other: UniquePtr to move from
     */
    FORCEINLINE TUniquePtr(TUniquePtr&& Other) noexcept
        : DeleterType(Move(Other))
        , Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /**
     * @brief: Move-constructor that takes a convertible type
     *
     * @param Other: UniquePtr to move from
     */
    template<
        typename OtherType,
        typename OtherDeleterType,
        typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TUniquePtr(TUniquePtr<OtherType[], OtherDeleterType>&& Other) noexcept
        : DeleterType(Move(Other))
        , Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /**
     * @brief: Destructor
     */
    FORCEINLINE ~TUniquePtr()
    {
        InternalRelease();
    }

    /**
     * @brief: Reset stored pointer to nullptr and return the old pointer
     *
     * @return: Returns the pointer previously stored
     */
    NODISCARD FORCEINLINE ElementType* Release() noexcept
    {
        ElementType* OldPointer = Ptr;
        Ptr = nullptr;
        return OldPointer;
    }

    /**
     * @brief: Resets the container by setting the pointer to a new value and releases the old one
     *
     * @param NewPointer: New pointer to store
     */
    FORCEINLINE void Reset(ElementType* NewPointer = nullptr) noexcept
    {
        TUniquePtr(NewPointer).Swap(*this);
    }

    /**
     * @brief: Resets the container by setting the pointer to a new value of a convertible type and releases the old one
     *
     * @param NewPointer: New pointer to store
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type Reset(OtherType* NewPointer) noexcept
    {
        Reset(static_cast<ElementType*>(NewPointer));
    }

    /**
     * @brief: Swaps this UniquePtr with another
     *
     * @param Other: Pointer to swap
     */
    FORCEINLINE void Swap(TUniquePtr& Other) noexcept
    {
        ElementType* Temp = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = Temp;
    }

    /**
     * @brief: Retrieve the stored pointer
     *
     * @return: Returns the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* Get() const noexcept
    {
        return Ptr;
    }

    /**
     * @brief: Retrieve the address of the stored pointer
     *
     * @return: Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    /**
     * @brief: Checks if the stored pointer is valid
     *
     * @return: Returns true if the stored pointer is not nullptr
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Ptr != nullptr);
    }

    /**
     * @brief: Retrieve a element at a certain index of the array
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& At(SizeType Index) const noexcept
    {
        Check(IsValid());
        return Get()[Index];
    }

public:

    /**
     * @brief: Bracket-operator to retrieve an element at a certain index
     *
     * @param Index: Index of the element to retrieve
     * @return: A reference to the element at the index
     */
    NODISCARD FORCEINLINE ElementType& operator[](SizeType Index) const noexcept
    {
        return At(Index);
    }

    /**
     * @brief: Retrieve the address of the stored pointer
     *
     * @return: Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /**
     * @brief: Assignment operator that takes a raw pointer
     *
     * @param RHS: Pointer to store
     * @return: A reference to this instance
     */
    FORCEINLINE TUniquePtr& operator=(ElementType* RHS) noexcept
    {
        TUniquePtr(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator
     *
     * @param RHS: UniquePtr to move from
     * @return: A reference to this instance
     */
    FORCEINLINE TUniquePtr& operator=(TUniquePtr&& RHS) noexcept
    {
        TUniquePtr(Move(RHS)).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator that takes a convertible type
     *
     * @param RHS: UniquePtr to move from
     * @return: A reference to this instance
     */
    template<
        typename OtherType,
        typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value,  TUniquePtr&>::Type 
        operator=(TUniquePtr<OtherType[], OtherDeleterType>&& RHS) noexcept
    {
        TUniquePtr(Move(RHS)).Swap(*this);
        return *this;
    }

    /**
     * @brief: Assignment operator that takes a nullptr
     *
     * @return: A reference to this instance
     */
    FORCEINLINE TUniquePtr& operator=(nullptr_type) noexcept
    {
        TUniquePtr().Swap(*this);
        return *this;
    }

    /**
     * @brief: Checks if the stored pointer is valid
     *
     * @return: Returns true if the stored pointer is not nullptr
     */
    NODISCARD FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

private:
    FORCEINLINE void InternalRelease() noexcept
    {
        if (Ptr)
        {
            DeleterType::DeleteElement(Ptr);
            Ptr = nullptr;
        }
    }

    ElementType* Ptr;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Operators

template<
    typename T,
    typename U>
NODISCARD FORCEINLINE bool operator==(const TUniquePtr<T>& LHS, U* RHS) noexcept
{
    return (LHS.Get() == RHS);
}

template<
    typename T,
    typename U>
NODISCARD FORCEINLINE bool operator==(T* LHS, const TUniquePtr<U>& RHS) noexcept
{
    return (LHS == RHS.Get());
}

template<
    typename T,
    typename U>
NODISCARD FORCEINLINE bool operator!=(const TUniquePtr<T>& LHS, U* RHS) noexcept
{
    return (LHS.Get() != RHS);
}

template<
    typename T,
    typename U>
NODISCARD FORCEINLINE bool operator!=(T* LHS, const TUniquePtr<U>& RHS) noexcept
{
    return (LHS != RHS.Get());
}

template<
    typename T,
    typename U>
NODISCARD FORCEINLINE bool operator==(const TUniquePtr<T>& LHS, const TUniquePtr<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

template<
    typename T,
    typename U>
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* MakeUnique - Creates a new object together with a UniquePtr */

template<
    typename T,
    typename... ArgTypes>
NODISCARD FORCEINLINE typename TEnableIf<!TIsArray<T>::Value, TUniquePtr<T>>::Type MakeUnique(ArgTypes&&... Args) noexcept
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

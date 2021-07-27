#pragma once
#include "Delete.h"

#include "Core/Templates/EnableIf.h"
#include "Core/Templates/RemoveExtent.h"
#include "Core/Templates/IsArray.h"
#include "Core/Templates/IsNullptr.h"
#include "Core/Templates/IsConvertible.h"
#include "Core/Templates/AddressOf.h"

/* TUniquePtr - Scalar values. Similar to std::unique_ptr */
template<typename T, typename DeleterType = TDefaultDelete<T>>
class TUniquePtr : private DeleterType // Using inheritance instead of composition to avoid extra memory usage
{
public:
    template<typename OtherType, typename OtherDeleterType>
    friend class TUniquePtr;

    typedef T ElementType;

    /* Cannot be copied */
    TUniquePtr( const TUniquePtr& Other ) = delete;
    TUniquePtr& operator=( const TUniquePtr& Other ) noexcept = delete;

    /* Default construct with nullptr */
    FORCEINLINE TUniquePtr() noexcept
        : DeleterType()
        , Ptr( nullptr )
    {
    }

    /* Set to nullptr */
    FORCEINLINE TUniquePtr( NullptrType ) noexcept
        : DeleterType()
        , Ptr( nullptr )
    {
    }

    /* Create from a raw pointer */
    FORCEINLINE explicit TUniquePtr( ElementType* InPtr ) noexcept
        : DeleterType()
        , Ptr( InPtr )
    {
    }

    /* Move from another */
    FORCEINLINE TUniquePtr( TUniquePtr&& Other ) noexcept
        : DeleterType( Move( Other ) )
        , Ptr( Other.Ptr )
    {
        Other.Ptr = nullptr;
    }

    /* Move from another with another type */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TUniquePtr( TUniquePtr<OtherType, OtherDeleterType>&& Other ) noexcept
        : DeleterType( Move( Other ) )
        , Ptr( Other.Ptr )
    {
        Other.Ptr = nullptr;
    }

    FORCEINLINE ~TUniquePtr()
    {
        InternalRelease();
    }

    /* Releases the ownership from the container and returns the pointer */
    FORCEINLINE ElementType* Release() noexcept
    {
        ElementType* OldPtr = Ptr;
        Ptr = nullptr;
        return OldPtr;
    }

    /* Resets the container by setting the pointer to a new value and releases the old one */
    FORCEINLINE void Reset( ElementType* NewPtr = nullptr ) noexcept
    {
        TUniquePtr( NewPtr ).Swap( *this );
    }

    /* Resets the container by setting the pointer to a new value and releases the old one */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type Reset( OtherType* NewPtr ) noexcept
    {
        Reset( static_cast<ElementType*>(NewPtr) )
    }

    /* Swaps two unique pointers */
    FORCEINLINE void Swap( TUniquePtr& Other ) noexcept
    {
        ElementType* Temp = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = Temp;
    }

    /* Return the raw pointer */
    FORCEINLINE ElementType* Get() const noexcept
    {
        return Ptr;
    }

    /* Return the address of the raw pointer */
    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    /* Return the dereferenced object */
    FORCEINLINE ElementType& Dereference() const noexcept
    {
        Assert( IsValid() );
        return *Ptr;
    }

    /* Ensures that the state of the container is valid */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Ptr != nullptr);
    }

    /* Return the raw pointer */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }

    /* Dereference the raw pointer */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    /* Return the address of the raw pointer */
    FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /* Assign from a raw pointer */
    FORCEINLINE TUniquePtr& operator=( ElementType* RHS ) noexcept
    {
        TUniquePtr( RHS ).Swap( *this );
        return *this;
    }

    /* Move-assign from another */
    FORCEINLINE TUniquePtr& operator=( TUniquePtr&& RHS ) noexcept
    {
        TUniquePtr( Move( RHS ) ).Swap( *this );
        return *this;
    }

    /* Move-assign from another, with another type */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TUniquePtr&>::Type operator=( TUniquePtr<OtherType, OtherDeleterType>&& RHS ) noexcept
    {
        TUniquePtr( Move( RHS ) ).Swap( *this );
        return *this;
    }

    /* Reset the container by assigning nullptr */
    FORCEINLINE TUniquePtr& operator=( NullptrType ) noexcept
    {
        TUniquePtr().Swap( *this );
        return *this;
    }

    /* Convert to bool */
    FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

private:
    FORCEINLINE void InternalRelease() noexcept
    {
        if ( Ptr )
        {
            DeleterType::DeleteElement( Ptr );
            Ptr = nullptr;
        }
    }

    ElementType* Ptr;
};

/* TUniquePtr - Array values. Similar to std::unique_ptr */
template<typename T, typename DeleterType>
class TUniquePtr<T[], DeleterType> : private DeleterType
{
public:
    template<typename OtherType, typename OtherDeleterType>
    friend class TUniquePtr;

    typedef T      ElementType;
    typedef uint32 SizeType;

    /* Cannot be copied */
    TUniquePtr( const TUniquePtr& Other ) = delete;
    TUniquePtr& operator=( const TUniquePtr& Other ) noexcept = delete;

    /* Default construct with nullptr */
    FORCEINLINE TUniquePtr() noexcept
        : DeleterType()
        , Ptr( nullptr )
    {
    }

    /* Set to nullptr */
    FORCEINLINE TUniquePtr( NullptrType ) noexcept
        : DeleterType()
        , Ptr( nullptr )
    {
    }

    /* Create from a raw pointer */
    FORCEINLINE explicit TUniquePtr( ElementType* InPtr ) noexcept
        : DeleterType()
        , Ptr( InPtr )
    {
    }

    /* Move from another */
    FORCEINLINE TUniquePtr( TUniquePtr&& Other ) noexcept
        : DeleterType( Move( Other ) )
        , Ptr( Other.Ptr )
    {
        Other.Ptr = nullptr;
    }

    /* Move from another with another type */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TUniquePtr( TUniquePtr<OtherType[], OtherDeleterType>&& Other ) noexcept
        : DeleterType( Move( Other ) )
        , Ptr( Other.Ptr )
    {
        Other.Ptr = nullptr;
    }

    FORCEINLINE ~TUniquePtr()
    {
        InternalRelease();
    }

    /* Releases the ownership from the container and returns the pointer */
    FORCEINLINE ElementType* Release() noexcept
    {
        ElementType* OldPtr = Ptr;
        Ptr = nullptr;
        return OldPtr;
    }

    /* Resets the container by setting the pointer to a new value and releases the old one */
    FORCEINLINE void Reset( ElementType* NewPtr = nullptr ) noexcept
    {
        TUniquePtr( NewPtr ).Swap( *this );
    }

    /* Resets the container by setting the pointer to a new value and releases the old one */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type Reset( OtherType* NewPtr ) noexcept
    {
        Reset( static_cast<ElementType*>(NewPtr) )
    }

    /* Swaps two unique pointers */
    FORCEINLINE void Swap( TUniquePtr& Other ) noexcept
    {
        ElementType* Temp = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = Temp;
    }

    /* Return the raw pointer */
    FORCEINLINE ElementType* Get() const noexcept
    {
        return Ptr;
    }

    /* Return the address of the raw pointer */
    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    /* Ensures that the state of the container is valid */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Ptr != nullptr);
    }

    FORCEINLINE ElementType& At( SizeType Index ) const noexcept
    {
        Assert( IsValid() );
        return Get()[Index];
    }

    /* Dereference the raw pointer */
    FORCEINLINE ElementType& operator[]( SizeType Index ) const noexcept
    {
        return At( Index );
    }

    /* Return the address of the raw pointer */
    FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /* Assign from a raw pointer */
    FORCEINLINE TUniquePtr& operator=( ElementType* RHS ) noexcept
    {
        TUniquePtr( RHS ).Swap( *this );
        return *this;
    }

    /* Move-assign from another */
    FORCEINLINE TUniquePtr& operator=( TUniquePtr&& RHS ) noexcept
    {
        TUniquePtr( Move( RHS ) ).Swap( *this );
    }

    /* Move-assign from another, with another type */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TUniquePtr&>::Type operator=( TUniquePtr<OtherType[], OtherDeleterType>&& RHS ) noexcept
    {
        TUniquePtr( Move( RHS ) ).Swap( *this );
        return *this;
    }

    /* Reset the container by assigning nullptr */
    FORCEINLINE TUniquePtr& operator=( NullptrType ) noexcept
    {
        TUniquePtr().Swap( *this );
        return *this;
    }

    /* Convert to bool */
    FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

private:
    FORCEINLINE void InternalRelease() noexcept
    {
        if ( Ptr )
        {
            DeleterType::DeleteElement( Ptr );
            Ptr = nullptr;
        }
    }

    ElementType* Ptr;
};

/* Check the equallity between uniqueptr and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==( const TUniquePtr<T>& LHS, U* RHS ) noexcept
{
    return (LHS.Get() == RHS);
}

/* Check the equallity between uniqueptr and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==( T* LHS, const TUniquePtr<U>& RHS ) noexcept
{
    return (LHS == RHS.Get());
}

/* Check the inequallity between uniqueptr and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TUniquePtr<T>& LHS, U* RHS ) noexcept
{
    return (LHS.Get() != RHS);
}

/* Check the inequallity between uniqueptr and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=( T* LHS, const TUniquePtr<U>& RHS ) noexcept
{
    return (LHS != RHS.Get());
}

/* Check the equallity between uniqueptrs */
template<typename T, typename U>
FORCEINLINE bool operator==( const TUniquePtr<T>& LHS, const TUniquePtr<U>& RHS ) noexcept
{
    return (LHS.Get() == RHS.Get());
}

/* Check the inequallity between uniqueptrs */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TUniquePtr<T>& LHS, const TUniquePtr<U>& RHS ) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/* Check the equallity between uniqueptr and nullptr */
template<typename T>
FORCEINLINE bool operator==( const TUniquePtr<T>& LHS, NullptrType ) noexcept
{
    return (LHS.Get() == nullptr);
}

/* Check the equallity between uniqueptr and nullptr */
template<typename T>
FORCEINLINE bool operator==( NullptrType, const TUniquePtr<T>& RHS ) noexcept
{
    return (nullptr == RHS.Get());
}

/* Check the inequallity between uniqueptr and nullptr */
template<typename T>
FORCEINLINE bool operator!=( const TUniquePtr<T>& LHS, NullptrType ) noexcept
{
    return (LHS.Get() != nullptr);
}

/* Check the inequallity between uniqueptr and nullptr */
template<typename T>
FORCEINLINE bool operator!=( NullptrType, const TUniquePtr<T>& RHS ) noexcept
{
    return (nullptr != RHS.Get());
}

/* MakeUnique - Creates a new object together with a UniquePtr */
template<typename T, typename... ArgTypes>
FORCEINLINE TEnableIf<!TIsArray<T>::Value, TUniquePtr<T>> MakeUnique( ArgTypes&&... Args ) noexcept
{
    T* UniquePtr = new T( Forward<ArgTypes>( Args )... );
    return TUniquePtr<T>( UniquePtr );
}

template<typename T>
FORCEINLINE TEnableIf<TIsArray<T>::Value, TUniquePtr<T>>MakeUnique( uint32 Size ) noexcept
{
    typedef typename TRemoveExtent<T>::Type Type;

    Type* UniquePtr = new Type[Size];
    return TUniquePtr<T>( UniquePtr );
}

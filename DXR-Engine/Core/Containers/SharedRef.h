#pragma once
#include "Core/RefCountedObject.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/IsConvertible.h"
#include "Core/Templates/IsNullptr.h"

/* TSharedRef - Helper class when using objects with RefCountedObject as a base */
template<typename T>
class TSharedRef
{
public:
    using ElementType = T;

    template<typename OtherType>
    friend class TSharedRef;

    /* Default constructor that set the ptr to nullptr */
    FORCEINLINE TSharedRef() noexcept
        : Ptr( nullptr )
    {
    }

    /* Copy-constructor that copy another ptr */
    FORCEINLINE TSharedRef( const TSharedRef& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        AddRef();
    }

    /* Copy-constructor that copy another ptr, by another convertible type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedRef( const TSharedRef<OtherType>& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        AddRef();
    }

    /* Move-constructor that move another ptr */
    FORCEINLINE TSharedRef( TSharedRef&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        Other.Ptr = nullptr;
    }

    /* Move-constructor that move another ptr, by another convertible type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedRef( TSharedRef<OtherType>&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        Other.Ptr = nullptr;
    }

    /* Create a sharedref from a raw pointer */
    FORCEINLINE TSharedRef( ElementType* InPtr ) noexcept
        : Ptr( InPtr )
    {
    }

    /* Create a sharedref from a raw pointer of a convertible type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedRef( OtherType* InPtr ) noexcept
        : Ptr( InPtr )
    {
    }

    FORCEINLINE ~TSharedRef()
    {
        Release();
    }

    /* Resets the container and sets to a potential new raw pointer */
    FORCEINLINE void Reset( ElementType* NewPtr = nullptr ) noexcept
    {
        TSharedRef( NewPtr ).Swap( *this );
    }

    /* Resets the container and sets to a potential new raw pointer of another type */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type Reset( OtherType* NewPtr ) noexcept
    {
        Reset( static_cast<ElementType*>(NewPtr) );
    }

    /* Swaps the pointers in the two containers */
    FORCEINLINE void Swap( TSharedRef& Other )
    {
        ElementType* Temp = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = Temp;
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
        Assert( IsValid() );
        Ptr->AddRef();
    }

    /* Retrive the rawpointer */
    FORCEINLINE ElementType* Get() const noexcept
    {
        return Ptr;
    }

    /* Retrive the current reference count of the object */
    FORCEINLINE ElementType* GetRefCount() const noexcept
    {
        Assert( IsValid() );
        return Ptr->GetRefCount();
    }

    /* Retrive the raw pointer and add a reference */
    FORCEINLINE ElementType* GetAndAddRef() noexcept
    {
        AddRef();
        return Ptr;
    }

    /* Retrive the pointer as another type that is convertible */
    template<typename CastType>
    FORCEINLINE typename TEnableIf<TIsConvertible<CastType*, ElementType*>::Value, CastType*>::Type GetAs() const noexcept
    {
        return static_cast<CastType*>(Ptr);
    }

    /* Get the address of the raw pointer */
    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    /* Return the dereferenced object */
    FORCEINLINE ElementType& Dereference() const noexcept
    {
        return *Ptr;
    }

    /* Checks weather the pointer is valid or not */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Ptr != nullptr);
    }

    /* Retrive the raw pointer */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }

    /* Retrive the address of the pointer */
    FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /* Dereference the pointer */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    /* Cheack weather the pointer is valid or not */
    FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /* Copy another sharedref into this */
    FORCEINLINE TSharedRef& operator=( const TSharedRef& RHS ) noexcept
    {
        TSharedRef( RHS ).Swap( *this );
        return *this;
    }

    /* Copy another sharedref into this of another type */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TSharedRef&>::Type operator=( const TSharedRef<OtherType>& RHS ) noexcept
    {
        TSharedRef( RHS ).Swap( *this );
        return *this;
    }

    /* Move another sharedref into this */
    FORCEINLINE TSharedRef& operator=( TSharedRef&& RHS ) noexcept
    {
        TSharedRef( RHS ).Swap( *this );
        return *this;
    }

    /* Move another sharedref into this of another type */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TSharedRef&>::Type operator=( TSharedRef<OtherType>&& RHS ) noexcept
    {
        TSharedRef( RHS ).Swap( *this );
        return *this;
    }

    /* Assign from a raw pointer */
    FORCEINLINE TSharedRef& operator=( ElementType* RHS ) noexcept
    {
        TSharedRef( RHS ).Swap( *this );
        return *this;
    }

    /* Assign from a pointer of another type */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, TSharedRef&>::Type operator=( OtherType* RHS ) noexcept
    {
        TSharedRef( RHS ).Swap( *this );
        return *this;
    }

    /* Set the pointer to nullptr */
    FORCEINLINE TSharedRef& operator=( NullptrType ) noexcept
    {
        TSharedRef().Swap( *this );
        return *this;
    }

private:
    FORCEINLINE void Release() noexcept
    {
        if ( Ptr )
        {
            Ptr->Release();
            Ptr = nullptr;
        }
    }

    ElementType* Ptr;
};

/* Check the equallity between sharedref and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==( const TSharedRef<T>& LHS, U* RHS ) noexcept
{
    return (LHS.Get() == RHS);
}

/* Check the equallity between sharedref and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==( T* LHS, const TSharedRef<U>& RHS ) noexcept
{
    return (LHS == RHS.Get());
}

/* Check the inequallity between sharedref and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TSharedRef<T>& LHS, U* RHS ) noexcept
{
    return (LHS.Get() != RHS);
}

/* Check the inequallity between sharedref and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=( T* LHS, const TSharedRef<U>& RHS ) noexcept
{
    return (LHS != RHS.Get());
}

/* Check the equallity between sharedrefs */
template<typename T, typename U>
FORCEINLINE bool operator==( const TSharedRef<T>& LHS, const TSharedRef<U>& RHS ) noexcept
{
    return (LHS.Get() == RHS.Get());
}

/* Check the equallity between sharedrefs */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TSharedRef<T>& LHS, const TSharedRef<U>& RHS ) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/* Check the equallity between sharedref and nullptr */
template<typename T>
FORCEINLINE bool operator==( const TSharedRef<T>& LHS, NullptrType ) noexcept
{
    return (LHS.Get() == nullptr);
}

/* Check the equallity between sharedref and nullptr */
template<typename T>
FORCEINLINE bool operator==( NullptrType, const TSharedRef<T>& RHS ) noexcept
{
    return (nullptr == RHS.Get());
}

/* Check the inequallity between sharedref and nullptr */
template<typename T>
FORCEINLINE bool operator!=( const TSharedRef<T>& LHS, NullptrType ) noexcept
{
    return (LHS.Get() != nullptr);
}

/* Check the inequallity between sharedref and nullptr */
template<typename T>
FORCEINLINE bool operator!=( NullptrType, const TSharedRef<T>& RHS ) noexcept
{
    return (nullptr != RHS.Get());
}

/* static_cast */
template<typename T, typename U>
FORCEINLINE TSharedRef<T> StaticCast( const TSharedRef<U>& Pointer )
{
    T* RawPointer = static_cast<T*>(Pointer.Get());
    RawPointer->AddRef();
    return TSharedRef<T>( RawPointer );
}

template<typename T, typename U>
FORCEINLINE TSharedRef<T> StaticCast( TSharedRef<U>&& Pointer )
{
    T* RawPointer = static_cast<T*>(Pointer.Get());
    return TSharedRef<T>( RawPointer );
}

/* const_cast */
template<typename T, typename U>
FORCEINLINE TSharedRef<T> ConstCast( const TSharedRef<U>& Pointer )
{
    T* RawPointer = const_cast<T*>(Pointer.Get());
    RawPointer->AddRef();
    return TSharedRef<T>( RawPointer );
}

template<typename T, typename U>
FORCEINLINE TSharedRef<T> ConstCast( TSharedRef<U>&& Pointer )
{
    T* RawPointer = const_cast<T*>(Pointer.Get());
    return TSharedRef<T>( RawPointer );
}

/* reinterpret_cast */
template<typename T, typename U>
FORCEINLINE TSharedRef<T> ReinterpretCast( const TSharedRef<U>& Pointer )
{
    T* RawPointer = reinterpret_cast<T*>(Pointer.Get());
    RawPointer->AddRef();
    return TSharedRef<T>( RawPointer );
}

template<typename T, typename U>
FORCEINLINE TSharedRef<T> ReinterpretCast( TSharedRef<U>&& Pointer )
{
    T* RawPointer = reinterpret_cast<T*>(Pointer.Get());
    return TSharedRef<T>( RawPointer );
}

/* dynamic_cast */
template<typename T, typename U>
FORCEINLINE TSharedRef<T> DynamicCast( const TSharedRef<U>& Pointer )
{
    T* RawPointer = dynamic_cast<T*>(Pointer.Get());
    RawPointer->AddRef();
    return TSharedRef<T>( RawPointer );
}

template<typename T, typename U>
FORCEINLINE TSharedRef<T> DynamicCast( TSharedRef<U>&& Pointer )
{
    T* RawPointer = dynamic_cast<T*>(Pointer.Get());
    return TSharedRef<T>( RawPointer );
}

/* Converts a raw pointer into a TSharedRef */
template<typename T, typename U>
FORCEINLINE TSharedRef<T> MakeSharedRef( U* InRefCountedObject )
{
    if ( InRefCountedObject )
    {
        InRefCountedObject->AddRef();
        return TSharedRef<T>( static_cast<T*>(InRefCountedObject) );
    }

    return TSharedRef<T>();
}
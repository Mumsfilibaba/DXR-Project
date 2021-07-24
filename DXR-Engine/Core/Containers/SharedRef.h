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
    template<typename OtherType>
    friend class TSharedRef;

    typedef T ElementType;

    /* Default constructor that set the ptr to nullptr */
    FORCEINLINE TSharedRef() noexcept
        : RefPtr( nullptr )
    {
    }

    /* Copy-constructor that copy another ptr */
    FORCEINLINE TSharedRef( const TSharedRef& Other ) noexcept
        : RefPtr( Other.RefPtr )
    {
        AddRef();
    }

    /* Copy-constructor that copy another ptr, by another convertible type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedRef( const TSharedRef<OtherType>& Other ) noexcept
        : RefPtr( Other.RefPtr )
    {
        AddRef();
    }

    /* Move-constructor that move another ptr */
    FORCEINLINE TSharedRef( TSharedRef&& Other ) noexcept
        : RefPtr( Other.RefPtr )
    {
        Other.RefPtr = nullptr;
    }

    /* Move-constructor that move another ptr, by another convertible type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedRef( TSharedRef<OtherType>&& Other ) noexcept
        : RefPtr( Other.RefPtr )
    {
        Other.RefPtr = nullptr;
    }

    /* Create a sharedref from a raw pointer */
    FORCEINLINE TSharedRef( ElementType* InPtr ) noexcept
        : RefPtr( InPtr )
    {
    }

    /* Create a sharedref from a raw pointer of a convertible type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedRef( OtherType* InPtr ) noexcept
        : RefPtr( InPtr )
    {
    }

    FORCEINLINE ~TSharedRef()
    {
        Release();
    }

    /* Resets the container and sets to a potential new raw pointer */ 
    FORCEINLINE ElementType* Reset( ElementType* NewPtr = nullptr ) noexcept
    {
        ElementType* OldPtr = RefPtr;
        if ( RefPtr != NewPtr )
        {
            Release();
            RefPtr = NewPtr;
        }

        return OldPtr;
    }

    /* Resets the container and sets to a potential new raw pointer of another type */ 
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE ElementType* Reset( OtherType* NewPtr ) noexcept
    {
        return Reset(static_cast<ElementType*>(NewPtr));
    }

    /* Releases the ownership of the pointer and returns the pointer */
    FORCEINLINE ElementType* ReleaseOwnership() noexcept
    {
        ElementType* OldPtr = RefPtr;
        RefPtr = nullptr;
        return OldPtr;
    }

    /* Adds a reference to the stored pointer */
    FORCEINLINE void AddRef() noexcept
    {
        Assert(IsValid());
        RefPtr->AddRef();
    }

    /* Retrive the rawpointer */
    FORCEINLINE ElementType* Get() const noexcept
    {
        return RefPtr;
    }

    /* Retrive the current reference count of the object */
    FORCEINLINE ElementType* GetRefCount() const noexcept
    {
        Assert(IsValid());
        return RefPtr->GetRefCount();
    }

    /* Retrive the raw pointer and add a reference */
    FORCEINLINE ElementType* GetAndAddRef() noexcept
    {
        AddRef();
        return RefPtr;
    }

    /* Retrive the pointer as another type that is convertible */
    template<typename CastType>
    FORCEINLINE TEnableIf<TIsConvertible<CastType*, ElementType*>::Value, CastType*> GetAs() const noexcept
    {
        return static_cast<CastType*>(RefPtr);
    }

    /* Get the address of the raw pointer */
    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &RefPtr;
    }

    /* Return the dereferenced object */
    FORCEINLINE ElementType& Dereference() const noexcept
    {
        return *RefPtr;
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

    /* Check the equallity between the pointer and a raw pointer */
    FORCEINLINE bool operator==( ElementType* RHS ) const noexcept
    {
        return (RefPtr == RHS);
    }

    /* Check the equallity between the pointer and another sharedref */
    FORCEINLINE bool operator==( const TSharedRef& RHS ) const noexcept
    {
        return (RefPtr == RHS.RefPtr);
    }

    /* Check the equallity between the pointer and a raw pointer */
    FORCEINLINE bool operator!=( ElementType* RHS ) const noexcept
    {
        return (RefPtr != RHS);
    }

    /* Check the equallity between the pointer and another sharedref */
    FORCEINLINE bool operator!=( const TSharedRef& RHS ) const noexcept
    {
        return (RefPtr != RHS.RefPtr);
    }

    /* Check if the pointer is nullptr */
    FORCEINLINE bool operator==( NullptrType ) const noexcept
    {
        return (RefPtr == nullptr);
    }

    /* Check if the pointer is nullptr */
    FORCEINLINE bool operator!=( NullptrType ) const noexcept
    {
        return (RefPtr != nullptr);
    }

    template<typename OtherType>
    FORCEINLINE TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, bool> operator==( OtherType* RHS ) const noexcept
    {
        return (RefPtr == RHS);
    }

    template<typename OtherType>
    friend FORCEINLINE TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, bool> operator==( OtherType* LHS, const TSharedRef& RHS ) noexcept
    {
        return (RHS == LHS);
    }

    template<typename OtherType>
    FORCEINLINE TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, bool> operator==( const TSharedRef<OtherType>& RHS ) const noexcept
    {
        return (RefPtr == RHS.RefPtr);
    }

    template<typename OtherType>
    FORCEINLINE TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, bool> operator!=( OtherType* RHS ) const noexcept
    {
        return (RefPtr != RHS);
    }

    template<typename OtherType>
    friend FORCEINLINE TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, bool> operator!=( OtherType* LHS, const TSharedRef& RHS ) noexcept
    {
        return (RHS != LHS);
    }

    template<typename OtherType>
    FORCEINLINE TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, bool> operator!=( const TSharedRef<OtherType>& RHS ) const noexcept
    {
        return (RefPtr != RHS.RefPtr);
    }

    FORCEINLINE operator bool() const noexcept
    {
        return (RefPtr != nullptr);
    }

    FORCEINLINE TSharedRef& operator=( const TSharedRef& Other ) noexcept
    {
        if ( this != std::addressof( Other ) )
        {
            Release();

            RefPtr = Other.RefPtr;
            AddRef();
        }

        return *this;
    }

    template<typename OtherType>
    FORCEINLINE TSharedRef& operator=( const TSharedRef<OtherType>& Other ) noexcept
    {
        static_assert(TIsConvertible<OtherType*, ElementType*>::Value);

        if ( this != std::addressof( Other ) )
        {
            Release();

            RefPtr = Other.RefPtr;
            AddRef();
        }

        return *this;
    }

    FORCEINLINE TSharedRef& operator=( TSharedRef&& Other ) noexcept
    {
        if ( this != std::addressof( Other ) )
        {
            Release();

            RefPtr = Other.RefPtr;
            Other.RefPtr = nullptr;
        }

        return *this;
    }

    template<typename OtherType>
    FORCEINLINE TSharedRef& operator=( TSharedRef<OtherType>&& Other ) noexcept
    {
        static_assert(TIsConvertible<OtherType*, ElementType*>::Value);

        if ( this != std::addressof( Other ) )
        {
            Release();

            RefPtr = Other.RefPtr;
            Other.RefPtr = nullptr;
        }

        return *this;
    }

    FORCEINLINE TSharedRef& operator=( ElementType* InPtr ) noexcept
    {
        if ( RefPtr != InPtr )
        {
            Release();
            RefPtr = InPtr;
        }

        return *this;
    }

    template<typename OtherType>
    FORCEINLINE TSharedRef& operator=( OtherType* InPtr ) noexcept
    {
        static_assert(TIsConvertible<OtherType*, ElementType*>::Value);

        if ( RefPtr != InPtr )
        {
            Release();
            RefPtr = InPtr;
        }

        return *this;
    }

    /* Set the pointer to nullptr */
    FORCEINLINE TSharedRef& operator=( NullptrType ) noexcept
    {
        Release();
        return *this;
    }

private:
    FORCEINLINE void Release() noexcept
    {
        if ( RefPtr )
        {
            RefPtr->Release();
            RefPtr = nullptr;
        }
    }

    ElementType* RefPtr;
};

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
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

    FORCEINLINE TSharedRef() noexcept
        : RefPtr( nullptr )
    {
    }

    FORCEINLINE TSharedRef( const TSharedRef& Other ) noexcept
        : RefPtr( Other.RefPtr )
    {
        AddRef();
    }

    template<typename OtherType>
    FORCEINLINE TSharedRef( const TSharedRef<OtherType>& Other ) noexcept
        : RefPtr( Other.RefPtr )
    {
        static_assert(TIsConvertible<OtherType*, ElementType*>::Value);
        AddRef();
    }

    FORCEINLINE TSharedRef( TSharedRef&& Other ) noexcept
        : RefPtr( Other.RefPtr )
    {
        Other.RefPtr = nullptr;
    }

    template<typename OtherType>
    FORCEINLINE TSharedRef( TSharedRef<OtherType>&& Other ) noexcept
        : RefPtr( Other.RefPtr )
    {
        static_assert(TIsConvertible<OtherType*, ElementType*>::Value);
        Other.RefPtr = nullptr;
    }

    FORCEINLINE TSharedRef( ElementType* InPtr ) noexcept
        : RefPtr( InPtr )
    {
    }

    template<typename OtherType>
    FORCEINLINE TSharedRef( OtherType* InPtr ) noexcept
        : RefPtr( InPtr )
    {
        static_assert(TIsConvertible<OtherType*, ElementType*>::Value);
    }

    FORCEINLINE ~TSharedRef()
    {
        Release();
    }

    FORCEINLINE ElementType* Reset( ElementType* NewPtr = nullptr ) noexcept
    {
        ElementType* WeakPtr = RefPtr;

        if ( RefPtr != NewPtr )
        {
            Release();
            RefPtr = NewPtr;
        }

        return WeakPtr;
    }

    FORCEINLINE ElementType* ReleaseOwnership() noexcept
    {
        ElementType* WeakPtr = RefPtr;
        RefPtr = nullptr;
        return WeakPtr;
    }

    FORCEINLINE void AddRef() noexcept
    {
        if ( RefPtr )
        {
            RefPtr->AddRef();
        }
    }

    FORCEINLINE void Swap( ElementType* InPtr ) noexcept
    {
        Reset( InPtr )
    }

    template<typename OtherType>
    FORCEINLINE void Swap( OtherType* InPtr ) noexcept
    {
        static_assert(TIsConvertible<OtherType*, ElementType*>::Value);
        Reset( InPtr )
    }

    FORCEINLINE ElementType* Get() const noexcept
    {
        return RefPtr;
    }

    FORCEINLINE ElementType* GetRefCount() const noexcept
    {
        return RefPtr->GetRefCount();
    }

    FORCEINLINE ElementType* GetAndAddRef() noexcept
    {
        AddRef();
        return RefPtr;
    }

    template<typename CastType>
    FORCEINLINE TEnableIf<TIsConvertible<CastType*, ElementType*>::Value, CastType*> GetAs() const noexcept
    {
        return static_cast<CastType*>(RefPtr);
    }

    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &RefPtr;
    }

    FORCEINLINE ElementType& Dereference() const noexcept
    {
        return *RefPtr;
    }

    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }

    FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    FORCEINLINE bool operator==( ElementType* InPtr ) const noexcept
    {
        return (RefPtr == InPtr);
    }

    FORCEINLINE bool operator==( const TSharedRef& Other ) const noexcept
    {
        return (RefPtr == Other.RefPtr);
    }

    FORCEINLINE bool operator!=( ElementType* InPtr ) const noexcept
    {
        return (RefPtr != InPtr);
    }

    FORCEINLINE bool operator!=( const TSharedRef& Other ) const noexcept
    {
        return (RefPtr != Other.RefPtr);
    }

    FORCEINLINE bool operator==( NullptrType ) const noexcept
    {
        return (RefPtr == nullptr);
    }

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
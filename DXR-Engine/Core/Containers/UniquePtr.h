#pragma once
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/IsArray.h"
#include "Core/Templates/RemoveExtent.h"
#include "Core/Templates/IsNullptr.h"
#include "Core/Templates/IsConvertible.h"

/* TUniquePtr - Scalar values. Similar to std::unique_ptr */
template<typename T>
class TUniquePtr
{
public:
    template<typename OtherType>
    friend class TUniquePtr;

    typedef T ElementType;

    TUniquePtr( const TUniquePtr& Other ) = delete;
    TUniquePtr& operator=( const TUniquePtr& Other ) noexcept = delete;

    FORCEINLINE TUniquePtr() noexcept
        : Ptr( nullptr )
    {
    }

    FORCEINLINE TUniquePtr( NullptrType ) noexcept
        : Ptr( nullptr )
    {
    }

    FORCEINLINE explicit TUniquePtr( ElementType* InPtr ) noexcept
        : Ptr( InPtr )
    {
    }

    FORCEINLINE TUniquePtr( TUniquePtr&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        Other.Ptr = nullptr;
    }

    template<typename OtherType>
    FORCEINLINE TUniquePtr( TUniquePtr<OtherType>&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        static_assert(TIsConvertible<OtherType*, ElementType*>::Value);
        Other.Ptr = nullptr;
    }

    FORCEINLINE ~TUniquePtr()
    {
        Reset();
    }

    FORCEINLINE ElementType* Release() noexcept
    {
        ElementType* WeakPtr = Ptr;
        Ptr = nullptr;
        return WeakPtr;
    }

    FORCEINLINE void Reset( ElementType* NewPtr = nullptr ) noexcept
    {
        if ( Ptr != NewPtr )
        {
            InternalRelease();
            Ptr = NewPtr;
        }
    }

    FORCEINLINE void Swap( TUniquePtr& Other ) noexcept
    {
        ElementType* TempPtr = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = TempPtr;
    }

    FORCEINLINE ElementType* Get() const noexcept
    {
        return Ptr;
    }

    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }

    FORCEINLINE ElementType& operator*() const noexcept
    {
        return (*Ptr);
    }

    FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    FORCEINLINE TUniquePtr& operator=( ElementType* NewPtr ) noexcept
    {
        Reset( NewPtr );
        return *this;
    }

    FORCEINLINE TUniquePtr& operator=( TUniquePtr&& Other ) noexcept
    {
        if ( this != std::addressof( Other ) )
        {
            Reset( Other.Ptr );
            Other.Ptr = nullptr;
        }

        return *this;
    }

    template<typename OtherType>
    FORCEINLINE TUniquePtr& operator=( TUniquePtr<OtherType>&& Other ) noexcept
    {
        static_assert(TIsConvertible<OtherType*, ElementType*>::Value);

        if ( this != std::addressof( Other ) )
        {
            Reset( Other.Ptr );
            Other.Ptr = nullptr;
        }

        return *this;
    }

    FORCEINLINE TUniquePtr& operator=( NullptrType ) noexcept
    {
        Reset();
        return *this;
    }

    FORCEINLINE bool operator==( const TUniquePtr& Other ) const noexcept
    {
        return (Ptr == Other.Ptr);
    }

    FORCEINLINE bool operator!=( const TUniquePtr& Other ) const noexcept
    {
        return !(*this == Other);
    }

    FORCEINLINE bool operator==( ElementType* InPtr ) const noexcept
    {
        return (Ptr == InPtr);
    }

    FORCEINLINE bool operator!=( ElementType* InPtr ) const noexcept
    {
        return !(*this == Other);
    }

    FORCEINLINE operator bool() const noexcept
    {
        return (Ptr != nullptr);
    }

private:
    FORCEINLINE void InternalRelease() noexcept
    {
        if ( Ptr )
        {
            delete Ptr;
            Ptr = nullptr;
        }
    }

    ElementType* Ptr;
};

/* TUniquePtr - Array values. Similar to std::unique_ptr */
template<typename T>
class TUniquePtr<T[]>
{
public:
    template<typename OtherType>
    friend class TUniquePtr;

    typedef T ElementType;

    TUniquePtr( const TUniquePtr& Other ) = delete;
    TUniquePtr& operator=( const TUniquePtr& Other ) noexcept = delete;

    FORCEINLINE TUniquePtr() noexcept
        : Ptr( nullptr )
    {
    }

    FORCEINLINE TUniquePtr( NullptrType ) noexcept
        : Ptr( nullptr )
    {
    }

    FORCEINLINE explicit TUniquePtr( ElementType* InPtr ) noexcept
        : Ptr( InPtr )
    {
    }

    FORCEINLINE TUniquePtr( TUniquePtr&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        Other.Ptr = nullptr;
    }

    template<typename OtherType>
    FORCEINLINE TUniquePtr( TUniquePtr<OtherType>&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        static_assert(TIsConvertible<OtherType*, ElementType*>::Value);
        Other.Ptr = nullptr;
    }

    FORCEINLINE ~TUniquePtr()
    {
        Reset();
    }

    FORCEINLINE ElementType* Release() noexcept
    {
        ElementType* WeakPtr = Ptr;
        Ptr = nullptr;
        return WeakPtr;
    }

    FORCEINLINE void Reset( ElementType* NewPtr = nullptr ) noexcept
    {
        if ( Ptr != NewPtr )
        {
            InternalRelease();
            Ptr = NewPtr;
        }
    }

    FORCEINLINE void Swap( TUniquePtr& Other ) noexcept
    {
        ElementType* TempPtr = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = TempPtr;
    }

    FORCEINLINE ElementType* Get() const noexcept
    {
        return Ptr;
    }

    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    FORCEINLINE ElementType& operator[]( uint32 Index ) noexcept
    {
        Assert( Ptr != nullptr );
        return Ptr[Index];
    }

    FORCEINLINE TUniquePtr& operator=( ElementType* InPtr ) noexcept
    {
        Reset( InPtr );
        return *this;
    }

    FORCEINLINE TUniquePtr& operator=( TUniquePtr&& Other ) noexcept
    {
        if ( this != std::addressof( Other ) )
        {
            Reset( Other.Ptr );
            Other.Ptr = nullptr;
        }

        return *this;
    }

    template<typename OtherType>
    FORCEINLINE TUniquePtr& operator=( TUniquePtr<OtherType>&& Other ) noexcept
    {
        static_assert(TIsConvertible<OtherType*, ElementType*>::Value);

        if ( this != std::addressof( Other ) )
        {
            Reset( Other.Ptr );
            Other.Ptr = nullptr;
        }

        return *this;
    }

    FORCEINLINE TUniquePtr& operator=( NullptrType ) noexcept
    {
        Reset();
        return *this;
    }

    FORCEINLINE bool operator==( const TUniquePtr& Other ) const noexcept
    {
        return (Ptr == Other.Ptr);
    }

    FORCEINLINE bool operator!=( const TUniquePtr& Other ) const noexcept
    {
        return !(*this == Other);
    }

    FORCEINLINE bool operator==( ElementType* InPtr ) const noexcept
    {
        return (Ptr == InPtr);
    }

    FORCEINLINE bool operator!=( ElementType* InPtr ) const noexcept
    {
        return !(*this == Other);
    }

    FORCEINLINE operator bool() const noexcept
    {
        return (Ptr != nullptr);
    }

private:
    FORCEINLINE void InternalRelease() noexcept
    {
        if ( Ptr )
        {
            delete Ptr;
            Ptr = nullptr;
        }
    }

    ElementType* Ptr;
};

/* MakeUnique - Creates a new object together with a UniquePtr */
template<typename T, typename... ArgTypes>
FORCEINLINE TEnableIf<!TIsArray<T>::Value, TUniquePtr<T>> MakeUnique( ArgTypes&&... Args ) noexcept
{
    T* UniquePtr = new T( ::Forward<ArgTypes>( Args )... );
    return ::Move( TUniquePtr<T>( UniquePtr ) );
}

template<typename T>
FORCEINLINE TEnableIf<TIsArray<T>::Value, TUniquePtr<T>>MakeUnique( uint32 Size ) noexcept
{
    typedef typename TRemoveExtent<T>::Type Type;

    Type* UniquePtr = new Type[Size];
    return ::Move( TUniquePtr<T>( UniquePtr ) );
}

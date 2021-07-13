#pragma once
#include "Utilities.h"

// TUniquePtr - Scalar values. Similar to std::unique_ptr

template<typename T>
class TUniquePtr
{
public:
    template<typename TOther>
    friend class TUniquePtr;

    TUniquePtr( const TUniquePtr& Other ) = delete;
    TUniquePtr& operator=( const TUniquePtr& Other ) noexcept = delete;

    FORCEINLINE TUniquePtr() noexcept
        : Ptr( nullptr )
    {
    }

    FORCEINLINE TUniquePtr( std::nullptr_t ) noexcept
        : Ptr( nullptr )
    {
    }

    FORCEINLINE explicit TUniquePtr( T* InPtr ) noexcept
        : Ptr( InPtr )
    {
    }

    FORCEINLINE TUniquePtr( TUniquePtr&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        Other.Ptr = nullptr;
    }

    template<typename TOther>
    FORCEINLINE TUniquePtr( TUniquePtr<TOther>&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Other.Ptr = nullptr;
    }

    FORCEINLINE ~TUniquePtr()
    {
        Reset();
    }

    FORCEINLINE T* Release() noexcept
    {
        T* WeakPtr = Ptr;
        Ptr = nullptr;
        return WeakPtr;
    }

    FORCEINLINE void Reset( T* NewPtr = nullptr ) noexcept
    {
        if ( Ptr != NewPtr )
        {
            InternalRelease();
            Ptr = NewPtr;
        }
    }

    FORCEINLINE void Swap( TUniquePtr& Other ) noexcept
    {
        T* TempPtr = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = TempPtr;
    }

    FORCEINLINE T* Get() const noexcept
    {
        return Ptr;
    }
    
    FORCEINLINE T* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    FORCEINLINE T* operator->() const noexcept
    {
        return Get();
    }
    
    FORCEINLINE T& operator*() const noexcept
    {
        return (*Ptr);
    }

    FORCEINLINE T* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    FORCEINLINE TUniquePtr& operator=( T* NewPtr ) noexcept
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

    template<typename TOther>
    FORCEINLINE TUniquePtr& operator=( TUniquePtr<TOther>&& Other ) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        if ( this != std::addressof( Other ) )
        {
            Reset( Other.Ptr );
            Other.Ptr = nullptr;
        }

        return *this;
    }

    FORCEINLINE TUniquePtr& operator=( std::nullptr_t ) noexcept
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

    FORCEINLINE bool operator==( T* InPtr ) const noexcept
    {
        return (Ptr == InPtr);
    }

    FORCEINLINE bool operator!=( T* InPtr ) const noexcept
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

    T* Ptr;
};

// TUniquePtr - Array values. Similar to std::unique_ptr

template<typename T>
class TUniquePtr<T[]>
{
public:
    template<typename TOther>
    friend class TUniquePtr;

    TUniquePtr( const TUniquePtr& Other ) = delete;
    TUniquePtr& operator=( const TUniquePtr& Other ) noexcept = delete;

    FORCEINLINE TUniquePtr() noexcept
        : Ptr( nullptr )
    {
    }

    FORCEINLINE TUniquePtr( std::nullptr_t ) noexcept
        : Ptr( nullptr )
    {
    }

    FORCEINLINE explicit TUniquePtr( T* InPtr ) noexcept
        : Ptr( InPtr )
    {
    }

    FORCEINLINE TUniquePtr( TUniquePtr&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        Other.Ptr = nullptr;
    }

    template<typename TOther>
    FORCEINLINE TUniquePtr( TUniquePtr<TOther>&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Other.Ptr = nullptr;
    }

    FORCEINLINE ~TUniquePtr()
    {
        Reset();
    }

    FORCEINLINE T* Release() noexcept
    {
        T* WeakPtr = Ptr;
        Ptr = nullptr;
        return WeakPtr;
    }

    FORCEINLINE void Reset( T* NewPtr = nullptr) noexcept
    {
        if ( Ptr != NewPtr )
        {
            InternalRelease();
            Ptr = NewPtr;
        }
    }

    FORCEINLINE void Swap( TUniquePtr& Other ) noexcept
    {
        T* TempPtr = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = TempPtr;
    }

    FORCEINLINE T* Get() const noexcept
    {
        return Ptr;
    }

    FORCEINLINE T* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    FORCEINLINE T* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    FORCEINLINE T& operator[]( uint32 Index ) noexcept
    {
        Assert( Ptr != nullptr );
        return Ptr[Index];
    }

    FORCEINLINE TUniquePtr& operator=( T* InPtr ) noexcept
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

    template<typename TOther>
    FORCEINLINE  TUniquePtr& operator=( TUniquePtr<TOther>&& Other ) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        if ( this != std::addressof( Other ) )
        {
            Reset( Other.Ptr );
            Other.Ptr = nullptr;
        }

        return *this;
    }

    FORCEINLINE TUniquePtr& operator=( std::nullptr_t ) noexcept
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

    FORCEINLINE bool operator==( T* InPtr ) const noexcept
    {
        return (Ptr == InPtr);
    }
    
    FORCEINLINE bool operator!=( T* InPtr ) const noexcept
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

    T* Ptr;
};

// MakeUnique - Creates a new object together with a UniquePtr

template<typename T, typename... TArgs>
FORCEINLINE TEnableIf<!TIsArray<T>, TUniquePtr<T>> MakeUnique( TArgs&&... Args ) noexcept
{
    T* UniquePtr = new T( Forward<TArgs>( Args )... );
    return Move( TUniquePtr<T>( UniquePtr ) );
}

template<typename T>
FORCEINLINE TEnableIf<TIsArray<T>, TUniquePtr<T>>MakeUnique( uint32 Size ) noexcept
{
    using TType = TRemoveExtent<T>;

    TType* UniquePtr = new TType[Size];
    return Move( TUniquePtr<T>( UniquePtr ) );
}

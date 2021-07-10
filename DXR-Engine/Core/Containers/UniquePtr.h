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

    TUniquePtr() noexcept
        : Ptr( nullptr )
    {
    }

    TUniquePtr( std::nullptr_t ) noexcept
        : Ptr( nullptr )
    {
    }

    explicit TUniquePtr( T* InPtr ) noexcept
        : Ptr( InPtr )
    {
    }

    TUniquePtr( TUniquePtr&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        Other.Ptr = nullptr;
    }

    template<typename TOther>
    TUniquePtr( TUniquePtr<TOther>&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Other.Ptr = nullptr;
    }

    ~TUniquePtr()
    {
        Reset();
    }

    T* Release() noexcept
    {
        T* WeakPtr = Ptr;
        Ptr = nullptr;
        return WeakPtr;
    }

    void Reset() noexcept
    {
        InternalRelease();
        Ptr = nullptr;
    }

    void Swap( TUniquePtr& Other ) noexcept
    {
        T* TempPtr = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = TempPtr;
    }

    T* Get() const noexcept
    {
        return Ptr;
    }
    T* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    T* operator->() const noexcept
    {
        return Get();
    }
    T& operator*() const noexcept
    {
        return (*Ptr);
    }
    T* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    TUniquePtr& operator=( T* InPtr ) noexcept
    {
        if ( Ptr != InPtr )
        {
            Reset();
            Ptr = InPtr;
        }

        return *this;
    }

    TUniquePtr& operator=( TUniquePtr&& Other ) noexcept
    {
        if ( this != std::addressof( Other ) )
        {
            Reset();
            Ptr = Other.Ptr;
            Other.Ptr = nullptr;
        }

        return *this;
    }

    template<typename TOther>
    TUniquePtr& operator=( TUniquePtr<TOther>&& Other ) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        if ( this != std::addressof( Other ) )
        {
            Reset();
            Ptr = Other.Ptr;
            Other.Ptr = nullptr;
        }

        return *this;
    }

    TUniquePtr& operator=( std::nullptr_t ) noexcept
    {
        Reset();
        return *this;
    }

    bool operator==( const TUniquePtr& Other ) const noexcept
    {
        return (Ptr == Other.Ptr);
    }
    bool operator!=( const TUniquePtr& Other ) const noexcept
    {
        return !(*this == Other);
    }

    bool operator==( T* InPtr ) const noexcept
    {
        return (Ptr == InPtr);
    }
    bool operator!=( T* InPtr ) const noexcept
    {
        return !(*this == Other);
    }

    operator bool() const noexcept
    {
        return (Ptr != nullptr);
    }

private:
    void InternalRelease() noexcept
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

    TUniquePtr() noexcept
        : Ptr( nullptr )
    {
    }

    TUniquePtr( std::nullptr_t ) noexcept
        : Ptr( nullptr )
    {
    }

    explicit TUniquePtr( T* InPtr ) noexcept
        : Ptr( InPtr )
    {
    }

    TUniquePtr( TUniquePtr&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        Other.Ptr = nullptr;
    }

    template<typename TOther>
    TUniquePtr( TUniquePtr<TOther>&& Other ) noexcept
        : Ptr( Other.Ptr )
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Other.Ptr = nullptr;
    }

    ~TUniquePtr()
    {
        Reset();
    }

    T* Release() noexcept
    {
        T* WeakPtr = Ptr;
        Ptr = nullptr;
        return WeakPtr;
    }

    void Reset() noexcept
    {
        InternalRelease();
        Ptr = nullptr;
    }

    void Swap( TUniquePtr& Other ) noexcept
    {
        T* TempPtr = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = TempPtr;
    }

    T* Get() const noexcept
    {
        return Ptr;
    }
    T* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    T* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    T& operator[]( uint32 Index ) noexcept
    {
        Assert( Ptr != nullptr );
        return Ptr[Index];
    }

    TUniquePtr& operator=( T* InPtr ) noexcept
    {
        if ( Ptr != InPtr )
        {
            Reset();
            Ptr = InPtr;
        }

        return *this;
    }

    TUniquePtr& operator=( TUniquePtr&& Other ) noexcept
    {
        if ( this != std::addressof( Other ) )
        {
            Reset();
            Ptr = Other.Ptr;
            Other.Ptr = nullptr;
        }

        return *this;
    }

    template<typename TOther>
    TUniquePtr& operator=( TUniquePtr<TOther>&& Other ) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        if ( this != std::addressof( Other ) )
        {
            Reset();
            Ptr = Other.Ptr;
            Other.Ptr = nullptr;
        }

        return *this;
    }

    TUniquePtr& operator=( std::nullptr_t ) noexcept
    {
        Reset();
        return *this;
    }

    bool operator==( const TUniquePtr& Other ) const noexcept
    {
        return (Ptr == Other.Ptr);
    }
    bool operator!=( const TUniquePtr& Other ) const noexcept
    {
        return !(*this == Other);
    }

    bool operator==( T* InPtr ) const noexcept
    {
        return (Ptr == InPtr);
    }
    bool operator!=( T* InPtr ) const noexcept
    {
        return !(*this == Other);
    }

    operator bool() const noexcept
    {
        return (Ptr != nullptr);
    }

private:
    void InternalRelease() noexcept
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
TEnableIf<!TIsArray<T>, TUniquePtr<T>> MakeUnique( TArgs&&... Args ) noexcept
{
    T* UniquePtr = new T( Forward<TArgs>( Args )... );
    return Move( TUniquePtr<T>( UniquePtr ) );
}

template<typename T>
TEnableIf<TIsArray<T>, TUniquePtr<T>>MakeUnique( uint32 Size ) noexcept
{
    using TType = TRemoveExtent<T>;

    TType* UniquePtr = new TType[Size];
    return Move( TUniquePtr<T>( UniquePtr ) );
}

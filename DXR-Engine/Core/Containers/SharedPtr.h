#pragma once
#include "UniquePtr.h"

#include "Templates/IsConvertible.h"

#include "Core/Threading/ThreadSafeInt.h"


// PtrControlBlock - Counting references in TWeak- and TSharedPtr

struct PtrControlBlock
{
public:
    typedef ThreadSafeInt32::Type RefType;

    FORCEINLINE PtrControlBlock() noexcept
        : WeakRefs( 0 )
        , StrongRefs( 0 )
    {
    }

    FORCEINLINE RefType AddWeakRef() noexcept
    {
        return WeakRefs++;
    }

    FORCEINLINE RefType AddStrongRef() noexcept
    {
        return StrongRefs++;
    }

    FORCEINLINE RefType ReleaseWeakRef() noexcept
    {
        return WeakRefs--;
    }

    FORCEINLINE RefType ReleaseStrongRef() noexcept
    {
        return StrongRefs--;
    }

    FORCEINLINE RefType GetWeakReferences() const noexcept
    {
        return WeakRefs.Load();
    }

    FORCEINLINE RefType GetStrongReferences() const noexcept
    {
        return StrongRefs.Load();
    }

private:
    ThreadSafeInt32 WeakRefs;
    ThreadSafeInt32 StrongRefs;
};

// TDelete

template<typename T>
struct TDelete
{
    using TType = T;

    FORCEINLINE void operator()( TType* InPtr ) noexcept
    {
        delete InPtr;
    }
};

template<typename T>
struct TDelete<T[]>
{
    using TType = typename TRemoveExtent<T>;

    FORCEINLINE void operator()( TType* InPtr ) noexcept
    {
        delete[] InPtr;
    }
};

// TPtrBase - Base class for TWeak- and TSharedPtr

template<typename T, typename D>
class TPtrBase
{
public:
    template<typename TOther, typename DOther>
    friend class TPtrBase;

    FORCEINLINE T* Get() const noexcept
    {
        return Ptr;
    }

    FORCEINLINE T* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    FORCEINLINE PtrControlBlock::RefType GetStrongReferences() const noexcept
    {
        return Counter ? Counter->GetStrongReferences() : 0;
    }

    FORCEINLINE PtrControlBlock::RefType GetWeakReferences() const noexcept
    {
        return Counter ? Counter->GetWeakReferences() : 0;
    }

    FORCEINLINE T* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    FORCEINLINE bool operator==( T* InPtr ) const noexcept
    {
        return (Ptr == InPtr);
    }

    FORCEINLINE bool operator!=( T* InPtr ) const noexcept
    {
        return !(*this == InPtr);
    }

    FORCEINLINE operator bool() const noexcept
    {
        return (Ptr != nullptr);
    }

protected:
    FORCEINLINE TPtrBase() noexcept
        : Ptr( nullptr )
        , Counter( nullptr )
    {
        static_assert(std::is_array_v<T> == std::is_array_v<D>, "Scalar types must have scalar TDelete");
        static_assert(std::is_invocable<D, T*>(), "TDelete must be a callable");
    }

    FORCEINLINE void InternalAddStrongRef() noexcept
    {
        // If the object has a InPtr there must be a Counter or something went wrong
        if ( Ptr )
        {
            Assert( Counter != nullptr );
            Counter->AddStrongRef();
        }
    }

    FORCEINLINE void InternalAddWeakRef() noexcept
    {
        // If the object has a InPtr there must be a Counter or something went wrong
        if ( Ptr )
        {
            Assert( Counter != nullptr );
            Counter->AddWeakRef();
        }
    }

    inline void InternalReleaseStrongRef() noexcept
    {
        // If the object has a InPtr there must be a Counter or something went wrong
        if ( Ptr )
        {
            Assert( Counter != nullptr );
            Counter->ReleaseStrongRef();

            // When releasing the last strong reference we can destroy the pointer and counter
            if ( Counter->GetStrongReferences() <= 0 )
            {
                if ( Counter->GetWeakReferences() <= 0 )
                {
                    delete Counter;
                }

                Deleter( Ptr );
                InternalClear();
            }
        }
    }

    inline void InternalReleaseWeakRef() noexcept
    {
        // If the object has a InPtr there must be a Counter or something went wrong
        if ( Ptr )
        {
            Assert( Counter != nullptr );
            Counter->ReleaseWeakRef();

            PtrControlBlock::RefType StrongRefs = Counter->GetStrongReferences();
            PtrControlBlock::RefType WeakRefs = Counter->GetWeakReferences();
            if ( WeakRefs <= 0 && StrongRefs <= 0 )
            {
                delete Counter;
            }
        }
    }

    inline void InternalSwap( TPtrBase& Other ) noexcept
    {
        T* TempPtr = Ptr;
        PtrControlBlock* TempBlock = Counter;

        Ptr = Other.Ptr;
        Counter = Other.Counter;

        Other.Ptr = TempPtr;
        Other.Counter = TempBlock;
    }

    inline void InternalMove( TPtrBase&& Other ) noexcept
    {
        Ptr = Other.Ptr;
        Counter = Other.Counter;

        Other.Ptr = nullptr;
        Other.Counter = nullptr;
    }

    template<typename TOther, typename DOther>
    inline void InternalMove( TPtrBase<TOther, DOther>&& Other ) noexcept
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);

        Ptr = static_cast<TOther*>(Other.Ptr);
        Counter = Other.Counter;

        Other.Ptr = nullptr;
        Other.Counter = nullptr;
    }

    FORCEINLINE void InternalConstructStrong( T* InPtr ) noexcept
    {
        Ptr = InPtr;
        Counter = new PtrControlBlock();
        InternalAddStrongRef();
    }

    template<typename TOther>
    FORCEINLINE void InternalConstructStrong( TOther* InPtr ) noexcept
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);

        Ptr = static_cast<T*>(InPtr);
        Counter = new PtrControlBlock();
        InternalAddStrongRef();
    }

    FORCEINLINE void InternalConstructStrong( const TPtrBase& Other ) noexcept
    {
        Ptr = Other.Ptr;
        Counter = Other.Counter;
        InternalAddStrongRef();
    }

    template<typename TOther, typename DOther>
    FORCEINLINE void InternalConstructStrong( const TPtrBase<TOther, DOther>& Other ) noexcept
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);

        Ptr = static_cast<T*>(Other.Ptr);
        Counter = Other.Counter;
        InternalAddStrongRef();
    }

    template<typename TOther, typename DOther>
    FORCEINLINE void InternalConstructStrong( const TPtrBase<TOther, DOther>& Other, T* InPtr ) noexcept
    {
        Ptr = InPtr;
        Counter = Other.Counter;
        InternalAddStrongRef();
    }

    template<typename TOther, typename DOther>
    FORCEINLINE void InternalConstructStrong( TPtrBase<TOther, DOther>&& Other, T* InPtr ) noexcept
    {
        Ptr = InPtr;
        Counter = Other.Counter;

        Other.Ptr = nullptr;
        Other.Counter = nullptr;
    }

    FORCEINLINE void InternalConstructWeak( T* InPtr ) noexcept
    {
        Ptr = InPtr;
        Counter = new PtrControlBlock();
        InternalAddWeakRef();
    }

    template<typename TOther>
    FORCEINLINE void InternalConstructWeak( TOther* InPtr ) noexcept
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);

        Ptr = static_cast<T*>(InPtr);
        Counter = new PtrControlBlock();
        InternalAddWeakRef();
    }

    FORCEINLINE void InternalConstructWeak( const TPtrBase& Other ) noexcept
    {
        Ptr = Other.Ptr;
        Counter = Other.Counter;
        InternalAddWeakRef();
    }

    template<typename TOther, typename DOther>
    FORCEINLINE void InternalConstructWeak( const TPtrBase<TOther, DOther>& Other ) noexcept
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);

        Ptr = static_cast<T*>(Other.Ptr);
        Counter = Other.Counter;
        InternalAddWeakRef();
    }

    FORCEINLINE void InternalDestructWeak() noexcept
    {
        InternalReleaseWeakRef();
        InternalClear();
    }

    FORCEINLINE void InternalDestructStrong() noexcept
    {
        InternalReleaseStrongRef();
        InternalClear();
    }

    FORCEINLINE void InternalClear() noexcept
    {
        Ptr = nullptr;
        Counter = nullptr;
    }

protected:
    T* Ptr;
    PtrControlBlock* Counter;
    D Deleter;
};

// Forward Declarations

template<typename TOther>
class TWeakPtr;

// TSharedPtr - RefCounted Scalar Pointer, similar to std::shared_ptr

template<typename T>
class TSharedPtr : public TPtrBase<T, TDelete<T>>
{
    using Base = typename TPtrBase<T, TDelete<T>>;

public:
    FORCEINLINE TSharedPtr() noexcept
        : Base()
    {
    }

    FORCEINLINE TSharedPtr( std::nullptr_t ) noexcept
        : Base()
    {
    }

    FORCEINLINE explicit TSharedPtr( T* InPtr ) noexcept
        : Base()
    {
        Base::InternalConstructStrong( InPtr );
    }

    FORCEINLINE TSharedPtr( const TSharedPtr& Other ) noexcept
        : Base()
    {
        Base::InternalConstructStrong( Other );
    }

    FORCEINLINE TSharedPtr( TSharedPtr&& Other ) noexcept
        : Base()
    {
        Base::InternalMove( Move( Other ) );
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr( const TSharedPtr<TOther>& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);
        Base::template InternalConstructStrong<TOther>( Other );
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr( TSharedPtr<TOther>&& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);
        Base::template InternalMove<TOther>( Move( Other ) );
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr( const TSharedPtr<TOther>& Other, T* InPtr ) noexcept
        : Base()
    {
        Base::template InternalConstructStrong<TOther>( Other, InPtr );
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr( TSharedPtr<TOther>&& Other, T* InPtr ) noexcept
        : Base()
    {
        Base::template InternalConstructStrong<TOther>( Move( Other ), InPtr );
    }

    template<typename TOther>
    FORCEINLINE explicit TSharedPtr( const TWeakPtr<TOther>& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);
        Base::template InternalConstructStrong<TOther>( Other );
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr( TUniquePtr<TOther>&& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);
        Base::template InternalConstructStrong<TOther, TDelete<T>>( Other.Release() );
    }

    FORCEINLINE ~TSharedPtr() noexcept
    {
        Reset();
    }

    FORCEINLINE void Reset( T* NewPtr = nullptr ) noexcept
    {
        if ( Base::Get() != NewPtr )
        {
            Base::InternalDestructStrong();
            Base::InternalConstructStrong( NewPtr );
        }
    }

    FORCEINLINE void Swap( TSharedPtr& Other ) noexcept
    {
        Base::InternalSwap( Other );
    }

    FORCEINLINE bool IsUnique() const noexcept
    {
        return (Base::GetStrongReferences() == 1);
    }

    FORCEINLINE T* operator->() const noexcept
    {
        return Base::Get();
    }

    FORCEINLINE T& operator*() const noexcept
    {
        Assert( Base::Ptr != nullptr );
        return *Base::Ptr;
    }

    FORCEINLINE TSharedPtr& operator=( const TSharedPtr& Other ) noexcept
    {
        TSharedPtr( Other ).Swap( *this );
        return *this;
    }

    FORCEINLINE TSharedPtr& operator=( TSharedPtr&& Other ) noexcept
    {
        TSharedPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr& operator=( const TSharedPtr<TOther>& Other ) noexcept
    {
        TSharedPtr( Other ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr& operator=( TSharedPtr<TOther>&& Other ) noexcept
    {
        TSharedPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    FORCEINLINE TSharedPtr& operator=( T* InPtr ) noexcept
    {
        Reset( InPtr );
        return *this;
    }

    FORCEINLINE TSharedPtr& operator=( std::nullptr_t ) noexcept
    {
        Reset();
        return *this;
    }

    FORCEINLINE bool operator==( const TSharedPtr& Other ) const noexcept
    {
        return (Base::Ptr == Other.Ptr);
    }

    FORCEINLINE bool operator!=( const TSharedPtr& Other ) const noexcept
    {
        return !(*this == Other);
    }
};

// TSharedPtr - RefCounted Pointer for array types, similar to std::shared_ptr

template<typename T>
class TSharedPtr<T[]> : public TPtrBase<T, TDelete<T[]>>
{
    using Base = typename TPtrBase<T, TDelete<T[]>>;

public:
    FORCEINLINE TSharedPtr() noexcept
        : Base()
    {
    }

    FORCEINLINE TSharedPtr( std::nullptr_t ) noexcept
        : Base()
    {
    }

    FORCEINLINE explicit TSharedPtr( T* InPtr ) noexcept
        : Base()
    {
        Base::InternalConstructStrong( InPtr );
    }

    FORCEINLINE TSharedPtr( const TSharedPtr& Other ) noexcept
        : Base()
    {
        Base::InternalConstructStrong( Other );
    }

    FORCEINLINE TSharedPtr( TSharedPtr&& Other ) noexcept
        : Base()
    {
        Base::InternalMove( Move( Other ) );
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr( const TSharedPtr<TOther[]>& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);
        Base::template InternalConstructStrong<TOther>( Other );
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr( const TSharedPtr<TOther[]>& Other, T* InPtr ) noexcept
        : Base()
    {
        Base::template InternalConstructStrong<TOther>( Other, InPtr );
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr( TSharedPtr<TOther[]>&& Other, T* InPtr ) noexcept
        : Base()
    {
        Base::template InternalConstructStrong<TOther>( Move( Other ), InPtr );
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr( TSharedPtr<TOther[]>&& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);
        Base::template InternalMove<TOther>( Move( Other ) );
    }

    template<typename TOther>
    FORCEINLINE explicit TSharedPtr( const TWeakPtr<TOther[]>& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);
        Base::template InternalConstructStrong<TOther>( Other );
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr( TUniquePtr<TOther[]>&& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value);
        Base::template InternalConstructStrong<TOther>( Other.Release() );
    }

    FORCEINLINE ~TSharedPtr()
    {
        Reset();
    }

    FORCEINLINE void Reset( T* NewPtr = nullptr ) noexcept
    {
        if ( Base::Get() != NewPtr )
        {
            Base::InternalDestructStrong();
            Base::InternalConstructStrong( NewPtr );
        }
    }

    FORCEINLINE void Swap( TSharedPtr& Other ) noexcept
    {
        Base::InternalSwap( Other );
    }

    FORCEINLINE bool IsUnique() const noexcept
    {
        return (Base::GetStrongReferences() == 1);
    }

    FORCEINLINE T& operator[]( uint32 Index ) noexcept
    {
        Assert( Base::Ptr != nullptr );
        return Base::Ptr[Index];
    }

    FORCEINLINE TSharedPtr& operator=( const TSharedPtr& Other ) noexcept
    {
        TSharedPtr( Other ).Swap( *this );
        return *this;
    }

    FORCEINLINE TSharedPtr& operator=( TSharedPtr&& Other ) noexcept
    {
        TSharedPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr& operator=( const TSharedPtr<TOther[]>& Other ) noexcept
    {
        TSharedPtr( Other ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    FORCEINLINE TSharedPtr& operator=( TSharedPtr<TOther[]>&& Other ) noexcept
    {
        TSharedPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    FORCEINLINE TSharedPtr& operator=( T* InPtr ) noexcept
    {
        Reset( InPtr );
        return *this;
    }

    FORCEINLINE TSharedPtr& operator=( std::nullptr_t ) noexcept
    {
        Reset();
        return *this;
    }

    FORCEINLINE bool operator==( const TSharedPtr& Other ) const noexcept
    {
        return (Base::Ptr == Other.Ptr);
    }

    FORCEINLINE bool operator!=( const TSharedPtr& Other ) const noexcept
    {
        return !(*this == Other);
    }
};

// TWeakPtr - Weak Pointer for scalar types, similar to std::weak_ptr

template<typename T>
class TWeakPtr : public TPtrBase<T, TDelete<T>>
{
    using Base = typename TPtrBase<T, TDelete<T>>;

public:
    FORCEINLINE TWeakPtr() noexcept
        : Base()
    {
    }

    FORCEINLINE TWeakPtr( const TSharedPtr<T>& Other ) noexcept
        : Base()
    {
        Base::InternalConstructWeak( Other );
    }

    template<typename TOther>
    FORCEINLINE TWeakPtr( const TSharedPtr<TOther>& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value, "TWeakPtr: Trying to convert non-convertable types");
        Base::template InternalConstructWeak<TOther>( Other );
    }

    FORCEINLINE TWeakPtr( const TWeakPtr& Other ) noexcept
        : Base()
    {
        Base::InternalConstructWeak( Other );
    }

    FORCEINLINE TWeakPtr( TWeakPtr&& Other ) noexcept
        : Base()
    {
        Base::InternalMove( Move( Other ) );
    }

    template<typename TOther>
    FORCEINLINE TWeakPtr( const TWeakPtr<TOther>& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value, "TWeakPtr: Trying to convert non-convertable types");
        Base::template InternalConstructWeak<TOther>( Other );
    }

    template<typename TOther>
    FORCEINLINE TWeakPtr( TWeakPtr<TOther>&& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value, "TWeakPtr: Trying to convert non-convertable types");
        Base::template InternalMove<TOther>( Move( Other ) );
    }

    FORCEINLINE ~TWeakPtr()
    {
        Reset();
    }

    FORCEINLINE void Reset( T* NewPtr = nullptr ) noexcept
    {
        if ( Base::Get() != NewPtr )
        {
            Base::InternalDestructWeak();
            Base::InternalContructWeak( NewPtr );
        }
    }

    FORCEINLINE void Swap( TWeakPtr& Other ) noexcept
    {
        Base::InternalSwap( Other );
    }

    FORCEINLINE bool IsExpired() const noexcept
    {
        return (Base::GetStrongReferences() < 1);
    }

    FORCEINLINE TSharedPtr<T> MakeShared() noexcept
    {
        const TWeakPtr& This = *this;
        return Move( TSharedPtr<T>( This ) );
    }

    FORCEINLINE T* operator->() const noexcept
    {
        return Base::Get();
    }

    FORCEINLINE T& operator*() const noexcept
    {
        Assert( Base::Ptr != nullptr );
        return *Base::Ptr;
    }

    FORCEINLINE TWeakPtr& operator=( const TWeakPtr& Other ) noexcept
    {
        TWeakPtr( Other ).Swap( *this );
        return *this;
    }

    FORCEINLINE TWeakPtr& operator=( TWeakPtr&& Other ) noexcept
    {
        TWeakPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    FORCEINLINE TWeakPtr& operator=( const TWeakPtr<TOther>& Other ) noexcept
    {
        TWeakPtr( Other ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    FORCEINLINE TWeakPtr& operator=( TWeakPtr<TOther>&& Other ) noexcept
    {
        TWeakPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    FORCEINLINE TWeakPtr& operator=( T* InPtr ) noexcept
    {
        Reset( InPtr );
        return *this;
    }

    FORCEINLINE TWeakPtr& operator=( std::nullptr_t ) noexcept
    {
        Reset();
        return *this;
    }

    FORCEINLINE bool operator==( const TWeakPtr& Other ) const noexcept
    {
        return (Base::Ptr == Other.Ptr);
    }

    FORCEINLINE bool operator!=( const TWeakPtr& Other ) const noexcept
    {
        return !(*this == Other);
    }
};

// TWeakPtr - Weak Pointer for array types, similar to std::weak_ptr

template<typename T>
class TWeakPtr<T[]> : public TPtrBase<T, TDelete<T[]>>
{
    using Base = typename TPtrBase<T, TDelete<T[]>>;

public:
    FORCEINLINE TWeakPtr() noexcept
        : Base()
    {
    }

    FORCEINLINE TWeakPtr( const TSharedPtr<T>& Other ) noexcept
        : Base()
    {
        Base::InternalConstructWeak( Other );
    }

    template<typename TOther>
    FORCEINLINE TWeakPtr( const TSharedPtr<TOther[]>& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value, "TWeakPtr: Trying to convert non-convertable types");
        Base::template InternalConstructWeak<TOther>( Other );
    }

    FORCEINLINE TWeakPtr( const TWeakPtr& Other ) noexcept
        : Base()
    {
        Base::InternalConstructWeak( Other );
    }

    FORCEINLINE TWeakPtr( TWeakPtr&& Other ) noexcept
        : Base()
    {
        Base::InternalMove( Move( Other ) );
    }

    template<typename TOther>
    FORCEINLINE TWeakPtr( const TWeakPtr<TOther[]>& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value, "TWeakPtr: Trying to convert non-convertable types");
        Base::template InternalConstructWeak<TOther>( Other );
    }

    template<typename TOther>
    FORCEINLINE TWeakPtr( TWeakPtr<TOther[]>&& Other ) noexcept
        : Base()
    {
        static_assert(TIsConvertible<TOther*, T*>::Value, "TWeakPtr: Trying to convert non-convertable types");
        Base::template InternalMove<TOther>( Move( Other ) );
    }

    FORCEINLINE ~TWeakPtr()
    {
        Reset();
    }

    FORCEINLINE void Reset( T* NewPtr ) noexcept
    {
        if ( Base::Get() != NewPtr )
        {
            Base::InternalDestructWeak();
            Base::InternalConstructWeak( NewPtr );
        }
    }

    FORCEINLINE void Swap( TWeakPtr& Other ) noexcept
    {
        Base::InternalSwap( Other );
    }

    FORCEINLINE bool IsExpired() const noexcept
    {
        return (Base::GetStrongReferences() < 1);
    }

    FORCEINLINE TSharedPtr<T[]> MakeShared() noexcept
    {
        const TWeakPtr& This = *this;
        return Move( TSharedPtr<T[]>( This ) );
    }

    FORCEINLINE T& operator[]( uint32 Index ) noexcept
    {
        Assert( Base::Ptr != nullptr );
        return Base::Ptr[Index];
    }

    FORCEINLINE TWeakPtr& operator=( const TWeakPtr& Other ) noexcept
    {
        TWeakPtr( Other ).Swap( *this );
        return *this;
    }

    FORCEINLINE TWeakPtr& operator=( TWeakPtr&& Other ) noexcept
    {
        TWeakPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    FORCEINLINE TWeakPtr& operator=( const TWeakPtr<TOther[]>& Other ) noexcept
    {
        TWeakPtr( Other ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    FORCEINLINE TWeakPtr& operator=( TWeakPtr<TOther[]>&& Other ) noexcept
    {
        TWeakPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    FORCEINLINE TWeakPtr& operator=( T* InPtr ) noexcept
    {
        Reset( InPtr );
        return *this;
    }

    FORCEINLINE TWeakPtr& operator=( std::nullptr_t ) noexcept
    {
        Reset();
        return *this;
    }

    FORCEINLINE bool operator==( const TWeakPtr& Other ) const noexcept
    {
        return (Base::Ptr == Other.Ptr);
    }

    FORCEINLINE bool operator!=( const TWeakPtr& Other ) const noexcept
    {
        return !(*this == Other);
    }
};

// MakeShared - Creates a new object together with a SharedPtr

template<typename T, typename... TArgs>
FORCEINLINE TEnableIf<!IsArray<T>, TSharedPtr<T>> MakeShared( TArgs&&... Args ) noexcept
{
    T* RefCountedPtr = new T( ::Forward<TArgs>( Args )... );
    return Move( TSharedPtr<T>( RefCountedPtr ) );
}

template<typename T>
FORCEINLINE TEnableIf<IsArray<T>, TSharedPtr<T>> MakeShared( uint32 Size ) noexcept
{
    using TType = TRemoveExtent<T>;

    TType* RefCountedPtr = new TType[Size];
    return Move( TSharedPtr<T>( RefCountedPtr ) );
}

// Casting functions

// static_cast
template<typename T0, typename T1>
FORCEINLINE TEnableIf<IsArray<T0> == IsArray<T1>, TSharedPtr<T0>> StaticCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = static_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Pointer, RawPointer ) );
}

template<typename T0, typename T1>
FORCEINLINE TEnableIf<IsArray<T0> == IsArray<T1>, TSharedPtr<T0>> StaticCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = static_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Move( Pointer ), RawPointer ) );
}

// const_cast
template<typename T0, typename T1>
FORCEINLINE TEnableIf<IsArray<T0> == IsArray<T1>, TSharedPtr<T0>> ConstCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = const_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Pointer, RawPointer ) );
}

template<typename T0, typename T1>
FORCEINLINE TEnableIf<IsArray<T0> == IsArray<T1>, TSharedPtr<T0>> ConstCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = const_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Move( Pointer ), RawPointer ) );
}

// reinterpret_cast
template<typename T0, typename T1>
FORCEINLINE TEnableIf<IsArray<T0> == IsArray<T1>, TSharedPtr<T0>> ReinterpretCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = reinterpret_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Pointer, RawPointer ) );
}

template<typename T0, typename T1>
FORCEINLINE TEnableIf<IsArray<T0> == IsArray<T1>, TSharedPtr<T0>> ReinterpretCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = reinterpret_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Move( Pointer ), RawPointer ) );
}

// dynamic_cast
template<typename T0, typename T1>
FORCEINLINE TEnableIf<IsArray<T0> == IsArray<T1>, TSharedPtr<T0>> DynamicCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = dynamic_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Pointer, RawPointer ) );
}

template<typename T0, typename T1>
FORCEINLINE TEnableIf<IsArray<T0> == IsArray<T1>, TSharedPtr<T0>> DynamicCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = dynamic_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Move( Pointer ), RawPointer ) );
}

#pragma once
#include "UniquePtr.h"

// PtrControlBlock - Counting references in TWeak- and TSharedPtr

struct PtrControlBlock
{
public:
    typedef uint32 RefType;

    PtrControlBlock() noexcept
        : WeakRefs( 0 )
        , StrongRefs( 0 )
    {
    }

    RefType AddWeakRef() noexcept
    {
        return WeakRefs++;
    }
    RefType AddStrongRef() noexcept
    {
        return StrongRefs++;
    }

    RefType ReleaseWeakRef() noexcept
    {
        return WeakRefs--;
    }
    RefType ReleaseStrongRef() noexcept
    {
        return StrongRefs--;
    }

    RefType GetWeakReferences() const noexcept
    {
        return WeakRefs;
    }
    RefType GetStrongReferences() const noexcept
    {
        return StrongRefs;
    }

private:
    RefType WeakRefs;
    RefType StrongRefs;
};

// TDelete

template<typename T>
struct TDelete
{
    using TType = T;

    void operator()( TType* InPtr ) noexcept
    {
        delete InPtr;
    }
};

template<typename T>
struct TDelete<T[]>
{
    using TType = TRemoveExtent<T>;

    void operator()( TType* InPtr ) noexcept
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

    T* Get() const noexcept
    {
        return Ptr;
    }
    T* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    PtrControlBlock::RefType GetStrongReferences() const noexcept
    {
        return Counter ? Counter->GetStrongReferences() : 0;
    }

    PtrControlBlock::RefType GetWeakReferences() const noexcept
    {
        return Counter ? Counter->GetWeakReferences() : 0;
    }

    T* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    bool operator==( T* InPtr ) const noexcept
    {
        return (Ptr == InPtr);
    }
    bool operator!=( T* InPtr ) const noexcept
    {
        return !(*this == InPtr);
    }

    operator bool() const noexcept
    {
        return (Ptr != nullptr);
    }

protected:
    TPtrBase() noexcept
        : Ptr( nullptr )
        , Counter( nullptr )
    {
        static_assert(std::is_array_v<T> == std::is_array_v<D>, "Scalar types must have scalar TDelete");
        static_assert(std::is_invocable<D, T*>(), "TDelete must be a callable");
    }

    void InternalAddStrongRef() noexcept
    {
        // If the object has a InPtr there must be a Counter or something went wrong
        if ( Ptr )
        {
            Assert( Counter != nullptr );
            Counter->AddStrongRef();
        }
    }

    void InternalAddWeakRef() noexcept
    {
        // If the object has a InPtr there must be a Counter or something went wrong
        if ( Ptr )
        {
            Assert( Counter != nullptr );
            Counter->AddWeakRef();
        }
    }

    void InternalReleaseStrongRef() noexcept
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

    void InternalReleaseWeakRef() noexcept
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

    void InternalSwap( TPtrBase& Other ) noexcept
    {
        T* TempPtr = Ptr;
        PtrControlBlock* TempBlock = Counter;

        Ptr = Other.Ptr;
        Counter = Other.Counter;

        Other.Ptr = TempPtr;
        Other.Counter = TempBlock;
    }

    void InternalMove( TPtrBase&& Other ) noexcept
    {
        Ptr = Other.Ptr;
        Counter = Other.Counter;

        Other.Ptr = nullptr;
        Other.Counter = nullptr;
    }

    template<typename TOther, typename DOther>
    void InternalMove( TPtrBase<TOther, DOther>&& Other ) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        Ptr = static_cast<TOther*>(Other.Ptr);
        Counter = Other.Counter;

        Other.Ptr = nullptr;
        Other.Counter = nullptr;
    }

    void InternalConstructStrong( T* InPtr ) noexcept
    {
        Ptr = InPtr;
        Counter = new PtrControlBlock();
        InternalAddStrongRef();
    }

    template<typename TOther, typename DOther>
    void InternalConstructStrong( TOther* InPtr ) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        Ptr = static_cast<T*>(InPtr);
        Counter = new PtrControlBlock();
        InternalAddStrongRef();
    }

    void InternalConstructStrong( const TPtrBase& Other ) noexcept
    {
        Ptr = Other.Ptr;
        Counter = Other.Counter;
        InternalAddStrongRef();
    }

    template<typename TOther, typename DOther>
    void InternalConstructStrong( const TPtrBase<TOther, DOther>& Other ) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        Ptr = static_cast<T*>(Other.Ptr);
        Counter = Other.Counter;
        InternalAddStrongRef();
    }

    template<typename TOther, typename DOther>
    void InternalConstructStrong( const TPtrBase<TOther, DOther>& Other, T* InPtr ) noexcept
    {
        Ptr = InPtr;
        Counter = Other.Counter;
        InternalAddStrongRef();
    }

    template<typename TOther, typename DOther>
    void InternalConstructStrong( TPtrBase<TOther, DOther>&& Other, T* InPtr ) noexcept
    {
        Ptr = InPtr;
        Counter = Other.Counter;

        Other.Ptr = nullptr;
        Other.Counter = nullptr;
    }

    void InternalConstructWeak( T* InPtr ) noexcept
    {
        Ptr = InPtr;
        Counter = new PtrControlBlock();
        InternalAddWeakRef();
    }

    template<typename TOther>
    void InternalConstructWeak( TOther* InPtr ) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        Ptr = static_cast<T*>(InPtr);
        Counter = new PtrControlBlock();
        InternalAddWeakRef();
    }

    void InternalConstructWeak( const TPtrBase& Other ) noexcept
    {
        Ptr = Other.Ptr;
        Counter = Other.Counter;
        InternalAddWeakRef();
    }

    template<typename TOther, typename DOther>
    void InternalConstructWeak( const TPtrBase<TOther, DOther>& Other ) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        Ptr = static_cast<T*>(Other.Ptr);
        Counter = Other.Counter;
        InternalAddWeakRef();
    }

    void InternalDestructWeak() noexcept
    {
        InternalReleaseWeakRef();
        InternalClear();
    }

    void InternalDestructStrong() noexcept
    {
        InternalReleaseStrongRef();
        InternalClear();
    }

    void InternalClear() noexcept
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
    using Base = TPtrBase<T, TDelete<T>>;

public:
    TSharedPtr() noexcept
        : Base()
    {
    }

    TSharedPtr( std::nullptr_t ) noexcept
        : Base()
    {
    }

    explicit TSharedPtr( T* InPtr ) noexcept
        : Base()
    {
        Base::InternalConstructStrong( InPtr );
    }

    TSharedPtr( const TSharedPtr& Other ) noexcept
        : Base()
    {
        Base::InternalConstructStrong( Other );
    }

    TSharedPtr( TSharedPtr&& Other ) noexcept
        : Base()
    {
        Base::InternalMove( Move( Other ) );
    }

    template<typename TOther>
    TSharedPtr( const TSharedPtr<TOther>& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Base::template InternalConstructStrong<TOther>( Other );
    }

    template<typename TOther>
    TSharedPtr( TSharedPtr<TOther>&& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Base::template InternalMove<TOther>( Move( Other ) );
    }

    template<typename TOther>
    TSharedPtr( const TSharedPtr<TOther>& Other, T* InPtr ) noexcept
        : Base()
    {
        Base::template InternalConstructStrong<TOther>( Other, InPtr );
    }

    template<typename TOther>
    TSharedPtr( TSharedPtr<TOther>&& Other, T* InPtr ) noexcept
        : Base()
    {
        Base::template InternalConstructStrong<TOther>( Move( Other ), InPtr );
    }

    template<typename TOther>
    explicit TSharedPtr( const TWeakPtr<TOther>& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Base::template InternalConstructStrong<TOther>( Other );
    }

    template<typename TOther>
    TSharedPtr( TUniquePtr<TOther>&& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Base::template InternalConstructStrong<TOther, TDelete<T>>( Other.Release() );
    }

    ~TSharedPtr() noexcept
    {
        Reset();
    }

    void Reset() noexcept
    {
        Base::InternalDestructStrong();
    }

    void Swap( TSharedPtr& Other ) noexcept
    {
        Base::InternalSwap( Other );
    }

    bool IsUnique() const noexcept
    {
        return (Base::GetStrongReferences() == 1);
    }

    T* operator->() const noexcept
    {
        return Base::Get();
    }

    T& operator*() const noexcept
    {
        Assert( Base::Ptr != nullptr );
        return *Base::Ptr;
    }

    TSharedPtr& operator=( const TSharedPtr& Other ) noexcept
    {
        TSharedPtr( Other ).Swap( *this );
        return *this;
    }

    TSharedPtr& operator=( TSharedPtr&& Other ) noexcept
    {
        TSharedPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    TSharedPtr& operator=( const TSharedPtr<TOther>& Other ) noexcept
    {
        TSharedPtr( Other ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    TSharedPtr& operator=( TSharedPtr<TOther>&& Other ) noexcept
    {
        TSharedPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    TSharedPtr& operator=( T* InPtr ) noexcept
    {
        if ( Base::Ptr != InPtr )
        {
            Reset();
            Base::InternalConstructStrong( InPtr );
        }

        return *this;
    }

    TSharedPtr& operator=( std::nullptr_t ) noexcept
    {
        Reset();
        return *this;
    }

    bool operator==( const TSharedPtr& Other ) const noexcept
    {
        return (Base::Ptr == Other.Ptr);
    }
    bool operator!=( const TSharedPtr& Other ) const noexcept
    {
        return !(*this == Other);
    }
};

// TSharedPtr - RefCounted Pointer for array types, similar to std::shared_ptr

template<typename T>
class TSharedPtr<T[]> : public TPtrBase<T, TDelete<T[]>>
{
    using Base = TPtrBase<T, TDelete<T[]>>;

public:
    TSharedPtr() noexcept
        : Base()
    {
    }

    TSharedPtr( std::nullptr_t ) noexcept
        : Base()
    {
    }

    explicit TSharedPtr( T* InPtr ) noexcept
        : Base()
    {
        Base::InternalConstructStrong( InPtr );
    }

    TSharedPtr( const TSharedPtr& Other ) noexcept
        : Base()
    {
        Base::InternalConstructStrong( Other );
    }

    TSharedPtr( TSharedPtr&& Other ) noexcept
        : Base()
    {
        Base::InternalMove( Move( Other ) );
    }

    template<typename TOther>
    TSharedPtr( const TSharedPtr<TOther[]>& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Base::template InternalConstructStrong<TOther>( Other );
    }

    template<typename TOther>
    TSharedPtr( const TSharedPtr<TOther[]>& Other, T* InPtr ) noexcept
        : Base()
    {
        Base::template InternalConstructStrong<TOther>( Other, InPtr );
    }

    template<typename TOther>
    TSharedPtr( TSharedPtr<TOther[]>&& Other, T* InPtr ) noexcept
        : Base()
    {
        Base::template InternalConstructStrong<TOther>( Move( Other ), InPtr );
    }

    template<typename TOther>
    TSharedPtr( TSharedPtr<TOther[]>&& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Base::template InternalMove<TOther>( Move( Other ) );
    }

    template<typename TOther>
    explicit TSharedPtr( const TWeakPtr<TOther[]>& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Base::template InternalConstructStrong<TOther>( Other );
    }

    template<typename TOther>
    TSharedPtr( TUniquePtr<TOther[]>&& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Base::template InternalConstructStrong<TOther>( Other.Release() );
    }

    ~TSharedPtr()
    {
        Reset();
    }

    void Reset() noexcept
    {
        Base::InternalDestructStrong();
    }

    void Swap( TSharedPtr& Other ) noexcept
    {
        Base::InternalSwap( Other );
    }

    bool IsUnique() const noexcept
    {
        return (Base::GetStrongReferences() == 1);
    }

    T& operator[]( uint32 Index ) noexcept
    {
        Assert( Base::Ptr != nullptr );
        return Base::Ptr[Index];
    }

    TSharedPtr& operator=( const TSharedPtr& Other ) noexcept
    {
        TSharedPtr( Other ).Swap( *this );
        return *this;
    }

    TSharedPtr& operator=( TSharedPtr&& Other ) noexcept
    {
        TSharedPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    TSharedPtr& operator=( const TSharedPtr<TOther[]>& Other ) noexcept
    {
        TSharedPtr( Other ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    TSharedPtr& operator=( TSharedPtr<TOther[]>&& Other ) noexcept
    {
        TSharedPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    TSharedPtr& operator=( T* InPtr ) noexcept
    {
        if ( this->InPtr != InPtr )
        {
            Reset();
            Base::InternalConstructStrong( InPtr );
        }

        return *this;
    }

    TSharedPtr& operator=( std::nullptr_t ) noexcept
    {
        Reset();
        return *this;
    }

    bool operator==( const TSharedPtr& Other ) const noexcept
    {
        return (Base::Ptr == Other.Ptr);
    }
    bool operator!=( const TSharedPtr& Other ) const noexcept
    {
        return !(*this == Other);
    }
};

// TWeakPtr - Weak Pointer for scalar types, similar to std::weak_ptr

template<typename T>
class TWeakPtr : public TPtrBase<T, TDelete<T>>
{
    using Base = TPtrBase<T, TDelete<T>>;

public:
    TWeakPtr() noexcept
        : Base()
    {
    }

    TWeakPtr( const TSharedPtr<T>& Other ) noexcept
        : Base()
    {
        Base::InternalConstructWeak( Other );
    }

    template<typename TOther>
    TWeakPtr( const TSharedPtr<TOther>& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>(), "TWeakPtr: Trying to convert non-convertable types");
        Base::template InternalConstructWeak<TOther>( Other );
    }

    TWeakPtr( const TWeakPtr& Other ) noexcept
        : Base()
    {
        Base::InternalConstructWeak( Other );
    }

    TWeakPtr( TWeakPtr&& Other ) noexcept
        : Base()
    {
        Base::InternalMove( Move( Other ) );
    }

    template<typename TOther>
    TWeakPtr( const TWeakPtr<TOther>& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>(), "TWeakPtr: Trying to convert non-convertable types");
        Base::template InternalConstructWeak<TOther>( Other );
    }

    template<typename TOther>
    TWeakPtr( TWeakPtr<TOther>&& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>(), "TWeakPtr: Trying to convert non-convertable types");
        Base::template InternalMove<TOther>( Move( Other ) );
    }

    ~TWeakPtr()
    {
        Reset();
    }

    void Reset() noexcept
    {
        Base::InternalDestructWeak();
    }

    void Swap( TWeakPtr& Other ) noexcept
    {
        Base::InternalSwap( Other );
    }

    bool IsExpired() const noexcept
    {
        return (Base::GetStrongReferences() < 1);
    }

    TSharedPtr<T> MakeShared() noexcept
    {
        const TWeakPtr& This = *this;
        return Move( TSharedPtr<T>( This ) );
    }

    T* operator->() const noexcept
    {
        return Base::Get();
    }

    T& operator*() const noexcept
    {
        Assert( Base::Ptr != nullptr );
        return *Base::Ptr;
    }

    TWeakPtr& operator=( const TWeakPtr& Other ) noexcept
    {
        TWeakPtr( Other ).Swap( *this );
        return *this;
    }

    TWeakPtr& operator=( TWeakPtr&& Other ) noexcept
    {
        TWeakPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    TWeakPtr& operator=( const TWeakPtr<TOther>& Other ) noexcept
    {
        TWeakPtr( Other ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    TWeakPtr& operator=( TWeakPtr<TOther>&& Other ) noexcept
    {
        TWeakPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    TWeakPtr& operator=( T* InPtr ) noexcept
    {
        if ( Base::Ptr != InPtr )
        {
            Reset();
            Base::InternalConstructWeak( InPtr );
        }

        return *this;
    }

    TWeakPtr& operator=( std::nullptr_t ) noexcept
    {
        Reset();
        return *this;
    }

    bool operator==( const TWeakPtr& Other ) const noexcept
    {
        return (Base::Ptr == Other.Ptr);
    }
    bool operator!=( const TWeakPtr& Other ) const noexcept
    {
        return !(*this == Other);
    }
};

// TWeakPtr - Weak Pointer for array types, similar to std::weak_ptr

template<typename T>
class TWeakPtr<T[]> : public TPtrBase<T, TDelete<T[]>>
{
    using Base = TPtrBase<T, TDelete<T[]>>;

public:
    TWeakPtr() noexcept
        : Base()
    {
    }

    TWeakPtr( const TSharedPtr<T>& Other ) noexcept
        : Base()
    {
        Base::InternalConstructWeak( Other );
    }

    template<typename TOther>
    TWeakPtr( const TSharedPtr<TOther[]>& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>(), "TWeakPtr: Trying to convert non-convertable types");
        Base::template InternalConstructWeak<TOther>( Other );
    }

    TWeakPtr( const TWeakPtr& Other ) noexcept
        : Base()
    {
        Base::InternalConstructWeak( Other );
    }

    TWeakPtr( TWeakPtr&& Other ) noexcept
        : Base()
    {
        Base::InternalMove( Move( Other ) );
    }

    template<typename TOther>
    TWeakPtr( const TWeakPtr<TOther[]>& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>(), "TWeakPtr: Trying to convert non-convertable types");
        Base::template InternalConstructWeak<TOther>( Other );
    }

    template<typename TOther>
    TWeakPtr( TWeakPtr<TOther[]>&& Other ) noexcept
        : Base()
    {
        static_assert(std::is_convertible<TOther*, T*>(), "TWeakPtr: Trying to convert non-convertable types");
        Base::template InternalMove<TOther>( Move( Other ) );
    }

    ~TWeakPtr()
    {
        Reset();
    }

    void Reset() noexcept
    {
        Base::InternalDestructWeak();
    }

    void Swap( TWeakPtr& Other ) noexcept
    {
        Base::InternalSwap( Other );
    }

    bool IsExpired() const noexcept
    {
        return (Base::GetStrongReferences() < 1);
    }

    TSharedPtr<T[]> MakeShared() noexcept
    {
        const TWeakPtr& This = *this;
        return Move( TSharedPtr<T[]>( This ) );
    }

    T& operator[]( uint32 Index ) noexcept
    {
        Assert( Base::Ptr != nullptr );
        return Base::Ptr[Index];
    }

    TWeakPtr& operator=( const TWeakPtr& Other ) noexcept
    {
        TWeakPtr( Other ).Swap( *this );
        return *this;
    }

    TWeakPtr& operator=( TWeakPtr&& Other ) noexcept
    {
        TWeakPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    TWeakPtr& operator=( const TWeakPtr<TOther[]>& Other ) noexcept
    {
        TWeakPtr( Other ).Swap( *this );
        return *this;
    }

    template<typename TOther>
    TWeakPtr& operator=( TWeakPtr<TOther[]>&& Other ) noexcept
    {
        TWeakPtr( Move( Other ) ).Swap( *this );
        return *this;
    }

    TWeakPtr& operator=( T* InPtr ) noexcept
    {
        if ( Base::Ptr != InPtr )
        {
            Reset();
            Base::InternalConstructWeak( InPtr );
        }

        return *this;
    }

    TWeakPtr& operator=( std::nullptr_t ) noexcept
    {
        Reset();
        return *this;
    }

    bool operator==( const TWeakPtr& Other ) const noexcept
    {
        return (Base::Ptr == Other.Ptr);
    }
    bool operator!=( const TWeakPtr& Other ) const noexcept
    {
        return !(*this == Other);
    }
};

// MakeShared - Creates a new object together with a SharedPtr

template<typename T, typename... TArgs>
TEnableIf<!TIsArray<T>, TSharedPtr<T>> MakeShared( TArgs&&... Args ) noexcept
{
    T* RefCountedPtr = new T( Forward<TArgs>( Args )... );
    return Move( TSharedPtr<T>( RefCountedPtr ) );
}

template<typename T>
TEnableIf<TIsArray<T>, TSharedPtr<T>> MakeShared( uint32 Size ) noexcept
{
    using TType = TRemoveExtent<T>;

    TType* RefCountedPtr = new TType[Size];
    return Move( TSharedPtr<T>( RefCountedPtr ) );
}

// Casting functions

// static_cast
template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> StaticCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = static_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Pointer, RawPointer ) );
}

template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> StaticCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = static_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Move( Pointer ), RawPointer ) );
}

// const_cast
template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> ConstCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = const_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Pointer, RawPointer ) );
}

template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> ConstCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = const_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Move( Pointer ), RawPointer ) );
}

// reinterpret_cast
template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> ReinterpretCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = reinterpret_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Pointer, RawPointer ) );
}

template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> ReinterpretCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = reinterpret_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Move( Pointer ), RawPointer ) );
}

// dynamic_cast
template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> DynamicCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = dynamic_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Pointer, RawPointer ) );
}

template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> DynamicCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = dynamic_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Move( Pointer ), RawPointer ) );
}

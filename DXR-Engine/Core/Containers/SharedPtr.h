#pragma once
#include "UniquePtr.h"

#include "Core/Templates/IsConvertible.h"
#include "Core/Threading/ThreadSafeInt.h"

/* CPointerReferenceCounter - Counting references in TWeak- and TSharedPtr */
class CPointerReferenceCounter
{
public:
    typedef ThreadSafeInt32::Type CounterType;

    /* Default constructor setting both counters to zero */
    FORCEINLINE CPointerReferenceCounter() noexcept
        : NumWeakRefs( 0 )
        , NumStrongRefs( 0 )
    {
    }

    /* Increase the weak reference count */
    FORCEINLINE CounterType AddWeakRef() noexcept
    {
        return NumWeakRefs++;
    }

    /* Increase the strong reference count */
    FORCEINLINE CounterType AddStrongRef() noexcept
    {
        return NumStrongRefs++;
    }

    /* Decrease the weak reference count */
    FORCEINLINE CounterType ReleaseWeakRef() noexcept
    {
        return NumWeakRefs--;
    }

    /* Increase the strong reference count */
    FORCEINLINE CounterType ReleaseStrongRef() noexcept
    {
        return NumStrongRefs--;
    }

    /* Retrive the weak reference count */
    FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        return WeakRefs.Load();
    }

    /* Retrive the strong reference count */
    FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        return NumStrongRefs.Load();
    }

private:
    ThreadSafeInt32 NumWeakRefs;
    ThreadSafeInt32 NumStrongRefs;
};

/* Helper object for shared ref counter */
template<typename T, typename DeleterType = TDefaultDelete<T>>
class TPointerReferencedStorage : private DeleterType
{
public:
    typedef T                                     ElementType;
    typedef CPointerReferenceCounter::CounterType CounterType;

    /* Cannot copy the storage */
    TPointerReferencedStorage(const TPointerReferencedStorage&) = delete;
    TPointerReferencedStorage& operator=(const TPointerReferencedStorage&) = delete;

    /* Default constructor that init both counter and pointer to nullptr */
    FORCEINLINE TPointerReferencedStorage() noexcept
        : DeleterType()
        , Ptr(nullptr)
        , Counter(nullptr)
    {
    }

    /* Move constructor */
    FORCEINLINE TPointerReferencedStorage( TPointerReferencedStorage&& Other) noexcept
        : DeleterType()
        , Ptr(Other.Ptr)
        , Counter(Other.Counter)
    {
        Other.Ptr = nullptr;
        Other.Counter = nullptr;
    }

    /* Move constructor */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TPointerReferencedStorage( TPointerReferencedStorage<OtherType, OtherDeleterType>&& Other) noexcept
        : DeleterType( ::Move(Other) )
        , Ptr(static_cast<ElementType*>(Other.Ptr))
        , Counter(Other.Counter)
    {
        Other.Ptr = nullptr;
        Other.Counter = nullptr;
    }

    /* Initialized a new counter if needed and adds a strong reference */
    FORCEINLINE void InitStrong(ElementType* NewPtr, CPointerReferenceCounter* NewCounter) noexcept
    {
        // If the pointer is nullptr, we do not care about the counter
        Ptr = NewPtr;
        if (Ptr)
        {
            if (Counter)
            {
                Counter = NewCounter; 
            }
            else
            {
                Counter = new CPointerReferenceCounter();
            }
            
            Assert(Counter != nullptr);
            Counter->AddStrongRef();
        }
        else
        {
            Counter = nullptr;
        }
    }

    /* Adds a weak ref */
    FORCEINLINE void InitWeak(ElementType* NewPtr, CPointerReferenceCounter* NewCounter) noexcept
    {
        // If the pointer is nullptr, we do not care about the counter...
        Ptr = NewPtr;
        if (Ptr)
        {
            // ...however, if we init a weak reference then there has to be a counter aswell, since weakrefs require a strong
            Counter = NewCounter;
            Assert(Counter != nullptr);
            Counter->AddWeakRef();
        }
        else
        {
            Counter = nullptr;
        }
    }

    /* Initialize the storage by a "move", does not add or remove any weak or strong references */
    FORCEINLINE void InitMove(ElementType* NewPtr, CPointerReferenceCounter* NewCounter) noexcept
    {
        // If the pointer is nullptr, we do not care about the counter...
        Ptr = NewPtr;
        if (Ptr)
        {
            // ...However if the Ptr is valid then the counter also has to be, otherwise it is not a valid move
            Assert(Counter != nullptr);
            Counter = NewCounter;
        }
        else
        {
            Counter = nullptr;
        }
    }

    /* Releases a strong ref and deletes the pointer if the strong refcount is zero, if the weak refcount is also zero the counter is deleted */
    FORCEINLINE void ReleaseStrong() noexcept
    {
        Assert(Counter != nullptr);
        Counter->ReleaseStrongRef();
        
        if (Counter->GetStrongRefCount() < 1)
        {
            DeleteElement(Ptr);

            if (Counter->GetWeakRefCount() < 1)
            {
                delete Counter;
                Counter = nullptr;
            }
        }
    }

    /* Relases a weak reference and if all references are gone, then delete the counter */
    FORCEINLINE void ReleaseWeak() noexcept
    {
        Assert(Counter != nullptr);
        Counter->ReleaseWeakRef();
        
        CounterType NumStrongRefs = Counter->GetStrongRefCount();
        CounterType NumWeakRefs   = Counter->GetWeakRefCount();
        if (NumStrongRefs < 1 && NumWeakRefs < 1)
        {
            delete Counter;
            Counter = nullptr;
        }
    }

    /* Swaps to storages */
    FORCEINLINE void Swap(TPointerReferencedStorage& Other) noexcept
    {
        TPointerReferencedStorage Temp( ::Move(*this) );
        *this = ::Move(Other);
        Other = ::Move(Temp);
    }

    /* Returns the pointer */
    FORCEINLINE ElementType* GetPointer() const noexcept
    {
        return Ptr;
    }

    /* Returns the counter */
    FORCEINLINE CPointerReferenceCounter* GetCounter() const noexcept
    {
        return Counter;
    }

    /* Retrive the weak reference count */
    FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        Assert( Counter != nullptr );
        return Counter->GetWeakRefCount();
    }

    /* Retrive the strong reference count */
    FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        Assert( Counter != nullptr );
        return Counter->GetStrongRefCount();
    }

    /* Assign another to this */
    TPointerReferencedStorage& operator=( TPointerReferencedStorage&& Other) noexcept
    {
        Ptr = Other.Ptr;
        Counter = Other.Counter;

        Other.Ptr = TempPtr;
        Other.Counter = TempCounter;

        return *this;
    }

    /* Assign another to this */
    template<typename OtherType, typename OtherDeleterType>
    typename TEnableIf<TIsConvertible<OtherType*, ElementType*, TAddLeftReference<TPointerReferencedStorage>::Type>::Value>::Type operator=( TPointerReferencedStorage<OtherType, OtherDeleterType>&& Other) noexcept
    {
        Ptr = static_cast<ElementType*>(Other.Ptr);
        Counter = Other.Counter;

        Other.Ptr = nullptr;
        Other.Counter = nullptr;

        return *this;
    }

private:
    ElementType* Ptr;
    CPointerReferenceCounter* Counter;
};

/* Forward declaration of TWeakPtr */
template<typename T, typename DeleterType = TDefaultDelete<T>>
class TWeakPtr;

/* TSharedPtr - RefCounted pointer, similar to std::shared_ptr */
template<typename T, typename DeleterType = TDefaultDelete<T>>
class TSharedPtr
{
public:
    typedef T                                                   ElementType;
    typedef TPointerReferencedStorage<ElementType, DeleterType> PointerStorage;
    typedef PointerStorage::CounterType                         CounterType;

    /* Default constructor setting both counter and pointer to nullptr */
    FORCEINLINE TSharedPtr() noexcept
        : Storage()
    {
    }

    /* Constructor setting both counter and pointer to nullptr */
    FORCEINLINE TSharedPtr( NullptrType ) noexcept
        : Storage()
    {
    }

    /* Constructor creating from a raw pointer, and the container takes ownership at this point */
    FORCEINLINE explicit TSharedPtr( ElementType* InPtr ) noexcept
        : Storage()
    {
        Storage.InitStrong(InPtr, nullptr);
    }

    /* Constructor creating from a raw pointer from a convertible type, and the container takes ownership at this point */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>>
    FORCEINLINE explicit TSharedPtr( OtherType* InPtr ) noexcept
        : Storage()
    {
        Storage.InitStrong(static_cast<ElementType*>(InPtr), nullptr);
    }

    /* Copy-constructor */
    FORCEINLINE TSharedPtr( const TSharedPtr& Other ) noexcept
        : Storage()
    {
        Storage.InitStrong( Other.Get(), Other.GetCounter() );
    }

    /* Constructor copy from another type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>>
    FORCEINLINE TSharedPtr( const TSharedPtr<OtherType>& Other ) noexcept
        : Storage()
    {
        Storage.InitStrong( static_cast<ElementType*>(Other.Get()), Other.GetCounter() );
    }

    /* Move constructor */
    FORCEINLINE TSharedPtr( TSharedPtr&& Other ) noexcept
        : Storage( ::Move(Other.Storage) )
    {
    }

    /* Move constructor from with another type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>>
    FORCEINLINE TSharedPtr( TSharedPtr<TOther>&& Other ) noexcept
        : Storage( ::Move(Other.Storage) )
    {
    }

    /* Copy constructor, were the counter is copied, but the pointer is taken from the raw-pointer, this enables casting */
    template<typename OtherType>
    FORCEINLINE explicit TSharedPtr( const TSharedPtr<OtherType>& Other, ElementType* InPtr ) noexcept
        : Storage()
    {
        Storage.InitStrong( InPtr, Other.GetCounter() );
    }

    /* Move constructor, were the counter is copied, but the pointer is taken from the raw-pointer, this enables casting */
    template<typename OtherType>
    FORCEINLINE explicit TSharedPtr( TSharedPtr<OtherType>&& Other, T* InPtr ) noexcept
        : Storage()
    {
        Storage.InitStrong( InPtr, Other.GetCounter() );
    }

    /* Constructor that creates a sharedptr from a weakptr */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>>
    FORCEINLINE explicit TSharedPtr( const TWeakPtr<OtherType>& Other ) noexcept
        : DeleterType()
        , Storage()
    {
        Storage.InitStrong( static_cast<ElementType*>(Other.Get()), Other.GetCounter() );
    }

    /* Constructor that creates a sharedptr from a uniqueptr */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>>
    FORCEINLINE TSharedPtr( TUniquePtr<OtherType>&& Other ) noexcept
        : DeleterType()
        , Storage()
    {
        Storage.InitStrong( static_cast<ElementType*>(Other.Release()), nullptr );
    }

    FORCEINLINE ~TSharedPtr() noexcept
    {
        Reset();
    }

    /* Reset the pointer and set it to a optional new pointer */
    FORCEINLINE void Reset( ElementType* NewPtr = nullptr ) noexcept
    {
        if ( Get() != NewPtr )
        {
            Storage.ReleaseStrong();
            Storage.InitStrong( NewPtr, nullptr );
        }
    }

    /* Reset the pointer and set it to a optional new pointer */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type Reset( OtherType* NewPtr = nullptr ) noexcept
    {
        Reset( static_cast<ElementType*>(NewPtr));
    }

    /* Swaps the contents of this and another container */
    FORCEINLINE void Swap( TSharedPtr& Other ) noexcept
    {
        TSharedPtr Temp( ::Move(*this) );
        *this = ::Move(Other);
        Other = ::Move(Temp);
    }

    /* Checks weather the strong reference count is one */
    FORCEINLINE bool IsUnique() const noexcept
    {
        return (Storage.GetStrongRefCount() == 1);
    }

    /* Cheacks weater the pointer is nullptr or not */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Get() != nullptr);
    }

    /* Return the raw pointer */
    FORCEINLINE ElementType* Get() const noexcept
    {
        return Ptr;
    }

    /* Returns the address of the raw pointer */
    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return &Ptr;
    }

    /* Returns the number of strong references */
    FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        return Storage.GetStrongRefCount();
    }

    /* Return the number of weak references */
    FORCEINLINE CounterTypeGetWeakReferences() const noexcept
    {
        return Storage.GetWeakRefCount();
    }

    /* Return the dereferenced pointer */
    FORCEINLINE ElementType& Dereference() const noexcept
    {
        Assert( IsValid() );
        return *Ptr;
    }

    /* Return the raw pointer */
    FORCEINLINE T* operator->() const noexcept
    {
        return Get();
    }

    /* Dereference the pointer */
    FORCEINLINE T& operator*() const noexcept
    {
        return Dereference();
    }

    /* Get the address of the pointer */
    FORCEINLINE T* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /* Cheack weather the pointer is nullptr or not */
    FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /* Copy assignment */
    FORCEINLINE TSharedPtr& operator=( const TSharedPtr& Other ) noexcept
    {
        TSharedPtr( Other ).Swap( *this );
        return *this;
    }

    /* Move assignment */
    FORCEINLINE TSharedPtr& operator=( TSharedPtr&& Other ) noexcept
    {
        TSharedPtr( ::Move( Other ) ).Swap( *this );
        return *this;
    }

    /* Copy assignment */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, typename TAddLeftReference<TSharedPtr>::Type>::Type operator=( const TSharedPtr<OtherType>& Other ) noexcept
    {
        TSharedPtr( Other ).Swap( *this );
        return *this;
    }

    /* Move assignment */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, typename TAddLeftReference<TSharedPtr>::Type>::Type operator=( TSharedPtr<OtherType>&& Other ) noexcept
    {
        TSharedPtr( ::Move( Other ) ).Swap( *this );
        return *this;
    }

    /* Assignment from raw */
    FORCEINLINE TSharedPtr& operator=( ElementType* NewPtr ) noexcept
    {
        Reset( NewPtr );
        return *this;
    }

    /* Assignment from nullptr */
    FORCEINLINE TSharedPtr& operator=( NullptrType ) noexcept
    {
        Reset();
        return *this;
    }

private:
    FORCEINLINE CPointerReferenceCounter* GetCounter() const noexcept
    {
        return Storage.GetCounter();
    }

    PointerStorage Storage;
};

/* TWeakPtr - Weak Pointer for scalar types, similar to std::weak_ptr */
template<typename T, typename DeleterType = TDefaultDelete<T>>
class TWeakPtr
{
public:
    typedef T                                                   ElementType;
    typedef TPointerReferencedStorage<ElementType, DeleterType> PointerStorage;
    typedef PointerStorage::CounterType                         CounterType;

    /* Default constructor */
    FORCEINLINE TWeakPtr() noexcept
        : Storage()
    {
    }

    /* Copy constructor */
    FORCEINLINE TWeakPtr( const TWeakPtr& Other ) noexcept
        : Storage()
    {
        Storage.InitWeak( Other.Get(), Other.GetCounter() );
    }

    /* Move constructor */
    FORCEINLINE TWeakPtr( TWeakPtr&& Other ) noexcept
        : Storage( ::Move(Other.Storage) )
    {
    }

    /* Copy construct from convertible type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>>
    FORCEINLINE TWeakPtr( const TWeakPtr<OtherType>& Other ) noexcept
        : Storage()
    {
        Storage.InitWeak( static_cast<ElementType*>(Other.Get()), Other.GetCounter() );
    }

    /* Move construct from convertible type */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>>
    FORCEINLINE TWeakPtr( TWeakPtr<OtherType>&& Other ) noexcept
        : Storage( ::Move(Other.Storage) )
    {
    }

    /* Construct from shared */
    FORCEINLINE TWeakPtr( const TSharedPtr<T>& Other ) noexcept
        : Storage()
    {
        Storage.InitWeak(Other.Get(), Other.GetCounter());
    }

    /* Construct from shared */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>>
    FORCEINLINE TWeakPtr( const TSharedPtr<OtherType>& Other ) noexcept
        : Base()
        : Storage()
    {
        Storage.InitWeak( static_cast<ElementType*>(Other.Get()), Other.GetCounter());
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
    
private:
    FORCEINLINE CPointerReferenceCounter* GetCounter() const noexcept
    {
        return Storage.GetCounter();
    }

    PointerStorage Storage;
};

/* Check the equallity between the pointer and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==( const TSharedPtr<T>& LHS, U* RHS ) noexcept
{
    return (LHS.Get() == RHS);
}

/* Check the equallity between the pointer and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==( T* LHS, const TSharedPtr<U>& RHS ) noexcept
{
    return (LHS == RHS.Get());
}

/* Check the equallity between the pointer and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TSharedPtr<T>& LHS, U* RHS ) noexcept
{
    return (LHS.Get() != RHS);
}

/* Check the equallity between the pointer and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=( T* LHS, const TSharedPtr<U>& RHS ) noexcept
{
    return (LHS != RHS.Get());
}

/* Check the equallity between the pointer and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==( const TSharedPtr<T>& LHS, const TSharedPtr<U>& RHS ) noexcept
{
    return (LHS.Get() == RHS.Get());
}

/* Check the equallity between the pointer and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TSharedPtr<T>& LHS, const TSharedPtr<U>& RHS ) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/* Check the equallity between the pointer and a raw pointer */
template<typename T>
FORCEINLINE bool operator==( const TSharedPtr<T>& LHS, NullptrType ) noexcept
{
    return (LHS.Get() == nullptr);
}

/* Check the equallity between the pointer and a raw pointer */
template<typename T>
FORCEINLINE bool operator==( NullptrType, const TSharedPtr<T>& RHS ) noexcept
{
    return (nullptr == RHS.Get());
}

/* Check the equallity between the pointer and a raw pointer */
template<typename T>
FORCEINLINE bool operator!=( const TSharedPtr<T>& LHS, NullptrType ) noexcept
{
    return (LHS.Get() != nullptr);
}

/* Check the equallity between the pointer and a raw pointer */
template<typename T>
FORCEINLINE bool operator!=( NullptrType, const TSharedPtr<T>& RHS ) noexcept
{
    return (nullptr != RHS.Get());
}

/* MakeShared - Creates a new object together with a SharedPtr */
template<typename T, typename... ArgTypes>
FORCEINLINE typename TEnableIf<!TIsArray<T>::Value, TSharedPtr<T>>::Type MakeShared( ArgTypes&&... Args ) noexcept
{
    T* RefCountedPtr = new T( ::Forward<TArgs>( Args )... );
    return TSharedPtr<T>( RefCountedPtr );
}

template<typename T>
FORCEINLINE typename TEnableIf<TIsArray<T>::Value, TSharedPtr<T>>::Type MakeShared( uint32 Size ) noexcept
{
    using TType = TRemoveExtent<T>;

    TType* RefCountedPtr = new TType[Size];
    return Move( TSharedPtr<T>( RefCountedPtr ) );
}

/* Casting functions */

/* static_cast*/
template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type StaticCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = static_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Pointer, RawPointer ) );
}

template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type StaticCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = static_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Move( Pointer ), RawPointer ) );
}

/* const_cast */
template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type ConstCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = const_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Pointer, RawPointer ) );
}

template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type ConstCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = const_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Move( Pointer ), RawPointer ) );
}

/* reinterpret_cast */
template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type ReinterpretCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = reinterpret_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Pointer, RawPointer ) );
}

template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type ReinterpretCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = reinterpret_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Move( Pointer ), RawPointer ) );
}

/* dynamic_cast */
template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type DynamicCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = dynamic_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Pointer, RawPointer ) );
}

template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type DynamicCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    using TType = TRemoveExtent<T0>;

    TType* RawPointer = dynamic_cast<TType*>(Pointer.Get());
    return Move( TSharedPtr<T0>( Move( Pointer ), RawPointer ) );
}

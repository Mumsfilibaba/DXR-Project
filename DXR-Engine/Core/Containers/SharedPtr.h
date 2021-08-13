#pragma once
#include "UniquePtr.h"

#include "Core/Templates/IsConvertible.h"
#include "Core/Templates/IsArray.h"
#include "Core/Templates/RemoveExtent.h"
#include "Core/Templates/AddReference.h"
#include "Core/Templates/EnableIf.h"
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
        return NumWeakRefs.Load();
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
    typedef typename TRemoveExtent<T>::Type       ElementType;
    typedef CPointerReferenceCounter::CounterType CounterType;

    /* Cannot copy the storage */
    TPointerReferencedStorage( const TPointerReferencedStorage& ) = delete;
    TPointerReferencedStorage& operator=( const TPointerReferencedStorage& ) = delete;

    /* Default constructor that init both counter and pointer to nullptr */
    FORCEINLINE TPointerReferencedStorage() noexcept
        : DeleterType()
        , Ptr( nullptr )
        , Counter( nullptr )
    {
    }

    /* Move constructor */
    FORCEINLINE TPointerReferencedStorage( TPointerReferencedStorage&& Other ) noexcept
        : DeleterType( Move( Other ) )
        , Ptr( nullptr )
        , Counter( nullptr )
    {
        InitMove( Other.Ptr, Other.Counter );

        Other.Ptr = nullptr;
        Other.Counter = nullptr;
    }

    /* Move constructor */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TPointerReferencedStorage( TPointerReferencedStorage<OtherType, OtherDeleterType>&& Other ) noexcept
        : DeleterType( Move( Other ) )
        , Ptr( nullptr )
        , Counter( nullptr )
    {
        InitMove( static_cast<ElementType*>(Other.Ptr), Other.Counter );

        Other.Ptr = nullptr;
        Other.Counter = nullptr;
    }

    /* Default constructor, the destruction of resources should be handled by the user of the class*/
    ~TPointerReferencedStorage() = default;

    /* Initialized a new counter if needed and adds a strong reference */
    FORCEINLINE void InitStrong( ElementType* NewPtr, CPointerReferenceCounter* NewCounter ) noexcept
    {
        // If the pointer is nullptr, we do not care about the counter
        Ptr = NewPtr;
        if ( Ptr )
        {
            if ( NewCounter )
            {
                Counter = NewCounter;
            }
            else
            {
                Counter = new CPointerReferenceCounter();
            }

            Assert( Counter != nullptr );
            Counter->AddStrongRef();
        }
        else
        {
            Counter = nullptr;
        }
    }

    /* Adds a weak ref */
    FORCEINLINE void InitWeak( ElementType* NewPtr, CPointerReferenceCounter* NewCounter ) noexcept
    {
        // If the pointer is nullptr, we do not care about the counter...
        Ptr = NewPtr;
        if ( Ptr )
        {
            // ...however, if we init a weak reference then there has to be a counter aswell, since weakrefs require a strong
            Counter = NewCounter;
            Assert( Counter != nullptr );
            Counter->AddWeakRef();
        }
        else
        {
            Counter = nullptr;
        }
    }

    /* Initialize the storage by a "move", does not add or remove any weak or strong references */
    FORCEINLINE void InitMove( ElementType* NewPtr, CPointerReferenceCounter* NewCounter ) noexcept
    {
        // If the pointer is nullptr, we do not care about the counter...
        Ptr = NewPtr;
        if ( Ptr )
        {
            // ...However if the Ptr is valid then the counter also has to be, otherwise it is not a valid move
            Counter = NewCounter;
            Assert( Counter != nullptr );
        }
        else
        {
            Counter = nullptr;
        }
    }

    /* Releases a strong ref and deletes the pointer if the strong refcount is zero, if the weak refcount is also zero the counter is deleted */
    FORCEINLINE void ReleaseStrong() noexcept
    {
        Assert( Counter != nullptr );
        Counter->ReleaseStrongRef();

        CounterType StrongRefs = Counter->GetStrongRefCount();
        if ( StrongRefs < 1 )
        {
            DeleteElement( Ptr );

            CounterType WeakRefs = Counter->GetWeakRefCount();
            if ( WeakRefs < 1 )
            {
                delete Counter;
                Counter = nullptr;
            }
        }
    }

    /* Relases a weak reference and if all references are gone, then delete the counter */
    FORCEINLINE void ReleaseWeak() noexcept
    {
        Assert( Counter != nullptr );
        Counter->ReleaseWeakRef();

        CounterType NumStrongRefs = Counter->GetStrongRefCount();
        CounterType NumWeakRefs   = Counter->GetWeakRefCount();
        if ( (NumStrongRefs < 1) && (NumWeakRefs < 1) )
        {
            delete Counter;
            Counter = nullptr;
        }
    }

    /* Resets to an empty storage without deleting the pointers. Used for Move pointers of other type. */
    FORCEINLINE void Reset() noexcept
    {
        Ptr     = nullptr;
        Counter = nullptr;
    }

    /* Swaps to storages */
    FORCEINLINE void Swap( TPointerReferencedStorage& Other ) noexcept
    {
        ElementType* TempPtr = Ptr;
        CPointerReferenceCounter* TempCounter = Counter;

        Ptr = Other.Ptr;
        Counter = Other.Counter;

        Other.Ptr = TempPtr;
        Other.Counter = TempCounter;
    }

    /* Returns the pointer */
    FORCEINLINE ElementType* GetPointer() const noexcept
    {
        return Ptr;
    }

    /* Returns the address of the raw pointer */
    FORCEINLINE ElementType* const* GetAddressOfPointer() const noexcept
    {
        return &Ptr;
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
    FORCEINLINE TPointerReferencedStorage& operator=( TPointerReferencedStorage&& RHS ) noexcept
    {
        TPointerReferencedStorage( Move( RHS ) ).Swap( *this );
        return *this;
    }

    /* Assign another to this */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value, TPointerReferencedStorage&>::Type
        operator=( TPointerReferencedStorage<OtherType, OtherDeleterType>&& RHS ) noexcept
    {
        TPointerReferencedStorage( Move( RHS ) ).Swap( *this );
        return *this;
    }

private:
    ElementType* Ptr;
    CPointerReferenceCounter* Counter;
};

/* Forward declaration of TWeakPtr */
template<typename T, typename DeleterType>
class TWeakPtr;

/* TSharedPtr - RefCounted pointer, similar to std::shared_ptr */
template<typename T, typename DeleterType = TDefaultDelete<T>>
class TSharedPtr
{
public:
    typedef typename TRemoveExtent<T>::Type                     ElementType;
    typedef TPointerReferencedStorage<ElementType, DeleterType> PointerStorage;
    typedef typename PointerStorage::CounterType                CounterType;
    typedef int32                                               SizeType;

    /* Enables conversion betweem the two classes */
    template<typename OtherType, typename OtherDeleterType>
    friend class TWeakPtr;

    template<typename OtherType, typename OtherDeleterType>
    friend class TSharedPtr;

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
        Storage.InitStrong( InPtr, nullptr );
    }

    /* Constructor creating from a raw pointer from a convertible type, and the container takes ownership at this point */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>>
    FORCEINLINE explicit TSharedPtr( typename TRemoveExtent<OtherType>::Type* InPtr ) noexcept
        : Storage()
    {
        Storage.InitStrong( static_cast<ElementType*>(InPtr), nullptr );
    }

    /* Copy-constructor */
    FORCEINLINE TSharedPtr( const TSharedPtr& Other ) noexcept
        : Storage()
    {
        Storage.InitStrong( Other.Get(), Other.GetCounter() );
    }

    /* Constructor copy from another type */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>>
    FORCEINLINE TSharedPtr( const TSharedPtr<OtherType, OtherDeleterType>& Other ) noexcept
        : Storage()
    {
        Storage.InitStrong( static_cast<ElementType*>(Other.Get()), Other.GetCounter() );
    }

    /* Move constructor */
    FORCEINLINE TSharedPtr( TSharedPtr&& Other ) noexcept
        : Storage( Move( Other.Storage ) )
    {
    }

    /* Move constructor from with another type */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>>
    FORCEINLINE TSharedPtr( TSharedPtr<OtherType, OtherDeleterType>&& Other ) noexcept
        : Storage( Move( Other.Storage ) )
    {
    }

    /* Copy constructor, were the counter is copied, but the pointer is taken from the raw-pointer, this enables casting */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE explicit TSharedPtr( const TSharedPtr<OtherType, OtherDeleterType>& Other, ElementType* InPtr ) noexcept
        : Storage()
    {
        Storage.InitStrong( InPtr, Other.GetCounter() );
    }

    /* Move constructor, were the counter is copied, but the pointer is taken from the raw-pointer, this enables casting */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE explicit TSharedPtr( TSharedPtr<OtherType, OtherDeleterType>&& Other, ElementType* InPtr ) noexcept
        : Storage()
    {
        Storage.InitMove(InPtr, Other.GetCounter());
        Other.Storage.Reset();
    }

    /* Constructor that creates a sharedptr from a weakptr */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>>
    FORCEINLINE explicit TSharedPtr( const TWeakPtr<OtherType, OtherDeleterType>& Other ) noexcept
        : Storage()
    {
        Storage.InitStrong( static_cast<ElementType*>(Other.Get()), Other.GetCounter() );
    }

    /* Constructor that creates a sharedptr from a uniqueptr */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>>
    FORCEINLINE TSharedPtr( TUniquePtr<OtherType, OtherDeleterType>&& Other ) noexcept
        : Storage()
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
    FORCEINLINE typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>::Type
        Reset( typename TRemoveExtent<OtherType>::Type* NewPtr = nullptr ) noexcept
    {
        Reset( static_cast<ElementType*>(NewPtr) );
    }

    /* Swaps the contents of this and another container */
    FORCEINLINE void Swap( TSharedPtr& Other ) noexcept
    {
        PointerStorage Temp = Move( Storage );
        Storage = Move( Other.Storage );
        Other.Storage = Move( Temp );
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
        return Storage.GetPointer();
    }

    /* Returns the address of the raw pointer */
    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return Storage.GetAddressOfPointer();
    }

    /* Returns the number of strong references */
    FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        return Storage.GetStrongRefCount();
    }

    /* Return the number of weak references */
    FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        return Storage.GetWeakRefCount();
    }

    /* Return the dereferenced pointer */
    FORCEINLINE ElementType& Dereference() const noexcept
    {
        Assert( IsValid() );
        return *Get();
    }

    /* Return the raw pointer */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }

    /* Dereference the pointer */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    /* Get the address of the pointer */
    FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /* Retrive element at a certain index */
    template<typename U = T>
    FORCEINLINE typename TEnableIf<TAnd<TIsSame<U, T>, TIsUnboundedArray<U>>::Value, typename TAddLValueReference<typename TRemoveExtent<U>::Type>::Type>::Type operator[]( SizeType Index ) const noexcept
    {
        Assert( IsValid() );
        return Get()[Index];
    }

    /* Cheack weather the pointer is nullptr or not */
    FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /* Copy assignment */
    FORCEINLINE TSharedPtr& operator=( const TSharedPtr& RHS ) noexcept
    {
        TSharedPtr( RHS ).Swap( *this );
        return *this;
    }

    /* Move assignment */
    FORCEINLINE TSharedPtr& operator=( TSharedPtr&& RHS ) noexcept
    {
        TSharedPtr( Move( RHS ) ).Swap( *this );
        return *this;
    }

    /* Copy assignment */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value, typename TAddReference<TSharedPtr>::LValue>::Type
        operator=( const TSharedPtr<OtherType, OtherDeleterType>& RHS ) noexcept
    {
        TSharedPtr( RHS ).Swap( *this );
        return *this;
    }

    /* Move assignment */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value, typename TAddReference<TSharedPtr>::LValue>::Type
        operator=( TSharedPtr<OtherType, OtherDeleterType>&& RHS ) noexcept
    {
        TSharedPtr( Move( RHS ) ).Swap( *this );
        return *this;
    }

    /* Assignment from raw */
    FORCEINLINE TSharedPtr& operator=( ElementType* RHS ) noexcept
    {
        TSharedPtr( RHS ).Swap( *this );
        return *this;
    }

    /* Assignment from nullptr */
    FORCEINLINE TSharedPtr& operator=( NullptrType ) noexcept
    {
        TSharedPtr().Swap( *this );
        return *this;
    }

private:
    FORCEINLINE CPointerReferenceCounter* GetCounter() const noexcept
    {
        return Storage.GetCounter();
    }

    PointerStorage Storage;
};

/* Check the equallity between sharedptr and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==( const TSharedPtr<T>& LHS, U* RHS ) noexcept
{
    return (LHS.Get() == RHS);
}

/* Check the equallity between sharedptr and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==( T* LHS, const TSharedPtr<U>& RHS ) noexcept
{
    return (LHS == RHS.Get());
}

/* Check the inequallity between sharedptr and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TSharedPtr<T>& LHS, U* RHS ) noexcept
{
    return (LHS.Get() != RHS);
}

/* Check the inequallity between sharedptr and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=( T* LHS, const TSharedPtr<U>& RHS ) noexcept
{
    return (LHS != RHS.Get());
}

/* Check the equallity between sharedptrs */
template<typename T, typename U>
FORCEINLINE bool operator==( const TSharedPtr<T>& LHS, const TSharedPtr<U>& RHS ) noexcept
{
    return (LHS.Get() == RHS.Get());
}

/* Check the inequallity between sharedptrs */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TSharedPtr<T>& LHS, const TSharedPtr<U>& RHS ) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/* Check the equallity between sharedptr and a nullptr */
template<typename T>
FORCEINLINE bool operator==( const TSharedPtr<T>& LHS, NullptrType ) noexcept
{
    return (LHS.Get() == nullptr);
}

/* Check the equallity between sharedptr and a nullptr */
template<typename T>
FORCEINLINE bool operator==( NullptrType, const TSharedPtr<T>& RHS ) noexcept
{
    return (nullptr == RHS.Get());
}

/* Check the equallity between sharedptr and a nullptr */
template<typename T>
FORCEINLINE bool operator!=( const TSharedPtr<T>& LHS, NullptrType ) noexcept
{
    return (LHS.Get() != nullptr);
}

/* Check the equallity between sharedptr and a nullptr */
template<typename T>
FORCEINLINE bool operator!=( NullptrType, const TSharedPtr<T>& RHS ) noexcept
{
    return (nullptr != RHS.Get());
}

/* Check the equallity between sharedptr and uniqueptr */
template<typename T, typename U>
FORCEINLINE bool operator==( const TSharedPtr<T>& LHS, const TUniquePtr<U>& RHS ) noexcept
{
    return (LHS.Get() == RHS.Get());
}

/* Check the equallity between sharedptr and uniqueptr */
template<typename T, typename U>
FORCEINLINE bool operator==( const TUniquePtr<T>& LHS, const TSharedPtr<U>& RHS ) noexcept
{
    return (LHS.Get() == RHS.Get());
}

/* Check the inequallity between sharedptr and uniqueptr */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TSharedPtr<T>& LHS, const TUniquePtr<U>& RHS ) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/* Check the inequallity between sharedptr and uniqueptr */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TUniquePtr<T>& LHS, const TSharedPtr<U>& RHS ) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/* TWeakPtr - Weak Pointer for scalar types, similar to std::weak_ptr */
template<typename T, typename DeleterType = TDefaultDelete<T>>
class TWeakPtr
{
public:
    typedef typename TRemoveExtent<T>::Type                     ElementType;
    typedef TPointerReferencedStorage<ElementType, DeleterType> PointerStorage;
    typedef typename PointerStorage::CounterType                CounterType;
    typedef int32                                               SizeType;

    /* Enables conversion betweem the two classes */
    template<typename OtherType, typename OtherDeleterType>
    friend class TSharedPtr;

    template<typename OtherType, typename OtherDeleterType>
    friend class TWeakPtr;

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
        : Storage( Move( Other.Storage ) )
    {
    }

    /* Copy construct from convertible type */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>>
    FORCEINLINE TWeakPtr( const TWeakPtr<OtherType, OtherDeleterType>& Other ) noexcept
        : Storage()
    {
        Storage.InitWeak( static_cast<ElementType*>(Other.Get()), Other.GetCounter() );
    }

    /* Move construct from convertible type */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>>
    FORCEINLINE TWeakPtr( TWeakPtr<OtherType, OtherDeleterType>&& Other ) noexcept
        : Storage( Move( Other.Storage ) )
    {
    }

    /* Construct from shared */
    FORCEINLINE TWeakPtr( const TSharedPtr<T, DeleterType>& Other ) noexcept
        : Storage()
    {
        Storage.InitWeak( Other.Get(), Other.GetCounter() );
    }

    /* Construct from shared */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>>
    FORCEINLINE TWeakPtr( const TSharedPtr<OtherType, OtherDeleterType>& Other ) noexcept
        : Storage()
    {
        Storage.InitWeak( static_cast<ElementType*>(Other.Get()), Other.GetCounter() );
    }

    FORCEINLINE ~TWeakPtr()
    {
        Reset();
    }

    /* Reset the pointer and set it to a optional new pointer */
    FORCEINLINE void Reset( ElementType* NewPtr = nullptr ) noexcept
    {
        if ( Get() != NewPtr )
        {
            Storage.ReleaseWeak();
            Storage.InitWeak( NewPtr, nullptr );
        }
    }

    /* Reset the pointer and set it to a optional new pointer */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type Reset( OtherType* NewPtr = nullptr ) noexcept
    {
        Reset( static_cast<ElementType*>(NewPtr) );
    }

    /* Swaps the contents of this and another container */
    FORCEINLINE void Swap( TWeakPtr& Other ) noexcept
    {
        PointerStorage Temp = Move( Storage );
        Storage = Move( Other.Storage );
        Other.Storage = Move( Temp );
    }

    /* Checks weather there are any strong references left */
    FORCEINLINE bool IsExpired() const noexcept
    {
        return (GetStrongRefCount() < 1);
    }

    /* Creates a shared pointer from this weakptr */
    FORCEINLINE TSharedPtr<T> MakeShared() noexcept
    {
        return TSharedPtr<T>( *this );
    }

    /* Checks weather the strong reference count is one */
    FORCEINLINE bool IsUnique() const noexcept
    {
        return (Storage.GetStrongRefCount() == 1);
    }

    /* Cheacks weater the pointer is nullptr or not */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Get() != nullptr) && !IsExpired();
    }

    /* Return the raw pointer */
    FORCEINLINE ElementType* Get() const noexcept
    {
        return Storage.GetPointer();
    }

    /* Returns the address of the raw pointer */
    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return Storage.GetAddressOfPointer();
    }

    /* Returns the number of strong references */
    FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        return Storage.GetStrongRefCount();
    }

    /* Return the number of weak references */
    FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        return Storage.GetWeakRefCount();
    }

    /* Return the dereferenced pointer */
    FORCEINLINE ElementType& Dereference() const noexcept
    {
        Assert( IsValid() );
        return *Get();
    }

    /* Return the raw pointer */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }

    /* Dereference the pointer */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    /* Get the address of the pointer */
    FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /* Retrive element at a certain index */
    template<typename U = T>
    FORCEINLINE typename TEnableIf<TAnd<TIsSame<U, T>, TIsUnboundedArray<U>>::Value, typename TAddLValueReference<typename TRemoveExtent<U>::Type>::Type>::Type operator[]( SizeType Index ) const noexcept
    {
        Assert( IsValid() );
        return Get()[Index];
    }

    /* Cheack weather the pointer is nullptr or not */
    FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /* Copy assignment */
    FORCEINLINE TWeakPtr& operator=( const TWeakPtr& RHS ) noexcept
    {
        TWeakPtr( RHS ).Swap( *this );
        return *this;
    }

    /* Move assignment */
    FORCEINLINE TWeakPtr& operator=( TWeakPtr&& RHS ) noexcept
    {
        TWeakPtr( Move( RHS ) ).Swap( *this );
        return *this;
    }

    /* Copy assignment */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, typename TAddReference<TWeakPtr>::LValue>::Type operator=( const TWeakPtr<OtherType, OtherDeleterType>& RHS ) noexcept
    {
        TWeakPtr( RHS ).Swap( *this );
        return *this;
    }

    /* Move assignment */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, typename TAddReference<TWeakPtr>::LValue>::Type operator=( TWeakPtr<OtherType, OtherDeleterType>&& RHS ) noexcept
    {
        TWeakPtr( Move( RHS ) ).Swap( *this );
        return *this;
    }

    /* Assign from pointer */
    FORCEINLINE TWeakPtr& operator=( ElementType* RHS ) noexcept
    {
        TWeakPtr( RHS ).Swap( *this );
        return *this;
    }

    /* Assign from nullptr */
    FORCEINLINE TWeakPtr& operator=( NullptrType ) noexcept
    {
        TWeakPtr().Swap( *this );
        return *this;
    }

private:
    FORCEINLINE CPointerReferenceCounter* GetCounter() const noexcept
    {
        return Storage.GetCounter();
    }

    PointerStorage Storage;
};

/* Check the equallity between weakptr and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==( const TWeakPtr<T>& LHS, U* RHS ) noexcept
{
    return (LHS.Get() == RHS);
}

/* Check the equallity between weakptr and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator==( T* LHS, const TWeakPtr<U>& RHS ) noexcept
{
    return (LHS == RHS.Get());
}

/* Check the inequallity between weakptr and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TWeakPtr<T>& LHS, U* RHS ) noexcept
{
    return (LHS.Get() != RHS);
}

/* Check the inequallity between weakptr and a raw pointer */
template<typename T, typename U>
FORCEINLINE bool operator!=( T* LHS, const TWeakPtr<U>& RHS ) noexcept
{
    return (LHS != RHS.Get());
}

/* Check the equallity between weakptrs */
template<typename T, typename U>
FORCEINLINE bool operator==( const TWeakPtr<T>& LHS, const TWeakPtr<U>& RHS ) noexcept
{
    return (LHS.Get() == RHS.Get());
}

/* Check the inequallity between weakptrs */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TWeakPtr<T>& LHS, const TWeakPtr<U>& RHS ) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/* Check the equallity between weakptr and nullptr */
template<typename T>
FORCEINLINE bool operator==( const TWeakPtr<T>& LHS, NullptrType ) noexcept
{
    return (LHS.Get() == nullptr);
}

/* Check the equallity between weakptr and nullptr */
template<typename T>
FORCEINLINE bool operator==( NullptrType, const TWeakPtr<T>& RHS ) noexcept
{
    return (nullptr == RHS.Get());
}

/* Check the inequallity between weakptr and nullptr */
template<typename T>
FORCEINLINE bool operator!=( const TWeakPtr<T>& LHS, NullptrType ) noexcept
{
    return (LHS.Get() != nullptr);
}

/* Check the inequallity between weakptr and nullptr */
template<typename T>
FORCEINLINE bool operator!=( NullptrType, const TWeakPtr<T>& RHS ) noexcept
{
    return (nullptr != RHS.Get());
}

/* Check the equallity between weakptr and sharedptr */
template<typename T, typename U>
FORCEINLINE bool operator==( const TWeakPtr<T>& LHS, const TSharedPtr<U>& RHS ) noexcept
{
    return (LHS.Get() == RHS.Get());
}

/* Check the equallity between weakptr and sharedptr */
template<typename T, typename U>
FORCEINLINE bool operator==( const TSharedPtr<T>& LHS, const TWeakPtr<U>& RHS ) noexcept
{
    return (LHS.Get() == RHS.Get());
}

/* Check the inequallity between weakptr and sharedptr */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TWeakPtr<T>& LHS, const TSharedPtr<U>& RHS ) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/* Check the inequallity between weakptr and sharedptr */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TSharedPtr<T>& LHS, const TWeakPtr<U>& RHS ) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/* Check the equallity between weakptr and uniqueptr */
template<typename T, typename U>
FORCEINLINE bool operator==( const TWeakPtr<T>& LHS, const TUniquePtr<U>& RHS ) noexcept
{
    return (LHS.Get() == RHS.Get());
}

/* Check the equallity between weakptr and uniqueptr */
template<typename T, typename U>
FORCEINLINE bool operator==( const TUniquePtr<T>& LHS, const TWeakPtr<U>& RHS ) noexcept
{
    return (LHS.Get() == RHS.Get());
}

/* Check the inequallity between weakptr and uniqueptr */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TWeakPtr<T>& LHS, const TUniquePtr<U>& RHS ) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/* Check the inequallity between weakptr and uniqueptr */
template<typename T, typename U>
FORCEINLINE bool operator!=( const TUniquePtr<T>& LHS, const TWeakPtr<U>& RHS ) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/* MakeShared - Creates a new object together with a SharedPtr */
template<typename T, typename... ArgTypes>
FORCEINLINE typename TEnableIf<!TIsArray<T>::Value, TSharedPtr<T>>::Type MakeShared( ArgTypes&&... Args ) noexcept
{
    typedef typename TRemoveExtent<T>::Type Type;

    Type* RefCountedPtr = new Type( Forward<ArgTypes>( Args )... );
    return TSharedPtr<T>( RefCountedPtr );
}

template<typename T>
FORCEINLINE typename TEnableIf<TIsArray<T>::Value, TSharedPtr<T>>::Type MakeShared( uint32 Size ) noexcept
{
    typedef typename TRemoveExtent<T>::Type Type;

    Type* RefCountedPtr = new Type[Size];
    return TSharedPtr<T>( RefCountedPtr );
}

/* Casting functions */

/* static_cast*/
template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type StaticCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    typedef typename TRemoveExtent<T0>::Type Type;

    Type* RawPointer = static_cast<Type*>(Pointer.Get());
    return TSharedPtr<T0>( Pointer, RawPointer );
}

template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type StaticCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    typedef typename TRemoveExtent<T0>::Type Type;

    Type* RawPointer = static_cast<Type*>(Pointer.Get());
    return TSharedPtr<T0>( Move( Pointer ), RawPointer );
}

/* const_cast */
template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type ConstCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    typedef typename TRemoveExtent<T0>::Type Type;

    Type* RawPointer = const_cast<Type*>(Pointer.Get());
    return TSharedPtr<T0>( Move( Pointer ), RawPointer );
}

template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type ConstCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    typedef typename TRemoveExtent<T0>::Type Type;

    Type* RawPointer = const_cast<Type*>(Pointer.Get());
    return TSharedPtr<T0>( Move( Pointer ), RawPointer );
}

/* reinterpret_cast */
template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type ReinterpretCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    typedef typename TRemoveExtent<T0>::Type Type;

    Type* RawPointer = reinterpret_cast<Type*>(Pointer.Get());
    return TSharedPtr<T0>( Move( Pointer ), RawPointer );
}

template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type ReinterpretCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    typedef typename TRemoveExtent<T0>::Type Type;

    Type* RawPointer = reinterpret_cast<Type*>(Pointer.Get());
    return TSharedPtr<T0>( Move( Pointer ), RawPointer );
}

/* dynamic_cast */
template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type DynamicCast( const TSharedPtr<T1>& Pointer ) noexcept
{
    typedef typename TRemoveExtent<T0>::Type Type;

    Type* RawPointer = dynamic_cast<Type*>(Pointer.Get());
    return TSharedPtr<T0>( Move( Pointer ), RawPointer );
}

template<typename T0, typename T1>
FORCEINLINE typename TEnableIf<TIsArray<T0>::Value == TIsArray<T1>::Value, TSharedPtr<T0>>::Type DynamicCast( TSharedPtr<T1>&& Pointer ) noexcept
{
    typedef typename TRemoveExtent<T0>::Type Type;

    Type* RawPointer = dynamic_cast<Type*>(Pointer.Get());
    return TSharedPtr<T0>( Move( Pointer ), RawPointer );
}

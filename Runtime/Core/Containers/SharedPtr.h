#pragma once
#include "UniquePtr.h"

#include "Core/Templates/IsConvertible.h"
#include "Core/Templates/IsArray.h"
#include "Core/Templates/RemoveExtent.h"
#include "Core/Templates/AddReference.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/And.h"
#include "Core/Threading/AtomicInt.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CPointerReferenceCounter - Counting references in TWeak- and TSharedPtr

class CPointerReferenceCounter
{
public:

    using CounterType = AtomicInt32::Type;

    FORCEINLINE CPointerReferenceCounter() noexcept
        : NumWeakRefs(0)
        , NumStrongRefs(0)
    {
    }

    FORCEINLINE CounterType AddWeakRef() noexcept
    {
        return NumWeakRefs++;
    }

    FORCEINLINE CounterType AddStrongRef() noexcept
    {
        return NumStrongRefs++;
    }

    FORCEINLINE CounterType ReleaseWeakRef() noexcept
    {
        return NumWeakRefs--;
    }

    FORCEINLINE CounterType ReleaseStrongRef() noexcept
    {
        return NumStrongRefs--;
    }

    FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        return NumWeakRefs.Load();
    }

    FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        return NumStrongRefs.Load();
    }

private:
    AtomicInt32 NumWeakRefs;
    AtomicInt32 NumStrongRefs;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper object for shared ref counter

template<typename T, typename DeleterType = TDefaultDelete<T>>
class TPointerReferencedStorage : private DeleterType
{
    using Super = DeleterType;

public:
    using ElementType = typename TRemoveExtent<T>::Type;
    using CounterType = CPointerReferenceCounter::CounterType;

    TPointerReferencedStorage(const TPointerReferencedStorage&) = delete;
    TPointerReferencedStorage& operator=(const TPointerReferencedStorage&) = delete;

    template<typename OtherType, typename OtherDeleterType>
    friend class TPointerReferencedStorage;

    FORCEINLINE TPointerReferencedStorage() noexcept
        : DeleterType()
        , Ptr(nullptr)
        , Counter(nullptr)
    {
    }

    FORCEINLINE TPointerReferencedStorage(TPointerReferencedStorage&& Other) noexcept
        : DeleterType(Move(Other))
        , Ptr(nullptr)
        , Counter(nullptr)
    {
        InitMove(Other.Ptr, Other.Counter);

        Other.Ptr = nullptr;
        Other.Counter = nullptr;
    }

    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TPointerReferencedStorage(TPointerReferencedStorage<OtherType, OtherDeleterType>&& Other) noexcept
        : DeleterType(Move(Other))
        , Ptr(nullptr)
        , Counter(nullptr)
    {
        InitMove(Other.Ptr, Other.Counter);

        Other.Ptr = nullptr;
        Other.Counter = nullptr;
    }

    ~TPointerReferencedStorage() = default;

    FORCEINLINE void InitStrong(ElementType* NewPtr, CPointerReferenceCounter* NewCounter) noexcept
    {
        // If the pointer is nullptr, we do not care about the counter
        Ptr = NewPtr;
        if (Ptr)
        {
            if (NewCounter)
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

    FORCEINLINE void InitWeak(ElementType* NewPtr, CPointerReferenceCounter* NewCounter) noexcept
    {
        // If the pointer is nullptr, we do not care about the counter...
        Ptr = NewPtr;
        if (Ptr)
        {
            // ...however, if we init a weak reference then there has to be a counter as well, since weak-refs require a strong
            Counter = NewCounter;
            Assert(Counter != nullptr);
            Counter->AddWeakRef();
        }
        else
        {
            Counter = nullptr;
        }
    }

    FORCEINLINE void InitMove(ElementType* NewPtr, CPointerReferenceCounter* NewCounter) noexcept
    {
        // If the pointer is nullptr, we do not care about the counter...
        Ptr = NewPtr;
        if (Ptr)
        {
            // ...However if the Ptr is valid then the counter also has to be, otherwise it is not a valid move
            Counter = NewCounter;
            Assert(Counter != nullptr);
        }
        else
        {
            Counter = nullptr;
        }
    }

    /** Releases a strong ref and deletes the pointer if the strong refcount is zero, if the weak refcount is also zero the counter is deleted */
    FORCEINLINE void ReleaseStrong() noexcept
    {
        Assert(Counter != nullptr);
        Counter->ReleaseStrongRef();

        CounterType StrongRefs = Counter->GetStrongRefCount();
        if (StrongRefs < 1)
        {
            Super::DeleteElement(Ptr);

            CounterType WeakRefs = Counter->GetWeakRefCount();
            if (WeakRefs < 1)
            {
                delete Counter;
                Counter = nullptr;
            }
        }
    }

    /** Releases a weak reference and if all references are gone, then delete the counter */
    FORCEINLINE void ReleaseWeak() noexcept
    {
        Assert(Counter != nullptr);
        Counter->ReleaseWeakRef();

        CounterType NumStrongRefs = Counter->GetStrongRefCount();
        CounterType NumWeakRefs = Counter->GetWeakRefCount();
        if ((NumStrongRefs < 1) && (NumWeakRefs < 1))
        {
            delete Counter;
            Counter = nullptr;
        }
    }

    FORCEINLINE void Reset() noexcept
    {
        Ptr = nullptr;
        Counter = nullptr;
    }

    FORCEINLINE void Swap(TPointerReferencedStorage& Other) noexcept
    {
        ElementType* TempPtr = Ptr;
        CPointerReferenceCounter* TempCounter = Counter;

        Ptr = Other.Ptr;
        Counter = Other.Counter;

        Other.Ptr = TempPtr;
        Other.Counter = TempCounter;
    }

    FORCEINLINE ElementType* GetPointer() const noexcept
    {
        return Ptr;
    }

    FORCEINLINE ElementType* const* GetAddressOfPointer() const noexcept
    {
        return &Ptr;
    }

    FORCEINLINE CPointerReferenceCounter* GetCounter() const noexcept
    {
        return Counter;
    }

    FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        Assert(Counter != nullptr);
        return Counter->GetWeakRefCount();
    }

    FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        Assert(Counter != nullptr);
        return Counter->GetStrongRefCount();
    }

    FORCEINLINE TPointerReferencedStorage& operator=(TPointerReferencedStorage&& Rhs) noexcept
    {
        TPointerReferencedStorage(Move(Rhs)).Swap(*this);
        return *this;
    }

    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value, TPointerReferencedStorage&>::Type
        operator=(TPointerReferencedStorage<OtherType, OtherDeleterType>&& Rhs) noexcept
    {
        TPointerReferencedStorage(Move(Rhs)).Swap(*this);
        return *this;
    }

private:
    ElementType* Ptr;
    CPointerReferenceCounter* Counter;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Forward declaration of TWeakPtr

template<typename T, typename DeleterType>
class TWeakPtr;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TSharedPtr - RefCounted pointer, similar to std::shared_ptr

template<typename T, typename DeleterType = TDefaultDelete<T>>
class TSharedPtr
{
public:

    using ElementType = typename TRemoveExtent<T>::Type;
    using PointerStorage = TPointerReferencedStorage<ElementType, DeleterType>;
    using CounterType = typename PointerStorage::CounterType;
    using SizeType = int32;

    template<typename OtherType, typename OtherDeleterType>
    friend class TWeakPtr;

    template<typename OtherType, typename OtherDeleterType>
    friend class TSharedPtr;

    /**
     * Default constructor 
     */
    FORCEINLINE TSharedPtr() noexcept
        : Storage()
    {
    }

    /**
     * Constructor setting both counter and pointer to nullptr 
     */
    FORCEINLINE TSharedPtr(NullptrType) noexcept
        : Storage()
    {
    }

    /**
     * Constructor that initializes a new SharedPtr from a raw-pointer
     * 
     * @param InPointer: Raw pointer to store
     */
    FORCEINLINE explicit TSharedPtr(ElementType* InPointer) noexcept
        : Storage()
    {
        Storage.InitStrong(InPointer, nullptr);
    }

    /**
     * Constructor that initializes a new SharedPtr from a raw-pointer of a convertible type
     *
     * @param InPointer: Raw pointer to store
     */
    template<typename OtherType, typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>::Type>
    FORCEINLINE explicit TSharedPtr(typename TRemoveExtent<OtherType>::Type* InPointer) noexcept
        : Storage()
    {
        Storage.InitStrong(static_cast<ElementType*>(InPointer), nullptr);
    }

    /**
     * Copy-constructor 
     * 
     * @param Other: SharedPtr to copy
     */
    FORCEINLINE TSharedPtr(const TSharedPtr& Other) noexcept
        : Storage()
    {
        Storage.InitStrong(Other.Get(), Other.GetCounter());
    }

    /**
     * Copy-constructor that copies from a convertible type
     *
     * @param Other: SharedPtr to copy
     */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedPtr(const TSharedPtr<OtherType, OtherDeleterType>& Other) noexcept
        : Storage()
    {
        Storage.InitStrong(static_cast<ElementType*>(Other.Get()), Other.GetCounter());
    }

    /**
     * Move-constructor
     *
     * @param Other: SharedPtr to move
     */
    FORCEINLINE TSharedPtr(TSharedPtr&& Other) noexcept
        : Storage(Move(Other.Storage))
    {
    }

    /**
     * Move-constructor that copies from a convertible type
     *
     * @param Other: SharedPtr to move
     */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedPtr(TSharedPtr<OtherType, OtherDeleterType>&& Other) noexcept
        : Storage(Move(Other.Storage))
    {
    }

    /**
     * Copy constructor that copy the counter, but the pointer is taken from the raw-pointer to enable casting 
     * 
     * @param Other: Container to take the counter from
     * @param InPointer: Raw pointer to store
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE explicit TSharedPtr(const TSharedPtr<OtherType, OtherDeleterType>& Other, ElementType* InPointer) noexcept
        : Storage()
    {
        Storage.InitStrong(InPointer, Other.GetCounter());
    }

    /**
     * Move constructor that move the counter, but the pointer is taken from the raw-pointer to enable casting
     *
     * @param Other: Container to take the counter from
     * @param InPointer: Raw pointer to store
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE explicit TSharedPtr(TSharedPtr<OtherType, OtherDeleterType>&& Other, ElementType* InPointer) noexcept
        : Storage()
    {
        Storage.InitMove(InPointer, Other.GetCounter());
        Other.Storage.Reset();
    }

    /**
     * Constructor that creates a SharedPtr from a WeakPtr 
     * 
     * @param Other: WeakPtr to take counter and pointer from
     */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>::Type>
    FORCEINLINE explicit TSharedPtr(const TWeakPtr<OtherType, OtherDeleterType>& Other) noexcept
        : Storage()
    {
        Storage.InitStrong(static_cast<ElementType*>(Other.Get()), Other.GetCounter());
    }

    /**
     * Constructor that creates a SharedPtr from a UniquePtr
     *
     * @param Other: UniquePtr to take counter and pointer from
     */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedPtr(TUniquePtr<OtherType, OtherDeleterType>&& Other) noexcept
        : Storage()
    {
        Storage.InitStrong(static_cast<ElementType*>(Other.Release()), nullptr);
    }

    /**
     * Destructor
     */
    FORCEINLINE ~TSharedPtr() noexcept
    {
        Reset();
    }

    /**
     * Reset the pointer and set it to a optional new pointer 
     * 
     * @param NewPointer: New pointer to store
     */
    FORCEINLINE void Reset(ElementType* NewPointer = nullptr) noexcept
    {
        if (Get() != NewPointer)
        {
            Storage.ReleaseStrong();
            Storage.InitStrong(NewPointer, nullptr);
        }
    }

    /**
     * Reset the pointer and set it to a optional new pointer of convertible type
     *
     * @param NewPointer: New pointer to store
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>::Type
        Reset(typename TRemoveExtent<OtherType>::Type* NewPointer = nullptr) noexcept
    {
        Reset(static_cast<ElementType*>(NewPointer));
    }

    /**
     * Swaps the contents of this and another container 
     * 
     * @param Other: SharedPtr to swap with 
     */
    FORCEINLINE void Swap(TSharedPtr& Other) noexcept
    {
        PointerStorage Temp = Move(Storage);
        Storage = Move(Other.Storage);
        Other.Storage = Move(Temp);
    }

    /**
     * Check weather the strong reference count is one
     * 
     * @return: Returns true if the strong reference count is equal to one
     */
    FORCEINLINE bool IsUnique() const noexcept
    {
        return (Storage.GetStrongRefCount() == 1);
    }

    /**
     * Check weather the pointer is nullptr or not
     *
     * @return: Returns true if the pointer is not nullptr
     */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Get() != nullptr);
    }

    /**
     * Return the raw pointer 
     * 
     * @return: Returns the stored pointer
     */
    FORCEINLINE ElementType* Get() const noexcept
    {
        return Storage.GetPointer();
    }

    /**
     * Return the address of the raw pointer
     *
     * @return: Returns the address of the stored pointer
     */
    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return Storage.GetAddressOfPointer();
    }

    /**
     * Returns the number of strong references 
     * 
     * @return: The number of strong references
     */
    FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        return Storage.GetStrongRefCount();
    }

    /**
     * Returns the number of weak references
     *
     * @return: The number of weak references
     */
    FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        return Storage.GetWeakRefCount();
    }

    /**
     * Dereference the pointer
     * 
     * @return: A reference to the object pointer to by the stored pointer
     */
    FORCEINLINE ElementType& Dereference() const noexcept
    {
        Assert(IsValid());
        return *Get();
    }

    /**
     * Return the raw pointer
     *
     * @return: Returns the stored pointer
     */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }
    /**
     * Dereference the pointer
     *
     * @return: A reference to the object pointer to by the stored pointer
     */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    /**
     * Return the address of the raw pointer
     *
     * @return: Returns the address of the stored pointer
     */
    FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /* Retrieve element at a certain index */
    template<typename U = T>
    FORCEINLINE typename TEnableIf<TAnd<TIsSame<U, T>, TIsUnboundedArray<U>>::Value, typename TAddLValueReference<typename TRemoveExtent<U>::Type>::Type>::Type
        operator[](SizeType Index) const noexcept
    {
        Assert(IsValid());
        return Get()[Index];
    }

    /**
     * Check weather the pointer is nullptr or not
     *
     * @return: Returns true if the pointer is not nullptr
     */
    FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /**
     * Copy-assignment operator
     * 
     * @param Rhs: SharedPtr to copy
     * @return: A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(const TSharedPtr& Rhs) noexcept
    {
        TSharedPtr(Rhs).Swap(*this);
        return *this;
    }

    /**
     * Move-assignment operator
     *
     * @param Rhs: SharedPtr to move
     * @return: A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(TSharedPtr&& Rhs) noexcept
    {
        TSharedPtr(Move(Rhs)).Swap(*this);
        return *this;
    }

    /**
     * Copy-assignment operator with a convertible type
     *
     * @param Rhs: SharedPtr to copy
     * @return: A reference to this instance
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value, typename TAddReference<TSharedPtr>::LValue>::Type
        operator=(const TSharedPtr<OtherType, OtherDeleterType>& Rhs) noexcept
    {
        TSharedPtr(Rhs).Swap(*this);
        return *this;
    }

    /**
     * Move-assignment operator with a convertible type
     *
     * @param Rhs: SharedPtr to move
     * @return: A reference to this instance
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value, typename TAddReference<TSharedPtr>::LValue>::Type
        operator=(TSharedPtr<OtherType, OtherDeleterType>&& Rhs) noexcept
    {
        TSharedPtr(Move(Rhs)).Swap(*this);
        return *this;
    }

    /**
     * Assignment operator that takes a raw-pointer
     * 
     * @param Rhs: Raw-pointer to store
     * @return: A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(ElementType* Rhs) noexcept
    {
        TSharedPtr(Rhs).Swap(*this);
        return *this;
    }

    /**
     * Assignment operator that takes a nullptr
     * 
     * @return: A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(NullptrType) noexcept
    {
        TSharedPtr().Swap(*this);
        return *this;
    }

private:
    FORCEINLINE CPointerReferenceCounter* GetCounter() const noexcept
    {
        return Storage.GetCounter();
    }

    PointerStorage Storage;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TSharedPtr operators

template<typename T, typename U>
FORCEINLINE bool operator==(const TSharedPtr<T>& Lhs, U* Rhs) noexcept
{
    return (Lhs.Get() == Rhs);
}

template<typename T, typename U>
FORCEINLINE bool operator==(T* Lhs, const TSharedPtr<U>& Rhs) noexcept
{
    return (Lhs == Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator!=(const TSharedPtr<T>& Lhs, U* Rhs) noexcept
{
    return (Lhs.Get() != Rhs);
}

template<typename T, typename U>
FORCEINLINE bool operator!=(T* Lhs, const TSharedPtr<U>& Rhs) noexcept
{
    return (Lhs != Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator==(const TSharedPtr<T>& Lhs, const TSharedPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() == Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator!=(const TSharedPtr<T>& Lhs, const TSharedPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() != Rhs.Get());
}

template<typename T>
FORCEINLINE bool operator==(const TSharedPtr<T>& Lhs, NullptrType) noexcept
{
    return (Lhs.Get() == nullptr);
}

template<typename T>
FORCEINLINE bool operator==(NullptrType, const TSharedPtr<T>& Rhs) noexcept
{
    return (nullptr == Rhs.Get());
}

template<typename T>
FORCEINLINE bool operator!=(const TSharedPtr<T>& Lhs, NullptrType) noexcept
{
    return (Lhs.Get() != nullptr);
}

template<typename T>
FORCEINLINE bool operator!=(NullptrType, const TSharedPtr<T>& Rhs) noexcept
{
    return (nullptr != Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator==(const TSharedPtr<T>& Lhs, const TUniquePtr<U>& Rhs) noexcept
{
    return (Lhs.Get() == Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator==(const TUniquePtr<T>& Lhs, const TSharedPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() == Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator!=(const TSharedPtr<T>& Lhs, const TUniquePtr<U>& Rhs) noexcept
{
    return (Lhs.Get() != Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator!=(const TUniquePtr<T>& Lhs, const TSharedPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() != Rhs.Get());
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TWeakPtr - Weak Pointer for scalar types, similar to std::weak_ptr

template<typename T, typename DeleterType = TDefaultDelete<T>>
class TWeakPtr
{
public:
    using ElementType = typename TRemoveExtent<T>::Type;
    using PointerStorage = TPointerReferencedStorage<ElementType, DeleterType>;
    using CounterType = typename PointerStorage::CounterType;
    using SizeType = int32;

    template<typename OtherType, typename OtherDeleterType>
    friend class TSharedPtr;

    template<typename OtherType, typename OtherDeleterType>
    friend class TWeakPtr;

    /**
     * Default constructor 
     */
    FORCEINLINE TWeakPtr() noexcept
        : Storage()
    {
    }

    /**
     * Copy-constructor
     *
     * @param Other: WeakPtr to copy
     */
    FORCEINLINE TWeakPtr(const TWeakPtr& Other) noexcept
        : Storage()
    {
        Storage.InitWeak(Other.Get(), Other.GetCounter());
    }

    /**
     * Move-constructor
     *
     * @param Other: WeakPtr to move
     */
    FORCEINLINE TWeakPtr(TWeakPtr&& Other) noexcept
        : Storage(Move(Other.Storage))
    {
    }

    /**
     * Copy-constructor that copies from a convertible type
     *
     * @param Other: WeakPtr to copy
     */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TWeakPtr(const TWeakPtr<OtherType, OtherDeleterType>& Other) noexcept
        : Storage()
    {
        Storage.InitWeak(static_cast<ElementType*>(Other.Get()), Other.GetCounter());
    }

    /**
     * Move-constructor that moves from a convertible type
     *
     * @param Other: WeakPtr to move
     */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TWeakPtr(TWeakPtr<OtherType, OtherDeleterType>&& Other) noexcept
        : Storage(Move(Other.Storage))
    {
    }

    /**
     * Constructor that constructs a WeakPtr from a SharedPtr
     * 
     * @param Other: SharedPtr to take pointer and counter from
     */
    FORCEINLINE TWeakPtr(const TSharedPtr<T, DeleterType>& Other) noexcept
        : Storage()
    {
        Storage.InitWeak(Other.Get(), Other.GetCounter());
    }

    /**
     * Constructor that constructs a WeakPtr from a SharedPtr of convertible type
     *
     * @param Other: SharedPtr to take pointer and counter from
     */
    template<typename OtherType, typename OtherDeleterType, typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TWeakPtr(const TSharedPtr<OtherType, OtherDeleterType>& Other) noexcept
        : Storage()
    {
        Storage.InitWeak(static_cast<ElementType*>(Other.Get()), Other.GetCounter());
    }

    /**
     * Destructor
     */
    FORCEINLINE ~TWeakPtr()
    {
        Reset();
    }

    /**
     * Reset the pointer and set it to a optional new pointer
     * 
     * @param NewPointer: New pointer to store
     */
    FORCEINLINE void Reset(ElementType* NewPointer = nullptr) noexcept
    {
        if (Get() != NewPointer)
        {
            Storage.ReleaseWeak();
            Storage.InitWeak(NewPointer, nullptr);
        }
    }

    /**
     * Reset the pointer and set it to a optional new pointer of a convertible type
     *
     * @param NewPointer: New pointer to store
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type Reset(OtherType* NewPtr = nullptr) noexcept
    {
        Reset(static_cast<ElementType*>(NewPtr));
    }

    /**
     * Swaps the contents of this and another container 
     * 
     * @param Other: Instance to swap with
     */
    FORCEINLINE void Swap(TWeakPtr& Other) noexcept
    {
        PointerStorage Temp = Move(Storage);
        Storage = Move(Other.Storage);
        Other.Storage = Move(Temp);
    }

    /**
     * Checks weather there are any strong references left
     * 
     * @return: Returns true if the strong reference count is less than one
     */
    FORCEINLINE bool IsExpired() const noexcept
    {
        return (GetStrongRefCount() < 1);
    }

    /**
     * Creates a shared pointer from this WeakPtr
     * 
     * @return: A new SharedPtr that holds the same pointer as this WeakPtr
     */
    FORCEINLINE TSharedPtr<T> MakeShared() noexcept
    {
        return TSharedPtr<T>(*this);
    }

    /**
     * Checks weather the strong reference count is one 
     * 
     * @return: Returns true if the strong reference count is one
     */
    FORCEINLINE bool IsUnique() const noexcept
    {
        return (Storage.GetStrongRefCount() == 1);
    }

    /**
     * Checks weather the pointer is nullptr or not and the pointer is not expired
     * 
     * @return: Returns true if the pointer is not nullptr and the strong reference count is more than zero
     */
    FORCEINLINE bool IsValid() const noexcept
    {
        return (Get() != nullptr) && !IsExpired();
    }

    /**
     * Retrieve the stored pointer
     * 
     * @return: Returns the stored pointer
     */
    FORCEINLINE ElementType* Get() const noexcept
    {
        return Storage.GetPointer();
    }

    /**
     * Retrieve the address of the stored pointer
     *
     * @return: Returns the address of the stored pointer
     */
    FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return Storage.GetAddressOfPointer();
    }

    /**
     * Retrieve the number of strong references
     *
     * @return: Returns the number of strong references
     */
    FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        return Storage.GetStrongRefCount();
    }

    /**
     * Retrieve the number of weak references
     *
     * @return: Returns the number of weak references
     */
    FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        return Storage.GetWeakRefCount();
    }

    /**
     * Dereference the pointer
     *
     * @return: A reference to the object pointer to by the stored pointer
     */
    FORCEINLINE ElementType& Dereference() const noexcept
    {
        Assert(IsValid());
        return *Get();
    }

    /**
     * Retrieve the stored pointer
     *
     * @return: Returns the stored pointer
     */
    FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }

    /**
     * Dereference the pointer
     *
     * @return: A reference to the object pointer to by the stored pointer
     */
    FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    /**
     * Retrieve the address of the stored pointer
     *
     * @return: Returns the address of the stored pointer
     */
    FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /**
     * Retrieve an element at a certain index 
     * 
     * @param Index: Index of the element to retrieve
     * @return: A reference to the retrieved element
     */
    template<typename U = T>
    FORCEINLINE typename TEnableIf<TAnd<TIsSame<U, T>, TIsUnboundedArray<U>>::Value, typename TAddLValueReference<typename TRemoveExtent<U>::Type>::Type>::Type
        operator[](SizeType Index) const noexcept
    {
        Assert(IsValid());
        return Get()[Index];
    }

    /**
     * Checks weather the pointer is nullptr or not and the pointer is not expired
     *
     * @return: Returns true if the pointer is not nullptr and the strong reference count is more than zero
     */
    FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /**
     * Copy-assignment operator
     * 
     * @param Rhs: WeakPtr to copy from
     * @return: Return the reference to this instance
     */
    FORCEINLINE TWeakPtr& operator=(const TWeakPtr& Rhs) noexcept
    {
        TWeakPtr(Rhs).Swap(*this);
        return *this;
    }

    /**
     * Move-assignment operator
     *
     * @param Rhs: WeakPtr to move from
     * @return: Return the reference to this instance
     */
    FORCEINLINE TWeakPtr& operator=(TWeakPtr&& Rhs) noexcept
    {
        TWeakPtr(Move(Rhs)).Swap(*this);
        return *this;
    }

    /**
     * Copy-assignment operator that takes a convertible type
     *
     * @param Rhs: WeakPtr to copy from
     * @return: Return the reference to this instance
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, typename TAddReference<TWeakPtr>::LValue>::Type
        operator=(const TWeakPtr<OtherType, OtherDeleterType>& Rhs) noexcept
    {
        TWeakPtr(Rhs).Swap(*this);
        return *this;
    }

    /**
     * Move-assignment operator that takes a convertible type
     *
     * @param Rhs: WeakPtr to move from
     * @return: Return the reference to this instance
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, typename TAddReference<TWeakPtr>::LValue>::Type
        operator=(TWeakPtr<OtherType, OtherDeleterType>&& Rhs) noexcept
    {
        TWeakPtr(Move(Rhs)).Swap(*this);
        return *this;
    }

    /**
     * Assignment operator that takes a raw-pointer
     * 
     * @param Rhs: Pointer to store
     * @return: Return the reference to this instance
     */
    FORCEINLINE TWeakPtr& operator=(ElementType* Rhs) noexcept
    {
        TWeakPtr(Rhs).Swap(*this);
        return *this;
    }

    /**
     * Assignment operator from a nullptr
     *
     * @return: Return the reference to this instance
     */
    FORCEINLINE TWeakPtr& operator=(NullptrType) noexcept
    {
        TWeakPtr().Swap(*this);
        return *this;
    }

private:
    FORCEINLINE CPointerReferenceCounter* GetCounter() const noexcept
    {
        return Storage.GetCounter();
    }

    PointerStorage Storage;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TWeakPtr operators

template<typename T, typename U>
FORCEINLINE bool operator==(const TWeakPtr<T>& Lhs, U* Rhs) noexcept
{
    return (Lhs.Get() == Rhs);
}

template<typename T, typename U>
FORCEINLINE bool operator==(T* Lhs, const TWeakPtr<U>& Rhs) noexcept
{
    return (Lhs == Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator!=(const TWeakPtr<T>& Lhs, U* Rhs) noexcept
{
    return (Lhs.Get() != Rhs);
}

template<typename T, typename U>
FORCEINLINE bool operator!=(T* Lhs, const TWeakPtr<U>& Rhs) noexcept
{
    return (Lhs != Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator==(const TWeakPtr<T>& Lhs, const TWeakPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() == Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator!=(const TWeakPtr<T>& Lhs, const TWeakPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() != Rhs.Get());
}

template<typename T>
FORCEINLINE bool operator==(const TWeakPtr<T>& Lhs, NullptrType) noexcept
{
    return (Lhs.Get() == nullptr);
}

template<typename T>
FORCEINLINE bool operator==(NullptrType, const TWeakPtr<T>& Rhs) noexcept
{
    return (nullptr == Rhs.Get());
}

template<typename T>
FORCEINLINE bool operator!=(const TWeakPtr<T>& Lhs, NullptrType) noexcept
{
    return (Lhs.Get() != nullptr);
}

template<typename T>
FORCEINLINE bool operator!=(NullptrType, const TWeakPtr<T>& Rhs) noexcept
{
    return (nullptr != Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator==(const TWeakPtr<T>& Lhs, const TSharedPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() == Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator==(const TSharedPtr<T>& Lhs, const TWeakPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() == Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator!=(const TWeakPtr<T>& Lhs, const TSharedPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() != Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator!=(const TSharedPtr<T>& Lhs, const TWeakPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() != Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator==(const TWeakPtr<T>& Lhs, const TUniquePtr<U>& Rhs) noexcept
{
    return (Lhs.Get() == Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator==(const TUniquePtr<T>& Lhs, const TWeakPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() == Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator!=(const TWeakPtr<T>& Lhs, const TUniquePtr<U>& Rhs) noexcept
{
    return (Lhs.Get() != Rhs.Get());
}

template<typename T, typename U>
FORCEINLINE bool operator!=(const TUniquePtr<T>& Lhs, const TWeakPtr<U>& Rhs) noexcept
{
    return (Lhs.Get() != Rhs.Get());
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Creation helpers

template<typename T, typename... ArgTypes>
FORCEINLINE typename TEnableIf<!TIsArray<T>::Value, TSharedPtr<T>>::Type MakeShared(ArgTypes&&... Args) noexcept
{
    typedef typename TRemoveExtent<T>::Type Type;

    Type* RefCountedPtr = dbg_new Type(Forward<ArgTypes>(Args)...);
    return TSharedPtr<T>(RefCountedPtr);
}

template<typename T>
FORCEINLINE typename TEnableIf<TIsArray<T>::Value, TSharedPtr<T>>::Type MakeShared(uint32 Size) noexcept
{
    typedef typename TRemoveExtent<T>::Type Type;

    Type* RefCountedPtr = dbg_new Type[Size];
    return TSharedPtr<T>(RefCountedPtr);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Casting functions

template<typename ToType, typename FromType>
FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type StaticCast(const TSharedPtr<FromType>& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = static_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Pointer, RawPointer);
}

template<typename ToType, typename FromType>
FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type StaticCast(TSharedPtr<FromType>&& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = static_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Move(Pointer), RawPointer);
}

template<typename ToType, typename FromType>
FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type ConstCast(const TSharedPtr<FromType>& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = const_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Pointer, RawPointer);
}

template<typename ToType, typename FromType>
FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type ConstCast(TSharedPtr<FromType>&& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = const_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Move(Pointer), RawPointer);
}

template<typename ToType, typename FromType>
FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type ReinterpretCast(const TSharedPtr<FromType>& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = reinterpret_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Pointer, RawPointer);
}

template<typename ToType, typename FromType>
FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type ReinterpretCast(TSharedPtr<FromType>&& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = reinterpret_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Move(Pointer), RawPointer);
}

template<typename ToType, typename FromType>
FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type DynamicCast(const TSharedPtr<FromType>& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = dynamic_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Pointer, RawPointer);
}

template<typename ToType, typename FromType>
FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type DynamicCast(TSharedPtr<FromType>&& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = dynamic_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Move(Pointer), RawPointer);
}

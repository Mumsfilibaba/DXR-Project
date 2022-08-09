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
// FPointerReferenceCounter - Counting references in TWeak- and TSharedPtr

class FPointerReferenceCounter
{
public:
    using CounterType = FAtomicInt32::Type;

    FORCEINLINE FPointerReferenceCounter() noexcept
        : NumWeakRefs(0)
        , NumStrongRefs(0)
    { }

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

    NODISCARD FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        return NumWeakRefs.Load();
    }

    NODISCARD FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        return NumStrongRefs.Load();
    }

private:
    FAtomicInt32 NumWeakRefs;
    FAtomicInt32 NumStrongRefs;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper object for shared ref counter

template<typename T, typename DeleterType = TDefaultDelete<T>>
class TPointerReferencedStorage 
    : private DeleterType
{
    using Super = DeleterType;

public:
    using ElementType = typename TRemoveExtent<T>::Type;
    using CounterType = FPointerReferenceCounter::CounterType;

    TPointerReferencedStorage(const TPointerReferencedStorage&) = delete;
    TPointerReferencedStorage& operator=(const TPointerReferencedStorage&) = delete;

    template<
        typename OtherType,
        typename OtherDeleterType>
    friend class TPointerReferencedStorage;
    
    ~TPointerReferencedStorage() = default;

    FORCEINLINE TPointerReferencedStorage() noexcept
        : DeleterType()
        , Ptr(nullptr)
        , Counter(nullptr)
    { }

    FORCEINLINE TPointerReferencedStorage(TPointerReferencedStorage&& Other) noexcept
        : DeleterType(Move(Other))
        , Ptr(nullptr)
        , Counter(nullptr)
    {
        InitMove(Other.Ptr, Other.Counter);

        Other.Ptr     = nullptr;
        Other.Counter = nullptr;
    }

    template<
        typename OtherType,
        typename OtherDeleterType,
        typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TPointerReferencedStorage(TPointerReferencedStorage<OtherType, OtherDeleterType>&& Other) noexcept
        : DeleterType(Move(Other))
        , Ptr(nullptr)
        , Counter(nullptr)
    {
        InitMove(Other.Ptr, Other.Counter);

        Other.Ptr     = nullptr;
        Other.Counter = nullptr;
    }


    FORCEINLINE void InitStrong(ElementType* NewPtr, FPointerReferenceCounter* NewCounter) noexcept
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
                Counter = new FPointerReferenceCounter();
            }

            Check(Counter != nullptr);
            Counter->AddStrongRef();
        }
        else
        {
            Counter = nullptr;
        }
    }

    FORCEINLINE void InitWeak(ElementType* NewPtr, FPointerReferenceCounter* NewCounter) noexcept
    {
        // If the pointer is nullptr, we do not care about the counter...
        Ptr = NewPtr;
        if (Ptr)
        {
            // ...however, if we init a weak reference then there has to be a counter as well, since weak-refs require a strong
            Counter = NewCounter;
            Check(Counter != nullptr);
            Counter->AddWeakRef();
        }
        else
        {
            Counter = nullptr;
        }
    }

    FORCEINLINE void InitMove(ElementType* NewPtr, FPointerReferenceCounter* NewCounter) noexcept
    {
        // If the pointer is nullptr, we do not care about the counter...
        Ptr = NewPtr;
        if (Ptr)
        {
            // ...However if the Ptr is valid then the counter also has to be, otherwise it is not a valid move
            Counter = NewCounter;
            Check(Counter != nullptr);
        }
        else
        {
            Counter = nullptr;
        }
    }

    /** Releases a strong ref and deletes the pointer if the strong refcount is zero, if the weak refcount is also zero the counter is deleted */
    FORCEINLINE void ReleaseStrong() noexcept
    {
        Check(Counter != nullptr);
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
        Check(Counter != nullptr);
        Counter->ReleaseWeakRef();

        CounterType NumStrongRefs = Counter->GetStrongRefCount();
        CounterType NumWeakRefs   = Counter->GetWeakRefCount();
        if ((NumStrongRefs < 1) && (NumWeakRefs < 1))
        {
            delete Counter;
            Counter = nullptr;
        }
    }

    FORCEINLINE void Reset() noexcept
    {
        Ptr     = nullptr;
        Counter = nullptr;
    }

    FORCEINLINE void Swap(TPointerReferencedStorage& Other) noexcept
    {
        ElementType*              TempPtr     = Ptr;
        FPointerReferenceCounter* TempCounter = Counter;

        Ptr     = Other.Ptr;
        Counter = Other.Counter;

        Other.Ptr     = TempPtr;
        Other.Counter = TempCounter;
    }

    NODISCARD FORCEINLINE ElementType* GetPointer() const noexcept
    {
        return Ptr;
    }

    NODISCARD FORCEINLINE ElementType* const* GetAddressOfPointer() const noexcept
    {
        return &Ptr;
    }

    NODISCARD FORCEINLINE FPointerReferenceCounter* GetCounter() const noexcept
    {
        return Counter;
    }

    NODISCARD FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        Check(Counter != nullptr);
        return Counter->GetWeakRefCount();
    }

    NODISCARD FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        Check(Counter != nullptr);
        return Counter->GetStrongRefCount();
    }

    FORCEINLINE TPointerReferencedStorage& operator=(TPointerReferencedStorage&& RHS) noexcept
    {
        TPointerReferencedStorage(Move(RHS)).Swap(*this);
        return *this;
    }

    template<
        typename OtherType,
        typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value, TPointerReferencedStorage&>::Type
        operator=(TPointerReferencedStorage<OtherType, OtherDeleterType>&& RHS) noexcept
    {
        TPointerReferencedStorage(Move(RHS)).Swap(*this);
        return *this;
    }

private:
    ElementType*              Ptr;
    FPointerReferenceCounter* Counter;
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
    using ElementType    = typename TRemoveExtent<T>::Type;
    using PointerStorage = TPointerReferencedStorage<ElementType, DeleterType>;
    using CounterType    = typename PointerStorage::CounterType;
    using SizeType       = int32;

    template<typename OtherType, typename OtherDeleterType>
    friend class TWeakPtr;

    template<typename OtherType, typename OtherDeleterType>
    friend class TSharedPtr;

    /**
     * @brief: Default constructor 
     */
    FORCEINLINE TSharedPtr() noexcept
        : Storage()
    { }

    /**
     * @brief: Constructor setting both counter and pointer to nullptr 
     */
    FORCEINLINE TSharedPtr(nullptr_type) noexcept
        : Storage()
    { }

    /**
     * @brief: Constructor that initializes a new SharedPtr from a raw-pointer
     * 
     * @param InPointer: Raw pointer to store
     */
    FORCEINLINE explicit TSharedPtr(ElementType* InPointer) noexcept
        : Storage()
    {
        Storage.InitStrong(InPointer, nullptr);
    }

    /**
     * @brief: Constructor that initializes a new SharedPtr from a raw-pointer of a convertible type
     *
     * @param InPointer: Raw pointer to store
     */
    template<
        typename OtherType,
        typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>::Type>
    FORCEINLINE explicit TSharedPtr(typename TRemoveExtent<OtherType>::Type* InPointer) noexcept
        : Storage()
    {
        Storage.InitStrong(static_cast<ElementType*>(InPointer), nullptr);
    }

    /**
     * @brief: Copy-constructor 
     * 
     * @param Other: SharedPtr to copy
     */
    FORCEINLINE TSharedPtr(const TSharedPtr& Other) noexcept
        : Storage()
    {
        Storage.InitStrong(Other.Get(), Other.GetCounter());
    }

    /**
     * @brief: Copy-constructor that copies from a convertible type
     *
     * @param Other: SharedPtr to copy
     */
    template<
        typename OtherType,
        typename OtherDeleterType,
        typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedPtr(const TSharedPtr<OtherType, OtherDeleterType>& Other) noexcept
        : Storage()
    {
        Storage.InitStrong(static_cast<ElementType*>(Other.Get()), Other.GetCounter());
    }

    /**
     * @brief: Move-constructor
     *
     * @param Other: SharedPtr to move
     */
    FORCEINLINE TSharedPtr(TSharedPtr&& Other) noexcept
        : Storage(Move(Other.Storage))
    { }

    /**
     * @brief: Move-constructor that copies from a convertible type
     *
     * @param Other: SharedPtr to move
     */
    template<
        typename OtherType,
        typename OtherDeleterType,
        typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedPtr(TSharedPtr<OtherType, OtherDeleterType>&& Other) noexcept
        : Storage(Move(Other.Storage))
    { }

    /**
     * @brief: Copy constructor that copy the counter, but the pointer is taken from the raw-pointer to enable casting 
     * 
     * @param Other: Container to take the counter from
     * @param InPointer: Raw pointer to store
     */
    template<
        typename OtherType,
        typename OtherDeleterType>
    FORCEINLINE explicit TSharedPtr(const TSharedPtr<OtherType, OtherDeleterType>& Other, ElementType* InPointer) noexcept
        : Storage()
    {
        Storage.InitStrong(InPointer, Other.GetCounter());
    }

    /**
     * @brief: Move constructor that move the counter, but the pointer is taken from the raw-pointer to enable casting
     *
     * @param Other: Container to take the counter from
     * @param InPointer: Raw pointer to store
     */
    template<
        typename OtherType,
        typename OtherDeleterType>
    FORCEINLINE explicit TSharedPtr(TSharedPtr<OtherType, OtherDeleterType>&& Other, ElementType* InPointer) noexcept
        : Storage()
    {
        Storage.InitMove(InPointer, Other.GetCounter());
        Other.Storage.Reset();
    }

    /**
     * @brief: Constructor that creates a SharedPtr from a WeakPtr 
     * 
     * @param Other: WeakPtr to take counter and pointer from
     */
    template<
        typename OtherType,
        typename OtherDeleterType,
        typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>::Type>
    FORCEINLINE explicit TSharedPtr(const TWeakPtr<OtherType, OtherDeleterType>& Other) noexcept
        : Storage()
    {
        Storage.InitStrong(static_cast<ElementType*>(Other.Get()), Other.GetCounter());
    }

    /**
     * @brief: Constructor that creates a SharedPtr from a UniquePtr
     *
     * @param Other: UniquePtr to take counter and pointer from
     */
    template<
        typename OtherType,
        typename OtherDeleterType,
        typename = typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value>::Type>
    FORCEINLINE TSharedPtr(TUniquePtr<OtherType, OtherDeleterType>&& Other) noexcept
        : Storage()
    {
        Storage.InitStrong(static_cast<ElementType*>(Other.Release()), nullptr);
    }

    /**
     * @brief: Destructor
     */
    FORCEINLINE ~TSharedPtr() noexcept
    {
        Reset();
    }

    /**
     * @brief: Reset the pointer and set it to a optional new pointer 
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
     * @brief: Reset the pointer and set it to a optional new pointer of convertible type
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
     * @brief: Swaps the contents of this and another container 
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
     * @brief: Check weather the strong reference count is one
     * 
     * @return: Returns true if the strong reference count is equal to one
     */
    NODISCARD FORCEINLINE bool IsUnique() const noexcept
    {
        return (Storage.GetStrongRefCount() == 1);
    }

    /**
     * @brief: Check weather the pointer is nullptr or not
     *
     * @return: Returns true if the pointer is not nullptr
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Get() != nullptr);
    }

    /**
     * @brief: Return the raw pointer 
     * 
     * @return: Returns the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* Get() const noexcept
    {
        return Storage.GetPointer();
    }

    /**
     * @brief: Return the address of the raw pointer
     *
     * @return: Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return Storage.GetAddressOfPointer();
    }

    /**
     * @brief: Returns the number of strong references 
     * 
     * @return: The number of strong references
     */
    NODISCARD FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        return Storage.GetStrongRefCount();
    }

    /**
     * @brief: Returns the number of weak references
     *
     * @return: The number of weak references
     */
    NODISCARD FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        return Storage.GetWeakRefCount();
    }

    /**
     * @brief: Dereference the pointer
     * 
     * @return: A reference to the object pointer to by the stored pointer
     */
    NODISCARD FORCEINLINE ElementType& Dereference() const noexcept
    {
        Check(IsValid());
        return *Get();
    }

public:

    /**
     * @brief: Return the raw pointer
     *
     * @return: Returns the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }
    /**
     * @brief: Dereference the pointer
     *
     * @return: A reference to the object pointer to by the stored pointer
     */
    NODISCARD FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    /**
     * @brief: Return the address of the raw pointer
     *
     * @return: Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

     /** @brief: Retrieve element at a certain index */
    template<typename U = T>
    NODISCARD FORCEINLINE typename TEnableIf<TAnd<TIsSame<U, T>, TIsUnboundedArray<U>>::Value, typename TAddLValueReference<typename TRemoveExtent<U>::Type>::Type>::Type
        operator[](SizeType Index) const noexcept
    {
        Check(IsValid());
        return Get()[Index];
    }

    /**
     * @brief: Check weather the pointer is nullptr or not
     *
     * @return: Returns true if the pointer is not nullptr
     */
    NODISCARD FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /**
     * @brief: Copy-assignment operator
     * 
     * @param RHS: SharedPtr to copy
     * @return: A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(const TSharedPtr& RHS) noexcept
    {
        TSharedPtr(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator
     *
     * @param RHS: SharedPtr to move
     * @return: A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(TSharedPtr&& RHS) noexcept
    {
        TSharedPtr(Move(RHS)).Swap(*this);
        return *this;
    }

    /**
     * @brief: Copy-assignment operator with a convertible type
     *
     * @param RHS: SharedPtr to copy
     * @return: A reference to this instance
     */
    template<
        typename OtherType,
        typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value, typename TAddReference<TSharedPtr>::LValue>::Type
        operator=(const TSharedPtr<OtherType, OtherDeleterType>& RHS) noexcept
    {
        TSharedPtr(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator with a convertible type
     *
     * @param RHS: SharedPtr to move
     * @return: A reference to this instance
     */
    template<
        typename OtherType,
        typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<typename TRemoveExtent<OtherType>::Type*, ElementType*>::Value, typename TAddReference<TSharedPtr>::LValue>::Type
        operator=(TSharedPtr<OtherType, OtherDeleterType>&& RHS) noexcept
    {
        TSharedPtr(Move(RHS)).Swap(*this);
        return *this;
    }

    /**
     * @brief: Assignment operator that takes a raw-pointer
     * 
     * @param RHS: Raw-pointer to store
     * @return: A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(ElementType* RHS) noexcept
    {
        TSharedPtr(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Assignment operator that takes a nullptr
     * 
     * @return: A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(nullptr_type) noexcept
    {
        TSharedPtr().Swap(*this);
        return *this;
    }

private:
    NODISCARD FORCEINLINE FPointerReferenceCounter* GetCounter() const noexcept
    {
        return Storage.GetCounter();
    }

    PointerStorage Storage;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TSharedPtr operators

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TSharedPtr<T>& LHS, U* RHS) noexcept
{
    return (LHS.Get() == RHS);
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(T* LHS, const TSharedPtr<U>& RHS) noexcept
{
    return (LHS == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TSharedPtr<T>& LHS, U* RHS) noexcept
{
    return (LHS.Get() != RHS);
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(T* LHS, const TSharedPtr<U>& RHS) noexcept
{
    return (LHS != RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TSharedPtr<T>& LHS, const TSharedPtr<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TSharedPtr<T>& LHS, const TSharedPtr<U>& RHS) noexcept
{
    return (LHS.Get() != RHS.Get());
}

template<typename T>
NODISCARD FORCEINLINE bool operator==(const TSharedPtr<T>& LHS, nullptr_type) noexcept
{
    return (LHS.Get() == nullptr);
}

template<typename T>
NODISCARD FORCEINLINE bool operator==(nullptr_type, const TSharedPtr<T>& RHS) noexcept
{
    return (nullptr == RHS.Get());
}

template<typename T>
NODISCARD FORCEINLINE bool operator!=(const TSharedPtr<T>& LHS, nullptr_type) noexcept
{
    return (LHS.Get() != nullptr);
}

template<typename T>
NODISCARD FORCEINLINE bool operator!=(nullptr_type, const TSharedPtr<T>& RHS) noexcept
{
    return (nullptr != RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TSharedPtr<T>& LHS, const TUniquePtr<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TUniquePtr<T>& LHS, const TSharedPtr<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TSharedPtr<T>& LHS, const TUniquePtr<U>& RHS) noexcept
{
    return (LHS.Get() != RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TUniquePtr<T>& LHS, const TSharedPtr<U>& RHS) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TWeakPtr - Weak Pointer for scalar types, similar to std::weak_ptr

template<typename T, typename DeleterType = TDefaultDelete<T>>
class TWeakPtr
{
public:
    using ElementType    = typename TRemoveExtent<T>::Type;
    using PointerStorage = TPointerReferencedStorage<ElementType, DeleterType>;
    using CounterType    = typename PointerStorage::CounterType;
    using SizeType       = int32;

    template<typename OtherType, typename OtherDeleterType>
    friend class TSharedPtr;

    template<typename OtherType, typename OtherDeleterType>
    friend class TWeakPtr;

    /**
     * @brief: Default constructor 
     */
    FORCEINLINE TWeakPtr() noexcept
        : Storage()
    { }

    /**
     * @brief: Copy-constructor
     *
     * @param Other: WeakPtr to copy
     */
    FORCEINLINE TWeakPtr(const TWeakPtr& Other) noexcept
        : Storage()
    {
        Storage.InitWeak(Other.Get(), Other.GetCounter());
    }

    /**
     * @brief: Move-constructor
     *
     * @param Other: WeakPtr to move
     */
    FORCEINLINE TWeakPtr(TWeakPtr&& Other) noexcept
        : Storage(Move(Other.Storage))
    { }

    /**
     * @brief: Copy-constructor that copies from a convertible type
     *
     * @param Other: WeakPtr to copy
     */
    template<
        typename OtherType,
        typename OtherDeleterType,
        typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TWeakPtr(const TWeakPtr<OtherType, OtherDeleterType>& Other) noexcept
        : Storage()
    {
        Storage.InitWeak(static_cast<ElementType*>(Other.Get()), Other.GetCounter());
    }

    /**
     * @brief: Move-constructor that moves from a convertible type
     *
     * @param Other: WeakPtr to move
     */
    template<
        typename OtherType,
        typename OtherDeleterType,
        typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TWeakPtr(TWeakPtr<OtherType, OtherDeleterType>&& Other) noexcept
        : Storage(Move(Other.Storage))
    { }

    /**
     * @brief: Constructor that constructs a WeakPtr from a SharedPtr
     * 
     * @param Other: SharedPtr to take pointer and counter from
     */
    FORCEINLINE TWeakPtr(const TSharedPtr<T, DeleterType>& Other) noexcept
        : Storage()
    {
        Storage.InitWeak(Other.Get(), Other.GetCounter());
    }

    /**
     * @brief: Constructor that constructs a WeakPtr from a SharedPtr of convertible type
     *
     * @param Other: SharedPtr to take pointer and counter from
     */
    template<
        typename OtherType,
        typename OtherDeleterType,
        typename = typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type>
    FORCEINLINE TWeakPtr(const TSharedPtr<OtherType, OtherDeleterType>& Other) noexcept
        : Storage()
    {
        Storage.InitWeak(static_cast<ElementType*>(Other.Get()), Other.GetCounter());
    }

    /**
     * @brief: Destructor
     */
    FORCEINLINE ~TWeakPtr()
    {
        Reset();
    }

    /**
     * @brief: Reset the pointer and set it to a optional new pointer
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
     * @brief: Reset the pointer and set it to a optional new pointer of a convertible type
     *
     * @param NewPointer: New pointer to store
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value>::Type Reset(OtherType* NewPtr = nullptr) noexcept
    {
        Reset(static_cast<ElementType*>(NewPtr));
    }

    /**
     * @brief: Swaps the contents of this and another container 
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
     * @brief: Checks weather there are any strong references left
     * 
     * @return: Returns true if the strong reference count is less than one
     */
    NODISCARD FORCEINLINE bool IsExpired() const noexcept
    {
        return (GetStrongRefCount() < 1);
    }

    /**
     * @brief: Creates a shared pointer from this WeakPtr
     * 
     * @return: A new SharedPtr that holds the same pointer as this WeakPtr
     */
    NODISCARD FORCEINLINE TSharedPtr<T> MakeShared() noexcept
    {
        return TSharedPtr<T>(*this);
    }

    /**
     * @brief: Checks weather the strong reference count is one 
     * 
     * @return: Returns true if the strong reference count is one
     */
    NODISCARD FORCEINLINE bool IsUnique() const noexcept
    {
        return (Storage.GetStrongRefCount() == 1);
    }

    /**
     * @brief: Checks weather the pointer is nullptr or not and the pointer is not expired
     * 
     * @return: Returns true if the pointer is not nullptr and the strong reference count is more than zero
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Get() != nullptr) && !IsExpired();
    }

    /**
     * @brief: Retrieve the stored pointer
     * 
     * @return: Returns the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* Get() const noexcept
    {
        return Storage.GetPointer();
    }

    /**
     * @brief: Retrieve the address of the stored pointer
     *
     * @return: Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* const* GetAddressOf() const noexcept
    {
        return Storage.GetAddressOfPointer();
    }

    /**
     * @brief: Retrieve the number of strong references
     *
     * @return: Returns the number of strong references
     */
    NODISCARD FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        return Storage.GetStrongRefCount();
    }

    /**
     * @brief: Retrieve the number of weak references
     *
     * @return: Returns the number of weak references
     */
    NODISCARD FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        return Storage.GetWeakRefCount();
    }

    /**
     * @brief: Dereference the pointer
     *
     * @return: A reference to the object pointer to by the stored pointer
     */
    NODISCARD FORCEINLINE ElementType& Dereference() const noexcept
    {
        Check(IsValid());
        return *Get();
    }

public:

    /**
     * @brief: Retrieve the stored pointer
     *
     * @return: Returns the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* operator->() const noexcept
    {
        return Get();
    }

    /**
     * @brief: Dereference the pointer
     *
     * @return: A reference to the object pointer to by the stored pointer
     */
    NODISCARD FORCEINLINE ElementType& operator*() const noexcept
    {
        return Dereference();
    }

    /**
     * @brief: Retrieve the address of the stored pointer
     *
     * @return: Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ElementType* const* operator&() const noexcept
    {
        return GetAddressOf();
    }

    /**
     * @brief: Retrieve an element at a certain index 
     * 
     * @param Index: Index of the element to retrieve
     * @return: A reference to the retrieved element
     */
    template<typename U = T>
    NODISCARD FORCEINLINE typename TEnableIf<TAnd<TIsSame<U, T>, TIsUnboundedArray<U>>::Value, typename TAddLValueReference<typename TRemoveExtent<U>::Type>::Type>::Type
        operator[](SizeType Index) const noexcept
    {
        Check(IsValid());
        return Get()[Index];
    }

    /**
     * @brief: Checks weather the pointer is nullptr or not and the pointer is not expired
     *
     * @return: Returns true if the pointer is not nullptr and the strong reference count is more than zero
     */
    NODISCARD FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /**
     * @brief: Copy-assignment operator
     * 
     * @param RHS: WeakPtr to copy from
     * @return: Return the reference to this instance
     */
    FORCEINLINE TWeakPtr& operator=(const TWeakPtr& RHS) noexcept
    {
        TWeakPtr(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator
     *
     * @param RHS: WeakPtr to move from
     * @return: Return the reference to this instance
     */
    FORCEINLINE TWeakPtr& operator=(TWeakPtr&& RHS) noexcept
    {
        TWeakPtr(Move(RHS)).Swap(*this);
        return *this;
    }

    /**
     * @brief: Copy-assignment operator that takes a convertible type
     *
     * @param RHS: WeakPtr to copy from
     * @return: Return the reference to this instance
     */
    template<
        typename OtherType,
        typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, typename TAddReference<TWeakPtr>::LValue>::Type
        operator=(const TWeakPtr<OtherType, OtherDeleterType>& RHS) noexcept
    {
        TWeakPtr(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator that takes a convertible type
     *
     * @param RHS: WeakPtr to move from
     * @return: Return the reference to this instance
     */
    template<
        typename OtherType,
        typename OtherDeleterType>
    FORCEINLINE typename TEnableIf<TIsConvertible<OtherType*, ElementType*>::Value, typename TAddReference<TWeakPtr>::LValue>::Type
        operator=(TWeakPtr<OtherType, OtherDeleterType>&& RHS) noexcept
    {
        TWeakPtr(Move(RHS)).Swap(*this);
        return *this;
    }

    /**
     * @brief: Assignment operator that takes a raw-pointer
     * 
     * @param RHS: Pointer to store
     * @return: Return the reference to this instance
     */
    FORCEINLINE TWeakPtr& operator=(ElementType* RHS) noexcept
    {
        TWeakPtr(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Assignment operator from a nullptr
     *
     * @return: Return the reference to this instance
     */
    FORCEINLINE TWeakPtr& operator=(nullptr_type) noexcept
    {
        TWeakPtr().Swap(*this);
        return *this;
    }

private:
    NODISCARD FORCEINLINE FPointerReferenceCounter* GetCounter() const noexcept
    {
        return Storage.GetCounter();
    }

    PointerStorage Storage;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TWeakPtr operators

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TWeakPtr<T>& LHS, U* RHS) noexcept
{
    return (LHS.Get() == RHS);
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(T* LHS, const TWeakPtr<U>& RHS) noexcept
{
    return (LHS == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TWeakPtr<T>& LHS, U* RHS) noexcept
{
    return (LHS.Get() != RHS);
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(T* LHS, const TWeakPtr<U>& RHS) noexcept
{
    return (LHS != RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TWeakPtr<T>& LHS, const TWeakPtr<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TWeakPtr<T>& LHS, const TWeakPtr<U>& RHS) noexcept
{
    return (LHS.Get() != RHS.Get());
}

template<typename T>
NODISCARD FORCEINLINE bool operator==(const TWeakPtr<T>& LHS, nullptr_type) noexcept
{
    return (LHS.Get() == nullptr);
}

template<typename T>
NODISCARD FORCEINLINE bool operator==(nullptr_type, const TWeakPtr<T>& RHS) noexcept
{
    return (nullptr == RHS.Get());
}

template<typename T>
NODISCARD FORCEINLINE bool operator!=(const TWeakPtr<T>& LHS, nullptr_type) noexcept
{
    return (LHS.Get() != nullptr);
}

template<typename T>
NODISCARD FORCEINLINE bool operator!=(nullptr_type, const TWeakPtr<T>& RHS) noexcept
{
    return (nullptr != RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TWeakPtr<T>& LHS, const TSharedPtr<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TSharedPtr<T>& LHS, const TWeakPtr<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TWeakPtr<T>& LHS, const TSharedPtr<U>& RHS) noexcept
{
    return (LHS.Get() != RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TSharedPtr<T>& LHS, const TWeakPtr<U>& RHS) noexcept
{
    return (LHS.Get() != RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TWeakPtr<T>& LHS, const TUniquePtr<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TUniquePtr<T>& LHS, const TWeakPtr<U>& RHS) noexcept
{
    return (LHS.Get() == RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TWeakPtr<T>& LHS, const TUniquePtr<U>& RHS) noexcept
{
    return (LHS.Get() != RHS.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TUniquePtr<T>& LHS, const TWeakPtr<U>& RHS) noexcept
{
    return (LHS.Get() != RHS.Get());
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Creation helpers

template<typename T>
NODISCARD FORCEINLINE TSharedPtr<T> MakeSharedPtr(T* InPointer) noexcept
{
    return TSharedPtr<T>(InPointer);
}

template<typename T, typename... ArgTypes>
NODISCARD FORCEINLINE typename TEnableIf<!TIsArray<T>::Value, TSharedPtr<T>>::Type MakeShared(ArgTypes&&... Args) noexcept
{
    typedef typename TRemoveExtent<T>::Type Type;

    Type* RefCountedPtr = dbg_new Type(Forward<ArgTypes>(Args)...);
    return TSharedPtr<T>(RefCountedPtr);
}

template<typename T>
NODISCARD FORCEINLINE typename TEnableIf<TIsArray<T>::Value, TSharedPtr<T>>::Type MakeShared(uint32 Size) noexcept
{
    typedef typename TRemoveExtent<T>::Type Type;

    Type* RefCountedPtr = dbg_new Type[Size];
    return TSharedPtr<T>(RefCountedPtr);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Casting functions

template<
    typename ToType,
    typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type StaticCastSharedPtr(const TSharedPtr<FromType>& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = static_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Pointer, RawPointer);
}

template<
    typename ToType,
    typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type StaticCastSharedPtr(TSharedPtr<FromType>&& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = static_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Move(Pointer), RawPointer);
}

template<
    typename ToType,
    typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type ConstCastSharedPtr(const TSharedPtr<FromType>& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = const_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Pointer, RawPointer);
}

template<
    typename ToType,
    typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type ConstCastSharedPtr(TSharedPtr<FromType>&& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = const_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Move(Pointer), RawPointer);
}

template<
    typename ToType, 
    typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type ReinterpretCastSharedPtr(const TSharedPtr<FromType>& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = reinterpret_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Pointer, RawPointer);
}

template<
    typename ToType,
    typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type ReinterpretCastSharedPtr(TSharedPtr<FromType>&& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = reinterpret_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Move(Pointer), RawPointer);
}

template<
    typename ToType,
    typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type DynamicCastSharedPtr(const TSharedPtr<FromType>& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = dynamic_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Pointer, RawPointer);
}

template<
    typename ToType,
    typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<TIsArray<ToType>::Value == TIsArray<FromType>::Value, TSharedPtr<ToType>>::Type DynamicCastSharedPtr(TSharedPtr<FromType>&& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = dynamic_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Move(Pointer), RawPointer);
}

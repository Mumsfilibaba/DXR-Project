#pragma once
#include "UniquePtr.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Threading/AtomicInt.h"

template<typename SharedType>
class TEnableSharedFromThis;

enum EThreadAccess
{
    // Reference counting is not threadsafe
    Unsafe,
    // Reference counting is threadsafe using interlocked functions
    Safe,
};

template<typename InObjectType, EThreadAccess ThreadAccess>
class TWeakPtr;

template<typename InObjectType, EThreadAccess ThreadAccess>
class TSharedPtr;

namespace SharedPointerInternal
{
    template<EThreadAccess ThreadAccess = EThreadAccess::Safe>
    class FReferenceCounter
    {
    public:
        typedef int32 ReferenceCountType;
        typedef TConditional<ThreadAccess == EThreadAccess::Safe, FAtomicInt32, int32>::Type ReferenceType;

        FReferenceCounter(const FReferenceCounter&)           = delete;
        FReferenceCounter(FReferenceCounter&&)                = delete;
        FReferenceCounter& operator(const FReferenceCounter&) = delete;
        FReferenceCounter& operator(FReferenceCounter&&)      = delete;

        // NOTE: Default constructor initializes references to zero, this means that the weakreference counter needs to be decreased
        // when releasing a strong reference 
        FReferenceCounter() noexcept
            : WeakReferences(1)
            , StrongReferences(1)
        {
        }

        virtual ~FReferenceCounter() = default;

        // NOTE: We would like to avoid virtual functions, but we avoid to many template variations 
        // this way. And this call only gets invoked when the object gets destroyed
        virtual void DestroyObject() = 0;

        FORCEINLINE ReferenceCountType AddWeakReference() noexcept
        {
            return WeakReferences++;
        }

        FORCEINLINE ReferenceCountType AddStrongReference() noexcept
        {
            CHECK(GetStrongReferenceCount() != 0);
            return StrongReferences++;
        }

        // Adds a referencecount if the refcount is not zero
        FORCEINLINE bool TryAddStrongReference() noexcept
        {
            if constexpr (ThreadAccess == EThreadAccess::Safe)
            {
                const auto StrongReferenceCount = StrongReferences.LoadRelaxed();
                while(true)
                {
                    if (StrongReferenceCount == 0)
                    {
                        return false;
                    }

                    if (StrongReferences.CompareExchange(StrongReferenceCount + 1, StrongReferenceCount))
                    {
                        return true;
                    }
                }
            }
            else
            {
                if (StrongReferences == 0)
                {
                    return false;
                }

                StrongReferences++;
                return true; 
            }
        }

        FORCEINLINE ReferenceCountType ReleaseWeakReference() noexcept
        {
            // NOTE: Ensure that this function is not called when the reference count is zero
            CHECK(GetWeakReferenceCount() > 0);

            const ReferenceCountType WeakReferenceCount = WeakReferences--;
            if (WeakReferenceCount == 0)
            {
                // NOTE: Destroy this object when the reference count reaches zero, this means that there are no weak nor any strong references
                delete this;
            }

            return WeakReferenceCount;
        }

        FORCEINLINE ReferenceCountType ReleaseStrongReference() noexcept
        {
            // NOTE: Ensure that this function is not called when the reference count is zero
            CHECK(GetStrongReferenceCount() > 0);

            const ReferenceCountType StrongReferenceCount = StrongReferences--;
            if (StrongReferenceCount == 0)
            {
                DestroyObject();
                ReleaseWeakReference();
            }

            return StrongReferenceCount;
        }

        NODISCARD FORCEINLINE ReferenceCountType GetWeakReferenceCount() const noexcept
        {
            if constexpr (ThreadAccess == EThreadAccess::Safe)
            {
                return WeakReferences.Load();
            }
            else
            {
                return WeakReferences;
            }
        }

        NODISCARD FORCEINLINE ReferenceCountType GetStrongReferenceCount() const noexcept
        {
            if constexpr (ThreadAccess == EThreadAccess::Safe)
            {
                return StrongReferences.Load();
            }
            else
            {
                return StrongReferences;
            }
        }

    private:
        ReferenceType WeakReferences;
        ReferenceType StrongReferences;
    };


    template<typename DeleterType, bool bIsEmpty = TIsEmpty<DeleterType>::Value>
    struct TDeleterLocation : public DeleterType
    {
        TDeleterLocation(DeleterType&& InDeleter) noexcept
            : DeleterType(::Move(InDeleter))
        {
        }

        template<typename ObjectType>
        void CallDelete(ObjectType* Object) noexcept
        {
            DeleterType::Call(Object);
        }
    };

    template<typename DeleterType>
    struct TDeleterLocation<DeleterType, false>
    {
    public:
        TDeleterLocation(DeleterType&& InDeleter) noexcept
            : DeleterType(::Move(InDeleter))
        {
        }

        template<typename ObjectType>
        void CallDelete(ObjectType* Object) noexcept
        {
            Deleter.Call(Object);
        }

    private:
        DeleterType Deleter;
    };


    // TDeleterReferenceCounter Is used to store the a pointer to the object together with the reference counter
    template<typename InObjectType, typename DeleterType, EThreadAccess InThreadAccess>
    class TDeleterReferenceCounter : public TDeleterLocation<DeleterType>, public FReferenceCounter<InThreadAccess>
    {
        typedef TDeleterLocation<DeleterType> Deleter;
        typedef typename TRemoveExtents<InObjectType>::Type ObjectType;

    public:
        TDeleterReferenceCounter(const TDeleterReferenceCounter&) = delete;
        TDeleterReferenceCounter(TDeleterReferenceCounter&&)      = delete;
        TDeleterReferenceCounter& operator=(const TDeleterReferenceCounter&) = delete;
        TDeleterReferenceCounter& operator=(TDeleterReferenceCounter&&)      = delete;

        TDeleterReferenceCounter(ObjectType* InObject)
            : FReferenceCounter<InThreadAccess>()
            , Deleter()
            , Object(InObject)
        {
        }

        virtual void DestroyObject() override final noexcept
        {
            Deleter::CallDelete(Object);
        }

        ObjectType* GetObjectPointer() noexcept
        {
            return reinterpret_cast<ObjectType*>(Memory.Data);
        }

    private:
        ObjectType* Object;
    };

    // TMonolithicReferenceCounter Is used to store the object together with the reference counter
    template<typename InObjectType, EThreadAccess InThreadAccess>
    class TMonolithicReferenceCounter : public FReferenceCounter<InThreadAccess>
    {
        typedef typename TRemoveExtents<InObjectType>::Type ObjectType;
    
    public:
        TMonolithicReferenceCounter(const TMonolithicReferenceCounter&) = delete;
        TMonolithicReferenceCounter(TMonolithicReferenceCounter&&)      = delete;
        TMonolithicReferenceCounter& operator=(const TMonolithicReferenceCounter&) = delete;
        TMonolithicReferenceCounter& operator=(TMonolithicReferenceCounter&&)      = delete;

        template<typename... ArgTypes>
        TMonolithicReferenceCounter(ArgTypes... Args)
            : FReferenceCounter<InThreadAccess>()
        {
            new(reinterpret_cast<void*>(Memory.Data)) ObjectType(::Forward<ArgTypes>(Args)...);
        }

        virtual void DestroyObject() override final noexcept
        {
            ::DestroyObject(GetObjectPointer());
        }

        ObjectType* GetObjectPointer() noexcept
        {
            return reinterpret_cast<ObjectType*>(Memory.Data);
        }

    private:
        TTypeAlignedBytes<InObjectType> Memory;
    };


    template<EThreadAccess InThreadAccess>
    class TSharedReference
    {
    public:
        template<EThreadAccess ThreadAccess>
        class TWeakReferenceCounter;

        TSharedReferenceCounter()
            : ReferenceCounter(nullptr)
        {
        }

        TSharedReferenceCounter(FReferenceCounter<InThreadAccess>* InReferenceCounter)
            : ReferenceCounter(InReferenceCounter)
        {
        }

        TSharedReferenceCounter(const TSharedReferenceCounter& Other)
            : ReferenceCounter(Other.ReferenceCounter)
        {
            if (ReferenceCounter)
            {
                ReferenceCounter->AddStrongReference();
            }
        }

        TSharedReferenceCounter(TSharedReferenceCounter&& Other)
            : ReferenceCounter(Other.ReferenceCounter)
        {
            Other.ReferenceCounter = nullptr;
        }

        TSharedReferenceCounter(const TWeakReferenceCounter<InThreadAccess>& Other)
            : ReferenceCounter(Other.ReferenceCounter)
        {
            if (ReferenceCounter)
            {
                if (!ReferenceCounter->TryAddStrongReference())
                {
                    ReferenceCounter = nullptr;
                }
            }
        }

        TSharedReferenceCounter(TWeakReferenceCounter<InThreadAccess>&& Other)
            : ReferenceCounter(Other.ReferenceCounter)
        {
            if (ReferenceCounter)
            {
                if (!ReferenceCounter->TryAddStrongReference())
                {
                    ReferenceCounter = nullptr;
                }
                
                Other.ReferenceCounter->ReleaseWeakReference();
                Other.ReferenceCounter = nullptr;
            }
        }

        ~TSharedReferenceCounter()
        {
            if (ReferenceCounter)
            {
                ReferenceCounter->ReleaseStrongReference();
            }
        }

        FORCEINLINE FReferenceCounter<InThreadAccess>* GetCounter()
        {
            return ReferenceCounter;
        }

        TSharedReferenceCounter& operator=(const TSharedReferenceCounter& Other) noexcept
        {
            auto NewCounter = Other.ReferenceCounter;
            auto OldCounter = ReferenceCounter;

            // Ensure that we don't assign the same counter again
            if (OldCounter != NewCounter)
            {
                ReferenceCounter = NewCounter;
                if (NewCounter)
                {
                    NewCounter->AddStrongReference();
                }

                if (OldCounter)
                {
                    OldCounter->ReleaseStrongReference();
                }
            }

            return *this;
        }

        TSharedReferenceCounter& operator=(TSharedReferenceCounter&& Other) noexcept
        {
            auto NewCounter = Other.ReferenceCounter;
            auto OldCounter = ReferenceCounter;

            // Ensure that we don't assign the same counter again
            if (OldCounter != NewCounter)
            {
                ReferenceCounter = NewCounter;
                Other.ReferenceCounter = nullptr;
                
                if (OldCounter)
                {
                    OldCounter->ReleaseStrongReference();
                }
            }

            return *this;
        }

    private:
        FReferenceCounter<InThreadAccess>* ReferenceCounter;
    };


    template<EThreadAccess ThreadAccess>
    class TWeakReferenceCounter
    {
    };
}

template<typename InObjectType, EThreadAccess InThreadAccess = EThreadAccess::Safe>
class TSharedPtr
{
public:
    using SizeType   = int32;
    using ObjectType = typename TRemoveExtent<InObjectType>::Type;

    template<typename OtherType, EThreadAccess InThreadAccess>
    friend class TWeakPtr;

    template<typename OtherType, EThreadAccess InThreadAccess>
    friend class TSharedPtr;

    static inline constexpr EThreadAccess ThreadAccess = InThreadAccess;

    /**
     * @brief - Default constructor 
     */
    FORCEINLINE TSharedPtr() noexcept
        : Referencer()
    {
    }

    /**
     * @brief - Constructor setting both counter and pointer to nullptr 
     */
    FORCEINLINE TSharedPtr(nullptr_type) noexcept
        : Referencer()
    {
    }

    /**
     * @brief          - Constructor that initializes a new SharedPtr from a raw-pointer
     * @param InObject - Pointer to store in the SharedPtr
     */
    FORCEINLINE explicit TSharedPtr(ObjectType* InObject) noexcept
        : Referencer(InObject)
    {
        EnableSharedFromThis(InObject);
    }

    /**
     * @brief          - Constructor that initializes a new SharedPtr from a raw-pointer of a convertible type
     * @param InObject - Pointer to store in the SharedPtr
     */
    template<typename OtherType>
    FORCEINLINE explicit TSharedPtr(typename TRemoveExtent<OtherType>::Type* InObject) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherType>::Type, ObjectType>::Value)
        : Referencer(InObject)
    {
        EnableSharedFromThis(InObject);
    }

    /**
     * @brief       - Copy-constructor 
     * @param Other - SharedPtr to copy
     */
    FORCEINLINE TSharedPtr(const TSharedPtr& Other) noexcept
        : Referencer(Other)
    {
    }

    /**
     * @brief       - Copy-constructor that copies from a convertible type
     * @param Other - SharedPtr to copy
     */
    template<typename OtherType>
    FORCEINLINE TSharedPtr(const TSharedPtr<OtherType, ThreadAccess>& Other) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherType>::Type, ObjectType>::Value)
        : Referencer(static_cast<ObjectType*>(Other.Get()), Other.GetCounter())
    {
    }

    /**
     * @brief       - Move-constructor
     * @param Other - SharedPtr to move
     */
    FORCEINLINE TSharedPtr(TSharedPtr&& Other) noexcept
        : Referencer(::Move(Other.Referencer))
    { 
    }

    /**
     * @brief       - Move-constructor that copies from a convertible type
     * @param Other - SharedPtr to move
     */
    template<typename OtherType>
    FORCEINLINE TSharedPtr(TSharedPtr<OtherType, ThreadAccess>&& Other) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherType>::Type, ObjectType>::Value)
        : Referencer(::Move(Other.Referencer))
    {
    }

    /**
     * @brief          - Copy constructor that copy the counter, but the pointer is taken from the raw-pointer to enable casting 
     * @param Other    - Container to take the counter from
     * @param InObject - Pointer to store in the SharedPtr
     */
    template<typename OtherType>
    FORCEINLINE explicit TSharedPtr(const TSharedPtr<OtherType, ThreadAccess>& Other, ObjectType* InObject) noexcept
        : Referencer(InObject, Other.GetCounter())
    {
    }

    /**
     * @brief           - Move constructor that move the counter, but the pointer is taken from the raw-pointer to enable casting
     * @param Other     - Container to take the counter from
     * @param InObject - Pointer to store in the SharedPtr
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE explicit TSharedPtr(TSharedPtr<OtherType, OtherDeleterType>&& Other, ObjectType* InObject) noexcept
        : Referencer(InObject, Other.GetCounter())
    {
        Storage.Reset();
    }

    /**
     * @brief       - Constructor that creates a SharedPtr from a WeakPtr 
     * @param Other - WeakPtr to take counter and pointer from
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE explicit TSharedPtr(const TWeakPtr<OtherType, OtherDeleterType>& Other) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherType>::Type, ObjectType>::Value)
        : Referencer()
    {
        Storage.InitStrong(static_cast<ObjectType*>(Other.Get()), Other.GetCounter());
    }

    /**
     * @brief       - Constructor that creates a SharedPtr from a UniquePtr
     * @param Other - UniquePtr to take counter and pointer from
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE TSharedPtr(TUniquePtr<OtherType, OtherDeleterType>&& Other) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherType>::Type, ObjectType>::Value)
        : Referencer(static_cast<ObjectType*>(Other.Release()))
    {
        EnableSharedFromThis(Referencer.GetPointer());
    }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TSharedPtr() noexcept
    {
        Reset();
    }

    /**
     * @brief            - Reset the pointer and set it to a optional new pointer 
     * @param NewPointer - New pointer to store
     */
    FORCEINLINE void Reset(ObjectType* NewPointer = nullptr) noexcept
    {
        if (Storage.GetPointer() != NewPointer)
        {
            Storage.ReleaseStrong();
            Storage.InitStrong(NewPointer, nullptr);
        }
    }

    /**
     * @brief            - Reset the pointer and set it to a optional new pointer of convertible type
     * @param NewPointer - New pointer to store
     */
    template<typename OtherType>
    FORCEINLINE void Reset(typename TRemoveExtent<OtherType>::Type* NewPointer = nullptr) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherType>::Type, ObjectType>::Value)
    {
        Reset(static_cast<ObjectType*>(NewPointer));
    }

    /**
     * @brief       - Swaps the contents of this and another container 
     * @param Other - SharedPtr to swap with 
     */
    FORCEINLINE void Swap(TSharedPtr& Other) noexcept
    {
        PointerStorage Temp = ::Move(Storage);
        Storage       = ::Move(Other.Storage);
        Other.Storage = ::Move(Temp);
    }

    /**
     * @brief  - Check weather the strong reference count is one
     * @return - Returns true if the strong reference count is equal to one
     */
    NODISCARD FORCEINLINE bool IsUnique() const noexcept
    {
        return (Storage.GetStrongRefCount() == 1);
    }

    /**
     * @brief  - Check weather the pointer is nullptr or not
     * @return - Returns true if the pointer is not nullptr
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Storage.GetPointer() != nullptr);
    }

    /**
     * @brief  - Return the raw pointer 
     * @return - Returns the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType* Get() const noexcept
    {
        return Storage.GetPointer();
    }

    /**
     * @brief  - Return the address of the raw pointer
     * @return - Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType* const* GetAddressOf() const noexcept
    {
        return Storage.GetAddressOfPointer();
    }

    /**
     * @brief  - Returns the number of strong references 
     * @return - The number of strong references
     */
    NODISCARD FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        return Storage.GetStrongRefCount();
    }

    /**
     * @brief  - Returns the number of weak references
     * @return - The number of weak references
     */
    NODISCARD FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        return Storage.GetWeakRefCount();
    }

public:

    /**
     * @brief  - Return the raw pointer
     * @return - Returns the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType* operator->() const noexcept
    {
        return Storage.GetPointer();
    }

    /**
     * @brief  - Dereference the pointer
     * @return - A reference to the object pointer to by the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType& operator*() const noexcept
    {
        return *Storage.GetPointer();
    }

    /**
     * @brief  - Return the address of the raw pointer
     * @return - Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType* const* operator&() const noexcept
    {
        return Storage.GetAddressOfPointer();
    }

     /** 
      * @brief  - Retrieve element at a certain index
      * @return - Return the element at the index
      */
    NODISCARD FORCEINLINE ObjectType& operator[](SizeType Index) const noexcept requires(TIsUnboundedArray<InObjectType>::Value)
    {
        CHECK(IsValid());
        auto* Pointer = Storage.GetPointer();
        return Pointer[Index];
    }

    /**
     * @brief  - Check weather the pointer is nullptr or not
     * @return - Returns true if the pointer is not nullptr
     */
    NODISCARD FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /**
     * @brief       - Copy-assignment operator
     * @param Other - SharedPtr to copy
     * @return      - A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(const TSharedPtr& Other) noexcept
    {
        TSharedPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - SharedPtr to move
     * @return      - A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(TSharedPtr&& Other) noexcept
    {
        TSharedPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Copy-assignment operator with a convertible type
     * @param Other - SharedPtr to copy
     * @return      - A reference to this instance
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE TSharedPtr& operator=(const TSharedPtr<OtherType, OtherDeleterType>& Other) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherType>::Type, ObjectType>::Value)
    {
        TSharedPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator with a convertible type
     * @param Other - SharedPtr to move
     * @return      - A reference to this instance
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE TSharedPtr& operator=(TSharedPtr<OtherType, OtherDeleterType>&& Other) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherType>::Type, ObjectType>::Value)
    {
        TSharedPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Assignment operator that takes a raw-pointer
     * @param Other - Raw pointer to store
     * @return      - A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(ObjectType* Other) noexcept
    {
        TSharedPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief  - Assignment operator that takes a nullptr
     * @return - A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(nullptr_type) noexcept
    {
        TSharedPtr().Swap(*this);
        return *this;
    }

private:
    FORCEINLINE void EnableSharedFromThis(ObjectType* InPointer) noexcept
    {
        if constexpr(TIsBaseOf<TEnableSharedFromThis<ObjectType>, ObjectType>::Value)
        {
            typedef typename TRemoveCV<ObjectType>::Type PureElementType;
            InPointer->WeakThisPointer = TSharedPtr<PureElementType>(*this, static_cast<PureElementType*>(InPointer));
        }
    }

    NODISCARD FORCEINLINE SharedPointerInternal::FSharedRefCounter* GetCounter() const noexcept
    {
        return Storage.GetCounter();
    }

    SharedPointerInternal::TSharedReferencer<InObjectType, InThreadAccess> Referencer;
};

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


template<typename InObjectType, EThreadAccess InThreadAccess = EThreadAccess::Safe>
class TWeakPtr
{
public:
    using ObjectType = typename TRemoveExtent<InObjectType>::Type;
    using SizeType    = int32;

    template<typename OtherType, EThreadAccess ThreadAccess>
    friend class TSharedPtr;

    template<typename OtherType, EThreadAccess ThreadAccess>
    friend class TWeakPtr;

    static inline constexpr EThreadAccess ThreadAccess = InThreadAccess;

    /**
     * @brief - Default constructor 
     */
    FORCEINLINE TWeakPtr() noexcept
        : Storage()
    {
    }

    /**
     * @brief - Constructor taking a nullptr
     */
    FORCEINLINE TWeakPtr(nullptr_type) noexcept
        : Storage()
    {
    }

    /**
     * @brief       - Copy - constructor
     * @param Other - WeakPtr to copy
     */
    FORCEINLINE TWeakPtr(const TWeakPtr& Other) noexcept
        : Storage()
    {
        Storage.InitWeak(Other.Get(), Other.GetCounter());
    }

    /**
     * @brief       - Move - constructor
     * @param Other - WeakPtr to move
     */
    FORCEINLINE TWeakPtr(TWeakPtr&& Other) noexcept
        : Storage(::Move(Other.Storage))
    {
    }

    /**
     * @brief       - Copy - constructor that copies from a convertible type
     * @param Other - WeakPtr to copy
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE TWeakPtr(const TWeakPtr<OtherType, OtherDeleterType>& Other) noexcept requires(TIsPointerConvertible<OtherType, ObjectType>::Value)
        : Storage()
    {
        Storage.InitWeak(static_cast<ObjectType*>(Other.Get()), Other.GetCounter());
    }

    /**
     * @brief       - Move-constructor that moves from a convertible type
     * @param Other - WeakPtr to move
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE TWeakPtr(TWeakPtr<OtherType, OtherDeleterType>&& Other) noexcept requires(TIsPointerConvertible<OtherType, ObjectType>::Value)
        : Storage(::Move(Other.Storage))
    {
    }

    /**
     * @brief       - Constructor that constructs a WeakPtr from a SharedPtr
     * @param Other - SharedPtr to take pointer and counter from
     */
    FORCEINLINE TWeakPtr(const TSharedPtr<InObjectType, DeleterType>& Other) noexcept
        : Storage()
    {
        Storage.InitWeak(Other.Get(), Other.GetCounter());
    }

    /**
     * @brief       - Constructor that constructs a WeakPtr from a SharedPtr of convertible type
     * @param Other - SharedPtr to take pointer and counter from
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE TWeakPtr(const TSharedPtr<OtherType, OtherDeleterType>& Other) noexcept requires(TIsPointerConvertible<OtherType, ObjectType>::Value)
        : Storage()
    {
        Storage.InitWeak(static_cast<ObjectType*>(Other.Get()), Other.GetCounter());
    }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TWeakPtr()
    {
        Reset();
    }

    /**
     * @brief            - Reset the pointer and set it to a optional new pointer
     * @param NewPointer - New pointer to store
     */
    FORCEINLINE void Reset(ObjectType* NewPointer = nullptr) noexcept
    {
        if (Storage.GetPointer() != NewPointer)
        {
            Storage.ReleaseWeak();
            Storage.InitWeak(NewPointer, nullptr);
        }
    }

    /**
     * @brief            - Reset the pointer and set it to a optional new pointer of a convertible type
     * @param NewPointer - New pointer to store
     */
    template<typename OtherType>
    FORCEINLINE void Reset(OtherType* NewPtr = nullptr) noexcept requires(TIsPointerConvertible<OtherType, ObjectType>::Value)
    {
        Reset(static_cast<ObjectType*>(NewPtr));
    }

    /**
     * @brief       - Swaps the contents of this and another container 
     * @param Other - Instance to swap with
     */
    FORCEINLINE void Swap(TWeakPtr& Other) noexcept
    {
        PointerStorage Temp = ::Move(Storage);
        Storage       = ::Move(Other.Storage);
        Other.Storage = ::Move(Temp);
    }

    /**
     * @brief  - Checks weather there are any strong references left
     * @return - Returns true if the strong reference count is less than one
     */
    NODISCARD FORCEINLINE bool IsExpired() const noexcept
    {
        return (Storage.GetStrongRefCount() < 1);
    }

    /**
     * @brief  - Creates a shared pointer from this WeakPtr
     * @return - A new SharedPtr that holds the same pointer as this WeakPtr
     */
    NODISCARD FORCEINLINE TSharedPtr<ObjectType> ToSharedPtr() noexcept
    {
        return TSharedPtr<ObjectType>(*this);
    }

    /**
     * @brief  - Checks weather the strong reference count is one 
     * @return - Returns true if the strong reference count is one
     */
    NODISCARD FORCEINLINE bool IsUnique() const noexcept
    {
        return (Storage.GetStrongRefCount() == 1);
    }

    /**
     * @brief  - Checks weather the pointer is nullptr or not and the pointer is not expired
     * @return - Returns true if the pointer is not nullptr and the strong reference count is more than zero
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Storage.GetPointer() != nullptr) && !IsExpired();
    }

    /**
     * @brief  - Retrieve the stored pointer
     * @return - Returns the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType* Get() const noexcept
    {
        return Storage.GetPointer();
    }

    /**
     * @brief  - Retrieve the address of the stored pointer
     * @return - Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType* const* GetAddressOf() const noexcept
    {
        return Storage.GetAddressOfPointer();
    }

    /**
     * @brief  - Retrieve the number of strong references
     * @return - Returns the number of strong references
     */
    NODISCARD FORCEINLINE CounterType GetStrongRefCount() const noexcept
    {
        return Storage.GetStrongRefCount();
    }

    /**
     * @brief  - Retrieve the number of weak references
     * @return - Returns the number of weak references
     */
    NODISCARD FORCEINLINE CounterType GetWeakRefCount() const noexcept
    {
        return Storage.GetWeakRefCount();
    }

    /**
     * @brief  - Dereference the pointer
     * @return - A reference to the object pointer to by the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType& Dereference() const noexcept
    {
        CHECK(IsValid());
        return *Storage.GetPointer();
    }

public:

    /**
     * @brief  - Retrieve the stored pointer
     * @return - Returns the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType* operator->() const noexcept
    {
        return Storage.GetPointer();
    }

    /**
     * @brief  - Dereference the pointer
     * @return - A reference to the object pointer to by the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType& operator*() const noexcept
    {
        return *Storage.GetPointer();
    }

    /**
     * @brief  - Retrieve the address of the stored pointer
     * @return - Returns the address of the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType* const* operator&() const noexcept
    {
        return Storage.GetAddressOfPointer();
    }

    /**
     * @brief       - Retrieve an element at a certain index 
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the retrieved element
     */
    NODISCARD FORCEINLINE ObjectType& operator[](SizeType Index) const noexcept requires(TIsUnboundedArray<InObjectType>::Value)
    {
        CHECK(IsValid());
        ObjectType* Pointer = Storage.GetPointer();
        return Pointer[Index];
    }

    /**
     * @brief  - Checks weather the pointer is nullptr or not and the pointer is not expired
     * @return - Returns true if the pointer is not nullptr and the strong reference count is more than zero
     */
    NODISCARD FORCEINLINE operator bool() const noexcept
    {
        return IsValid();
    }

    /**
     * @brief       - Copy-assignment operator
     * @param Other - WeakPtr to copy from
     * @return      - Return the reference to this instance
     */
    FORCEINLINE TWeakPtr& operator=(const TWeakPtr& Other) noexcept
    {
        TWeakPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - WeakPtr to move from
     * @return      - Return the reference to this instance
     */
    FORCEINLINE TWeakPtr& operator=(TWeakPtr&& Other) noexcept
    {
        TWeakPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Copy-assignment operator that takes a convertible type
     * @param Other - WeakPtr to copy from
     * @return      - Return the reference to this instance
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE TWeakPtr& operator=(const TWeakPtr<OtherType, OtherDeleterType>& Other) noexcept requires(TIsPointerConvertible<OtherType, ObjectType>::Value)
    {
        TWeakPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator that takes a convertible type
     * @param Other - WeakPtr to move from
     * @return      - Return the reference to this instance
     */
    template<typename OtherType, typename OtherDeleterType>
    FORCEINLINE TWeakPtr& operator=(TWeakPtr<OtherType, OtherDeleterType>&& Other) noexcept requires(TIsPointerConvertible<OtherType, ObjectType>::Value)
    {
        TWeakPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Assignment operator that takes a raw-pointer
     * @param Other - Pointer to store
     * @return      - Return the reference to this instance
     */
    FORCEINLINE TWeakPtr& operator=(ObjectType* Other) noexcept
    {
        TWeakPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief  - Assignment operator from a nullptr
     * @return - Return the reference to this instance
     */
    FORCEINLINE TWeakPtr& operator=(nullptr_type) noexcept
    {
        TWeakPtr().Swap(*this);
        return *this;
    }

private:
    NODISCARD FORCEINLINE SharedPointerInternal::FSharedRefCounter* GetCounter() const noexcept
    {
        return Storage.GetCounter();
    }

    PointerStorage Storage;
};


template<typename SharedType>
class TEnableSharedFromThis
{
    template<typename, typename>
    friend class TSharedPtr;

protected:
    ~TEnableSharedFromThis() = default;

    TEnableSharedFromThis()
        : WeakThisPointer()
    {
    }

    TEnableSharedFromThis(const TEnableSharedFromThis&)
        : WeakThisPointer()
    {
    }

    TEnableSharedFromThis& operator=(const TEnableSharedFromThis&)
    {
        return *this;
    }

public:
    NODISCARD TSharedPtr<SharedType> AsSharedPtr()
    {
        return TSharedPtr<SharedType>(WeakThisPointer);
    }

    NODISCARD TSharedPtr<const SharedType> AsSharedPtr() const
    {
        return TSharedPtr<const SharedType>(WeakThisPointer);
    }

    NODISCARD TWeakPtr<SharedType> AsWeakPtr()
    {
        return TWeakPtr<SharedType>(WeakThisPointer);
    }

    NODISCARD TWeakPtr<const SharedType> AsWeakPtr() const
    {
        return TWeakPtr<const SharedType>(WeakThisPointer);
    }

private:
    mutable TWeakPtr<SharedType> WeakThisPointer;
};


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


template<typename T>
NODISCARD FORCEINLINE TSharedPtr<T> MakeSharedPtr(T* InPointer) noexcept
{
    return TSharedPtr<T>(InPointer);
}

template<typename T, typename... ArgTypes>
NODISCARD FORCEINLINE typename TEnableIf<TNot<TIsArray<T>>::Value, TSharedPtr<T>>::Type MakeShared(ArgTypes&&... Args) noexcept
{
    typedef typename TRemoveExtent<T>::Type Type;

    Type* RefCountedPtr = new Type(::Forward<ArgTypes>(Args)...);
    return TSharedPtr<T>(RefCountedPtr);
}

template<typename T>
NODISCARD FORCEINLINE typename TEnableIf<TIsArray<T>::Value, TSharedPtr<T>>::Type MakeShared(uint32 Size) noexcept
{
    typedef typename TRemoveExtent<T>::Type Type;

    Type* RefCountedPtr = new Type[Size];
    return TSharedPtr<T>(RefCountedPtr);
}


template<typename ToType, typename FromType>
constexpr bool IsSameArrayType()
{
    return (TIsArray<ToType>::Value == TIsArray<FromType>::Value);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<IsSameArrayType<ToType, FromType>(), TSharedPtr<ToType>>::Type StaticCastSharedPtr(const TSharedPtr<FromType>& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = static_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Pointer, RawPointer);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<IsSameArrayType<ToType, FromType>(), TSharedPtr<ToType>>::Type StaticCastSharedPtr(TSharedPtr<FromType>&& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = static_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(::Move(Pointer), RawPointer);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<IsSameArrayType<ToType, FromType>(), TSharedPtr<ToType>>::Type ConstCastSharedPtr(const TSharedPtr<FromType>& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = const_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Pointer, RawPointer);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<IsSameArrayType<ToType, FromType>(), TSharedPtr<ToType>>::Type ConstCastSharedPtr(TSharedPtr<FromType>&& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = const_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(::Move(Pointer), RawPointer);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<IsSameArrayType<ToType, FromType>(), TSharedPtr<ToType>>::Type ReinterpretCastSharedPtr(const TSharedPtr<FromType>& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = reinterpret_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Pointer, RawPointer);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<IsSameArrayType<ToType, FromType>(), TSharedPtr<ToType>>::Type ReinterpretCastSharedPtr(TSharedPtr<FromType>&& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = reinterpret_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(::Move(Pointer), RawPointer);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<IsSameArrayType<ToType, FromType>(), TSharedPtr<ToType>>::Type DynamicCastSharedPtr(const TSharedPtr<FromType>& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = dynamic_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(Pointer, RawPointer);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE typename TEnableIf<IsSameArrayType<ToType, FromType>(), TSharedPtr<ToType>>::Type DynamicCastSharedPtr(TSharedPtr<FromType>&& Pointer) noexcept
{
    typedef typename TRemoveExtent<ToType>::Type Type;

    Type* RawPointer = dynamic_cast<Type*>(Pointer.Get());
    return TSharedPtr<ToType>(::Move(Pointer), RawPointer);
}

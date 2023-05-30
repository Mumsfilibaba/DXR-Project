#pragma once
#include "UniquePtr.h"
#include "Core/Templates/Utility.h"
#include "Core/Threading/AtomicInt.h"

enum EThreadAccess
{
    // Reference counting is not ThreadSafe
    Unsafe,
    // Reference counting is ThreadSafe using interlocked functions
    Safe,
};

template<typename SharedType, EThreadAccess ThreadAccess>
class TSharedFromThis;

template<typename InObjectType, EThreadAccess ThreadAccess>
class TWeakPtr;

template<typename InObjectType, EThreadAccess ThreadAccess>
class TSharedPtr;

namespace SharedPointerInternal
{
    template<EThreadAccess ThreadAccess = EThreadAccess::Safe>
    class FReferenceHandler
    {
    public:
        typedef int32 ReferenceCountType;
        typedef TConditional<ThreadAccess == EThreadAccess::Safe, FAtomicInt32, int32>::Type ReferenceType;

        FReferenceHandler(const FReferenceHandler&) = delete;
        FReferenceHandler(FReferenceHandler&&)      = delete;
        FReferenceHandler& operator=(const FReferenceHandler&) = delete;
        FReferenceHandler& operator=(FReferenceHandler&&)      = delete;

        // NOTE: Default constructor initializes references to zero, this means that the Weak-Reference
        // counter needs to be decreased when releasing a strong reference 
        FReferenceHandler() noexcept
            : WeakReferences(1)
            , StrongReferences(1)
        {
        }

        virtual ~FReferenceHandler() = default;

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

        // Adds a ReferenceCount if the refcount is not zero
        FORCEINLINE bool TryAddStrongReference() noexcept
        {
            if constexpr (ThreadAccess == EThreadAccess::Safe)
            {
                const auto StrongReferenceCount = StrongReferences.RelaxedLoad();
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

            const ReferenceCountType WeakReferenceCount = --WeakReferences;
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

            const ReferenceCountType StrongReferenceCount = --StrongReferences;
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


    // TPointerReferenceHandler is used to store the a pointer to the object together with the reference counter
    template<typename InObjectType, typename DeleterType, EThreadAccess InThreadAccess>
    class TPointerReferenceHandler : public TDeleterLocation<DeleterType>, public FReferenceHandler<InThreadAccess>
    {
        typedef typename TRemoveExtent<InObjectType>::Type ObjectType;

    public:
        TPointerReferenceHandler(const TPointerReferenceHandler&) = delete;
        TPointerReferenceHandler(TPointerReferenceHandler&&)      = delete;
        TPointerReferenceHandler& operator=(const TPointerReferenceHandler&) = delete;
        TPointerReferenceHandler& operator=(TPointerReferenceHandler&&)      = delete;

        TPointerReferenceHandler(ObjectType* InObject, DeleterType&& Deleter)
            : FReferenceHandler<InThreadAccess>()
            , TDeleterLocation<DeleterType>(::Forward<DeleterType>(Deleter))
            , Object(InObject)
        {
        }

        virtual void DestroyObject() override final
        {
            TDeleterLocation<DeleterType>::CallDelete(Object);
            Object = nullptr;
        }

        ObjectType* GetObjectPointer() noexcept
        {
            return reinterpret_cast<ObjectType*>(Object);
        }

    private:
        ObjectType* Object;
    };

    // TObjectReferenceHandler Is used to store the object together with the reference counter
    template<typename InObjectType, EThreadAccess InThreadAccess>
    class TObjectReferenceHandler : public FReferenceHandler<InThreadAccess>
    {
        typedef typename TRemoveExtent<InObjectType>::Type ObjectType;
    
    public:
        TObjectReferenceHandler(const TObjectReferenceHandler&) = delete;
        TObjectReferenceHandler(TObjectReferenceHandler&&)      = delete;
        TObjectReferenceHandler& operator=(const TObjectReferenceHandler&) = delete;
        TObjectReferenceHandler& operator=(TObjectReferenceHandler&&)      = delete;

        template<typename... ArgTypes>
        explicit TObjectReferenceHandler(ArgTypes... Args)
            : FReferenceHandler<InThreadAccess>()
        {
            new(reinterpret_cast<void*>(Memory.Data)) ObjectType(::Forward<ArgTypes>(Args)...);
        }

        virtual void DestroyObject() override final
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
    class FWeakReference;

    template<EThreadAccess InThreadAccess>
    class FSharedReference
    {
    public:
        friend class FWeakReference<InThreadAccess>;

        FSharedReference() = default;

        FSharedReference(FReferenceHandler<InThreadAccess>* InReferenceCounter)
            : ReferenceHandler(InReferenceCounter)
        {
        }

        FSharedReference(const FSharedReference& Other)
            : ReferenceHandler(Other.ReferenceHandler)
        {
            if (ReferenceHandler)
            {
                ReferenceHandler->AddStrongReference();
            }
        }

        FSharedReference(FSharedReference&& Other)
            : ReferenceHandler(Other.ReferenceHandler)
        {
            Other.ReferenceHandler = nullptr;
        }

        FSharedReference(const FWeakReference<InThreadAccess>& Other)
            : ReferenceHandler(Other.ReferenceHandler)
        {
            if (ReferenceHandler)
            {
                if (!ReferenceHandler->TryAddStrongReference())
                {
                    ReferenceHandler = nullptr;
                }
            }
        }

        FSharedReference(FWeakReference<InThreadAccess>&& Other)
            : ReferenceHandler(Other.ReferenceHandler)
        {
            if (ReferenceHandler)
            {
                if (!ReferenceHandler->TryAddStrongReference())
                {
                    ReferenceHandler = nullptr;
                }
                
                Other.ReferenceHandler->ReleaseWeakReference();
                Other.ReferenceHandler = nullptr;
            }
        }

        ~FSharedReference()
        {
            if (ReferenceHandler)
            {
                ReferenceHandler->ReleaseStrongReference();
            }
        }

        NODISCARD FORCEINLINE auto GetStrongReferenceCount() const noexcept
        {
            return ReferenceHandler ? ReferenceHandler->GetStrongReferenceCount() : 0u;
        }

        NODISCARD FORCEINLINE auto GetWeakReferenceCount() const noexcept
        {
            return ReferenceHandler ? ReferenceHandler->GetWeakReferenceCount() : 0u;
        }

        NODISCARD FORCEINLINE bool IsUnique() const
        {
            return (ReferenceHandler != nullptr) && (ReferenceHandler->GetStrongReferenceCount() == 1u);
        }
        
        FSharedReference& operator=(const FSharedReference& Other) noexcept
        {
            auto NewCounter = Other.ReferenceHandler;
            auto OldCounter = ReferenceHandler;

            // Ensure that we don't assign the same counter again
            if (OldCounter != NewCounter)
            {
                ReferenceHandler = NewCounter;
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

        FSharedReference& operator=(FSharedReference&& Other) noexcept
        {
            auto NewCounter = Other.ReferenceHandler;
            auto OldCounter = ReferenceHandler;

            // Ensure that we don't assign the same counter again
            if (OldCounter != NewCounter)
            {
                ReferenceHandler = NewCounter;
                Other.ReferenceHandler = nullptr;
                
                if (OldCounter)
                {
                    OldCounter->ReleaseStrongReference();
                }
            }

            return *this;
        }

    private:
        FReferenceHandler<InThreadAccess>* ReferenceHandler{nullptr};
    };


    template<EThreadAccess InThreadAccess>
    class FWeakReference
    {
    public:
        friend class FSharedReference<InThreadAccess>;

        FWeakReference() = default;

        FWeakReference(FReferenceHandler<InThreadAccess>* InReferenceCounter)
            : ReferenceHandler(InReferenceCounter)
        {
        }

        FWeakReference(const FWeakReference& Other)
            : ReferenceHandler(Other.ReferenceHandler)
        {
            if (ReferenceHandler)
            {
                ReferenceHandler->AddWeakReference();
            }
        }

        FWeakReference(FWeakReference&& Other)
            : ReferenceHandler(Other.ReferenceHandler)
        {
            Other.ReferenceHandler = nullptr;
        }

        FWeakReference(const FSharedReference<InThreadAccess>& Other)
            : ReferenceHandler(Other.ReferenceHandler)
        {
            if (ReferenceHandler)
            {
                ReferenceHandler->AddWeakReference();
            }
        }

        FWeakReference(FSharedReference<InThreadAccess>&& Other)
            : ReferenceHandler(Other.ReferenceHandler)
        {
            if (ReferenceHandler)
            {
                ReferenceHandler->AddWeakReference();
                Other.ReferenceHandler->ReleaseStrongReference();
                Other.ReferenceHandler = nullptr;
            }
        }

        ~FWeakReference()
        {
            if (ReferenceHandler)
            {
                ReferenceHandler->ReleaseWeakReference();
            }
        }

        NODISCARD FORCEINLINE auto GetStrongReferenceCount() const noexcept
        {
            return ReferenceHandler ? ReferenceHandler->GetStrongReferenceCount() : 0u;
        }

        NODISCARD FORCEINLINE auto GetWeakReferenceCount() const noexcept
        {
            return ReferenceHandler ? ReferenceHandler->GetWeakReferenceCount() : 0u;
        }

        NODISCARD FORCEINLINE bool IsUnique() const
        {
            return (ReferenceHandler != nullptr) && (ReferenceHandler->GetStrongReferenceCount() == 1u); 
        }

        FWeakReference& operator=(const FWeakReference& Other) noexcept
        {
            auto NewCounter = Other.ReferenceHandler;
            auto OldCounter = ReferenceHandler;

            // Ensure that we don't assign the same counter again
            if (OldCounter != NewCounter)
            {
                ReferenceHandler = NewCounter;
                if (NewCounter)
                {
                    NewCounter->AddWeakReference();
                }

                if (OldCounter)
                {
                    OldCounter->ReleaseWeakReference();
                }
            }

            return *this;
        }

        FWeakReference& operator=(FWeakReference&& Other) noexcept
        {
            auto NewCounter = Other.ReferenceHandler;
            auto OldCounter = ReferenceHandler;

            // Ensure that we don't assign the same counter again
            if (OldCounter != NewCounter)
            {
                ReferenceHandler = NewCounter;
                Other.ReferenceHandler = nullptr;
                
                if (OldCounter)
                {
                    OldCounter->ReleaseWeakReference();
                }
            }

            return *this;
        }

    private:
        FReferenceHandler<InThreadAccess>* ReferenceHandler{nullptr};
    };

    template<EThreadAccess ThreadAccess, typename ObjectType>
    FORCEINLINE auto CreateReferenceHandler(typename TRemoveExtent<ObjectType>::Type* NewObject)
    {
        return new TPointerReferenceHandler<ObjectType, TDefaultDelete<ObjectType>, ThreadAccess>(NewObject, TDefaultDelete<ObjectType>());
    }

    template<EThreadAccess ThreadAccess, typename ObjectType, typename DeleterType>
    FORCEINLINE auto CreateReferenceHandlerWithDeleter(ObjectType* NewObject, DeleterType&& Deleter)
    {
        return new TPointerReferenceHandler<ObjectType, DeleterType, ThreadAccess>(NewObject, ::Forward<DeleterType>(Deleter));
    }

    template<EThreadAccess ThreadAccess, typename ObjectType, typename... ArgTypes>
    FORCEINLINE auto CreateObjectAndReferenceHandler(ArgTypes&&... Args)
    {
        return new TObjectReferenceHandler<ObjectType, ThreadAccess>(::Forward<ArgTypes>(Args)...);
    }


    template<typename ObjectType, EThreadAccess ThreadAccess>
    struct TSharedReferenceProxy
    {
        TSharedReferenceProxy()
            : Object(nullptr)
            , ReferenceHandler(nullptr)
        {
        }

        TSharedReferenceProxy(ObjectType* InObject, FReferenceHandler<ThreadAccess>* InReferenceHandler)
            : Object(InObject)
            , ReferenceHandler(InReferenceHandler)
        {
        }

        ObjectType* Object;
        FReferenceHandler<ThreadAccess>* ReferenceHandler;
    };
}


template<typename InObjectType, EThreadAccess InThreadAccess = EThreadAccess::Safe>
class TSharedPtr
{
public:
    using SizeType   = int32;
    using ObjectType = typename TRemoveExtent<InObjectType>::Type;

    template<typename OtherObjectType, EThreadAccess InThreadAccess>
    friend class TWeakPtr;

    template<typename OtherObjectType, EThreadAccess InThreadAccess>
    friend class TSharedPtr;

    static inline constexpr EThreadAccess ThreadAccess = InThreadAccess;

    /** @brief - Default constructor */
    TSharedPtr() = default;

    /**
     * @brief - Constructor setting both counter and pointer to nullptr 
     */
    FORCEINLINE TSharedPtr(nullptr_type) noexcept
        : Object(nullptr)
        , ReferenceHandler()
    {
    }

    /**
     * @brief          - Constructor that initializes a new SharedPtr from a raw-pointer
     * @param InObject - Pointer to store in the SharedPtr
     */
    FORCEINLINE explicit TSharedPtr(ObjectType* InObject) noexcept
        : Object(InObject)
        , ReferenceHandler(SharedPointerInternal::CreateReferenceHandler<ThreadAccess, InObjectType>(InObject))
    {
        EnableSharedFromThis(InObject);
    }

    /**
     * @brief          - Constructor that initializes a new SharedPtr from a raw-pointer of a convertible type
     * @param InObject - Pointer to store in the SharedPtr
     * @param Deleter  - Deleter object used to delete the pointer when the last reference is released
     */
    template<typename OtherObjectType, typename DeleterType>
    FORCEINLINE explicit TSharedPtr(OtherObjectType* InObject, DeleterType&& InDeleter) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherObjectType>::Type, ObjectType>::Value)
        : Object(InObject)
        , ReferenceHandler(SharedPointerInternal::CreateReferenceHandlerWithDeleter<ThreadAccess>(InObject, ::Forward<DeleterType>(InDeleter)))
    {
        EnableSharedFromThis(InObject);
    }
    
    /**
     * @brief         - Constructor that initializes a new SharedPtr from TSharedReferenceProxy which is used my MakeShared
     * @param InProxy - Proxy container
     */
    FORCEINLINE explicit TSharedPtr(SharedPointerInternal::TSharedReferenceProxy<ObjectType, ThreadAccess>&& InProxy) noexcept
        : Object(InProxy.Object)
        , ReferenceHandler(InProxy.ReferenceHandler)
    {
        EnableSharedFromThis(InProxy.Object);
        
        InProxy.Object           = nullptr;
        InProxy.ReferenceHandler = nullptr;
    }

    /**
     * @brief       - Copy-constructor 
     * @param Other - SharedPtr to copy
     */
    FORCEINLINE TSharedPtr(const TSharedPtr& Other) noexcept
        : Object(Other.Object)
        , ReferenceHandler(Other.ReferenceHandler)
    {
    }

    /**
     * @brief       - Copy-constructor that copies from a convertible type
     * @param Other - SharedPtr to copy
     */
    template<typename OtherObjectType>
    FORCEINLINE TSharedPtr(const TSharedPtr<OtherObjectType, ThreadAccess>& Other) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherObjectType>::Type, ObjectType>::Value)
        : Object(Other.Object)
        , ReferenceHandler(Other.ReferenceHandler)
    {
    }

    /**
     * @brief       - Move-constructor
     * @param Other - SharedPtr to move
     */
    FORCEINLINE TSharedPtr(TSharedPtr&& Other) noexcept
        : Object(Other.Object)
        , ReferenceHandler(::Move(Other.ReferenceHandler))
    {
        Other.Object = nullptr;
    }

    /**
     * @brief       - Move-constructor that copies from a convertible type
     * @param Other - SharedPtr to move
     */
    template<typename OtherObjectType>
    FORCEINLINE TSharedPtr(TSharedPtr<OtherObjectType, ThreadAccess>&& Other) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherObjectType>::Type, ObjectType>::Value)
        : Object(Other.Object)
        , ReferenceHandler(::Move(Other.ReferenceHandler))
    {
        Other.Object = nullptr;
    }

    /**
     * @brief          - Copy constructor that copy the counter, but the pointer is taken from the raw-pointer to enable casting 
     * @param Other    - Container to take the counter from
     * @param InObject - Pointer to store in the SharedPtr
     */
    template<typename OtherObjectType>
    FORCEINLINE explicit TSharedPtr(const TSharedPtr<OtherObjectType, ThreadAccess>& Other, ObjectType* InObject) noexcept
        : Object(InObject)
        , ReferenceHandler(Other.ReferenceHandler)
    {
    }

    /**
     * @brief          - Move constructor that move the counter, but the pointer is taken from the raw-pointer to enable casting
     * @param Other    - Container to take the counter from
     * @param InObject - Pointer to store in the SharedPtr
     */
    template<typename OtherObjectType>
    FORCEINLINE explicit TSharedPtr(TSharedPtr<OtherObjectType>&& Other, ObjectType* InObject) noexcept
        : Object(InObject)
        , ReferenceHandler(::Move(Other.ReferenceHandler))
    {
        Other.Object = nullptr;
    }

    /**
     * @brief       - Constructor that creates a SharedPtr from a WeakPtr 
     * @param Other - WeakPtr to take counter and pointer from
     */
    template<typename OtherObjectType>
    FORCEINLINE explicit TSharedPtr(const TWeakPtr<OtherObjectType, ThreadAccess>& Other) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherObjectType>::Type, ObjectType>::Value)
        : Object(Other.Object)
        , ReferenceHandler(Other.ReferenceHandler)
    {
    }

    /**
     * @brief       - Constructor that creates a SharedPtr from a UniquePtr
     * @param Other - UniquePtr to take counter and pointer from
     */
    template<typename OtherObjectType, typename OtherDeleterType>
    FORCEINLINE TSharedPtr(TUniquePtr<OtherObjectType, OtherDeleterType>&& Other) noexcept requires(TIsPointerConvertible<typename TRemoveExtent<OtherObjectType>::Type, ObjectType>::Value)
        : Object(Other.Release())
        , ReferenceHandler(SharedPointerInternal::CreateReferenceHandlerWithDeleter<ThreadAccess>(Object, ::Move(Other.GetDeleter())))
    {
        EnableSharedFromThis(Object);
    }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TSharedPtr() noexcept
    {
        Reset();
    }

    /**
     * @brief - Reset the pointer
     */
    FORCEINLINE void Reset() noexcept
    {
        Object = nullptr;
        ReferenceHandler = SharedPointerInternal::FSharedReference<ThreadAccess>();
    }

    /**
     * @brief       - Swaps the contents of this and another container 
     * @param Other - SharedPtr to swap with 
     */
    FORCEINLINE void Swap(TSharedPtr& Other) noexcept
    {
        ObjectType* OldObject = Object;
        Object = Other.Object;
        Other.Object = OldObject;
        
        SharedPointerInternal::FSharedReference<ThreadAccess> OldReferenceCounter = ::Move(ReferenceHandler);
        ReferenceHandler = ::Move(Other.ReferenceHandler);
        Other.ReferenceHandler = ::Move(OldReferenceCounter);
    }

    /**
     * @brief  - Check weather the strong reference count is one
     * @return - Returns true if the strong reference count is equal to one
     */
    NODISCARD FORCEINLINE bool IsUnique() const noexcept
    {
        return ReferenceHandler.IsUnique();
    }

    /**
     * @brief  - Check weather the pointer is nullptr or not
     * @return - Returns true if the pointer is not nullptr
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Object != nullptr);
    }

    /**
     * @brief  - Return the raw pointer 
     * @return - Returns the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType* Get() const noexcept
    {
        return Object;
    }

    /**
     * @brief  - Returns the number of strong references 
     * @return - The number of strong references
     */
    NODISCARD FORCEINLINE auto GetStrongReferenceCount() const noexcept
    {
        return ReferenceHandler.GetStrongReferenceCount();
    }

    /**
     * @brief  - Returns the number of weak references
     * @return - The number of weak references
     */
    NODISCARD FORCEINLINE auto GetWeakReferenceCount() const noexcept
    {
        return ReferenceHandler.GetWeakReferenceCount();
    }

public:

    /**
     * @brief  - Return the raw pointer
     * @return - Returns the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType* operator->() const noexcept
    {
        return Object;
    }

    /**
     * @brief  - Dereference the pointer
     * @return - A reference to the object pointer to by the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType& operator*() const noexcept
    {
        return *Object;
    }

     /** 
      * @brief  - Retrieve element at a certain index
      * @return - Return the element at the index
      */
    NODISCARD FORCEINLINE ObjectType& operator[](SizeType Index) const noexcept requires(TIsArray<InObjectType>::Value)
    {
        CHECK(IsValid());
        return Object[Index];
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
    template<typename OtherObjectType>
    FORCEINLINE TSharedPtr& operator=(const TSharedPtr<OtherObjectType>& Other) noexcept requires(TIsPointerConvertible<OtherObjectType, ObjectType>::Value)
    {
        TSharedPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator with a convertible type
     * @param Other - SharedPtr to move
     * @return      - A reference to this instance
     */
    template<typename OtherObjectType>
    FORCEINLINE TSharedPtr& operator=(TSharedPtr<OtherObjectType>&& Other) noexcept requires(TIsPointerConvertible<OtherObjectType, ObjectType>::Value)
    {
        TSharedPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    /**
     * @brief            - Assignment operator that takes a raw-pointer
     * @param NewPointer - Raw pointer to store
     * @return           - A reference to this instance
     */
    FORCEINLINE TSharedPtr& operator=(ObjectType* NewPointer) noexcept
    {
        TSharedPtr(NewPointer).Swap(*this);
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
        if constexpr(TIsBaseOf<TSharedFromThis<ObjectType>, ObjectType>::Value)
        {
            typedef typename TRemoveCV<ObjectType>::Type PureElementType;
            InPointer->WeakThisPointer = TSharedPtr<PureElementType>(*this, static_cast<PureElementType*>(InPointer));
        }
    }

    ObjectType* Object{nullptr};
    SharedPointerInternal::FSharedReference<ThreadAccess> ReferenceHandler;
};


template<typename InObjectType, EThreadAccess InThreadAccess = EThreadAccess::Safe>
class TWeakPtr
{
public:
    using SizeType   = int32;
    using ObjectType = typename TRemoveExtent<InObjectType>::Type;

    template<typename OtherObjectType, EThreadAccess ThreadAccess>
    friend class TWeakPtr;

    template<typename OtherObjectType, EThreadAccess ThreadAccess>
    friend class TSharedPtr;

    static inline constexpr EThreadAccess ThreadAccess = InThreadAccess;

    /** @brief - Default constructor */
    TWeakPtr() = default;
    
    /**
     * @brief - Constructor taking a nullptr
     */
    FORCEINLINE TWeakPtr(nullptr_type) noexcept
        : Object(nullptr)
        , ReferenceHandler()
    {
    }

    /**
     * @brief       - Copy - constructor
     * @param Other - WeakPtr to copy
     */
    FORCEINLINE TWeakPtr(const TWeakPtr& Other) noexcept
        : Object(Other.Object)
        , ReferenceHandler(Other.ReferenceHandler)
    {
    }

    /**
     * @brief       - Move - constructor
     * @param Other - WeakPtr to move
     */
    FORCEINLINE TWeakPtr(TWeakPtr&& Other) noexcept
        : Object(Other.Object)
        , ReferenceHandler(::Move(Other.ReferenceHandler))
    {
    }

    /**
     * @brief       - Copy - constructor that copies from a convertible type
     * @param Other - WeakPtr to copy
     */
    template<typename OtherObjectType>
    FORCEINLINE TWeakPtr(const TWeakPtr<OtherObjectType, ThreadAccess>& Other) noexcept requires(TIsPointerConvertible<OtherObjectType, ObjectType>::Value)
        : Object(Other.Object)
        , ReferenceHandler(Other.ReferenceHandler)
    {
    }

    /**
     * @brief       - Move-constructor that moves from a convertible type
     * @param Other - WeakPtr to move
     */
    template<typename OtherObjectType>
    FORCEINLINE TWeakPtr(TWeakPtr<OtherObjectType, ThreadAccess>&& Other) noexcept requires(TIsPointerConvertible<OtherObjectType, ObjectType>::Value)
        : Object(Other.Object)
        , ReferenceHandler(::Move(Other.ReferenceHandler))
    {
    }

    /**
     * @brief       - Constructor that constructs a WeakPtr from a SharedPtr
     * @param Other - SharedPtr to take pointer and counter from
     */
    FORCEINLINE TWeakPtr(const TSharedPtr<InObjectType, ThreadAccess>& Other) noexcept
        : Object(Other.Object)
        , ReferenceHandler(Other.ReferenceHandler)
    {
    }

    /**
     * @brief       - Constructor that constructs a WeakPtr from a SharedPtr of convertible type
     * @param Other - SharedPtr to take pointer and counter from
     */
    template<typename OtherObjectType>
    FORCEINLINE TWeakPtr(const TSharedPtr<OtherObjectType, ThreadAccess>& Other) noexcept requires(TIsPointerConvertible<OtherObjectType, ObjectType>::Value)
        : Object(Other.Object)
        , ReferenceHandler(Other.ReferenceHandler)
    {
    }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TWeakPtr()
    {
        Reset();
    }

    /**
     * @brief - Reset the pointer
     */
    FORCEINLINE void Reset() noexcept
    {
        Object = nullptr;
        ReferenceHandler = SharedPointerInternal::FWeakReference<ThreadAccess>();
    }

    /**
     * @brief            - Reset the pointer and set it to a optional new pointer of a convertible type
     * @param NewPointer - New pointer to store
     */
    template<typename OtherObjectType>
    FORCEINLINE void Reset(OtherObjectType* NewPtr = nullptr) noexcept requires(TIsPointerConvertible<OtherObjectType, ObjectType>::Value)
    {
        Reset(static_cast<ObjectType*>(NewPtr));
    }

    /**
     * @brief       - Swaps the contents of this and another container 
     * @param Other - Instance to swap with
     */
    FORCEINLINE void Swap(TWeakPtr& Other) noexcept
    {
        ObjectType* OldObject = Object;
        Object = Other.Object;
        Other.Object = OldObject;

        SharedPointerInternal::FWeakReference<ThreadAccess> OldReferenceCounter = ::Move(ReferenceHandler);
        ReferenceHandler = ::Move(Other.ReferenceHandler);
        Other.ReferenceHandler = ::Move(OldReferenceCounter);
    }

    /**
     * @brief  - Checks weather there are any strong references left
     * @return - Returns true if the strong reference count is less than one
     */
    NODISCARD FORCEINLINE bool IsExpired() const noexcept
    {
        return (ReferenceHandler.GetStrongReferenceCount() < 1);
    }

    /**
     * @brief  - Creates a shared pointer from this WeakPtr
     * @return - A new SharedPtr that holds the same pointer as this WeakPtr
     */
    NODISCARD FORCEINLINE TSharedPtr<InObjectType> ToSharedPtr() noexcept
    {
        return TSharedPtr<InObjectType>(*this);
    }

    /**
     * @brief  - Checks weather the strong reference count is one 
     * @return - Returns true if the strong reference count is one
     */
    NODISCARD FORCEINLINE bool IsUnique() const noexcept
    {
        return (ReferenceHandler.GetStrongReferenceCount() == 1);
    }

    /**
     * @brief  - Checks weather the pointer is nullptr or not and the pointer is not expired
     * @return - Returns true if the pointer is not nullptr and the strong reference count is more than zero
     */
    NODISCARD FORCEINLINE bool IsValid() const noexcept
    {
        return (Object != nullptr) && !IsExpired();
    }

    /**
     * @brief  - Retrieve the stored pointer
     * @return - Returns the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType* Get() const noexcept
    {
        return Object;
    }

    /**
     * @brief  - Returns the number of strong references 
     * @return - The number of strong references
     */
    NODISCARD FORCEINLINE auto GetStrongReferenceCount() const noexcept
    {
        return ReferenceHandler.GetStrongReferenceCount();
    }

    /**
     * @brief  - Returns the number of weak references
     * @return - The number of weak references
     */
    NODISCARD FORCEINLINE auto GetWeakReferenceCount() const noexcept
    {
        return ReferenceHandler.GetWeakReferenceCount();
    }

public:

    /**
     * @brief  - Retrieve the stored pointer
     * @return - Returns the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType* operator->() const noexcept
    {
        return Object;
    }

    /**
     * @brief  - Dereference the pointer
     * @return - A reference to the object pointer to by the stored pointer
     */
    NODISCARD FORCEINLINE ObjectType& operator*() const noexcept
    {
        return *Object;
    }

    /**
     * @brief       - Retrieve an element at a certain index 
     * @param Index - Index of the element to retrieve
     * @return      - A reference to the retrieved element
     */
    NODISCARD FORCEINLINE ObjectType& operator[](SizeType Index) const noexcept requires(TIsUnboundedArray<InObjectType>::Value)
    {
        CHECK(IsValid());
        return Object[Index];
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
    template<typename OtherObjectType>
    FORCEINLINE TWeakPtr& operator=(const TWeakPtr<OtherObjectType, ThreadAccess>& Other) noexcept requires(TIsPointerConvertible<OtherObjectType, ObjectType>::Value)
    {
        TWeakPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator that takes a convertible type
     * @param Other - WeakPtr to move from
     * @return      - Return the reference to this instance
     */
    template<typename OtherObjectType>
    FORCEINLINE TWeakPtr& operator=(TWeakPtr<OtherObjectType, ThreadAccess>&& Other) noexcept requires(TIsPointerConvertible<OtherObjectType, ObjectType>::Value)
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
    ObjectType* Object{nullptr};
    SharedPointerInternal::FWeakReference<ThreadAccess> ReferenceHandler;
};


template<typename SharedType, EThreadAccess InThreadAccess = EThreadAccess::Safe>
class TSharedFromThis
{
    template<typename ObjectType, EThreadAccess ThreadAccess>
    friend class TSharedPtr;

protected:
    ~TSharedFromThis() = default;

    TSharedFromThis()
        : WeakThisPointer()
    {
    }

    TSharedFromThis(const TSharedFromThis&)
        : WeakThisPointer()
    {
    }

    TSharedFromThis& operator=(const TSharedFromThis&)
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


template<typename ObjectType, EThreadAccess ThreadAccess = EThreadAccess::Safe, typename... ArgTypes>
NODISCARD FORCEINLINE TSharedPtr<ObjectType> MakeShared(ArgTypes&&... Args) noexcept requires(TNot<TIsArray<ObjectType>>::Value)
{
    auto NewReferenceCounter = new SharedPointerInternal::TObjectReferenceHandler<ObjectType, ThreadAccess>(::Forward<ArgTypes>(Args)...);
    return TSharedPtr<ObjectType, ThreadAccess>(SharedPointerInternal::TSharedReferenceProxy(NewReferenceCounter->GetObjectPointer(), NewReferenceCounter));
}

template<typename ObjectType, EThreadAccess ThreadAccess = EThreadAccess::Safe>
NODISCARD FORCEINLINE TSharedPtr<ObjectType> MakeShared() noexcept requires(TIsBoundedArray<ObjectType>::Value)
{
    auto NewReferenceCounter = new SharedPointerInternal::TObjectReferenceHandler<ObjectType, ThreadAccess>();
    return TSharedPtr<ObjectType, ThreadAccess>(SharedPointerInternal::TSharedReferenceProxy(NewReferenceCounter->GetObjectPointer(), NewReferenceCounter));
}

template<typename ObjectType, EThreadAccess ThreadAccess = EThreadAccess::Safe>
NODISCARD FORCEINLINE TSharedPtr<ObjectType> MakeShared(uint32 Size) noexcept requires(TIsUnboundedArray<ObjectType>::Value)
{
    typedef typename TRemoveExtent<ObjectType>::Type ConstructType;
    return TSharedPtr<ObjectType, ThreadAccess>(new ConstructType[Size]);
}


template<typename ToType, typename FromType>
NODISCARD FORCEINLINE TSharedPtr<ToType> StaticCastSharedPtr(const TSharedPtr<FromType>& Object) noexcept requires(TIsArray<ToType>::Value == TIsArray<FromType>::Value)
{
    typedef typename TRemoveExtent<ToType>::Type Type;
    Type* NewObject = static_cast<Type*>(Object.Get());
    return TSharedPtr<ToType>(Object, NewObject);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE TSharedPtr<ToType> StaticCastSharedPtr(TSharedPtr<FromType>&& Object) noexcept requires(TIsArray<ToType>::Value == TIsArray<FromType>::Value)
{
    typedef typename TRemoveExtent<ToType>::Type Type;
    Type* NewObject = static_cast<Type*>(Object.Get());
    return TSharedPtr<ToType>(::Move(Object), NewObject);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE TSharedPtr<ToType> ConstCastSharedPtr(const TSharedPtr<FromType>& Object) noexcept requires(TIsArray<ToType>::Value == TIsArray<FromType>::Value)
{
    typedef typename TRemoveExtent<ToType>::Type Type;
    Type* NewObject = const_cast<Type*>(Object.Get());
    return TSharedPtr<ToType>(Object, NewObject);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE TSharedPtr<ToType> ConstCastSharedPtr(TSharedPtr<FromType>&& Object) noexcept requires(TIsArray<ToType>::Value == TIsArray<FromType>::Value)
{
    typedef typename TRemoveExtent<ToType>::Type Type;
    Type* NewObject = const_cast<Type*>(Object.Get());
    return TSharedPtr<ToType>(::Move(Object), NewObject);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE TSharedPtr<ToType> ReinterpretCastSharedPtr(const TSharedPtr<FromType>& Object) noexcept requires(TIsArray<ToType>::Value == TIsArray<FromType>::Value)
{
    typedef typename TRemoveExtent<ToType>::Type Type;
    Type* NewObject = reinterpret_cast<Type*>(Object.Get());
    return TSharedPtr<ToType>(Object, NewObject);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE TSharedPtr<ToType> ReinterpretCastSharedPtr(TSharedPtr<FromType>&& Object) noexcept requires(TIsArray<ToType>::Value == TIsArray<FromType>::Value)
{
    typedef typename TRemoveExtent<ToType>::Type Type;
    Type* NewObject = reinterpret_cast<Type*>(Object.Get());
    return TSharedPtr<ToType>(::Move(Object), NewObject);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE TSharedPtr<ToType> DynamicCastSharedPtr(const TSharedPtr<FromType>& Object) noexcept requires(TIsArray<ToType>::Value == TIsArray<FromType>::Value)
{
    typedef typename TRemoveExtent<ToType>::Type Type;
    Type* NewObject = dynamic_cast<Type*>(Object.Get());
    return TSharedPtr<ToType>(Object, NewObject);
}

template<typename ToType, typename FromType>
NODISCARD FORCEINLINE TSharedPtr<ToType> DynamicCastSharedPtr(TSharedPtr<FromType>&& Object) noexcept requires(TIsArray<ToType>::Value == TIsArray<FromType>::Value)
{
    typedef typename TRemoveExtent<ToType>::Type Type;
    Type* NewObject = dynamic_cast<Type*>(Object.Get());
    return TSharedPtr<ToType>(::Move(Object), NewObject);
}

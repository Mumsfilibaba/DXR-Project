#pragma once
#include "UniquePtr.h"

// PtrControlBlock - Counting references in TWeak- and TSharedPtr

struct PtrControlBlock
{
public:
    typedef uint32 RefType;

     PtrControlBlock() noexcept
        : mWeakRefs(0)
        , mStrongRefs(0)
    {
    }

    RefType AddWeakRef() noexcept { return mWeakRefs++; }
    RefType AddStrongRef() noexcept { return mStrongRefs++; }

    RefType ReleaseWeakRef() noexcept { return mWeakRefs--; }
    RefType ReleaseStrongRef() noexcept { return mStrongRefs--; }

    RefType GetWeakReferences() const noexcept { return mWeakRefs; }
    RefType GetStrongReferences() const noexcept { return mStrongRefs; }

private:
    RefType mWeakRefs;
    RefType mStrongRefs;
};

// TDelete

template<typename T>
struct TDelete
{
    using TType = T;

    void operator()(TType* Ptr) noexcept
    {
        delete Ptr;
    }
};

template<typename T>
struct TDelete<T[]>
{
    using TType = TRemoveExtent<T>;

    void operator()(TType* Ptr) noexcept
    {
        delete[] Ptr;
    }
};

// TPtrBase - Base class for TWeak- and TSharedPtr

template<typename T, typename D>
class TPtrBase
{
public:
    template<typename TOther, typename DOther>
    friend class TPtrBase;

    T* Get() const noexcept { return mPtr; }
    T* const* GetAddressOf() const noexcept { return &mPtr; }

    PtrControlBlock::RefType GetStrongReferences() const noexcept
    {
        return mCounter ? mCounter->GetStrongReferences() : 0;
    }

    PtrControlBlock::RefType GetWeakReferences() const noexcept
    {
        return mCounter ? mCounter->GetWeakReferences() : 0;
    }

    T* const* operator&() const noexcept { return GetAddressOf(); }

    bool operator==(T* Ptr) const noexcept { return (mPtr == Ptr); }
    bool operator!=(T* Ptr) const noexcept { return !(*this == Ptr); }

    operator bool() const noexcept { return (mPtr != nullptr); }

protected:
    TPtrBase() noexcept
        : mPtr(nullptr)
        , mCounter(nullptr)
    {
        static_assert(std::is_array_v<T> == std::is_array_v<D>, "Scalar types must have scalar TDelete");
        static_assert(std::is_invocable<D, T*>(), "TDelete must be a callable");
    }

    void InternalAddStrongRef() noexcept
    {
        // If the object has a Ptr there must be a Counter or something went wrong
        if (mPtr)
        {
            Assert(mCounter != nullptr);
            mCounter->AddStrongRef();
        }
    }

    void InternalAddWeakRef() noexcept
    {
        // If the object has a Ptr there must be a Counter or something went wrong
        if (mPtr)
        {
            Assert(mCounter != nullptr);
            mCounter->AddWeakRef();
        }
    }

    void InternalReleaseStrongRef() noexcept
    {
        // If the object has a Ptr there must be a Counter or something went wrong
        if (mPtr)
        {
            Assert(mCounter != nullptr);
            mCounter->ReleaseStrongRef();

            // When releasing the last strong reference we can destroy the pointer and counter
            if (mCounter->GetStrongReferences() <= 0)
            {
                if (mCounter->GetWeakReferences() <= 0)
                {
                    delete mCounter;
                }
                
                mDeleter(mPtr);
                InternalClear();
            }
        }
    }

    void InternalReleaseWeakRef() noexcept
    {
        // If the object has a Ptr there must be a Counter or something went wrong
        if (mPtr)
        {
            Assert(mCounter != nullptr);
            mCounter->ReleaseWeakRef();
            
            PtrControlBlock::RefType StrongRefs = mCounter->GetStrongReferences();
            PtrControlBlock::RefType WeakRefs   = mCounter->GetWeakReferences();
            if (WeakRefs <= 0 && StrongRefs <= 0)
            {
                delete mCounter;
            }
        }
    }

    void InternalSwap(TPtrBase& Other) noexcept
    {
        T* TempPtr = mPtr;
        PtrControlBlock* TempBlock = mCounter;

        mPtr     = Other.mPtr;
        mCounter = Other.mCounter;

        Other.mPtr     = TempPtr;
        Other.mCounter = TempBlock;
    }

    void InternalMove(TPtrBase&& Other) noexcept
    {
        mPtr     = Other.mPtr;
        mCounter = Other.mCounter;

        Other.mPtr     = nullptr;
        Other.mCounter = nullptr;
    }

    template<typename TOther, typename DOther>
    void InternalMove(TPtrBase<TOther, DOther>&& Other) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        mPtr     = static_cast<TOther*>(Other.mPtr);
        mCounter = Other.mCounter;

        Other.mPtr     = nullptr;
        Other.mCounter = nullptr;
    }

    void InternalConstructStrong(T* Ptr) noexcept
    {
        mPtr     = Ptr;
        mCounter = new PtrControlBlock();
        InternalAddStrongRef();
    }

    template<typename TOther, typename DOther>
    void InternalConstructStrong(TOther* Ptr) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        mPtr     = static_cast<T*>(Ptr);
        mCounter = new PtrControlBlock();
        InternalAddStrongRef();
    }

    void InternalConstructStrong(const TPtrBase& Other) noexcept
    {
        mPtr     = Other.mPtr;
        mCounter = Other.mCounter;
        InternalAddStrongRef();
    }

    template<typename TOther, typename DOther>
    void InternalConstructStrong(const TPtrBase<TOther, DOther>& Other) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        mPtr     = static_cast<T*>(Other.mPtr);
        mCounter = Other.mCounter;
        InternalAddStrongRef();
    }
    
    template<typename TOther, typename DOther>
    void InternalConstructStrong(const TPtrBase<TOther, DOther>& Other, T* Ptr) noexcept
    {
        mPtr     = Ptr;
        mCounter = Other.mCounter;
        InternalAddStrongRef();
    }
    
    template<typename TOther, typename DOther>
    void InternalConstructStrong(TPtrBase<TOther, DOther>&& Other, T* Ptr) noexcept
    {
        mPtr     = Ptr;
        mCounter = Other.mCounter;

        Other.mPtr     = nullptr;
        Other.mCounter = nullptr;
    }

    void InternalConstructWeak(T* Ptr) noexcept
    {
        mPtr		= Ptr;
        mCounter	= new PtrControlBlock();
        InternalAddWeakRef();
    }

    template<typename TOther>
    void InternalConstructWeak(TOther* Ptr) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        mPtr     = static_cast<T*>(Ptr);
        mCounter = new PtrControlBlock();
        InternalAddWeakRef();
    }

    void InternalConstructWeak(const TPtrBase& Other) noexcept
    {
        mPtr     = Other.mPtr;
        mCounter = Other.mCounter;
        InternalAddWeakRef();
    }

    template<typename TOther, typename DOther>
    void InternalConstructWeak(const TPtrBase<TOther, DOther>& Other) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        mPtr     = static_cast<T*>(Other.mPtr);
        mCounter = Other.mCounter;
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
        mPtr     = nullptr;
        mCounter = nullptr;
    }

protected:
    T* mPtr;
    PtrControlBlock* mCounter;
    D mDeleter;
};

// Forward Declarations

template<typename TOther>
class TWeakPtr;

// TSharedPtr - RefCounted Scalar Pointer, similar to std::shared_ptr

template<typename T>
class TSharedPtr : public TPtrBase<T, TDelete<T>>
{
    using TBase = TPtrBase<T, TDelete<T>>;

public:
    TSharedPtr() noexcept
        : TBase()
    {
    }

    TSharedPtr(std::nullptr_t) noexcept
        : TBase()
    {
    }

    explicit TSharedPtr(T* Ptr) noexcept
        : TBase()
    {
        TBase::InternalConstructStrong(Ptr);
    }

    TSharedPtr(const TSharedPtr& Other) noexcept
        : TBase()
    {
        TBase::InternalConstructStrong(Other);
    }

    TSharedPtr(TSharedPtr&& Other) noexcept
        : TBase()
    {
        TBase::InternalMove(::Move(Other));
    }

    template<typename TOther>
    TSharedPtr(const TSharedPtr<TOther>& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        TBase::template InternalConstructStrong<TOther>(Other);
    }

    template<typename TOther>
    TSharedPtr(TSharedPtr<TOther>&& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        TBase::template InternalMove<TOther>(::Move(Other));
    }
    
    template<typename TOther>
    TSharedPtr(const TSharedPtr<TOther>& Other, T* Ptr) noexcept
        : TBase()
    {
        TBase::template InternalConstructStrong<TOther>(Other, Ptr);
    }
    
    template<typename TOther>
    TSharedPtr(TSharedPtr<TOther>&& Other, T* Ptr) noexcept
        : TBase()
    {
        TBase::template InternalConstructStrong<TOther>(::Move(Other), Ptr);
    }

    template<typename TOther>
    explicit TSharedPtr(const TWeakPtr<TOther>& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        TBase::template InternalConstructStrong<TOther>(Other);
    }

    template<typename TOther>
    TSharedPtr(TUniquePtr<TOther>&& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        TBase::template InternalConstructStrong<TOther, TDelete<T>>(Other.Release());
    }

    ~TSharedPtr() noexcept
    {
        Reset();
    }

    void Reset() noexcept { TBase::InternalDestructStrong(); }

    void Swap(TSharedPtr& Other) noexcept { TBase::InternalSwap(Other); }

    bool IsUnique() const noexcept { return (TBase::GetStrongReferences() == 1); }

    T* operator->() const noexcept { return TBase::Get(); }
    
    T& operator*() const noexcept
    {
        Assert(TBase::mPtr != nullptr);
        return *TBase::mPtr;
    }
    
    TSharedPtr& operator=(const TSharedPtr& Other) noexcept
    {
        TSharedPtr(Other).Swap(*this);
        return *this;
    }

    TSharedPtr& operator=(TSharedPtr&& Other) noexcept
    {
        TSharedPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    template<typename TOther>
    TSharedPtr& operator=(const TSharedPtr<TOther>& Other) noexcept
    {
        TSharedPtr(Other).Swap(*this);
        return *this;
    }

    template<typename TOther>
    TSharedPtr& operator=(TSharedPtr<TOther>&& Other) noexcept
    {
        TSharedPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    TSharedPtr& operator=(T* Ptr) noexcept
    {
        if (TBase::mPtr != Ptr)
        {
            Reset();
            TBase::InternalConstructStrong(Ptr);
        }

        return *this;
    }

    TSharedPtr& operator=(std::nullptr_t) noexcept
    {
        Reset();
        return *this;
    }

    bool operator==(const TSharedPtr& Other) const noexcept { return (TBase::mPtr == Other.mPtr); }
    bool operator!=(const TSharedPtr& Other) const noexcept { return !(*this == Other); }
};

// TSharedPtr - RefCounted Pointer for array types, similar to std::shared_ptr

template<typename T>
class TSharedPtr<T[]> : public TPtrBase<T, TDelete<T[]>>
{
    using TBase = TPtrBase<T, TDelete<T[]>>;

public:
    TSharedPtr() noexcept
        : TBase()
    {
    }

    TSharedPtr(std::nullptr_t) noexcept
        : TBase()
    {
    }

    explicit TSharedPtr(T* Ptr) noexcept
        : TBase()
    {
        TBase::InternalConstructStrong(Ptr);
    }

    TSharedPtr(const TSharedPtr& Other) noexcept
        : TBase()
    {
        TBase::InternalConstructStrong(Other);
    }

    TSharedPtr(TSharedPtr&& Other) noexcept
        : TBase()
    {
        TBase::InternalMove(::Move(Other));
    }

    template<typename TOther>
    TSharedPtr(const TSharedPtr<TOther[]>& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        TBase::template InternalConstructStrong<TOther>(Other);
    }
    
    template<typename TOther>
    TSharedPtr(const TSharedPtr<TOther[]>& Other, T* Ptr) noexcept
        : TBase()
    {
        TBase::template InternalConstructStrong<TOther>(Other, Ptr);
    }
    
    template<typename TOther>
    TSharedPtr(TSharedPtr<TOther[]>&& Other, T* Ptr) noexcept
        : TBase()
    {
        TBase::template InternalConstructStrong<TOther>(::Move(Other), Ptr);
    }

    template<typename TOther>
    TSharedPtr(TSharedPtr<TOther[]>&& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        TBase::template InternalMove<TOther>(::Move(Other));
    }

    template<typename TOther>
    explicit TSharedPtr(const TWeakPtr<TOther[]>& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        TBase::template InternalConstructStrong<TOther>(Other);
    }

    template<typename TOther>
    TSharedPtr(TUniquePtr<TOther[]>&& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>());
        TBase::template InternalConstructStrong<TOther>(Other.Release());
    }

    ~TSharedPtr()
    {
        Reset();
    }

    void Reset() noexcept { TBase::InternalDestructStrong(); }

    void Swap(TSharedPtr& Other) noexcept { TBase::InternalSwap(Other); }

    bool IsUnique() const noexcept { return (TBase::GetStrongReferences() == 1); }

    T& operator[](uint32 Index) noexcept
    {
        Assert(TBase::mPtr != nullptr);
        return TBase::mPtr[Index];
    }
    
    TSharedPtr& operator=(const TSharedPtr& Other) noexcept
    {
        TSharedPtr(Other).Swap(*this);
        return *this;
    }

    TSharedPtr& operator=(TSharedPtr&& Other) noexcept
    {
        TSharedPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    template<typename TOther>
    TSharedPtr& operator=(const TSharedPtr<TOther[]>& Other) noexcept
    {
        TSharedPtr(Other).Swap(*this);
        return *this;
    }

    template<typename TOther>
    TSharedPtr& operator=(TSharedPtr<TOther[]>&& Other) noexcept
    {
        TSharedPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    TSharedPtr& operator=(T* Ptr) noexcept
    {
        if (this->Ptr != Ptr)
        {
            Reset();
            TBase::InternalConstructStrong(Ptr);
        }

        return *this;
    }

    TSharedPtr& operator=(std::nullptr_t) noexcept
    {
        Reset();
        return *this;
    }

    bool operator==(const TSharedPtr& Other) const noexcept { return (TBase::mPtr == Other.mPtr); }
    bool operator!=(const TSharedPtr& Other) const noexcept { return !(*this == Other); }
};

// TWeakPtr - Weak Pointer for scalar types, similar to std::weak_ptr

template<typename T>
class TWeakPtr : public TPtrBase<T, TDelete<T>>
{
    using TBase = TPtrBase<T, TDelete<T>>;

public:
    TWeakPtr() noexcept
        : TBase()
    {
    }

    TWeakPtr(const TSharedPtr<T>& Other) noexcept
        : TBase()
    {
        TBase::InternalConstructWeak(Other);
    }

    template<typename TOther>
    TWeakPtr(const TSharedPtr<TOther>& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>(), "TWeakPtr: Trying to convert non-convertable types");
        TBase::template InternalConstructWeak<TOther>(Other);
    }

    TWeakPtr(const TWeakPtr& Other) noexcept
        : TBase()
    {
        TBase::InternalConstructWeak(Other);
    }

    TWeakPtr(TWeakPtr&& Other) noexcept
        : TBase()
    {
        TBase::InternalMove(::Move(Other));
    }

    template<typename TOther>
    TWeakPtr(const TWeakPtr<TOther>& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>(), "TWeakPtr: Trying to convert non-convertable types");
        TBase::template InternalConstructWeak<TOther>(Other);
    }

    template<typename TOther>
    TWeakPtr(TWeakPtr<TOther>&& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>(), "TWeakPtr: Trying to convert non-convertable types");
        TBase::template InternalMove<TOther>(::Move(Other));
    }

    ~TWeakPtr()
    {
        Reset();
    }

    void Reset() noexcept { TBase::InternalDestructWeak(); }

    void Swap(TWeakPtr& Other) noexcept { TBase::InternalSwap(Other); }

    bool IsExpired() const noexcept { return (TBase::GetStrongReferences() < 1); }

    TSharedPtr<T> MakeShared() noexcept
    {
        const TWeakPtr& This = *this;
        return ::Move(TSharedPtr<T>(This));
    }

    T* operator->() const noexcept { return TBase::Get(); }
    
    T& operator*() const noexcept
    {
        Assert(TBase::mPtr != nullptr);
        return *TBase::mPtr;
    }
    
    TWeakPtr& operator=(const TWeakPtr& Other) noexcept
    {
        TWeakPtr(Other).Swap(*this);
        return *this;
    }

    TWeakPtr& operator=(TWeakPtr&& Other) noexcept
    {
        TWeakPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    template<typename TOther>
    TWeakPtr& operator=(const TWeakPtr<TOther>& Other) noexcept
    {
        TWeakPtr(Other).Swap(*this);
        return *this;
    }

    template<typename TOther>
    TWeakPtr& operator=(TWeakPtr<TOther>&& Other) noexcept
    {
        TWeakPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    TWeakPtr& operator=(T* Ptr) noexcept
    {
        if (TBase::mPtr != Ptr)
        {
            Reset();
            TBase::InternalConstructWeak(Ptr);
        }

        return *this;
    }

    TWeakPtr& operator=(std::nullptr_t) noexcept
    {
        Reset();
        return *this;
    }

    bool operator==(const TWeakPtr& Other) const noexcept { return (TBase::mPtr == Other.mPtr); }
    bool operator!=(const TWeakPtr& Other) const noexcept { return !(*this == Other); }
};

// TWeakPtr - Weak Pointer for array types, similar to std::weak_ptr

template<typename T>
class TWeakPtr<T[]> : public TPtrBase<T, TDelete<T[]>>
{
    using TBase = TPtrBase<T, TDelete<T[]>>;

public:
    TWeakPtr() noexcept
        : TBase()
    {
    }

    TWeakPtr(const TSharedPtr<T>& Other) noexcept
        : TBase()
    {
        TBase::InternalConstructWeak(Other);
    }

    template<typename TOther>
    TWeakPtr(const TSharedPtr<TOther[]>& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>(), "TWeakPtr: Trying to convert non-convertable types");
        TBase::template InternalConstructWeak<TOther>(Other);
    }

    TWeakPtr(const TWeakPtr& Other) noexcept
        : TBase()
    {
        TBase::InternalConstructWeak(Other);
    }

    TWeakPtr(TWeakPtr&& Other) noexcept
        : TBase()
    {
        TBase::InternalMove(::Move(Other));
    }

    template<typename TOther>
    TWeakPtr(const TWeakPtr<TOther[]>& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>(), "TWeakPtr: Trying to convert non-convertable types");
        TBase::template InternalConstructWeak<TOther>(Other);
    }

    template<typename TOther>
    TWeakPtr(TWeakPtr<TOther[]>&& Other) noexcept
        : TBase()
    {
        static_assert(std::is_convertible<TOther*, T*>(), "TWeakPtr: Trying to convert non-convertable types");
        TBase::template InternalMove<TOther>(::Move(Other));
    }

    ~TWeakPtr()
    {
        Reset();
    }

    void Reset() noexcept { TBase::InternalDestructWeak(); }

    void Swap(TWeakPtr& Other) noexcept { TBase::InternalSwap(Other); }

    bool IsExpired() const noexcept { return (TBase::GetStrongReferences() < 1); }

    TSharedPtr<T[]> MakeShared() noexcept
    {
        const TWeakPtr& This = *this;
        return ::Move(TSharedPtr<T[]>(This));
    }
    
    T& operator[](uint32 Index) noexcept
    {
        Assert(TBase::mPtr != nullptr);
        return TBase::mPtr[Index];
    }

    TWeakPtr& operator=(const TWeakPtr& Other) noexcept
    {
        TWeakPtr(Other).Swap(*this);
        return *this;
    }

    TWeakPtr& operator=(TWeakPtr&& Other) noexcept
    {
        TWeakPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    template<typename TOther>
    TWeakPtr& operator=(const TWeakPtr<TOther[]>& Other) noexcept
    {
        TWeakPtr(Other).Swap(*this);
        return *this;
    }

    template<typename TOther>
    TWeakPtr& operator=(TWeakPtr<TOther[]>&& Other) noexcept
    {
        TWeakPtr(::Move(Other)).Swap(*this);
        return *this;
    }

    TWeakPtr& operator=(T* Ptr) noexcept
    {
        if (TBase::mPtr != Ptr)
        {
            Reset();
            TBase::InternalConstructWeak(Ptr);
        }

        return *this;
    }

    TWeakPtr& operator=(std::nullptr_t) noexcept
    {
        Reset();
        return *this;
    }

    bool operator==(const TWeakPtr& Other) const noexcept { return (TBase::mPtr == Other.mPtr); }
    bool operator!=(const TWeakPtr& Other) const noexcept { return !(*this == Other); }
};

// MakeShared - Creates a new object together with a SharedPtr

template<typename T, typename... TArgs>
TEnableIf<!TIsArray<T>, TSharedPtr<T>> MakeShared(TArgs&&... Args) noexcept
{
    T* RefCountedPtr = new T(Forward<TArgs>(Args)...);
    return ::Move(TSharedPtr<T>(RefCountedPtr));
}

template<typename T>
TEnableIf<TIsArray<T>, TSharedPtr<T>> MakeShared(uint32 Size) noexcept
{
    using TType = TRemoveExtent<T>;

    TType* RefCountedPtr = new TType[Size];
    return ::Move(TSharedPtr<T>(RefCountedPtr));
}

// Casting functions

// static_cast
template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> StaticCast(const TSharedPtr<T1>& Pointer) noexcept
{
    using TType = TRemoveExtent<T0>;
    
    TType* RawPointer = static_cast<TType*>(Pointer.Get());
    return ::Move(TSharedPtr<T0>(Pointer, RawPointer));
}

template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> StaticCast(TSharedPtr<T1>&& Pointer) noexcept
{
    using TType = TRemoveExtent<T0>;
    
    TType* RawPointer = static_cast<TType*>(Pointer.Get());
    return ::Move(TSharedPtr<T0>(::Move(Pointer), RawPointer));
}

// const_cast
template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> ConstCast(const TSharedPtr<T1>& Pointer) noexcept
{
    using TType = TRemoveExtent<T0>;
    
    TType* RawPointer = const_cast<TType*>(Pointer.Get());
    return ::Move(TSharedPtr<T0>(Pointer, RawPointer));
}

template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> ConstCast(TSharedPtr<T1>&& Pointer) noexcept
{
    using TType = TRemoveExtent<T0>;
    
    TType* RawPointer = const_cast<TType*>(Pointer.Get());
    return ::Move(TSharedPtr<T0>(::Move(Pointer), RawPointer));
}

// reinterpret_cast
template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> ReinterpretCast(const TSharedPtr<T1>& Pointer) noexcept
{
    using TType = TRemoveExtent<T0>;
    
    TType* RawPointer = reinterpret_cast<TType*>(Pointer.Get());
    return ::Move(TSharedPtr<T0>(Pointer, RawPointer));
}

template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> ReinterpretCast(TSharedPtr<T1>&& Pointer) noexcept
{
    using TType = TRemoveExtent<T0>;
    
    TType* RawPointer = reinterpret_cast<TType*>(Pointer.Get());
    return ::Move(TSharedPtr<T0>(::Move(Pointer), RawPointer));
}

// dynamic_cast
template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> DynamicCast(const TSharedPtr<T1>& Pointer) noexcept
{
    using TType = TRemoveExtent<T0>;
    
    TType* RawPointer = dynamic_cast<TType*>(Pointer.Get());
    return ::Move(TSharedPtr<T0>(Pointer, RawPointer));
}

template<typename T0, typename T1>
TEnableIf<TIsArray<T0> == TIsArray<T1>, TSharedPtr<T0>> DynamicCast(TSharedPtr<T1>&& Pointer) noexcept
{
    using TType = TRemoveExtent<T0>;
    
    TType* RawPointer = dynamic_cast<TType*>(Pointer.Get());
    return ::Move(TSharedPtr<T0>(::Move(Pointer), RawPointer));
}

#pragma once
#include "Utilities.h"

// TUniquePtr - Scalar values. Similar to std::unique_ptr

template<typename T>
class TUniquePtr
{
public:
    template<typename TOther>
    friend class TUniquePtr;

    TUniquePtr(const TUniquePtr& Other) = delete;
    TUniquePtr& operator=(const TUniquePtr& Other) noexcept = delete;

    TUniquePtr() noexcept
        : mPtr(nullptr)
    {
    }

    TUniquePtr(std::nullptr_t) noexcept
        : mPtr(nullptr)
    {
    }

    explicit TUniquePtr(T* InPtr) noexcept
        : mPtr(InPtr)
    {
    }

    TUniquePtr(TUniquePtr&& Other) noexcept
        : mPtr(Other.mPtr)
    {
        Other.mPtr = nullptr;
    }

    template<typename TOther>
    TUniquePtr(TUniquePtr<TOther>&& Other) noexcept
        : mPtr(Other.mPtr)
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Other.mPtr = nullptr;
    }

    ~TUniquePtr()
    {
        Reset();
    }

    T* Release() noexcept
    {
        T* WeakPtr = mPtr;
        mPtr = nullptr;
        return WeakPtr;
    }

    void Reset() noexcept
    {
        InternalRelease();
        mPtr = nullptr;
    }

    void Swap(TUniquePtr& Other) noexcept
    {
        T* TempPtr = mPtr;
        mPtr       = Other.mPtr;
        Other.mPtr = TempPtr;
    }

    T* Get() const noexcept { return mPtr; }
    T* const* GetAddressOf() const noexcept { return &mPtr; }

    T* operator->() const noexcept { return Get(); }
    T& operator*() const noexcept { return (*mPtr); }
    T* const* operator&() const noexcept { return GetAddressOf(); }

    TUniquePtr& operator=(T* InPtr) noexcept
    {
        if (mPtr != InPtr)
        {
            Reset();
            mPtr = InPtr;
        }

        return *this;
    }

    TUniquePtr& operator=(TUniquePtr&& Other) noexcept
    {
        if (this != std::addressof(Other))
        {
            Reset();
            mPtr       = Other.mPtr;
            Other.mPtr = nullptr;
        }

        return *this;
    }

    template<typename TOther>
    TUniquePtr& operator=(TUniquePtr<TOther>&& Other) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        if (this != std::addressof(Other))
        {
            Reset();
            mPtr       = Other.mPtr;
            Other.mPtr = nullptr;
        }

        return *this;
    }

    TUniquePtr& operator=(std::nullptr_t) noexcept
    {
        Reset();
        return *this;
    }

    bool operator==(const TUniquePtr& Other) const noexcept { return (mPtr == Other.mPtr); }
    bool operator!=(const TUniquePtr& Other) const noexcept { return !(*this == Other); }

    bool operator==(T* InPtr) const noexcept { return (mPtr == InPtr); }
    bool operator!=(T* InPtr) const noexcept { return !(*this == Other); }

    operator bool() const noexcept { return (mPtr != nullptr); }

private:
    void InternalRelease() noexcept
    {
        if (mPtr)
        {
            delete mPtr;
            mPtr = nullptr;
        }
    }

    T* mPtr;
};

// TUniquePtr - Array values. Similar to std::unique_ptr

template<typename T>
class TUniquePtr<T[]>
{
public:
    template<typename TOther>
    friend class TUniquePtr;

    TUniquePtr(const TUniquePtr& Other) = delete;
    TUniquePtr& operator=(const TUniquePtr& Other) noexcept = delete;

    TUniquePtr() noexcept
        : mPtr(nullptr)
    {
    }

    TUniquePtr(std::nullptr_t) noexcept
        : mPtr(nullptr)
    {
    }

    explicit TUniquePtr(T* InPtr) noexcept
        : mPtr(InPtr)
    {
    }

    TUniquePtr(TUniquePtr&& Other) noexcept
        : mPtr(Other.mPtr)
    {
        Other.mPtr = nullptr;
    }

    template<typename TOther>
    TUniquePtr(TUniquePtr<TOther>&& Other) noexcept
        : mPtr(Other.mPtr)
    {
        static_assert(std::is_convertible<TOther*, T*>());
        Other.mPtr = nullptr;
    }

    ~TUniquePtr()
    {
        Reset();
    }

    T* Release() noexcept
    {
        T* WeakPtr = mPtr;
        mPtr        = nullptr;
        return WeakPtr;
    }

    void Reset() noexcept
    {
        InternalRelease();
        mPtr = nullptr;
    }

    void Swap(TUniquePtr& Other) noexcept
    {
        T* TempPtr = mPtr;
        mPtr        = Other.mPtr;
        Other.mPtr  = TempPtr;
    }

    T* Get() const noexcept { return mPtr; }
    T* const* GetAddressOf() const noexcept { return &mPtr; }

    T* const* operator&() const noexcept { return GetAddressOf(); }

    T& operator[](uint32 Index) noexcept
    {
        Assert(mPtr != nullptr);
        return mPtr[Index];
    }

    TUniquePtr& operator=(T* InPtr) noexcept
    {
        if (mPtr != InPtr)
        {
            Reset();
            mPtr = InPtr;
        }

        return *this;
    }

    TUniquePtr& operator=(TUniquePtr&& Other) noexcept
    {
        if (this != std::addressof(Other))
        {
            Reset();
            mPtr = Other.mPtr;
            Other.mPtr = nullptr;
        }

        return *this;
    }

    template<typename TOther>
    TUniquePtr& operator=(TUniquePtr<TOther>&& Other) noexcept
    {
        static_assert(std::is_convertible<TOther*, T*>());

        if (this != std::addressof(Other))
        {
            Reset();
            mPtr = Other.mPtr;
            Other.mPtr = nullptr;
        }

        return *this;
    }

    TUniquePtr& operator=(std::nullptr_t) noexcept
    {
        Reset();
        return *this;
    }

    bool operator==(const TUniquePtr& Other) const noexcept { return (mPtr == Other.mPtr); }
    bool operator!=(const TUniquePtr& Other) const noexcept { return !(*this == Other); }

    bool operator==(T* InPtr) const noexcept { return (mPtr == InPtr); }
    bool operator!=(T* InPtr) const noexcept { return !(*this == Other); }

    operator bool() const noexcept { return (mPtr != nullptr); }

private:
    void InternalRelease() noexcept
    {
        if (mPtr)
        {
            delete mPtr;
            mPtr = nullptr;
        }
    }

    T* mPtr;
};

// MakeUnique - Creates a new object together with a UniquePtr

template<typename T, typename... TArgs>
TEnableIf<!TIsArray<T>, TUniquePtr<T>> MakeUnique(TArgs&&... Args) noexcept
{
    T* UniquePtr = new T(::Forward<TArgs>(Args)...);
    return ::Move(TUniquePtr<T>(UniquePtr));
}

template<typename T>
TEnableIf<TIsArray<T>, TUniquePtr<T>>MakeUnique(uint32 Size) noexcept
{
    using TType = TRemoveExtent<T>;

    TType* UniquePtr = new TType[Size];
    return ::Move(TUniquePtr<T>(UniquePtr));
}

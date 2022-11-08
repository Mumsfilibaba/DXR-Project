#pragma once
#include "Core/Core.h"

template<typename T>
class TAutoPtr
{
public:
    TAutoPtr(const TAutoPtr&) = delete;
    TAutoPtr& operator=(const TAutoPtr&) = delete;

    FORCEINLINE TAutoPtr()
        : Pointer(nullptr)
    { }

    FORCEINLINE explicit TAutoPtr(T* InPointer)
        : Pointer(InPointer)
    { }

    FORCEINLINE TAutoPtr(TAutoPtr&& Other)
        : Pointer(Other.Pointer)
    { 
        Other.Pointer = nullptr;
    }

    FORCEINLINE ~TAutoPtr()
    {
        Release();
    }

    FORCEINLINE TAutoPtr& operator=(TAutoPtr&& Other)
    {
        Release();
        Pointer = Other.Pointer;
        Other.Pointer = nullptr;
        return *this;
    }

    FORCEINLINE T* operator->() const
    {
        return Pointer;
    }

    FORCEINLINE T& operator*() const
    {
        return *Pointer;
    }

    FORCEINLINE operator bool() const
    {
        return (Pointer != nullptr);
    }

    FORCEINLINE T* Get() const
    {
        return Pointer;
    }

private:
    FORCEINLINE void Release()
    {
        if (Pointer)
        {
            delete Pointer;
            Pointer = nullptr;
        }
    }

    T* Pointer;
};

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TAutoPtr<T>& LHS, U* Other) noexcept
{
	return (LHS.Get() == Other);
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(T* LHS, const TAutoPtr<U>& Other) noexcept
{
	return (LHS == Other.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TAutoPtr<T>& LHS, U* Other) noexcept
{
	return (LHS.Get() != Other);
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(T* LHS, const TAutoPtr<U>& Other) noexcept
{
	return (LHS != Other.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TAutoPtr<T>& LHS, const TAutoPtr<U>& Other) noexcept
{
	return (LHS.Get() == Other.Get());
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TAutoPtr<T>& LHS, const TAutoPtr<U>& Other) noexcept
{
	return (LHS.Get() != Other.Get());
}

template<typename T>
NODISCARD FORCEINLINE bool operator==(const TAutoPtr<T>& LHS, nullptr_type) noexcept
{
	return (LHS.Get() == nullptr);
}

template<typename T>
NODISCARD FORCEINLINE bool operator==(nullptr_type, const TAutoPtr<T>& Other) noexcept
{
	return (nullptr == Other.Get());
}

template<typename T>
NODISCARD FORCEINLINE bool operator!=(const TAutoPtr<T>& LHS, nullptr_type) noexcept
{
	return (LHS.Get() != nullptr);
}

template<typename T>
NODISCARD FORCEINLINE bool operator!=(nullptr_type, const TAutoPtr<T>& Other) noexcept
{
	return (nullptr != Other.Get());
}
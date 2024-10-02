#pragma once
#include "Core.h"
#include "Core/Templates/TypeTraits.h"

struct IRefCounted
{
    virtual ~IRefCounted() = default;

    /** @return - Adds a reference and returns the new reference count */
    virtual int32 AddRef() const = 0;

    /** @return - Removes a reference and returns the new reference count */
    virtual int32 Release() const = 0;

    /** @return - Returns the current reference count */
    virtual int32 GetRefCount() const = 0;
};

template<typename T>
FORCEINLINE typename TEnableIf<TIsBaseOf<IRefCounted, T>::Value, typename TAddPointer<T>::Type>::Type AddRef(T* InRefCounted)
{
    if (InRefCounted)
    {
        InRefCounted->AddRef();
    }

    return InRefCounted;
}
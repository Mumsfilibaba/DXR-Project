#pragma once
#include "CoreModule.h"

#include "Core/Threading/AtomicInt.h"
#include "Core/Templates/IsBaseOf.h"
#include "Core/Templates/EnableIf.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Base-class for intrusive ref-counted object

class CORE_API CRefCounted
{
public:

    CRefCounted();
    virtual ~CRefCounted();

    int32 AddRef();
    int32 Release();

    int32 GetRefCount() const;

private:
    mutable AtomicInt32 StrongReferences;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Add a reference to a RefCounted object safely

template<typename T>
FORCEINLINE typename TEnableIf<TIsBaseOf<CRefCounted, T>::Value, T*>::Type AddRef(T* InRefCounted)
{
    if (InRefCounted)
    {
        InRefCounted->AddRef();
    }

    return InRefCounted;
}

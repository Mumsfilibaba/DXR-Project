#pragma once
#include "IRefCounted.h"

#include "Core/Threading/AtomicInt.h"
#include "Core/Templates/IsBaseOf.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/AddPointer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRefCounted

class CORE_API FRefCounted 
    : public IRefCounted
{
protected:
    FRefCounted();
    virtual ~FRefCounted();

public:
    virtual int32 AddRef()  override;
    virtual int32 Release() override;

    virtual int32 GetRefCount() const override;

private:
    FAtomicInt32 StrongReferences;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Add a reference to a RefCounted object safely

template<typename T>
FORCEINLINE typename TEnableIf<TIsBaseOf<FRefCounted, T>::Value, typename TAddPointer<T>::Type>::Type AddRef(T* InRefCounted)
{
    if (InRefCounted)
    {
        InRefCounted->AddRef();
    }

    return InRefCounted;
}

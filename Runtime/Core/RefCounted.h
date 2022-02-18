#pragma once
#include "Core.h"

#include "Core/Threading/AtomicInt.h"
#include "Core/Templates/IsBaseOf.h"
#include "Core/Templates/EnableIf.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRefCounted - Base-class for intrusive ref-counted object

class CORE_API CRefCounted
{
public:

    // CRefCounted is not copy- or moveable
    CRefCounted(const CRefCounted&) = delete;
    CRefCounted(CRefCounted&&) = delete;
    CRefCounted& operator=(const CRefCounted&) = delete;
    CRefCounted& operator=(CRefCounted&&) = delete;

    CRefCounted();
    virtual ~CRefCounted();

    /**
     * Adds a reference
     * 
     * @return: Returns the new reference count
     */
    int32 AddRef();
    
    /**
     * Removes a reference
     *
     * @return: Returns the new reference count
     */
    int32 Release();

    /**
     * Retrieve the reference count
     *
     * @return: Returns the current reference count
     */
    int32 GetNumReferences() const;

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

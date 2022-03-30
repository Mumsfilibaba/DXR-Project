#pragma once
#include "Core.h"

#include "Core/Threading/AtomicInt.h"
#include "Core/Templates/IsBaseOf.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/ClassUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRefCounted - Base-class for intrusive ref-counted object

class CORE_API CRefCounted : CNonCopyable, CNonMovable
{
public:

    CRefCounted();
    virtual ~CRefCounted();

    /**
     * @brief: Adds a reference
     * 
     * @return: Returns the new reference count
     */
    int32 AddRef();
    
    /**
     * @brief: Removes a reference
     *
     * @return: Returns the new reference count
     */
    int32 Release();

    /**
     * @brief: Retrieve the reference count
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

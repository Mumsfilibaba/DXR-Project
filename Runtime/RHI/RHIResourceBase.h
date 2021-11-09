#pragma once
#include "RHITypes.h"

#include "Core/RefCounted.h"
#include "Core/Containers/String.h"

class CRHIResource : public CRefCounted
{
public:
 
    CRHIResource() = default;
    ~CRHIResource() = default;

    /* Retrive a handle to the native resource, if there is any */
    virtual void* GetNativeResource() const { return nullptr; }

    /* Returns true if the native resource is valid to use */
    virtual bool IsValid() const { return false; }

    virtual void SetName( const CString& InName )
    {
        Name = InName;
    }

    FORCEINLINE const CString& GetName() const
    {
        return Name;
    }

private:
    CString Name;
};
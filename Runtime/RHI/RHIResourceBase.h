#pragma once
#include "RHITypes.h"

#include "Core/RefCounted.h"
#include "Core/Containers/String.h"

class CRHIResource : public CRefCounted
{
public:
    virtual void* GetNativeResource() const
    {
        return nullptr;
    }

    virtual bool IsValid() const
    {
        return false;
    }

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
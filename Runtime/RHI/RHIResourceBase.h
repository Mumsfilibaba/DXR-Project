#pragma once
#include "RHITypes.h"

#include "Core/RefCounted.h"
#include "Core/Containers/String.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CRHIResource : public CRefCounted
{
public:

    CRHIResource() = default;
    ~CRHIResource() = default;

    /* Returns true if the native resource is valid to use */
    virtual bool IsValid() const { return false; }

    /* Retrive a handle to the native resource, nullptr is valid since not all RHI has handles for all resources*/
    virtual void* GetNativeResource() const { return nullptr; }

    /* Sets a debug name on the resource */
    virtual void SetName(const CString& InName) { Name = InName; }

    FORCEINLINE const CString& GetName() const
    {
        return Name;
    }

private:
    CString Name;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CRHIMemoryResource : public CRHIResource
{
public:

    CRHIMemoryResource() = default;
    ~CRHIMemoryResource() = default;

    /* Cast to a buffer */
    virtual class CRHIBuffer* AsBuffer() { return nullptr; }
    /* Cast to a texture */
    virtual class CRHITexture* AsTexture() { return nullptr; }
};
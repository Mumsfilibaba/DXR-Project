#pragma once
#include "RHITypes.h"

#include "Core/RefCounted.h"
#include "Core/Containers/String.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RHIObject

class CRHIObject : public CRefCounted
{
public:

    CRHIObject() = default;
    ~CRHIObject() = default;

    /**
     * Returns true if the native resource is valid to use 
     * 
     * @return: Returns true if the resource is backed by a native resource
     */ 
    virtual bool IsValid() const { return false; }

    /**
     * Retrieve a handle to the native resource, nullptr is valid since not all RHI has handles for all resources
     * 
     * @return: Returns a pointer to the native resource that is currently being used
     */
    virtual void* GetNativeResource() const { return nullptr; }

    /**
     * Sets a debug name on the resource
     * 
     * @param InName: Debug name for the resource
     */
    virtual void SetName(const CString& InName) { Name = InName; }

    /**
     * Retrieve the debug-name
     * 
     * @return: Returns the debug-name
     */
    FORCEINLINE const CString& GetName() const
    {
        return Name;
    }

private:
    CString Name;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RHIResource

class CRHIResource : public CRHIObject
{
public:

    CRHIResource() = default;
    ~CRHIResource() = default;

    /**
     * Cast to a Buffer 
     * 
     * @return: Returns a pointer to a Buffer if the resource or nullptr if its not a Buffer
     */
    virtual class CRHIBuffer* AsBuffer() { return nullptr; }
    
    /**
     * Cast to a Texture
     *
     * @return: Returns a pointer to a Texture if the resource or nullptr if its not a Texture
     */
    virtual class CRHITexture* AsTexture() { return nullptr; }
};
#pragma once
#include "RHITypes.h"

#include "Core/RefCounted.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<class CRHIObject>   CRHIObjectRef;
typedef TSharedRef<class CRHIResource> CRHIResourceRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIObject

class CRHIObject : public CRefCounted
{
public:

    CRHIObject()  = default;
    ~CRHIObject() = default;

    virtual bool IsValid() const { return false; }

    virtual void* GetNativeResource() const { return nullptr; }

    virtual void SetName(const String& InName) { Name = InName; }

    inline const String& GetName() const { return Name; }

private:
    String Name;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIResourceDimension

enum class ERHIResourceType : uint8
{
    Unknown        = 0,
    Buffer         = 1,
    ConstantBuffer = 2,
    Texture        = 3,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIResource

class CRHIResource : public CRHIObject
{
public:

    CRHIResource(ERHIResourceType InResourceType)
        : CRHIObject()
        , Type(InResourceType)
    { }
    
    ~CRHIResource() = default;

    virtual class CRHIBuffer*         AsBuffer()         { return nullptr; }
    virtual class CRHIConstantBuffer* AsConstantBuffer() { return nullptr; }

    virtual class CRHITexture* AsTexture() { return nullptr; }

    inline ERHIResourceType GetType() const { return Type; }

private:
    ERHIResourceType Type;
};

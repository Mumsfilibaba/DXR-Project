#pragma once
#include "MetalObject.h"

#include "RHI/RHIResources.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalBuffer

class CMetalBuffer : public CMetalObject
{
public:
    
    CMetalBuffer(CMetalDeviceContext* DeviceContext)
        : CMetalObject(DeviceContext)
        , Buffer(nil)
    { }
    
    ~CMetalBuffer()
    {
        NSSafeRelease(Buffer);
    }
    
public:
    
    void SetMTLBuffer(id<MTLBuffer> InBuffer) { Buffer = [InBuffer retain]; }
    
    id<MTLBuffer> GetMTLBuffer() const { return Buffer; }
    
private:
    id<MTLBuffer> Buffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalVertexBuffer

class CMetalVertexBuffer : public CMetalBuffer, public CRHIVertexBuffer
{
public:

    CMetalVertexBuffer(CMetalDeviceContext* DeviceContext, const CRHIVertexBufferInitializer& Initializer)
        : CMetalBuffer(DeviceContext)
        , CRHIVertexBuffer(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLBuffer()); }

    virtual void* GetRHIBaseBuffer() override final { return reinterpret_cast<void*>(static_cast<CMetalBuffer*>(this)); }
    
    virtual void SetName(const String& InName) override final
    {
        @autoreleasepool
        {
            id<MTLBuffer> BufferHandle = GetMTLBuffer();
            if (BufferHandle)
            {
                BufferHandle.label = InName.GetNSString();
            }
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalIndexBuffer

class CMetalIndexBuffer : public CMetalBuffer, public CRHIIndexBuffer
{
public:
    
    CMetalIndexBuffer(CMetalDeviceContext* DeviceContext, const CRHIIndexBufferInitializer& Initializer)
        : CMetalBuffer(DeviceContext)
        , CRHIIndexBuffer(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLBuffer()); }

    virtual void* GetRHIBaseBuffer() override final { return reinterpret_cast<void*>(static_cast<CMetalBuffer*>(this)); }
    
    virtual void SetName(const String& InName) override final
    {
        @autoreleasepool
        {
            id<MTLBuffer> BufferHandle = GetMTLBuffer();
            if (BufferHandle)
            {
                BufferHandle.label = InName.GetNSString();
            }
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalConstantBuffer

class CMetalConstantBuffer : public CMetalBuffer, public CRHIConstantBuffer
{
public:
    
    CMetalConstantBuffer(CMetalDeviceContext* DeviceContext, const CRHIConstantBufferInitializer& Initializer)
        : CMetalBuffer(DeviceContext)
        , CRHIConstantBuffer(Initializer)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIConstantBuffer Interface
    
    virtual CRHIDescriptorHandle GetBindlessHandle() const override final { return CRHIDescriptorHandle(); }

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLBuffer()); }

    virtual void* GetRHIBaseBuffer() override final { return reinterpret_cast<void*>(static_cast<CMetalBuffer*>(this)); }
    
    virtual void SetName(const String& InName) override final
    {
        @autoreleasepool
        {
            id<MTLBuffer> BufferHandle = GetMTLBuffer();
            if (BufferHandle)
            {
                BufferHandle.label = InName.GetNSString();
            }
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalGenericBuffer

class CMetalGenericBuffer : public CMetalBuffer, public CRHIGenericBuffer
{
public:
    
    CMetalGenericBuffer(CMetalDeviceContext* DeviceContext, const CRHIGenericBufferInitializer& Initializer)
        : CMetalBuffer(DeviceContext)
        , CRHIGenericBuffer(Initializer)
    { }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLBuffer()); }

    virtual void* GetRHIBaseBuffer() override final { return reinterpret_cast<void*>(static_cast<CMetalBuffer*>(this)); }
    
    virtual void SetName(const String& InName) override final
    {
        @autoreleasepool
        {
            id<MTLBuffer> BufferHandle = GetMTLBuffer();
            if (BufferHandle)
            {
                BufferHandle.label = InName.GetNSString();
            }
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// GetMetalBuffer

inline CMetalBuffer* GetMetalBuffer(CRHIBuffer* Buffer)
{
    return Buffer ? reinterpret_cast<CMetalBuffer*>(Buffer->GetRHIBaseBuffer()) : nullptr;
}

#pragma clang diagnostic pop

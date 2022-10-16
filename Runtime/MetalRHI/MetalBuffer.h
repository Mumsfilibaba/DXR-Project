#pragma once
#include "MetalObject.h"

#include "RHI/RHIResources.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

class FMetalBuffer 
    : public FMetalObject
{
public:
    
    FMetalBuffer(FMetalDeviceContext* DeviceContext)
        : FMetalObject(DeviceContext)
        , Buffer(nil)
    { }
    
    ~FMetalBuffer()
    {
        NSSafeRelease(Buffer);
    }
    
    FORCEINLINE id<MTLBuffer> GetMTLBuffer() const 
    { 
        return Buffer; 
    }

    FORCEINLINE void SetMTLBuffer(id<MTLBuffer> InBuffer) 
    { 
        Buffer = [InBuffer retain]; 
    } 
    
private:
    id<MTLBuffer> Buffer;
};


class FMetalVertexBuffer 
    : public FMetalBuffer
    , public FRHIVertexBuffer
{
public:

    FMetalVertexBuffer(FMetalDeviceContext* DeviceContext, const FRHIVertexBufferInitializer& Initializer)
        : FMetalBuffer(DeviceContext)
        , FRHIVertexBuffer(Initializer)
    { }

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLBuffer()); }
    virtual void* GetRHIBaseBuffer()   override final       { return reinterpret_cast<void*>(static_cast<FMetalBuffer*>(this)); }
    
    virtual void SetName(const FString& InName) override final
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


class FMetalIndexBuffer 
    : public FMetalBuffer
    , public FRHIIndexBuffer
{
public:
    
    FMetalIndexBuffer(FMetalDeviceContext* DeviceContext, const FRHIIndexBufferInitializer& Initializer)
        : FMetalBuffer(DeviceContext)
        , FRHIIndexBuffer(Initializer)
    { }

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLBuffer()); }
    virtual void* GetRHIBaseBuffer()   override final       { return reinterpret_cast<void*>(static_cast<FMetalBuffer*>(this)); }
    
    virtual void SetName(const FString& InName) override final
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


class FMetalConstantBuffer 
    : public FMetalBuffer
    , public FRHIConstantBuffer
{
public:
    
    FMetalConstantBuffer(FMetalDeviceContext* DeviceContext, const FRHIConstantBufferInitializer& Initializer)
        : FMetalBuffer(DeviceContext)
        , FRHIConstantBuffer(Initializer)
    { }

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLBuffer()); }
    virtual void* GetRHIBaseBuffer()   override final       { return reinterpret_cast<void*>(static_cast<FMetalBuffer*>(this)); }
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) override final
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


class FMetalGenericBuffer 
    : public FMetalBuffer
    , public FRHIGenericBuffer
{
public:
    
    FMetalGenericBuffer(FMetalDeviceContext* DeviceContext, const FRHIGenericBufferInitializer& Initializer)
        : FMetalBuffer(DeviceContext)
        , FRHIGenericBuffer(Initializer)
    { }

    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLBuffer()); }
    virtual void* GetRHIBaseBuffer()   override final       { return reinterpret_cast<void*>(static_cast<FMetalBuffer*>(this)); }
    
    virtual void SetName(const FString& InName) override final
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


inline FMetalBuffer* GetMetalBuffer(FRHIBuffer* Buffer)
{
    return Buffer ? reinterpret_cast<FMetalBuffer*>(Buffer->GetRHIBaseBuffer()) : nullptr;
}

#pragma clang diagnostic pop

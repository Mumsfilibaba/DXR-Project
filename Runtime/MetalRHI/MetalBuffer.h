#pragma once
#include "MetalDeviceChild.h"
#include "MetalRefCounted.h"
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalBuffer> FMetalBufferRef;

class FMetalBuffer : public FRHIBuffer, public FMetalDeviceChild
{
public:
    FMetalBuffer(FMetalDeviceContext* DeviceContext, const FRHIBufferInfo& InBufferInfo);
    ~FMetalBuffer();

    bool Initialize(EResourceAccess InInitialAccess, const void* InInitialData);

public:

    // FRHIBuffer Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetMTLBuffer()); }
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;
    
public:
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

inline FMetalBuffer* GetMetalBuffer(FRHIBuffer* Buffer)
{
    return Buffer ? reinterpret_cast<FMetalBuffer*>(Buffer->GetRHINativeHandle()) : nullptr;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

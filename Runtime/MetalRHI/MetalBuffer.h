#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalDeviceChild.h"
DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalBufferRHI> FMetalBufferRef;

class FMetalBufferRHI : public FRHIBuffer, public FMetalDeviceChild
{
public:
    FMetalBufferRHI(FMetalDevice* InDevice, const FRHIBufferDesc& InBufferDesc);
    ~FMetalBufferRHI();

    // FRHIBuffer Interface
    virtual void* GetRHINativeResource() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX)   override final;
    virtual void  Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;
    
    bool Initialize(EResourceAccess InInitialAccess, const void* InInitialData);
    
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

inline FMetalBufferRHI* GetMetalBuffer(FRHIBuffer* Buffer)
{
    return static_cast<FMetalBufferRHI*>(Buffer);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

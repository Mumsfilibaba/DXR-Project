#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalResource.h"
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
    
    bool Initialize(ERHIResourceState InInitialAccess, const void* InInitialData);
    
    FORCEINLINE id<MTLBuffer> GetMTLBuffer() const 
    { 
        return Buffer; 
    }

    FORCEINLINE NSUInteger GetMetalBindOffset() const
    {
        return IsHeapPlaced() ? 0 : static_cast<NSUInteger>(ResourceStorage.GetResourceOffset());
    }

    FORCEINLINE void SetMTLBuffer(id<MTLBuffer> InBuffer) 
    { 
        [InBuffer retain];
        [Buffer release];
        Buffer = InBuffer;
    } 

    FORCEINLINE uint64 GetLastWriteValue() const
    {
        return LastUsedValue;
    }

    FORCEINLINE FMetalQueue* GetLastUsedQueue() const
    {
        return LastUsedQueue;
    }

    FORCEINLINE uint64 GetLastUsedValue() const
    {
        return LastUsedValue;
    }

    void StampLastUse(FMetalQueue* InQueue, uint64 InValue);

    FORCEINLINE bool IsHeapPlaced() const
    {
        return ResourceStorage.IsPlacedResource();
    }

    FORCEINLINE const FMetalResourceStorage& GetResourceStorage() const
    {
        return ResourceStorage;
    }

private:
    id<MTLBuffer>        Buffer;
    FMetalResourceStorage ResourceStorage;
    mutable FRHIDescriptorHandle BindlessHandle;
    FMetalQueue*         LastUsedQueue;
    uint64               LastUsedValue;
};

inline FMetalBufferRHI* GetMetalBuffer(FRHIBuffer* Buffer)
{
    return static_cast<FMetalBufferRHI*>(Buffer);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

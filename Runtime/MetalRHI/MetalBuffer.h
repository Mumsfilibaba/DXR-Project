#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalRelocatable.h"
#include "MetalRHI/MetalResource.h"
DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalQueue;

typedef TSharedRef<class FMetalBufferRHI> FMetalBufferRef;

class FMetalBufferRHI : public FRHIBuffer, public FMetalDeviceChild, public FMetalRelocatable
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
    bool RelocateTransientStorage(uint64 SizeInBytes, const void* SourceData, FMetalQueue* Queue);

    FORCEINLINE id<MTLBuffer> GetMTLBuffer() const
    {
        return ResourceStorage.GetBuffer();
    }

    FORCEINLINE NSUInteger GetMetalBindOffset() const
    {
        return IsHeapPlaced() ? 0 : static_cast<NSUInteger>(ResourceStorage.GetResourceOffset());
    }

    FORCEINLINE bool IsHeapPlaced() const
    {
        return ResourceStorage.IsPlacedResource();
    }

    FORCEINLINE const FMetalResourceStorage& GetResourceStorage() const
    {
        return ResourceStorage;
    }

    FORCEINLINE FMetalResidencyEntry* GetResidencyEntry() const
    {
        return ResourceStorage.GetResidencyEntry();
    }

protected:

    // FMetalRelocatable Interface
    virtual FMetalResourceStorage& GetRelocatableStorage() override final;
    virtual void OnStorageSwapped() override final;

private:
    void FreeBindlessHandle();
    void UpdateMemoryStats();

    FMetalResourceStorage         ResourceStorage;
    mutable FRHIDescriptorHandle  BindlessHandle;
    mutable FMetalResidencyEntry* PinnedEntry;
    int64                         TrackedMemory;
};

inline FMetalBufferRHI* GetMetalBuffer(FRHIBuffer* Buffer)
{
    return static_cast<FMetalBufferRHI*>(Buffer);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

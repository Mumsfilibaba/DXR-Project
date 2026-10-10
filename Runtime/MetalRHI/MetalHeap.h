#pragma once
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalResidencyManager.h"

class FMetalHeap : public FMetalDeviceChild
{
public:
    FMetalHeap(FMetalDevice* InDevice, id<MTLHeap> InHeap, uint64 InSize);
    ~FMetalHeap();

    void DeferredRelease();
    void SetDebugName(const String& Name);

    FORCEINLINE id<MTLHeap> GetMTLHeap() const
    {
        return Heap;
    }

    FORCEINLINE uint64 GetSize() const
    {
        return Size;
    }

    FORCEINLINE FMetalResidencyEntry& GetResidencyEntry()
    {
        return ResidencyEntry;
    }

private:
    id<MTLHeap>          Heap;
    uint64               Size;
    FMetalResidencyEntry ResidencyEntry;
};

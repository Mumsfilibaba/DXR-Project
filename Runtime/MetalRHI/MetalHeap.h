#pragma once
#include "MetalRHI/MetalDeviceChild.h"

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

    FORCEINLINE MTLStorageMode GetStorageMode() const
    {
        return StorageMode;
    }

private:
    id<MTLHeap>    Heap;
    uint64         Size;
    MTLStorageMode StorageMode;
};

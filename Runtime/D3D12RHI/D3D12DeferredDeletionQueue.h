#pragma once
#include "D3D12DeviceChild.h"
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"

class FD3D12DeferredDeletionQueue : public FD3D12DeviceChild
{
    enum class EDeferredObjectType
    {
        D3DResource = 1,
        Resource    = 2,
        RHIResource = 3,
    };

    struct FDeferredObject
    {
        EDeferredObjectType Type;
        uint64              FenceValue;

        union
        {
            IRefCounted*    RHIResource;
            FD3D12Resource* Resource;
            ID3D12Resource* D3DResource;
        };
    };

public:
    FD3D12DeferredDeletionQueue(FD3D12Device* InDevice);
    ~FD3D12DeferredDeletionQueue();

    void Tick();

    void DeferDeletion(IRefCounted* InResource, uint64 FenceValue);

    void DeferDeletion(FD3D12Resource* InResource, uint64 FenceValue);

    void DeferDeletion(ID3D12Resource* InResource, uint64 FenceValue);

private:
    TQueue<FDeferredObject> Queue;
};

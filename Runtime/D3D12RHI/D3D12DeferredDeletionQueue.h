#pragma once
#include "D3D12DeviceChild.h"

#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"

class FD3D12DeferredDeletionQueue
    : public FD3D12DeviceChild
{
    enum class EElementType
    {
        D3D12Resource = 1
    };

    struct FQueueElement
    {
        EElementType Type;

        TComPtr<ID3D12Resource> D3D12Resource;
    };

public:
    FD3D12DeferredDeletionQueue(FD3D12Device* InDevice);
    ~FD3D12DeferredDeletionQueue() = default;


private:
    //TQueue<> DeletionQueue;
    FCriticalSection DeletionQueueCS;
};

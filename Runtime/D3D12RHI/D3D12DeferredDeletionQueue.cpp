#include "D3D12DeferredDeletionQueue.h"

FD3D12DeferredDeletionQueue::FD3D12DeferredDeletionQueue(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , Queue()
{
}

FD3D12DeferredDeletionQueue::~FD3D12DeferredDeletionQueue()
{
    while (!Queue.IsEmpty())
    {
        Tick();
    }
}

void FD3D12DeferredDeletionQueue::Tick()
{
    while (true)
    {
        FDeferredObject Object;
        if (Queue.Peek(Object))
        {
            FD3D12CommandListManager* CommandListManager = GetDevice()->GetCommandListManager(ED3D12CommandQueueType::Direct);
            CHECK(CommandListManager != nullptr);

            const uint64 CompletedValue = CommandListManager->GetFenceManager().GetCompletedValue();
            if (CompletedValue >= Object.FenceValue)
            {
                if (Object.Type == EDeferredObjectType::RHIResource)
                {
                    Object.RHIResource->Release();
                }
                else if (Object.Type == EDeferredObjectType::Resource)
                {
                    Object.Resource->Release();
                }
                else if (Object.Type == EDeferredObjectType::D3DResource)
                {
                    Object.D3DResource->Release();
                }

                Queue.Dequeue();
            }
        }
        else
        {
            break;
        }
    }
}

void FD3D12DeferredDeletionQueue::DeferDeletion(IRefCounted* InResource, uint64 FenceValue)
{
    CHECK(InResource != nullptr);
    InResource->AddRef();

    FDeferredObject Object;
    Object.Type        = EDeferredObjectType::RHIResource;
    Object.FenceValue  = FenceValue;
    Object.RHIResource = InResource;

    Queue.Enqueue(Object);
}

void FD3D12DeferredDeletionQueue::DeferDeletion(FD3D12Resource* InResource, uint64 FenceValue)
{
    CHECK(InResource != nullptr);
    InResource->AddRef();

    FDeferredObject Object;
    Object.Type       = EDeferredObjectType::Resource;
    Object.FenceValue = FenceValue;
    Object.Resource   = InResource;

    Queue.Enqueue(Object);
}


void FD3D12DeferredDeletionQueue::DeferDeletion(ID3D12Resource* InResource, uint64 FenceValue)
{
    CHECK(InResource != nullptr);
    InResource->AddRef();

    FDeferredObject Object;
    Object.Type        = EDeferredObjectType::D3DResource;
    Object.FenceValue  = FenceValue;
    Object.D3DResource = InResource;

    Queue.Enqueue(Object);
}
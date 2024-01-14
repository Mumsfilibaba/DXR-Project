#include "D3D12DeferredDeletionQueue.h"
#include "D3D12RHI.h"
#include "Core/Threading/TaskManager.h"

FD3D12DeferredDeletionQueue::FD3D12DeferredDeletionQueue(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
{
}

bool FD3D12DeferredDeletionQueue::Initialize()
{
    Thread = FPlatformThreadMisc::CreateThread(this);
    if (!Thread)
    {
        LOG_ERROR("[FD3D12DeferredDeletionQueue] Failed to Create Thread");
        return false;
    }

    Thread->SetName("D3D12 Worker");

    if (!Thread->Start())
    {
        LOG_ERROR("[FD3D12DeferredDeletionQueue] Failed to Start Thread");
        return false;
    }

    return true;
}

bool FD3D12DeferredDeletionQueue::Start()
{
    bIsRunning = true;
    return true;
}

int32 FD3D12DeferredDeletionQueue::Run()
{
    FD3D12CommandListManager* CommandListManager = FD3D12RHI::GetRHI()->GetDevice()->GetCommandListManager(ED3D12CommandQueueType::Direct);
    CHECK(CommandListManager != nullptr);

    while (bIsRunning)
    {
        if (Tasks.IsEmpty())
        {
            FPlatformThreadMisc::Pause();
            continue;
        }

        FD3D12DeferredObject Object;
        if (Tasks.Dequeue(Object))
        {
            uint64 CompletedValue = CommandListManager->GetFenceManager().GetCompletedValue();
            while (CompletedValue < Object.FenceValue)
            {
                FPlatformThreadMisc::Pause();
                CompletedValue = CommandListManager->GetFenceManager().GetCompletedValue();
            }

            if (Object.Type == FD3D12DeferredObject::EDeferredObjectType::RHIResource)
            {
                Object.RHIResource->Release();
            }
            else if (Object.Type == FD3D12DeferredObject::EDeferredObjectType::Resource)
            {
                Object.Resource->Release();
            }
            else if (Object.Type == FD3D12DeferredObject::EDeferredObjectType::D3DResource)
            {
                Object.D3DResource->Release();
            }
            else if (Object.Type == FD3D12DeferredObject::EDeferredObjectType::OnlineDescriptorBlock)
            {
                FD3D12OnlineDescriptorHeap* Heap = Object.OnlineDescriptorBlock.Heap;
                Heap->RecycleBlock(Object.OnlineDescriptorBlock.Block);
            }
        }
    }

    return 0;
}

void FD3D12DeferredDeletionQueue::Stop()
{
    bIsRunning = false;
}

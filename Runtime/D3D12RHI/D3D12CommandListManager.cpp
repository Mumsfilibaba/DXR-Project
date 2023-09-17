#include "D3D12CommandListManager.h"
#include "D3D12CommandAllocator.h"
#include "D3D12Device.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Threading/ScopedLock.h"

TAutoConsoleVariable<bool> CVarEnableGPUTimeout(
    "D3D12RHI.EnableGPUTimeout",
    "Enables or disables the GPU timeout on all ID3D12CommandQueues",
    true);


FD3D12CommandListManager::FD3D12CommandListManager(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : FD3D12DeviceChild(InDevice)
    , QueueType(InQueueType)
    , CommandListType(ToCommandListType(InQueueType))
    , FenceManager(InDevice)
    , CommandQueue(nullptr)
    , CommandLists()
{
}

bool FD3D12CommandListManager::Initialize()
{
    // CommandQueue
    {
        TComPtr<ID3D12CommandQueue> NewCommandQueue;

        D3D12_COMMAND_QUEUE_DESC Desc;
        FMemory::Memzero(&Desc);

        Desc.Type     = CommandListType;
        Desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        Desc.NodeMask = GetDevice()->GetNodeMask();
        Desc.Flags    = CVarEnableGPUTimeout.GetValue() ? D3D12_COMMAND_QUEUE_FLAG_NONE : D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;

        HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandQueue(&Desc, IID_PPV_ARGS(&NewCommandQueue));
        if (FAILED(Result))
        {
            D3D12_ERROR("[FD3D12Device]: Failed to create CommandQueue '%s'", ToString(QueueType));
            return false;
        }
        else
        {
            D3D12_INFO("[FD3D12Device]: Created CommandQueue '%s'", ToString(QueueType));
            CommandQueue = NewCommandQueue;

            const FStringWide WideName = CharToWide(FString::CreateFormatted("CommandQueue %s", ToString(QueueType)));
            NewCommandQueue->SetName(WideName.GetCString());
        }
    }

    // Fences
    if (!FenceManager.Initialize())
    {
        return false;
    }

    return true;
}

FD3D12CommandListRef FD3D12CommandListManager::ObtainCommandList(FD3D12CommandAllocator& CommandAllocator, ID3D12PipelineState* InitialPipelineState)
{
    TScopedLock Lock(CommandListsCS);

    FD3D12CommandListRef CommandList;
    if (CommandLists.IsEmpty())
    {
        CommandList = new FD3D12CommandList(GetDevice());
        if (!CommandList->Initialize(CommandListType, CommandAllocator, InitialPipelineState))
        {
            return nullptr;
        }
    }
    else
    {
        CommandList = CommandLists.LastElement();
        CommandLists.Pop();
    }

    return CommandList;
}

void FD3D12CommandListManager::ReleaseCommandList(FD3D12CommandListRef InCommandList)
{
    CHECK(InCommandList != nullptr);
    
    TScopedLock Lock(CommandListsCS);
    CommandLists.Emplace(InCommandList);
}

FD3D12FenceSyncPoint FD3D12CommandListManager::ExecuteCommandList(FD3D12CommandListRef InCommandList, bool bWaitForCompletion)
{
    CHECK(InCommandList != nullptr);

    ID3D12CommandList* CommandList = InCommandList->GetCommandList();
    CommandQueue->ExecuteCommandLists(1, &CommandList);

    const uint64 FenceValue = FenceManager.SignalGPU(QueueType);
    if (bWaitForCompletion)
    {
        FenceManager.WaitForFence(FenceValue);
    }

    return FD3D12FenceSyncPoint(FenceManager.GetFence(), FenceValue);
}
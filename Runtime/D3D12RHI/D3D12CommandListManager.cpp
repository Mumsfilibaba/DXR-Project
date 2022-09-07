#include "D3D12CommandListManager.h"
#include "D3D12CommandAllocator.h"
#include "D3D12Device.h"

#include "Core/Debug/Console/ConsoleInterface.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console-Variables

TAutoConsoleVariable<bool> CVarEnableGPUTimeout("D3D12RHI.EnableGPUTimeout", true);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandListManager

FD3D12CommandListManager::FD3D12CommandListManager(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : FD3D12DeviceChild(InDevice)
    , QueueType(InQueueType)
    , CommandListType(ToCommandListType(InQueueType))
    , CommandQueue(nullptr)
    , CommandLists()
{ }

bool FD3D12CommandListManager::Initialize()
{
    TComPtr<ID3D12CommandQueue> NewCommandQueue;

    D3D12_COMMAND_QUEUE_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Type     = CommandListType;
    Desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    Desc.NodeMask = GetDevice()->GetNodeMask();
    Desc.Flags    = CVarEnableGPUTimeout.GetBool() ? D3D12_COMMAND_QUEUE_FLAG_NONE : D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;

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

    return true;
}

FD3D12CommandListRef FD3D12CommandListManager::ObtainCommandList(FD3D12CommandAllocator& CommandAllocator, ID3D12PipelineState* InitialPipelineState)
{
    FD3D12CommandListRef CommandList;
    if (CommandLists.IsEmpty())
    {
        CommandList = dbg_new FD3D12CommandList(GetDevice());
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
    Check(InCommandList != nullptr);
    CommandLists.Emplace(InCommandList);
}

void FD3D12CommandListManager::ExecuteCommandList(FD3D12CommandListRef InCommandList)
{
    Check(InCommandList != nullptr);

    ID3D12CommandList* CommandList = InCommandList->GetCommandList();
    CommandQueue->ExecuteCommandLists(1, &CommandList);
}
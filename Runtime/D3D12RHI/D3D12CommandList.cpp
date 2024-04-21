#include "D3D12Device.h"
#include "D3D12CommandList.h"

FD3D12CommandAllocator::FD3D12CommandAllocator(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : FD3D12DeviceChild(InDevice)
    , Allocator(nullptr)
    , QueueType(InQueueType)
{
}

bool FD3D12CommandAllocator::Initialize()
{
    const D3D12_COMMAND_LIST_TYPE Type = ToCommandListType(QueueType);

    HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandAllocator(Type, IID_PPV_ARGS(&Allocator));
    if (SUCCEEDED(Result))
    {
        D3D12_INFO("[FD3D12CommandAllocator]: Created CommandAllocator");
        return true;
    }
    else
    {
        D3D12_ERROR("[FD3D12CommandAllocator]: FAILED to create CommandAllocator");
        return false;
    }
}

bool FD3D12CommandAllocator::Reset()
{
    HRESULT Result = Allocator->Reset();
    if (Result == DXGI_ERROR_DEVICE_REMOVED)
    {
        D3D12DeviceRemovedHandlerRHI(GetDevice());
    }

    return SUCCEEDED(Result);
}

FD3D12CommandAllocatorManager::FD3D12CommandAllocatorManager(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType)
    : FD3D12DeviceChild(InDevice)
    , QueueType(InQueueType)
    , CommandListType(ToCommandListType(QueueType))
    , AvailableAllocators()
    , CommandAllocators()
{
}

FD3D12CommandAllocatorManager::~FD3D12CommandAllocatorManager()
{
    DestroyAllocators();
}

FD3D12CommandAllocator* FD3D12CommandAllocatorManager::ObtainAllocator()
{
    TScopedLock Lock(CommandAllocatorsCS);

    FD3D12CommandAllocator* CommandAllocator;
    if (!AvailableAllocators.IsEmpty())
    {
        AvailableAllocators.Dequeue(CommandAllocator);
        if (!CommandAllocator->Reset())
        {
            DEBUG_BREAK();
            return nullptr;
        }
    }
    else
    {
        CommandAllocator = new FD3D12CommandAllocator(GetDevice(), QueueType);
        if (!CommandAllocator->Initialize())
        {
            DEBUG_BREAK();
            return nullptr;
        }

        CommandAllocators.Add(CommandAllocator);
    }

    return CommandAllocator;
}

void FD3D12CommandAllocatorManager::RecycleAllocator(FD3D12CommandAllocator* InAllocator)
{
    CHECK(InAllocator != nullptr);

    TScopedLock Lock(CommandAllocatorsCS);
    AvailableAllocators.Enqueue(InAllocator);
}

void FD3D12CommandAllocatorManager::DestroyAllocators()
{
    for (FD3D12CommandAllocator* CommandAllocator : CommandAllocators)
    {
        delete CommandAllocator;
    }

    CommandAllocators.Clear();
    AvailableAllocators.Clear();
}

FD3D12CommandList::FD3D12CommandList(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , CmdList(nullptr)
    , CmdList1(nullptr)
    , CmdList2(nullptr)
    , CmdList3(nullptr)
    , CmdList4(nullptr)
    , CmdList5(nullptr)
    , CmdList6(nullptr)
    , NumCommands(0)
    , bIsReady(false)
{
}

bool FD3D12CommandList::Initialize(D3D12_COMMAND_LIST_TYPE Type, FD3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline)
{
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandList(1, Type, Allocator->GetD3D12Allocator(), InitalPipeline, IID_PPV_ARGS(&CmdList));
    if (SUCCEEDED(Result))
    {
        NumCommands = 0;
        CmdList->Close();

        LOG_INFO("[FD3D12CommandList]: Created CommandList");

        // TODO: Ensure that this compiles on different WinSDK versions
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList1>(&CmdList1)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList1");
        }

        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList2>(&CmdList2)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList2");
        }

        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList3>(&CmdList3)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList3");
        }

        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList4>(&CmdList4)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList4");
        }

        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList5>(&CmdList5)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList5");
        }

        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList6>(&CmdList6)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList6");
        }

        return true;
    }
    else
    {
        D3D12_ERROR("[FD3D12CommandList]: FAILED to create CommandList");
        return false;
    }
}

bool FD3D12CommandList::Reset(FD3D12CommandAllocator* Allocator)
{
    bIsReady = true;

    HRESULT Result = CmdList->Reset(Allocator->GetD3D12Allocator(), nullptr);
    if (Result == DXGI_ERROR_DEVICE_REMOVED)
    {
        D3D12DeviceRemovedHandlerRHI(GetDevice());
    }

    return SUCCEEDED(Result);
}

bool FD3D12CommandList::Close()
{
    bIsReady = false;

    HRESULT Result = CmdList->Close();
    if (Result == DXGI_ERROR_DEVICE_REMOVED)
    {
        D3D12DeviceRemovedHandlerRHI(GetDevice());
    }

    NumCommands = 0;
    return SUCCEEDED(Result);
}
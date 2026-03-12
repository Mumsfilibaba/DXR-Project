#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12CommandList.h"

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
        D3D12_ERROR_CRITICAL("[FD3D12CommandAllocator]: FAILED to create CommandAllocator");
        return false;
    }
}

bool FD3D12CommandAllocator::Reset()
{
    HRESULT Result = Allocator->Reset();
#if D3D12_ENABLE_DEVICE_LOST_CHECK
    if (Result == DXGI_ERROR_DEVICE_REMOVED)
    {
        D3D12DeviceRemovedHandlerRHI(GetDevice());
    }
#endif

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
    for (FD3D12CommandAllocator* CommandAllocator : CommandAllocators)
    {
        delete CommandAllocator;
    }
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

    #ifdef __ID3D12GraphicsCommandList1_INTERFACE_DEFINED__
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList1>(&CmdList1)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList1");
        }
    #endif

    #ifdef __ID3D12GraphicsCommandList2_INTERFACE_DEFINED__
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList2>(&CmdList2)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList2");
        }
    #endif

    #ifdef __ID3D12GraphicsCommandList3_INTERFACE_DEFINED__
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList3>(&CmdList3)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList3");
        }
    #endif

    #ifdef __ID3D12GraphicsCommandList4_INTERFACE_DEFINED__
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList4>(&CmdList4)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList4");
        }
    #endif

    #ifdef __ID3D12GraphicsCommandList5_INTERFACE_DEFINED__
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList5>(&CmdList5)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList5");
        }
    #endif

    #ifdef __ID3D12GraphicsCommandList6_INTERFACE_DEFINED__
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList6>(&CmdList6)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList6");
        }
    #endif

    #ifdef __ID3D12GraphicsCommandList7_INTERFACE_DEFINED__
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList7>(&CmdList7)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList7");
        }
    #endif

    #ifdef __ID3D12GraphicsCommandList8_INTERFACE_DEFINED__
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList8>(&CmdList8)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList8");
        }
    #endif

    #ifdef __ID3D12GraphicsCommandList9_INTERFACE_DEFINED__
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList9>(&CmdList9)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList9");
        }
    #endif

    #ifdef __ID3D12GraphicsCommandList10_INTERFACE_DEFINED__
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList10>(&CmdList10)))
        {
            D3D12_WARNING("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList10");
        }
    #endif

        return true;
    }
    else
    {
        D3D12_ERROR_CRITICAL("[FD3D12CommandList]: FAILED to create CommandList");
        return false;
    }
}

bool FD3D12CommandList::Reset(FD3D12CommandAllocator* Allocator)
{
    bIsReady = true;
    ResidencySet.Reset();

    HRESULT Result = CmdList->Reset(Allocator->GetD3D12Allocator(), nullptr);
#if D3D12_ENABLE_DEVICE_LOST_CHECK
    if (Result == DXGI_ERROR_DEVICE_REMOVED)
    {
        D3D12DeviceRemovedHandlerRHI(GetDevice());
    }
#endif

    return SUCCEEDED(Result);
}

bool FD3D12CommandList::Close()
{
    bIsReady = false;

    HRESULT Result = CmdList->Close();
#if D3D12_ENABLE_DEVICE_LOST_CHECK
    if (Result == DXGI_ERROR_DEVICE_REMOVED)
    {
        D3D12DeviceRemovedHandlerRHI(GetDevice());
    }
#endif

    NumCommands = 0;
    return SUCCEEDED(Result);
}

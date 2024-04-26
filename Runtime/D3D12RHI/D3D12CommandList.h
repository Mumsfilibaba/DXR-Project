#pragma once
#include "D3D12Resource.h"
#include "D3D12RootSignature.h"
#include "D3D12Descriptors.h"
#include "D3D12ResourceViews.h"
#include "D3D12Fence.h"
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"

class FD3D12ComputePipelineState;

class FD3D12CommandAllocator : public FD3D12DeviceChild
{
public:
    FD3D12CommandAllocator(const FD3D12CommandAllocator&) = delete;
    FD3D12CommandAllocator& operator=(const FD3D12CommandAllocator&) = delete;

    FD3D12CommandAllocator(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType);
    ~FD3D12CommandAllocator() = default;

    bool Initialize();
    bool Reset();

    ID3D12CommandAllocator* GetD3D12Allocator() const
    {
        return Allocator.Get();
    }

    void SetDebugName(const FString& Name)
    {
        FStringWide WideName = CharToWide(Name);
        Allocator->SetName(WideName.GetCString());
    }

    ED3D12CommandQueueType GetQueueType() const
    {
        return QueueType;
    }

private:
    ED3D12CommandQueueType          QueueType;
    TComPtr<ID3D12CommandAllocator> Allocator;
};

class FD3D12CommandAllocatorManager : public FD3D12DeviceChild
{
public:
    FD3D12CommandAllocatorManager(FD3D12Device* InDevice, ED3D12CommandQueueType InQueueType);
    ~FD3D12CommandAllocatorManager();

    FD3D12CommandAllocator* ObtainAllocator();
    void RecycleAllocator(FD3D12CommandAllocator* InAllocator);
    void DestroyAllocators();

    ED3D12CommandQueueType GetQueueType() const
    {
        return QueueType;
    }

private:
    ED3D12CommandQueueType          QueueType;
    D3D12_COMMAND_LIST_TYPE         CommandListType;
    TQueue<FD3D12CommandAllocator*> AvailableAllocators;
    TArray<FD3D12CommandAllocator*> CommandAllocators;
    FCriticalSection                CommandAllocatorsCS;
};

class FD3D12CommandList : public FD3D12DeviceChild
{
    template<typename CommandListInterfaceType>
    struct FCommandList
    {
        FCommandList(FD3D12CommandList* InCommandListParent, CommandListInterfaceType* InCommandListInterface)
            : CommandListParent(InCommandListParent)
            , CommandListInterface(InCommandListInterface)
        {    
        }

        CommandListInterfaceType* operator->()
        {
            CommandListParent->NumCommands++;
            return CommandListInterface;
        }

        FD3D12CommandList*        CommandListParent;
        CommandListInterfaceType* CommandListInterface;
    };

public:
    FD3D12CommandList(const FD3D12CommandList&) = delete;
    FD3D12CommandList& operator=(const FD3D12CommandList&) = delete;

    FD3D12CommandList(FD3D12Device* InDevice);
    ~FD3D12CommandList() = default;
    
    bool Initialize(D3D12_COMMAND_LIST_TYPE Type, FD3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline);
    bool Reset(FD3D12CommandAllocator* Allocator);
    bool Close();

    FORCEINLINE bool IsReady() const
    {
        return bIsReady;
    }

    FORCEINLINE uint32 GetNumCommands() const
    {
        return NumCommands;
    }

    FORCEINLINE void SetDebugName(const FString& Name)
    {
        FStringWide WideName = CharToWide(Name);
        CmdList->SetName(WideName.GetCString());
    }

    FORCEINLINE FCommandList<ID3D12GraphicsCommandList> operator->()
    {
        return FCommandList(this, CmdList.Get());
    }

    FORCEINLINE FCommandList<ID3D12GraphicsCommandList> GetGraphicsCommandList()
    {
        return FCommandList(this, CmdList.Get());
    }

    FORCEINLINE FCommandList<ID3D12GraphicsCommandList1> GetGraphicsCommandList1()
    {
        return FCommandList(this, CmdList1.Get());
    }

    FORCEINLINE FCommandList<ID3D12GraphicsCommandList2> GetGraphicsCommandList2()
    {
        return FCommandList(this, CmdList2.Get());
    }

    FORCEINLINE FCommandList<ID3D12GraphicsCommandList3> GetGraphicsCommandList3()
    {
        return FCommandList(this, CmdList3.Get());
    }

    FORCEINLINE FCommandList<ID3D12GraphicsCommandList4> GetGraphicsCommandList4()
    {
        return FCommandList(this, CmdList4.Get());
    }

    FORCEINLINE FCommandList<ID3D12GraphicsCommandList5> GetGraphicsCommandList5()
    {
        return FCommandList(this, CmdList5.Get());
    }

    FORCEINLINE FCommandList<ID3D12GraphicsCommandList6> GetGraphicsCommandList6()
    {
        return FCommandList(this, CmdList6.Get());
    }

    FORCEINLINE ID3D12CommandList* GetCommandList() const
    {
        return CmdList.Get();
    }

private:
    TComPtr<ID3D12GraphicsCommandList>  CmdList;
    TComPtr<ID3D12GraphicsCommandList1> CmdList1;
    TComPtr<ID3D12GraphicsCommandList2> CmdList2;
    TComPtr<ID3D12GraphicsCommandList3> CmdList3;
    TComPtr<ID3D12GraphicsCommandList4> CmdList4;
    TComPtr<ID3D12GraphicsCommandList5> CmdList5;
    TComPtr<ID3D12GraphicsCommandList6> CmdList6;

    uint32 NumCommands;
    bool   bIsReady;
};
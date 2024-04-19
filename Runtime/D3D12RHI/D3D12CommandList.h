#pragma once
#include "D3D12Resource.h"
#include "D3D12RootSignature.h"
#include "D3D12Descriptors.h"
#include "D3D12CommandAllocator.h"
#include "D3D12ResourceViews.h"
#include "D3D12RefCounted.h"

class FD3D12ComputePipelineState;

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
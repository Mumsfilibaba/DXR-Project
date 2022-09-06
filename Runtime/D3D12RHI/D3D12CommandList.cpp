#include "D3D12Device.h"
#include "D3D12CommandList.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandList

bool FD3D12CommandList::Initialize(D3D12_COMMAND_LIST_TYPE Type, FD3D12CommandAllocator& Allocator, ID3D12PipelineState* InitalPipeline)
{
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandList(1, Type, Allocator.GetAllocator(), InitalPipeline, IID_PPV_ARGS(&CmdList));
    if (SUCCEEDED(Result))
    {
        CmdList->Close();

        LOG_INFO("[FD3D12CommandList]: Created CommandList");

        // TODO: Ensure that this compiles on different WinSDK versions
        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList1>(&CmdList1)))
        {
            D3D12_ERROR("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList1");
        }

        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList2>(&CmdList2)))
        {
            D3D12_ERROR("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList2");
        }

        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList3>(&CmdList3)))
        {
            D3D12_ERROR("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList3");
        }

        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList4>(&CmdList4)))
        {
            D3D12_ERROR("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList4");
        }

        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList5>(&CmdList5)))
        {
            D3D12_ERROR("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList5");
        }

        if (FAILED(CmdList.GetAs<ID3D12GraphicsCommandList6>(&CmdList6)))
        {
            D3D12_ERROR("[FD3D12CommandList]: FAILED to retrieve ID3D12GraphicsCommandList6");
        }

        return true;
    }
    else
    {
        D3D12_ERROR("[FD3D12CommandList]: FAILED to create CommandList");
        return false;
    }
}
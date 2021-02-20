#include "D3D12DescriptorCache.h"

D3D12DescriptorCache::D3D12DescriptorCache(D3D12Device* InDevice)
    : D3D12DeviceChild(InDevice)
{
}

void D3D12DescriptorCache::SetUnorderedAccessViews(D3D12UnorderedAccessViewCache& UAVCache, D3D12CommandListHandle& CmdList, D3D12RootSignature& RootSignature)
{
    for (UInt32 i = 0; i < ShaderVisibility_Count; i++)
    {
        Int32 RootParameterIndex = RootSignature.GetRootParameterIndex(i, ShaderResource_UAV);
    }
}

void D3D12DescriptorCache::CommitGraphicsDescriptorTables(D3D12CommandListHandle& CmdList, D3D12RootSignature& RootSignature)
{
    ID3D12CommandList* DxCmdList = CmdList.GetCommandList();
}

void D3D12DescriptorCache::CommitComputeDescriptorTables(D3D12CommandListHandle& CmdList, D3D12RootSignature& RootSignature)
{
    ID3D12CommandList* DxCmdList = CmdList.GetCommandList();

    // For each space

    // For each shader resource
}
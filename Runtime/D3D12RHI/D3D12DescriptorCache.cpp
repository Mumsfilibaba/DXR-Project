#include "D3D12DescriptorCache.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12RHIInstance.h"
#include "D3D12RHICommandContext.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12DescriptorCache

CD3D12DescriptorCache::CD3D12DescriptorCache(CD3D12Device* InDevice)
    : CD3D12DeviceObject(InDevice)
    , NullCBV(nullptr)
    , NullSRV(nullptr)
    , NullUAV(nullptr)
    , NullSampler(nullptr)
    , ShaderResourceViewCache()
    , UnorderedAccessViewCache()
    , ConstantBufferViewCache()
    , SamplerStateCache()
    , RangeSizes()
{
}

CD3D12DescriptorCache::~CD3D12DescriptorCache()
{
    SafeDelete(NullCBV);
    SafeRelease(NullSRV);
    SafeRelease(NullUAV);
    SafeRelease(NullSampler);
}

bool CD3D12DescriptorCache::Init()
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
    CMemory::Memzero(&CBVDesc);

    CBVDesc.BufferLocation = 0;
    CBVDesc.SizeInBytes = 0;

    NullCBV = dbg_new CD3D12ConstantBufferView(GetDevice(), GD3D12RHIInstance->GetResourceOfflineDescriptorHeap());
    if (!NullCBV->AllocateHandle())
    {
        return false;
    }

    if (!NullCBV->CreateView(nullptr, CBVDesc))
    {
        return false;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
    CMemory::Memzero(&UAVDesc);

    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    UAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    UAVDesc.Texture2D.MipSlice = 0;
    UAVDesc.Texture2D.PlaneSlice = 0;

    NullUAV = dbg_new CD3D12UnorderedAccessView(GetDevice(), GD3D12RHIInstance->GetResourceOfflineDescriptorHeap());
    if (!NullUAV->AllocateHandle())
    {
        return false;
    }

    if (!NullUAV->CreateView(nullptr, nullptr, UAVDesc))
    {
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    CMemory::Memzero(&SRVDesc);

    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Texture2D.MipLevels = 1;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    SRVDesc.Texture2D.PlaneSlice = 0;

    NullSRV = dbg_new CD3D12ShaderResourceView(GetDevice(), GD3D12RHIInstance->GetResourceOfflineDescriptorHeap());
    if (!NullSRV->AllocateHandle())
    {
        return false;
    }

    if (!NullSRV->CreateView(nullptr, SRVDesc))
    {
        return false;
    }

    D3D12_SAMPLER_DESC SamplerDesc;
    CMemory::Memzero(&SamplerDesc);

    SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.BorderColor[0] = 1.0f;
    SamplerDesc.BorderColor[1] = 1.0f;
    SamplerDesc.BorderColor[2] = 1.0f;
    SamplerDesc.BorderColor[3] = 1.0f;
    SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerDesc.MaxAnisotropy = 1;
    SamplerDesc.MaxLOD = FLT_MAX;
    SamplerDesc.MinLOD = -FLT_MAX;
    SamplerDesc.MipLODBias = 0.0f;

    NullSampler = dbg_new CD3D12RHISamplerState(GetDevice(), GD3D12RHIInstance->GetSamplerOfflineDescriptorHeap());
    if (!NullSampler->Init(SamplerDesc))
    {
        return false;
    }

    for (uint32 i = 0; i < D3D12_CACHED_DESCRIPTORS_COUNT; i++)
    {
        RangeSizes[i] = 1;
    }

    // Start by resetting the descriptor cache
    Reset();
    return true;
}

void CD3D12DescriptorCache::CommitGraphicsDescriptors(CD3D12CommandList& CmdList, CD3D12CommandBatch* CmdBatch, CD3D12RootSignature* RootSignature)
{
    TRACE_FUNCTION_SCOPE();

    Assert(CmdBatch != nullptr);
    Assert(RootSignature != nullptr);

    // Vertex and render-targets
    VertexBufferCache.CommitState(CmdList);
    RenderTargetCache.CommitState(CmdList);

    // Allocate descriptors for resources and samplers 
    ID3D12Device* DxDevice = GetDevice()->GetDevice();
    ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

    CD3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    CD3D12OnlineDescriptorHeap* SamplerHeap = CmdBatch->GetOnlineSamplerDescriptorHeap();

    AllocateDescriptorsAndSetHeaps(DxCmdList, ResourceHeap, SamplerHeap);

    // Copy and bind resources and samplers
    for (uint32 i = 0; i < ShaderVisibility_Count; i++)
    {
        const EShaderVisibility ShaderVisibility = static_cast<EShaderVisibility>(i);

        int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderVisibility, EResourceType::ResourceType_CBV);
        CopyAndBindGraphicsDescriptors(DxDevice, DxCmdList, ConstantBufferViewCache, ParameterIndex, ShaderVisibility);

        ParameterIndex = RootSignature->GetRootParameterIndex(ShaderVisibility, EResourceType::ResourceType_SRV);
        CopyAndBindGraphicsDescriptors(DxDevice, DxCmdList, ShaderResourceViewCache, ParameterIndex, ShaderVisibility);

        ParameterIndex = RootSignature->GetRootParameterIndex(ShaderVisibility, EResourceType::ResourceType_UAV);
        CopyAndBindGraphicsDescriptors(DxDevice, DxCmdList, UnorderedAccessViewCache, ParameterIndex, ShaderVisibility);

        ParameterIndex = RootSignature->GetRootParameterIndex(ShaderVisibility, EResourceType::ResourceType_Sampler);
        CopyAndBindGraphicsDescriptors(DxDevice, DxCmdList, SamplerStateCache, ParameterIndex, ShaderVisibility);
    }
}

void CD3D12DescriptorCache::CommitComputeDescriptors(CD3D12CommandList& CmdList, CD3D12CommandBatch* CmdBatch, CD3D12RootSignature* RootSignature)
{
    TRACE_FUNCTION_SCOPE();

    Assert(CmdBatch != nullptr);
    Assert(RootSignature != nullptr);

    ID3D12Device* DxDevice = GetDevice()->GetDevice();
    ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

    CD3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    CD3D12OnlineDescriptorHeap* SamplerHeap = CmdBatch->GetOnlineSamplerDescriptorHeap();

    AllocateDescriptorsAndSetHeaps(DxCmdList, ResourceHeap, SamplerHeap);

    const EShaderVisibility ShaderVisibility = ShaderVisibility_All;

    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderVisibility, EResourceType::ResourceType_CBV);
    CopyAndBindComputeDescriptors(DxDevice, DxCmdList, ConstantBufferViewCache, ParameterIndex);

    ParameterIndex = RootSignature->GetRootParameterIndex(ShaderVisibility, EResourceType::ResourceType_SRV);
    CopyAndBindComputeDescriptors(DxDevice, DxCmdList, ShaderResourceViewCache, ParameterIndex);

    ParameterIndex = RootSignature->GetRootParameterIndex(ShaderVisibility, EResourceType::ResourceType_UAV);
    CopyAndBindComputeDescriptors(DxDevice, DxCmdList, UnorderedAccessViewCache, ParameterIndex);

    ParameterIndex = RootSignature->GetRootParameterIndex(ShaderVisibility, EResourceType::ResourceType_Sampler);
    CopyAndBindComputeDescriptors(DxDevice, DxCmdList, SamplerStateCache, ParameterIndex);
}

void CD3D12DescriptorCache::Reset()
{
    VertexBufferCache.Reset();
    RenderTargetCache.Reset();

    ConstantBufferViewCache.Reset(NullCBV);
    ShaderResourceViewCache.Reset(NullSRV);
    UnorderedAccessViewCache.Reset(NullUAV);
    SamplerStateCache.Reset(NullSampler);

    PreviousDescriptorHeaps[0] = nullptr;
    PreviousDescriptorHeaps[1] = nullptr;
}

void CD3D12DescriptorCache::AllocateDescriptorsAndSetHeaps(ID3D12GraphicsCommandList* CmdList, CD3D12OnlineDescriptorHeap* ResourceHeap, CD3D12OnlineDescriptorHeap* SamplerHeap)
{
    uint32 NumConstantBuffersViews = ConstantBufferViewCache.CountNeededDescriptors();
    uint32 NumShaderResourceViews = ShaderResourceViewCache.CountNeededDescriptors();
    uint32 NumUnorderedAccessViews = UnorderedAccessViewCache.CountNeededDescriptors();

    uint32 NumResourceDescriptors = NumConstantBuffersViews + NumShaderResourceViews + NumUnorderedAccessViews;
    if (!ResourceHeap->HasSpace(NumResourceDescriptors))
    {
        ConstantBufferViewCache.InvalidateAll();
        ShaderResourceViewCache.InvalidateAll();
        UnorderedAccessViewCache.InvalidateAll();

        ResourceHeap->AllocateFreshHeap();

        NumConstantBuffersViews = ConstantBufferViewCache.CountNeededDescriptors();
        NumShaderResourceViews = ShaderResourceViewCache.CountNeededDescriptors();
        NumUnorderedAccessViews = UnorderedAccessViewCache.CountNeededDescriptors();
        NumResourceDescriptors = NumConstantBuffersViews + NumShaderResourceViews + NumUnorderedAccessViews;
    }

    ConstantBufferViewCache.PrepareForCopy();
    ShaderResourceViewCache.PrepareForCopy();
    UnorderedAccessViewCache.PrepareForCopy();

    uint32 NumSamplerDescriptors = SamplerStateCache.CountNeededDescriptors();
    if (!SamplerHeap->HasSpace(NumSamplerDescriptors))
    {
        SamplerStateCache.InvalidateAll();
        SamplerHeap->AllocateFreshHeap();

        NumSamplerDescriptors = SamplerStateCache.CountNeededDescriptors();
    }

    SamplerStateCache.PrepareForCopy();

    D3D12_ERROR(NumResourceDescriptors <= D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT, "[D3D12]: Trying to bind more Resource Descriptors (NumDescriptors=" + ToString(NumResourceDescriptors) + ") than the maximum (MaxResourceDescriptors=" + ToString(D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT) + ") ");
    uint32 ResourceDescriptorHandle = ResourceHeap->AllocateHandles(NumResourceDescriptors);

    D3D12_ERROR(NumSamplerDescriptors <= D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT, "[D3D12]: Trying to bind more Sampler Descriptors (NumDescriptors=" + ToString(NumSamplerDescriptors) + ") than the maximum (MaxSamplerDescriptors=" + ToString(D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT) + ") ");
    uint32 SamplerDescriptorHandle = SamplerHeap->AllocateHandles(NumSamplerDescriptors);

    ID3D12DescriptorHeap* DescriptorHeaps[] =
    {
        ResourceHeap->GetNativeHeap(),
        SamplerHeap->GetNativeHeap()
    };

    if (PreviousDescriptorHeaps[0] != DescriptorHeaps[0] || PreviousDescriptorHeaps[1] != DescriptorHeaps[1])
    {
        CmdList->SetDescriptorHeaps(ArrayCount(DescriptorHeaps), DescriptorHeaps);

        PreviousDescriptorHeaps[0] = DescriptorHeaps[0];
        PreviousDescriptorHeaps[1] = DescriptorHeaps[1];
    }

    if (NumConstantBuffersViews > 0)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE HostHandle = ResourceHeap->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
        D3D12_GPU_DESCRIPTOR_HANDLE DeviceHandle = ResourceHeap->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);

        ConstantBufferViewCache.SetAllocatedDescriptorHandles(HostHandle, DeviceHandle, ResourceHeap->GetDescriptorHandleIncrementSize());
        ResourceDescriptorHandle += NumConstantBuffersViews;
    }

    if (NumShaderResourceViews > 0)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE HostHandle = ResourceHeap->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
        D3D12_GPU_DESCRIPTOR_HANDLE DeviceHandle = ResourceHeap->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);

        ShaderResourceViewCache.SetAllocatedDescriptorHandles(HostHandle, DeviceHandle, ResourceHeap->GetDescriptorHandleIncrementSize());
        ResourceDescriptorHandle += NumShaderResourceViews;
    }

    if (NumUnorderedAccessViews > 0)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE HostHandle = ResourceHeap->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
        D3D12_GPU_DESCRIPTOR_HANDLE DeviceHandle = ResourceHeap->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);
        UnorderedAccessViewCache.SetAllocatedDescriptorHandles(HostHandle, DeviceHandle, ResourceHeap->GetDescriptorHandleIncrementSize());
    }

    if (NumSamplerDescriptors > 0)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE HostHandle = SamplerHeap->GetCPUDescriptorHandleAt(SamplerDescriptorHandle);
        D3D12_GPU_DESCRIPTOR_HANDLE DeviceHandle = SamplerHeap->GetGPUDescriptorHandleAt(SamplerDescriptorHandle);
        SamplerStateCache.SetAllocatedDescriptorHandles(HostHandle, DeviceHandle, SamplerHeap->GetDescriptorHandleIncrementSize());
    }
}

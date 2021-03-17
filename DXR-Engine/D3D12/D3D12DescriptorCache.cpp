#include "D3D12DescriptorCache.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12RenderLayer.h"
#include "D3D12CommandContext.h"

D3D12DescriptorCache::D3D12DescriptorCache(D3D12Device* InDevice)
    : D3D12DeviceChild(InDevice)
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

D3D12DescriptorCache::~D3D12DescriptorCache()
{
    SafeDelete(NullCBV);
    SafeDelete(NullSRV);
    SafeDelete(NullUAV);
    SafeDelete(NullSampler);
}

bool D3D12DescriptorCache::Init()
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
    Memory::Memzero(&CBVDesc);

    CBVDesc.BufferLocation = 0;
    CBVDesc.SizeInBytes    = 0;

    NullCBV = DBG_NEW D3D12ConstantBufferView(GetDevice(), gD3D12RenderLayer->GetResourceOfflineDescriptorHeap());
    if (!NullCBV->Init())
    {
        return false;
    }

    if (!NullCBV->CreateView(nullptr, CBVDesc))
    {
        return false;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
    Memory::Memzero(&UAVDesc);

    UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
    UAVDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    UAVDesc.Texture2D.MipSlice   = 0;
    UAVDesc.Texture2D.PlaneSlice = 0;

    NullUAV = DBG_NEW D3D12UnorderedAccessView(GetDevice(), gD3D12RenderLayer->GetResourceOfflineDescriptorHeap());
    if (!NullUAV->Init())
    {
        return false;
    }

    if (!NullUAV->CreateView(nullptr, nullptr, UAVDesc))
    {
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    Memory::Memzero(&SRVDesc);

    SRVDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Format                        = DXGI_FORMAT_R8G8B8A8_UNORM;
    SRVDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Texture2D.MipLevels           = 1;
    SRVDesc.Texture2D.MostDetailedMip     = 0;
    SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    SRVDesc.Texture2D.PlaneSlice          = 0;

    NullSRV = DBG_NEW D3D12ShaderResourceView(GetDevice(), gD3D12RenderLayer->GetResourceOfflineDescriptorHeap());
    if (!NullSRV->Init())
    {
        return false;
    }

    if (!NullSRV->CreateView(nullptr, SRVDesc))
    {
        return false;
    }

    D3D12_SAMPLER_DESC SamplerDesc;
    Memory::Memzero(&SamplerDesc);

    SamplerDesc.AddressU       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressV       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressW       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.BorderColor[0] = 1.0f;
    SamplerDesc.BorderColor[1] = 1.0f;
    SamplerDesc.BorderColor[2] = 1.0f;
    SamplerDesc.BorderColor[3] = 1.0f;
    SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    SamplerDesc.Filter         = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerDesc.MaxAnisotropy  = 1;
    SamplerDesc.MaxLOD         = FLT_MAX;
    SamplerDesc.MinLOD         = -FLT_MAX;
    SamplerDesc.MipLODBias     = 0.0f;

    NullSampler = DBG_NEW D3D12SamplerState(GetDevice(), gD3D12RenderLayer->GetSamplerOfflineDescriptorHeap());
    if (!NullSampler->Init(SamplerDesc))
    {
        return false;
    }

    for (uint32 i = 0; i < NUM_DESCRIPTORS; i++)
    {
        RangeSizes[i] = 1;
    }

    return true;
}

void D3D12DescriptorCache::CommitGraphicsDescriptors(D3D12CommandListHandle& CmdList, D3D12CommandBatch* CmdBatch, D3D12RootSignature* RootSignature)
{
    Assert(CmdBatch != nullptr);
    Assert(RootSignature != nullptr);

    VertexBufferCache.CommitState(CmdList);
    RenderTargetCache.CommitState(CmdList);

    ID3D12GraphicsCommandList* DxCmdList    = CmdList.GetGraphicsCommandList();
    D3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    D3D12OnlineDescriptorHeap* SamplerHeap  = CmdBatch->GetOnlineSamplerDescriptorHeap();

    CopyDescriptorsAndSetHeaps(DxCmdList, ResourceHeap, SamplerHeap);

    for (uint32 i = 0; i < ShaderVisibility_Count; i++)
    {
        EShaderVisibility Visibility = (EShaderVisibility)i;

        uint64 NumCBVs = ConstantBufferViewCache.DescriptorRangeLengths[Visibility];
        int32  ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_CBV);
        if (ParameterIndex >= 0 && NumCBVs > 0)
        {
            D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ConstantBufferViewCache.Descriptors[Visibility];
            DxCmdList->SetGraphicsRootDescriptorTable(ParameterIndex, GpuHandle);
        }

        uint64 NumSRVs = ShaderResourceViewCache.DescriptorRangeLengths[Visibility];
        ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_SRV);
        if (ParameterIndex >= 0 && NumSRVs > 0)
        {
            D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ShaderResourceViewCache.Descriptors[Visibility];
            DxCmdList->SetGraphicsRootDescriptorTable(ParameterIndex, GpuHandle);
        }

        uint64 NumUAVs = UnorderedAccessViewCache.DescriptorRangeLengths[Visibility];
        ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_UAV);
        if (ParameterIndex >= 0 && NumUAVs > 0)
        {
            D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = UnorderedAccessViewCache.Descriptors[Visibility];
            DxCmdList->SetGraphicsRootDescriptorTable(ParameterIndex, GpuHandle);
        }

        uint64 NumSamplers = SamplerStateCache.DescriptorRangeLengths[Visibility];
        ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_Sampler);
        if (ParameterIndex >= 0 && NumSamplers > 0)
        {
            D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = SamplerStateCache.Descriptors[Visibility];
            DxCmdList->SetGraphicsRootDescriptorTable(ParameterIndex, GpuHandle);
        }
    }
}

void D3D12DescriptorCache::CommitComputeDescriptors(D3D12CommandListHandle& CmdList, D3D12CommandBatch* CmdBatch, D3D12RootSignature* RootSignature)
{
    Assert(CmdBatch != nullptr);
    Assert(RootSignature != nullptr);

    ID3D12GraphicsCommandList* DxCmdList    = CmdList.GetGraphicsCommandList();
    D3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    D3D12OnlineDescriptorHeap* SamplerHeap  = CmdBatch->GetOnlineSamplerDescriptorHeap();

    CopyDescriptorsAndSetHeaps(DxCmdList, ResourceHeap, SamplerHeap);

    EShaderVisibility Visibility = ShaderVisibility_All;

    uint64 NumCBVs = ConstantBufferViewCache.DescriptorRangeLengths[Visibility];
    int32  ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_CBV);
    if (ParameterIndex >= 0 && NumCBVs > 0)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ConstantBufferViewCache.Descriptors[Visibility];
        DxCmdList->SetComputeRootDescriptorTable(ParameterIndex, GpuHandle);
    }

    uint64 NumSRVs = ShaderResourceViewCache.DescriptorRangeLengths[Visibility];
    ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_SRV);
    if (ParameterIndex >= 0 && NumSRVs > 0)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ShaderResourceViewCache.Descriptors[Visibility];
        DxCmdList->SetComputeRootDescriptorTable(ParameterIndex, GpuHandle);
    }

    uint64 NumUAVs = UnorderedAccessViewCache.DescriptorRangeLengths[Visibility];
    ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_UAV);
    if (ParameterIndex >= 0 && NumUAVs > 0)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = UnorderedAccessViewCache.Descriptors[Visibility];
        DxCmdList->SetComputeRootDescriptorTable(ParameterIndex, GpuHandle);
    }

    uint64 NumSamplers = SamplerStateCache.DescriptorRangeLengths[Visibility];
    ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_Sampler);
    if (ParameterIndex >= 0 && NumSamplers > 0)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = SamplerStateCache.Descriptors[Visibility];
        DxCmdList->SetComputeRootDescriptorTable(ParameterIndex, GpuHandle);
    }
}

void D3D12DescriptorCache::Reset()
{
    VertexBufferCache.Reset();
    RenderTargetCache.Reset();

    ConstantBufferViewCache.Reset();
    ShaderResourceViewCache.Reset();
    UnorderedAccessViewCache.Reset();
    SamplerStateCache.Reset();

    PreviousDescriptorHeaps[0] = nullptr;
    PreviousDescriptorHeaps[1] = nullptr;
}

void D3D12DescriptorCache::CopyDescriptorsAndSetHeaps(
    ID3D12GraphicsCommandList* CmdList, 
    D3D12OnlineDescriptorHeap* ResourceHeap, 
    D3D12OnlineDescriptorHeap* SamplerHeap)
{
    uint32 NumResourceDescriptors =
        ConstantBufferViewCache.CountNeededDescriptors() +
        ShaderResourceViewCache.CountNeededDescriptors() +
        UnorderedAccessViewCache.CountNeededDescriptors();

    if (!ResourceHeap->HasSpace(NumResourceDescriptors))
    {
        ResourceHeap->AllocateFreshHeap();

        ConstantBufferViewCache.InvalidateAll();
        ShaderResourceViewCache.InvalidateAll();
        UnorderedAccessViewCache.InvalidateAll();
    }

    ConstantBufferViewCache.PrepareForCopy(NullCBV);
    ShaderResourceViewCache.PrepareForCopy(NullSRV);
    UnorderedAccessViewCache.PrepareForCopy(NullUAV);

    uint32 NumSamplerDescriptors = SamplerStateCache.CountNeededDescriptors();
    if (!SamplerHeap->HasSpace(NumSamplerDescriptors))
    {
        SamplerHeap->AllocateFreshHeap();
        SamplerStateCache.InvalidateAll();
    }
    
    SamplerStateCache.PrepareForCopy(NullSampler);

    Assert(NumResourceDescriptors < D3D12_MAX_ONLINE_DESCRIPTOR_COUNT);
    uint32 ResourceDescriptorHandle = ResourceHeap->AllocateHandles(NumResourceDescriptors);

    Assert(NumSamplerDescriptors < D3D12_MAX_ONLINE_DESCRIPTOR_COUNT);
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

    ID3D12Device* DxDevice = GetDevice()->GetDevice();
    if (ConstantBufferViewCache.TotalNumDescriptors > 0)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = ResourceHeap->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ResourceHeap->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);

        UINT DestDescriptorRangeSize = ConstantBufferViewCache.TotalNumDescriptors;
        DxDevice->CopyDescriptors(1, &CpuHandle, &DestDescriptorRangeSize, DestDescriptorRangeSize, ConstantBufferViewCache.CopyDescriptors, RangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        ConstantBufferViewCache.SetGPUHandles(GpuHandle, ResourceHeap->GetDescriptorHandleIncrementSize());
        ResourceDescriptorHandle += DestDescriptorRangeSize;
    }
    if (ShaderResourceViewCache.TotalNumDescriptors > 0)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = ResourceHeap->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ResourceHeap->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);

        UINT DestDescriptorRangeSize = ShaderResourceViewCache.TotalNumDescriptors;
        DxDevice->CopyDescriptors(1, &CpuHandle, &DestDescriptorRangeSize, DestDescriptorRangeSize, ShaderResourceViewCache.CopyDescriptors, RangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        ShaderResourceViewCache.SetGPUHandles(GpuHandle, ResourceHeap->GetDescriptorHandleIncrementSize());
        ResourceDescriptorHandle += DestDescriptorRangeSize;
    }
    if (UnorderedAccessViewCache.TotalNumDescriptors > 0)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = ResourceHeap->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ResourceHeap->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);

        UINT DestDescriptorRangeSize = UnorderedAccessViewCache.TotalNumDescriptors;
        DxDevice->CopyDescriptors(1, &CpuHandle, &DestDescriptorRangeSize, DestDescriptorRangeSize, UnorderedAccessViewCache.CopyDescriptors, RangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        UnorderedAccessViewCache.SetGPUHandles(GpuHandle, ResourceHeap->GetDescriptorHandleIncrementSize());
        ResourceDescriptorHandle += DestDescriptorRangeSize;
    }
    if (SamplerStateCache.TotalNumDescriptors > 0)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = SamplerHeap->GetCPUDescriptorHandleAt(SamplerDescriptorHandle);
        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = SamplerHeap->GetGPUDescriptorHandleAt(SamplerDescriptorHandle);

        UINT DestDescriptorRangeSize = SamplerStateCache.TotalNumDescriptors;
        DxDevice->CopyDescriptors(1, &CpuHandle, &DestDescriptorRangeSize, DestDescriptorRangeSize, SamplerStateCache.CopyDescriptors, RangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        SamplerStateCache.SetGPUHandles(GpuHandle, SamplerHeap->GetDescriptorHandleIncrementSize());
    }
}

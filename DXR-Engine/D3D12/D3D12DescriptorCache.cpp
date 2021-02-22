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

Bool D3D12DescriptorCache::Init()
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

    for (UInt32 i = 0; i < NUM_DESCRIPTORS; i++)
    {
        RangeSizes[i] = 1;
    }

    return true;
}

void D3D12DescriptorCache::CommitGraphicsDescriptorTables(D3D12CommandListHandle& CmdList, D3D12CommandBatch* CmdBatch, D3D12RootSignature* RootSignature)
{
    Assert(CmdBatch != nullptr);
    Assert(RootSignature != nullptr);

    ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

    UInt32 NumResourceDescriptors = 0;
    UInt32 NumSamplerDescriptors  = 0;
    for (UInt32 i = 0; i < ShaderVisibility_Count; i++)
    {
        UInt64 NumCBVs = ConstantBufferViewCache.DescriptorRangeLengths[i];
        NumResourceDescriptors += NumCBVs;

        UInt64 NumSRVs = ShaderResourceViewCache.DescriptorRangeLengths[i];
        NumResourceDescriptors += NumSRVs;

        UInt64 NumUAVs = UnorderedAccessViewCache.DescriptorRangeLengths[i];
        NumResourceDescriptors += NumUAVs;

        UInt64 NumSamplers = SamplerStateCache.DescriptorRangeLengths[i];
        NumSamplerDescriptors += NumSamplers;

        Assert(NumResourceDescriptors < D3D12_MAX_ONLINE_DESCRIPTOR_HEAP_COUNT);
        Assert(NumSamplerDescriptors < D3D12_MAX_ONLINE_DESCRIPTOR_HEAP_COUNT);
    }

    D3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    UInt32 ResourceDescriptorHandle = ResourceHeap->AllocateHandles(NumResourceDescriptors);
    Assert(ResourceDescriptorHandle < D3D12_MAX_ONLINE_DESCRIPTOR_HEAP_COUNT);

    D3D12OnlineDescriptorHeap* SamplerHeap = CmdBatch->GetOnlineSamplerDescriptorHeap();
    UInt32 SamplerDescriptorHandle = SamplerHeap->AllocateHandles(NumSamplerDescriptors);
    Assert(SamplerDescriptorHandle < D3D12_MAX_ONLINE_DESCRIPTOR_HEAP_COUNT);

    ID3D12DescriptorHeap* DescriptorHeaps[] =
    {
        ResourceHeap->GetNativeHeap(),
        SamplerHeap->GetNativeHeap()
    };

    DxCmdList->SetDescriptorHeaps(ArrayCount(DescriptorHeaps), DescriptorHeaps);
    DxCmdList->SetGraphicsRootSignature(RootSignature->GetRootSignature());

    for (UInt32 i = 0; i < ShaderVisibility_Count; i++)
    {
        EShaderVisibility Visibility = (EShaderVisibility)i;

        UInt64 NumCBVs = ConstantBufferViewCache.DescriptorRangeLengths[i];
        Int32  ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_CBV);
        if (ParameterIndex >= 0 && NumCBVs > 0)
        {
            UINT DestDescriptorRangeSize = NumCBVs;

            D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = ResourceHeap->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
            GetDevice()->GetDevice()->CopyDescriptors(
                1, &CpuHandle, &DestDescriptorRangeSize,
                NumCBVs, ConstantBufferViewCache.Descriptors[i], RangeSizes,
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ResourceHeap->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);
            DxCmdList->SetGraphicsRootDescriptorTable(ParameterIndex, GpuHandle);

            ResourceDescriptorHandle += NumCBVs;
        }

        UInt64 NumSRVs = ShaderResourceViewCache.DescriptorRangeLengths[i];
        ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_SRV);
        if (ParameterIndex >= 0 && NumSRVs > 0)
        {
            UINT DestDescriptorRangeSize = NumSRVs;

            D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = ResourceHeap->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
            GetDevice()->GetDevice()->CopyDescriptors(
                1, &CpuHandle, &DestDescriptorRangeSize,
                NumSRVs, ShaderResourceViewCache.Descriptors[i], RangeSizes,
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ResourceHeap->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);
            DxCmdList->SetGraphicsRootDescriptorTable(ParameterIndex, GpuHandle);

            ResourceDescriptorHandle += NumSRVs;
        }

        UInt64 NumUAVs = UnorderedAccessViewCache.DescriptorRangeLengths[i];
        ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_UAV);
        if (ParameterIndex >= 0 && NumUAVs > 0)
        {
            UINT DestDescriptorRangeSize = NumUAVs;

            D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = ResourceHeap->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
            GetDevice()->GetDevice()->CopyDescriptors(
                1, &CpuHandle, &DestDescriptorRangeSize,
                NumUAVs, UnorderedAccessViewCache.Descriptors[i], RangeSizes,
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ResourceHeap->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);
            DxCmdList->SetGraphicsRootDescriptorTable(ParameterIndex, GpuHandle);

            ResourceDescriptorHandle += NumUAVs;
        }

        UInt64 NumSamplers = SamplerStateCache.DescriptorRangeLengths[i];
        ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_Sampler);
        if (ParameterIndex >= 0 && NumSamplers > 0)
        {
            UINT DestDescriptorRangeSize = NumSamplers;

            D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = SamplerHeap->GetCPUDescriptorHandleAt(SamplerDescriptorHandle);
            GetDevice()->GetDevice()->CopyDescriptors(
                1, &CpuHandle, &DestDescriptorRangeSize,
                NumSamplers, SamplerStateCache.Descriptors[i], RangeSizes,
                D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

            D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = SamplerHeap->GetGPUDescriptorHandleAt(SamplerDescriptorHandle);
            DxCmdList->SetGraphicsRootDescriptorTable(ParameterIndex, GpuHandle);

            SamplerDescriptorHandle += NumSamplers;
        }
    }
}

void D3D12DescriptorCache::CommitComputeDescriptorTables(D3D12CommandListHandle& CmdList, D3D12CommandBatch* CmdBatch, D3D12RootSignature* RootSignature)
{
    Assert(CmdBatch != nullptr);
    Assert(RootSignature != nullptr);

    ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

    UInt32 NumResourceDescriptors = 0;
    UInt32 NumSamplerDescriptors  = 0;
    for (UInt32 i = 0; i < ShaderVisibility_Count; i++)
    {
        UInt64 NumCBVs = ConstantBufferViewCache.DescriptorRangeLengths[i];
        NumResourceDescriptors += NumCBVs;

        UInt64 NumSRVs = ShaderResourceViewCache.DescriptorRangeLengths[i];
        NumResourceDescriptors += NumSRVs;

        UInt64 NumUAVs = UnorderedAccessViewCache.DescriptorRangeLengths[i];
        NumResourceDescriptors += NumUAVs;

        UInt64 NumSamplers = SamplerStateCache.DescriptorRangeLengths[i];
        NumSamplerDescriptors += NumSamplers;
        
        Assert(NumResourceDescriptors < D3D12_MAX_ONLINE_DESCRIPTOR_HEAP_COUNT);
        Assert(NumSamplerDescriptors < D3D12_MAX_ONLINE_DESCRIPTOR_HEAP_COUNT);
    }

    D3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    UInt32 ResourceDescriptorHandle = ResourceHeap->AllocateHandles(NumResourceDescriptors);
    Assert(ResourceDescriptorHandle < D3D12_MAX_ONLINE_DESCRIPTOR_HEAP_COUNT);

    D3D12OnlineDescriptorHeap* SamplerHeap = CmdBatch->GetOnlineSamplerDescriptorHeap();
    UInt32 SamplerDescriptorHandle = SamplerHeap->AllocateHandles(NumSamplerDescriptors);
    Assert(SamplerDescriptorHandle < D3D12_MAX_ONLINE_DESCRIPTOR_HEAP_COUNT);

    ID3D12DescriptorHeap* DescriptorHeaps[] =
    {
        ResourceHeap->GetNativeHeap(),
        SamplerHeap->GetNativeHeap()
    };

    DxCmdList->SetDescriptorHeaps(ArrayCount(DescriptorHeaps), DescriptorHeaps);
    DxCmdList->SetGraphicsRootSignature(RootSignature->GetRootSignature());

    EShaderVisibility Visibility = ShaderVisibility_All;

    UInt64 NumCBVs        = ConstantBufferViewCache.DescriptorRangeLengths[Visibility];
    Int32  ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_CBV);
    if (ParameterIndex >= 0 && NumCBVs > 0)
    {
        UINT DestDescriptorRangeSize = NumCBVs;

        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = ResourceHeap->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
        GetDevice()->GetDevice()->CopyDescriptors(
            1, &CpuHandle, &DestDescriptorRangeSize, 
            NumCBVs, ConstantBufferViewCache.Descriptors[Visibility], RangeSizes,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ResourceHeap->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);
        DxCmdList->SetComputeRootDescriptorTable(ParameterIndex, GpuHandle);

        ResourceDescriptorHandle += NumCBVs;
    }

    UInt64 NumSRVs = ShaderResourceViewCache.DescriptorRangeLengths[Visibility];
    ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_SRV);
    if (ParameterIndex >= 0 && NumSRVs > 0)
    {
        UINT DestDescriptorRangeSize = NumSRVs;

        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = ResourceHeap->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
        GetDevice()->GetDevice()->CopyDescriptors(
            1, &CpuHandle, &DestDescriptorRangeSize,
            NumSRVs, ShaderResourceViewCache.Descriptors[Visibility], RangeSizes,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ResourceHeap->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);
        DxCmdList->SetComputeRootDescriptorTable(ParameterIndex, GpuHandle);

        ResourceDescriptorHandle += NumSRVs;
    }

    UInt64 NumUAVs = UnorderedAccessViewCache.DescriptorRangeLengths[Visibility];
    ParameterIndex = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_UAV);
    if (ParameterIndex >= 0 && NumUAVs > 0)
    {
        UINT DestDescriptorRangeSize = NumUAVs;

        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = ResourceHeap->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
        GetDevice()->GetDevice()->CopyDescriptors(
            1, &CpuHandle, &DestDescriptorRangeSize,
            NumUAVs, UnorderedAccessViewCache.Descriptors[Visibility], RangeSizes,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = ResourceHeap->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);
        DxCmdList->SetComputeRootDescriptorTable(ParameterIndex, GpuHandle);

        ResourceDescriptorHandle += NumUAVs;
    }

    UInt64 NumSamplers = SamplerStateCache.DescriptorRangeLengths[Visibility];
    ParameterIndex     = RootSignature->GetRootParameterIndex(Visibility, EResourceType::ResourceType_Sampler);
    if (ParameterIndex >= 0 && NumSamplers > 0)
    {
        UINT DestDescriptorRangeSize = NumSamplers;

        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = SamplerHeap->GetCPUDescriptorHandleAt(SamplerDescriptorHandle);
        GetDevice()->GetDevice()->CopyDescriptors(
            1, &CpuHandle, &DestDescriptorRangeSize,
            NumSamplers, SamplerStateCache.Descriptors[Visibility], RangeSizes,
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = SamplerHeap->GetGPUDescriptorHandleAt(SamplerDescriptorHandle);
        DxCmdList->SetComputeRootDescriptorTable(ParameterIndex, GpuHandle);

        SamplerDescriptorHandle += NumSamplers;
    }
}

void D3D12DescriptorCache::Reset()
{
    ConstantBufferViewCache.Reset();
    ShaderResourceViewCache.Reset();
    UnorderedAccessViewCache.Reset();
    SamplerStateCache.Reset();

    for (UInt32 i = 0; i < ShaderVisibility_Count; i++)
    {
        EShaderVisibility Visibility = (EShaderVisibility)i;
        for (UInt32 d = 0; d < D3D12_DEFAULT_CONSTANT_BUFFER_COUNT; d++)
        {
            ConstantBufferViewCache.Set(NullCBV, Visibility, d);
        }
        for (UInt32 d = 0; d < D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT; d++)
        {
            ShaderResourceViewCache.Set(NullSRV, Visibility, d);
        }
        for (UInt32 d = 0; d < D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT; d++)
        {
            UnorderedAccessViewCache.Set(NullUAV, Visibility, d);
        }
        for (UInt32 d = 0; d < D3D12_DEFAULT_SAMPLER_STATE_COUNT; d++)
        {
            SamplerStateCache.Set(NullSampler, Visibility, d);
        }
    }
}

void D3D12DescriptorCache::CopyDescriptors(D3D12CommandListHandle& CmdList, D3D12CommandBatch* CmdBatch, D3D12RootSignature* RootSignature)
{
}

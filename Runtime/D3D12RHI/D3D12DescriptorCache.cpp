#include "D3D12DescriptorCache.h"
#include "D3D12Descriptors.h"
#include "D3D12CoreInterface.h"
#include "D3D12CommandContext.h"

#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Templates/EnumUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12DescriptorCache

FD3D12DescriptorCache::FD3D12DescriptorCache(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , NullCBV(nullptr)
    , NullSRV(nullptr)
    , NullUAV(nullptr)
    , NullSampler(nullptr)
    , ShaderResourceViewCache()
    , UnorderedAccessViewCache()
    , ConstantBufferViewCache()
    , SamplerStateCache()
    , RangeSizes()
{ }

FD3D12DescriptorCache::~FD3D12DescriptorCache()
{
    SafeDelete(NullCBV);
    SafeRelease(NullSRV);
    SafeRelease(NullUAV);
    SafeRelease(NullSampler);
}

bool FD3D12DescriptorCache::Initialize()
{
    FD3D12CoreInterface* D3D12CoreInterface = GetDevice()->GetAdapter()->GetCoreInterface();
    
    D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
    FMemory::Memzero(&CBVDesc);

    CBVDesc.BufferLocation = 0;
    CBVDesc.SizeInBytes    = 0;

    NullCBV = dbg_new FD3D12ConstantBufferView(GetDevice(), D3D12CoreInterface->GetResourceOfflineDescriptorHeap());
    if (!NullCBV->AllocateHandle())
    {
        return false;
    }

    if (!NullCBV->CreateView(nullptr, CBVDesc))
    {
        return false;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
    FMemory::Memzero(&UAVDesc);

    UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
    UAVDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    UAVDesc.Texture2D.MipSlice   = 0;
    UAVDesc.Texture2D.PlaneSlice = 0;

    NullUAV = dbg_new FD3D12UnorderedAccessView(GetDevice(), D3D12CoreInterface->GetResourceOfflineDescriptorHeap(), nullptr);
    if (!NullUAV->AllocateHandle())
    {
        return false;
    }

    if (!NullUAV->CreateView(nullptr, nullptr, UAVDesc))
    {
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    FMemory::Memzero(&SRVDesc);

    SRVDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Format                        = DXGI_FORMAT_R8G8B8A8_UNORM;
    SRVDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Texture2D.MipLevels           = 1;
    SRVDesc.Texture2D.MostDetailedMip     = 0;
    SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    SRVDesc.Texture2D.PlaneSlice          = 0;

    NullSRV = dbg_new FD3D12ShaderResourceView(GetDevice(), D3D12CoreInterface->GetResourceOfflineDescriptorHeap(), nullptr);
    if (!NullSRV->AllocateHandle())
    {
        return false;
    }

    if (!NullSRV->CreateView(nullptr, SRVDesc))
    {
        return false;
    }

    D3D12_SAMPLER_DESC SamplerDesc;
    FMemory::Memzero(&SamplerDesc);

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

    NullSampler = dbg_new FD3D12SamplerState(GetDevice(), D3D12CoreInterface->GetSamplerOfflineDescriptorHeap());
    if (!NullSampler->CreateSampler(SamplerDesc))
    {
        return false;
    }

    for (uint32 Index = 0; Index < D3D12_CACHED_DESCRIPTORS_COUNT; ++Index)
    {
        RangeSizes[Index] = 1;
    }

    // Start by resetting the descriptor cache
    Reset();
    return true;
}

void FD3D12DescriptorCache::PrepareGraphicsDescriptors(FD3D12CommandList& CmdList, FD3D12CommandBatch* CmdBatch, FD3D12RootSignature* RootSignature, FD3D12PipelineStageMask PipelineMask)
{
    // TRACE_FUNCTION_SCOPE();

    Check(CmdBatch      != nullptr);
    Check(RootSignature != nullptr);

    // Vertex and render-targets
    VertexBufferCache.CommitState(CmdList, CmdBatch);
    RenderTargetCache.CommitState(CmdList);

    // Allocate descriptors for resources and samplers 
    ID3D12Device*              DxDevice  = GetDevice()->GetD3D12Device();
    ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

    FD3D12OnlineDescriptorManager* ResourceManager = CmdBatch->GetResourceDescriptorManager();
    FD3D12OnlineDescriptorManager* SamplerManager  = CmdBatch->GetSamplerDescriptorManager();

    AllocateDescriptorsAndSetHeaps(DxCmdList, ResourceManager, SamplerManager, PipelineMask);

    // Copy and bind resources and samplers
    for (uint32 ShaderStage = ShaderVisibility_Vertex; ShaderStage < ShaderVisibility_Count; ++ShaderStage)
    {
        const EShaderVisibility ShaderVisibility = static_cast<EShaderVisibility>(ShaderStage);
        if (PipelineMask.CheckShaderVisibility(ShaderVisibility))
        {
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
}

void FD3D12DescriptorCache::PrepareComputeDescriptors(FD3D12CommandList& CmdList, FD3D12CommandBatch* CmdBatch, FD3D12RootSignature* RootSignature)
{
    // TRACE_FUNCTION_SCOPE();

    Check(CmdBatch      != nullptr);
    Check(RootSignature != nullptr);

    ID3D12Device*              DxDevice  = GetDevice()->GetD3D12Device();
    ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

    FD3D12OnlineDescriptorManager* ResourceDescriptors = CmdBatch->GetResourceDescriptorManager();
    FD3D12OnlineDescriptorManager* SamplerDescriptors  = CmdBatch->GetSamplerDescriptorManager();

    AllocateDescriptorsAndSetHeaps(DxCmdList, ResourceDescriptors, SamplerDescriptors, FD3D12PipelineStageMask::ComputeMask());

    int32 ParameterIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, EResourceType::ResourceType_CBV);
    CopyAndBindComputeDescriptors(DxDevice, DxCmdList, ConstantBufferViewCache, ParameterIndex);

    ParameterIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, EResourceType::ResourceType_SRV);
    CopyAndBindComputeDescriptors(DxDevice, DxCmdList, ShaderResourceViewCache, ParameterIndex);

    ParameterIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, EResourceType::ResourceType_UAV);
    CopyAndBindComputeDescriptors(DxDevice, DxCmdList, UnorderedAccessViewCache, ParameterIndex);

    ParameterIndex = RootSignature->GetRootParameterIndex(ShaderVisibility_All, EResourceType::ResourceType_Sampler);
    CopyAndBindComputeDescriptors(DxDevice, DxCmdList, SamplerStateCache, ParameterIndex);
}

void FD3D12DescriptorCache::Reset()
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

void FD3D12DescriptorCache::AllocateDescriptorsAndSetHeaps( ID3D12GraphicsCommandList* CmdList
                                                          , FD3D12OnlineDescriptorManager* ResourceDescriptors
                                                          , FD3D12OnlineDescriptorManager* SamplerDescriptors
                                                          , FD3D12PipelineStageMask PipelineMask)
{
    // Count descriptors
    uint32 NumResourceDescriptors = 0;
    uint32 NumSamplerDescriptors  = 0;
    for (uint32 ShaderStage = 0; ShaderStage < ShaderVisibility_Count; ++ShaderStage)
    {
        const EShaderVisibility ShaderVisibility = static_cast<EShaderVisibility>(ShaderStage);
        if (PipelineMask.CheckShaderVisibility(ShaderVisibility))
        {
            NumResourceDescriptors += ConstantBufferViewCache.GetNumDescriptorsForStage(ShaderVisibility);
            NumResourceDescriptors += ShaderResourceViewCache.GetNumDescriptorsForStage(ShaderVisibility);
            NumResourceDescriptors += UnorderedAccessViewCache.GetNumDescriptorsForStage(ShaderVisibility);

            NumSamplerDescriptors += SamplerStateCache.GetNumDescriptorsForStage(ShaderVisibility);
        }
    }

    if (!ResourceDescriptors->HasSpace(NumResourceDescriptors))
    {
        Check(false);

        // ConstantBufferViewCache.InvalidateAll();
        // ShaderResourceViewCache.InvalidateAll();
        // UnorderedAccessViewCache.InvalidateAll();

        // ResourceDescriptors->AllocateFreshHeap();

        // NumConstantBuffersViews = ConstantBufferViewCache.CountNeededDescriptors();
        // NumShaderResourceViews  = ShaderResourceViewCache.CountNeededDescriptors();
        // NumUnorderedAccessViews = UnorderedAccessViewCache.CountNeededDescriptors();
        // NumResourceDescriptors  = NumConstantBuffersViews + NumShaderResourceViews + NumUnorderedAccessViews;
    }

    if (!SamplerDescriptors->HasSpace(NumSamplerDescriptors))
    {
        Check(false);

        // SamplerStateCache.InvalidateAll();
        // SamplerDescriptors->AllocateFreshHeap();

        // NumSamplerDescriptors = SamplerStateCache.CountNeededDescriptors();
    }

    // Allocate handles
    uint32 ResourceDescriptorHandle = ResourceDescriptors->AllocateHandles(NumResourceDescriptors);
    D3D12_ERROR_COND( NumResourceDescriptors <= D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT
                    , "Trying to bind more Resource Descriptors (NumDescriptors=%u) than the maximum (MaxResourceDescriptors=%u)"
                    , NumResourceDescriptors
                    , D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT);

    uint32 SamplerDescriptorHandle = SamplerDescriptors->AllocateHandles(NumSamplerDescriptors);
    D3D12_ERROR_COND( NumSamplerDescriptors <= D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT
                    , "Trying to bind more Sampler Descriptors (NumDescriptors=%u) than the maximum (MaxSamplerDescriptors=%u)"
                    , NumSamplerDescriptors
                    , D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT);

    // Bind the heaps
    ID3D12DescriptorHeap* DescriptorHeaps[] =
    {
        ResourceDescriptors->GetD3D12Heap(),
        SamplerDescriptors->GetD3D12Heap()
    };

    if (PreviousDescriptorHeaps[0] != DescriptorHeaps[0] || PreviousDescriptorHeaps[1] != DescriptorHeaps[1])
    {
        CmdList->SetDescriptorHeaps(ArrayCount(DescriptorHeaps), DescriptorHeaps);

        PreviousDescriptorHeaps[0] = DescriptorHeaps[0];
        PreviousDescriptorHeaps[1] = DescriptorHeaps[1];
    }

    // Copy over the descriptors to the descriptor tables
    for (uint32 ShaderStage = 0; ShaderStage < ShaderVisibility_Count; ++ShaderStage)
    {
        const EShaderVisibility ShaderVisibility = static_cast<EShaderVisibility>(ShaderStage);
        if (PipelineMask.CheckShaderVisibility(ShaderVisibility))
        {
            // ConstantBuffers
            uint32 NumDescriptors = ConstantBufferViewCache.GetNumDescriptorsForStage(ShaderVisibility);
            if (NumDescriptors > 0)
            {
                ConstantBufferViewCache.PrepareDescriptorsForCopy(ShaderVisibility);
                
                const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = ResourceDescriptors->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
                const D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = ResourceDescriptors->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);

                ConstantBufferViewCache.SetDescriptorTable(ShaderVisibility, CPUHandle, GPUHandle);
                ResourceDescriptorHandle += NumDescriptors;
            }

            // ShaderResourceViews
            NumDescriptors = ShaderResourceViewCache.GetNumDescriptorsForStage(ShaderVisibility);
            if (NumDescriptors > 0)
            {
                ShaderResourceViewCache.PrepareDescriptorsForCopy(ShaderVisibility);
                
                const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = ResourceDescriptors->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
                const D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = ResourceDescriptors->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);

                ShaderResourceViewCache.SetDescriptorTable(ShaderVisibility, CPUHandle, GPUHandle);
                ResourceDescriptorHandle += NumDescriptors;
            }

            // UnorderedAccessViews
            NumDescriptors = UnorderedAccessViewCache.GetNumDescriptorsForStage(ShaderVisibility);
            if (NumDescriptors > 0)
            {
                UnorderedAccessViewCache.PrepareDescriptorsForCopy(ShaderVisibility);
                
                const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = ResourceDescriptors->GetCPUDescriptorHandleAt(ResourceDescriptorHandle);
                const D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = ResourceDescriptors->GetGPUDescriptorHandleAt(ResourceDescriptorHandle);
                UnorderedAccessViewCache.SetDescriptorTable(ShaderVisibility, CPUHandle, GPUHandle);
            }

            // SamplerStates
            NumDescriptors = SamplerStateCache.GetNumDescriptorsForStage(ShaderVisibility);
            if (NumDescriptors > 0)
            {
                SamplerStateCache.PrepareDescriptorsForCopy(ShaderVisibility);
                
                const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = SamplerDescriptors->GetCPUDescriptorHandleAt(SamplerDescriptorHandle);
                const D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = SamplerDescriptors->GetGPUDescriptorHandleAt(SamplerDescriptorHandle);
                SamplerStateCache.SetDescriptorTable(ShaderVisibility, CPUHandle, GPUHandle);
            }
        }
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12VertexBufferCache

void FD3D12VertexBufferCache::CommitState(FD3D12CommandList& CmdList, FD3D12CommandBatch* CmdBatch)
{
    ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();
    if (bVertexBuffersDirty)
    {
        TStaticArray<D3D12_VERTEX_BUFFER_VIEW, D3D12_MAX_VERTEX_BUFFER_SLOTS> VertexBufferViews;

        for (uint32 Index = 0; Index < NumVertexBuffers; ++Index)
        {
            FD3D12VertexBuffer* VertexBuffer = VertexBuffers[Index];
            if (VertexBuffer)
            {
                VertexBufferViews[Index] = VertexBuffer->GetView();
                CmdBatch->AddInUseResource(VertexBuffer);
            }
            else
            {
                FMemory::Memzero(&VertexBufferViews[Index]);
            }
        }

        DxCmdList->IASetVertexBuffers(0, NumVertexBuffers, VertexBufferViews.Data());
        bVertexBuffersDirty = false;
    }

    if (bIndexBufferDirty)
    {
        D3D12_INDEX_BUFFER_VIEW IndexBufferView;
        if (!IndexBuffer)
        {
            IndexBufferView.Format         = DXGI_FORMAT_R32_UINT;
            IndexBufferView.BufferLocation = 0;
            IndexBufferView.SizeInBytes    = 0;
        }
        else
        {
            IndexBufferView = IndexBuffer->GetView();
            CmdBatch->AddInUseResource(IndexBuffer);
        }

        DxCmdList->IASetIndexBuffer(&IndexBufferView);
        bIndexBufferDirty = false;
    }
}

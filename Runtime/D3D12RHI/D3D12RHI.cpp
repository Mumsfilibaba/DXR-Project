#include "Core/Misc/ConsoleManager.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Threading/ScopedLock.h"
#include "CoreApplication/Windows/WindowsWindow.h"
#include "D3D12RHI/D3D12CommandList.h"
#include "D3D12RHI/D3D12Fence.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12RayTracing.h"
#include "D3D12RHI/D3D12PipelineState.h"
#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12Buffer.h"
#include "D3D12RHI/D3D12SamplerState.h"
#include "D3D12RHI/D3D12SwapChain.h"
#include "D3D12RHI/D3D12Query.h"
#include "D3D12RHI/D3D12Loader.h"
#include "D3D12RHI/D3D12ResidencyManager.h"
#include "D3D12RHI/D3D12Stats.h"
#include "RHI/RHIStats.h"

#include <dxgidebug.h>

IMPLEMENT_ENGINE_MODULE(FD3D12ModuleRHI, D3D12RHI);

static TAutoConsoleVariable<bool> CVarEnablePix(
    "D3D12RHI.EnablePIX",
    "Enables loading of PIX when creating device to capture frame's programmatically",
    false);

FD3D12DeviceRHI* FD3D12DeviceRHI::GD3D12DeviceRHI = nullptr;

FD3D12TextureRHI* FD3D12DeviceRHI::ResourceCast(FRHITexture* Texture)
{
    if (Texture)
    {
        return static_cast<FD3D12TextureBase*>(Texture)->GetTextureInterface();
    }

    return nullptr;
}

FD3D12RenderTargetViewRHI* FD3D12DeviceRHI::ResourceCast(FRHIRenderTargetView* RenderTargetView)
{
    if (RenderTargetView)
    {
        return static_cast<FD3D12RenderTargetViewBase*>(RenderTargetView)->GetRenderTargetViewInterface();
    }

    return nullptr;
}

FD3D12UnorderedAccessViewRHI* FD3D12DeviceRHI::ResourceCast(FRHIUnorderedAccessView* UnorderedAccessView)
{
    if (UnorderedAccessView)
    {
        return static_cast<FD3D12UnorderedAccessViewBase*>(UnorderedAccessView)->GetUnorderedAccessViewInterface();
    }

    return nullptr;
}

const FD3D12TextureRHI* FD3D12DeviceRHI::ResourceCast(const FRHITexture* Texture)
{
    if (Texture)
    {
        return static_cast<const FD3D12TextureBase*>(Texture)->GetTextureInterface();
    }

    return nullptr;
}

const FD3D12RenderTargetViewRHI* FD3D12DeviceRHI::ResourceCast(const FRHIRenderTargetView* RenderTargetView)
{
    if (RenderTargetView)
    {
        return static_cast<const FD3D12RenderTargetViewBase*>(RenderTargetView)->GetRenderTargetViewInterface();
    }

    return nullptr;
}

const FD3D12UnorderedAccessViewRHI* FD3D12DeviceRHI::ResourceCast(const FRHIUnorderedAccessView* UnorderedAccessView)
{
    if (UnorderedAccessView)
    {
        return static_cast<const FD3D12UnorderedAccessViewBase*>(UnorderedAccessView)->GetUnorderedAccessViewInterface();
    }

    return nullptr;
}

FRHIDevice* FD3D12ModuleRHI::CreateDevice()
{
    TUniquePtr<FD3D12DeviceRHI> NewRHI = MakeUniquePtr<FD3D12DeviceRHI>();
    if (!NewRHI->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewRHI.Release();
    }
}

ERHIType FD3D12DeviceRHI::GetRHIType() const
{
    return ERHIType::D3D12;
}

FD3D12DeviceRHI::FD3D12DeviceRHI()
    : FRHIDevice()
    , Device(nullptr)
    , DirectCommandContext(nullptr)
{
    if (!GD3D12DeviceRHI)
    {
        GD3D12DeviceRHI = this;
    }
}

FD3D12DeviceRHI::~FD3D12DeviceRHI()
{
    const auto FlushDeletionQueues = [this]()
    {
        // NOTE: Objects could contain other objects, that now need to be flushed
        if (FRHICommandListExecutor::IsInitialized())
        {
            FRHICommandListExecutor::Get().FlushDeletedResources();
        }

        while (!DeferredObjects.IsEmpty())
        {
            TArray<FD3D12DeferredObject> Items;
            {
                TScopedLock Lock(DeferredObjectsCS);
                Items = Move(DeferredObjects);
            }

            FD3D12DeferredObject::ProcessItems(Items);

            // NOTE: Objects could contain other objects, that now need to be flushed
            if (FRHICommandListExecutor::IsInitialized())
            {
                FRHICommandListExecutor::Get().FlushDeletedResources();
            }
        }
    };

    // Flush the default context before flushing the submission queue
    if (DirectCommandContext)
    {
        DirectCommandContext->Flush();
    }

    if (Device)
    {
        FD3D12Queue* DirectQueue = Device->GetQueue(ED3D12CommandQueueType::Direct);
        DirectQueue->ProcessCommandQueue();
    }

    // Flush any objects that might need the context...
    FlushDeletionQueues();

    // ...then delete the context
    SAFE_DELETE(DirectCommandContext);

    // Delete all samplers
    {
        TScopedLock Lock(SamplerStateMapCS);
        SamplerStateMap.Clear();
    }

    // ... Finally, delete all remaining resources
    FlushDeletionQueues();

    // Destroy the device and adapter
    SAFE_DELETE(Device);
    SAFE_DELETE(Adapter);

    D3D12Loader::Release();

    if (GD3D12DeviceRHI == this)
    {
        GD3D12DeviceRHI = nullptr;
    }
}

bool FD3D12DeviceRHI::Initialize()
{
    const bool bEnablePIX = CVarEnablePix.GetValue();

    // Load Library and Function-Pointers etc.
    const bool bResult = D3D12Loader::Initialize(bEnablePIX);
    if (!bResult)
    {
        return false;
    }

    Adapter = new FD3D12Adapter();
    if (!Adapter->Initialize())
    {
        return false;
    }

    Device = new FD3D12Device(Adapter);
    if (!Device->Initialize())
    {
        return false;
    }

    // Initialize context
    DirectCommandContext = new FD3D12CommandContext(GetDevice(), ED3D12CommandQueueType::Direct);
    if (!(DirectCommandContext && DirectCommandContext->Initialize()))
    {
        return false;
    }

    // Ensure that we have initialized the device feature support
    if (!InitializeDeviceFeatureSupport())
    {
        return false;
    }

    return true;
}

void FD3D12DeviceRHI::BeginFrame()
{
    BeginFrame(nullptr);
}

void FD3D12DeviceRHI::BeginFrame(FD3D12CommandContext* InCommandContext)
{
    if (!Device)
    {
        return;
    }

    Device->GetQueue(ED3D12CommandQueueType::Direct)->ProcessCommandQueue();
    Device->BeginFrame(InCommandContext);

    if (FD3D12ResidencyManager* ResidencyManager = Device->GetResidencyManager())
    {
        ResidencyManager->Tick();
    }
}

void FD3D12DeviceRHI::EndFrame()
{
    if (!Device)
    {
        return;
    }

    if (FD3D12ResidencyManager* ResidencyManager = Device->GetResidencyManager())
    {
        ResidencyManager->EvictIfNeeded();
    }

    Device->GetPipelineStateManager().SaveCacheDataAsync();

#if D3D12_ENABLE_STATS
    if (FD3D12BufferAllocator* BufferAlloc = Device->GetBufferAllocator())
    {
        BufferAlloc->UpdateMemoryStats();
    }
    if (FD3D12TextureAllocator* TextureAlloc = Device->GetTextureAllocator())
    {
        TextureAlloc->UpdateMemoryStats();
    }
    if (FD3D12UploadHeapAllocator* UploadAlloc = Device->GetUploadHeapAllocator())
    {
        UploadAlloc->UpdateMemoryStats();
    }

    {
        FRHIVideoMemoryInfo LocalMemory;
        if (QueryVideoMemoryInfo(EVideoMemoryType::Local, LocalMemory))
        {
            STAT_SET(STAT_RHI_LocalMemoryBudget, LocalMemory.MemoryBudget);
            STAT_SET(STAT_RHI_LocalMemoryUsage,  LocalMemory.MemoryUsage);
        }

        FRHIVideoMemoryInfo NonLocalMemory;
        if (QueryVideoMemoryInfo(EVideoMemoryType::NonLocal, NonLocalMemory))
        {
            STAT_SET(STAT_RHI_NonLocalMemoryBudget, NonLocalMemory.MemoryBudget);
            STAT_SET(STAT_RHI_NonLocalMemoryUsage,  NonLocalMemory.MemoryUsage);
        }
    }
#endif
}

bool FD3D12DeviceRHI::InitializeDeviceFeatureSupport()
{
    // -------------------------------------------------------------------------------------------
    // Baseline Defaults
    // -------------------------------------------------------------------------------------------

    RHI::DefaultSwapChainFormat = GetD3D12DefaultBackBufferFormat();

    RHI::bSupportsGeometryShaders                       = true; // Geometry Shaders are always supported
    RHI::bSupportRenderTargetArrayIndexFromVertexShader = false;

    RHI::bSupportsViewInstancing     = false;
    RHI::MaxViewInstanceCount        = 1;

    RHI::bSupportsRayTracing         = false;
    RHI::RayTracingTier              = ERayTracingTier::NotSupported;
    RHI::RayTracingMaxRecursionDepth = 0;

    RHI::bSupportsVRS                = false;
    RHI::ShadingRateTier             = EShadingRateTier::NotSupported;
    RHI::ShadingRateImageTileSize    = 0;

    RHI::bSupportDrawIndirect        = true;
    RHI::bSupportMultiDrawIndirect   = true;
    RHI::MaxDrawIndirectCount        = uint32(~0u);

    // -------------------------------------------------------------------------------------------
    // Texture / image limits (canonical D3D12 defines)
    // -------------------------------------------------------------------------------------------

    RHI::MaxTexture1DSize        = D3D12_REQ_TEXTURE1D_U_DIMENSION;
    RHI::MaxTexture1DArrayLayers = D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
    RHI::MaxTexture2DSize        = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    RHI::MaxTexture2DArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    RHI::MaxTexture3DWidth       = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    RHI::MaxTexture3DHeight      = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    RHI::MaxTexture3DDepth       = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    RHI::MaxCubeTextureSize      = D3D12_REQ_TEXTURECUBE_DIMENSION;
    RHI::MaxCubeArrayCount       = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION / RHI_NUM_CUBE_FACES;

    // -------------------------------------------------------------------------------------------
    // Buffer / memory limits
    // -------------------------------------------------------------------------------------------

    RHI::MaxBufferSize              = uint64(~0ull);
    RHI::MaxConstantBufferSize      = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
    RHI::MaxStorageBufferSize       = uint64(~0ull);
    RHI::StructuredBufferMinStride  = 0;
    RHI::StructuredBufferMaxStride  = uint32(~0u);
    RHI::RawBufferRequiredAlignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;

    // -------------------------------------------------------------------------------------------
    // SV_RenderTargetArrayIndex from VS
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS Features = {};
        if (SUCCEEDED(GetDevice()->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &Features, sizeof(Features))))
        {
            RHI::bSupportRenderTargetArrayIndexFromVertexShader = 
                !!Features.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
        }
    }

    // -------------------------------------------------------------------------------------------
    // Ray Tracing (DXR)
    // -------------------------------------------------------------------------------------------

    if (GD3D12RayTracingTier >= D3D12_RAYTRACING_TIER_1_0)
    {
        RHI::bSupportsRayTracing         = true;
        RHI::RayTracingMaxRecursionDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;

        if (GD3D12RayTracingTier == D3D12_RAYTRACING_TIER_1_1)
        {
            RHI::RayTracingTier = ERayTracingTier::Tier1_1;
        }
        else
        {
            RHI::RayTracingTier = ERayTracingTier::Tier1;
        }
    }
    else
    {
        RHI::bSupportsRayTracing         = false;
        RHI::RayTracingMaxRecursionDepth = 0;
        RHI::RayTracingTier              = ERayTracingTier::NotSupported;
    }

    // -------------------------------------------------------------------------------------------
    // View Instancing
    // -------------------------------------------------------------------------------------------

    if (GD3D12ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED)
    {
        RHI::bSupportsViewInstancing = true;
        RHI::MaxViewInstanceCount    = D3D12_MAX_VIEW_INSTANCE_COUNT;
    }
    else
    {
        RHI::bSupportsViewInstancing = false;
        RHI::MaxViewInstanceCount    = 1;
    }

    // -------------------------------------------------------------------------------------------
    // Variable Rate Shading (VRS)
    // -------------------------------------------------------------------------------------------

    switch (GD3D12VariableRateShadingTier)
    {
    default:
    case D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED:
        RHI::ShadingRateTier          = EShadingRateTier::NotSupported;
        RHI::bSupportsVRS             = false;
        RHI::ShadingRateImageTileSize = 0;
        break;

    case D3D12_VARIABLE_SHADING_RATE_TIER_1:
        RHI::ShadingRateTier = EShadingRateTier::Tier1;
        RHI::bSupportsVRS    = true;
        break;

    case D3D12_VARIABLE_SHADING_RATE_TIER_2:
        RHI::ShadingRateTier = EShadingRateTier::Tier2;
        RHI::bSupportsVRS    = true;
        break;
    }

    if (RHI::bSupportsVRS)
    {
        // Tile size is not cached
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 Features6 = {};
        if (SUCCEEDED(GetDevice()->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &Features6, sizeof(Features6))))
        {
            RHI::ShadingRateImageTileSize = Features6.ShadingRateImageTileSize;
        }
        else
        {
            RHI::ShadingRateImageTileSize = 0;
        }
    }

    RHI::bSupportsDynamicDepthBias           = GD3D12SupportDynamicDepthBias;
    RHI::bSupportsStreamOutput               = true;
    RHI::bSupportsTimestampQueries           = true;
    RHI::bSupportsPipelineStatisticsQueries  = true;
    RHI::bSupportsGPUTimestampBubblesRemoval = true;

    return true;
}


FRHITexture* FD3D12DeviceRHI::CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FD3D12TextureRHIRef NewTexture = new FD3D12TextureRHI(GetDevice(), InTextureDesc);
    if (!NewTexture->Initialize(DirectCommandContext, InInitialState, InInitialData))
    {
        return nullptr;
    }

#if D3D12_ENABLE_STATS
    {
        const int64 AllocatedSize = static_cast<int64>(NewTexture->GetResourceStorage().GetSize());
        if (InTextureDesc.IsRenderTarget() || InTextureDesc.IsDepthStencil())
        {
            STAT_ADD(STAT_RHI_RenderTargetMemory, AllocatedSize);
        }
        else
        {
            STAT_ADD(STAT_RHI_TextureMemory, AllocatedSize);
        }
    }
#endif

    TickCoreProgression();
    return NewTexture.ReleaseOwnership();
}

FRHIBuffer* FD3D12DeviceRHI::CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState, const void* InInitialData)
{
    FD3D12BufferRHIRef NewBuffer = new FD3D12BufferRHI(GetDevice(), InBufferDesc);
    if (!NewBuffer->Initialize(DirectCommandContext, InInitialState, InInitialData))
    {
        return nullptr;
    }

#if D3D12_ENABLE_STATS
    {
        const int64 AllocatedSize = static_cast<int64>(NewBuffer->GetResourceStorage().GetSize());
        if (InBufferDesc.IsVertexBuffer())
        {
            STAT_ADD(STAT_RHI_VertexBufferMemory, AllocatedSize);
        }
        else if (InBufferDesc.IsIndexBuffer())
        {
            STAT_ADD(STAT_RHI_IndexBufferMemory, AllocatedSize);
        }
        else if (InBufferDesc.IsConstantBuffer())
        {
            STAT_ADD(STAT_RHI_ConstantBufferMemory, AllocatedSize);
        }
        else if (InBufferDesc.IsShaderResourceBuffer() || InBufferDesc.IsUnorderedAccessBuffer())
        {
            STAT_ADD(STAT_RHI_StructuredBufferMemory, AllocatedSize);
        }
        else
        {
            STAT_ADD(STAT_RHI_MiscBufferMemory, AllocatedSize);
        }

        if (InBufferDesc.IsReadBack())
        {
            STAT_ADD(STAT_RHI_ReadbackMemory, AllocatedSize);
        }
        if (InBufferDesc.IsDynamic() || InBufferDesc.IsTransient())
        {
            STAT_ADD(STAT_RHI_UploadMemory, AllocatedSize);
        }
    }
#endif

    TickCoreProgression();
    return NewBuffer.ReleaseOwnership();
}

FRHISamplerState* FD3D12DeviceRHI::CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc)
{
    TScopedLock Lock(SamplerStateMapCS);

    FD3D12SamplerStateRHIRef Result;

    // Check if there already is an existing sampler state with this description
    if (FD3D12SamplerStateRHIRef* ExistingSamplerState = SamplerStateMap.Find(InSamplerDesc))
    {
        Result = *ExistingSamplerState;
    }
    else
    {
        D3D12_SAMPLER_DESC Desc = {};
        Desc.AddressU       = ConvertSamplerMode(InSamplerDesc.AddressU);
        Desc.AddressV       = ConvertSamplerMode(InSamplerDesc.AddressV);
        Desc.AddressW       = ConvertSamplerMode(InSamplerDesc.AddressW);
        Desc.ComparisonFunc = ConvertComparisonFunc(InSamplerDesc.ComparisonFunc);
        Desc.Filter         = ConvertSamplerFilter(InSamplerDesc.Filter);
        Desc.MaxAnisotropy  = InSamplerDesc.MaxAnisotropy;
        Desc.MaxLOD         = InSamplerDesc.MaxLOD;
        Desc.MinLOD         = InSamplerDesc.MinLOD;
        Desc.MipLODBias     = InSamplerDesc.MipLODBias;
        
        Memory::Memcpy(Desc.BorderColor, InSamplerDesc.BorderColor.RGBA, sizeof(Desc.BorderColor));

        Result = new FD3D12SamplerStateRHI(GetDevice(), GetDevice()->GetSamplerOfflineDescriptorHeap(), InSamplerDesc);
        if (!Result->CreateSampler(Desc))
        {
            return nullptr;
        }
        else
        {
            SamplerStateMap.Add(InSamplerDesc, Result);
        }
    }

    return Result.ReleaseOwnership();
}

FRHISceneAccelerationStructure* FD3D12DeviceRHI::CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc)
{
    FRHISceneAccelerationStructureBuildDesc BuildDesc;
    BuildDesc.Instances    = InSceneDesc.Instances.Data();
    BuildDesc.NumInstances = InSceneDesc.Instances.Size();
    BuildDesc.bUpdate      = false;

    DirectCommandContext->StartContext();

    FD3D12SceneAccelerationStructureRHIRef D3D12Scene = new FD3D12SceneAccelerationStructureRHI(GetDevice(), InSceneDesc);
    if (!D3D12Scene->Build(*DirectCommandContext, BuildDesc))
    {
        DEBUG_BREAK();
        D3D12Scene.Reset();
    }

    DirectCommandContext->FinishContext();

    TickCoreProgression();
    return D3D12Scene.ReleaseOwnership();
}

FRHIGeometryAccelerationStructure* FD3D12DeviceRHI::CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
{
    FRHIGeometryAccelerationStructureBuildDesc BuildDesc;
    BuildDesc.VertexBuffer = InGeometryDesc.VertexBuffer;
    BuildDesc.NumVertices  = InGeometryDesc.NumVertices;
    BuildDesc.IndexBuffer  = InGeometryDesc.IndexBuffer;
    BuildDesc.NumIndices   = InGeometryDesc.NumIndices;
    BuildDesc.IndexFormat  = InGeometryDesc.IndexFormat;
    BuildDesc.bUpdate      = false;

    DirectCommandContext->StartContext();

    FD3D12GeometryAccelerationStructureRHIRef D3D12Geometry = new FD3D12GeometryAccelerationStructureRHI(GetDevice(), InGeometryDesc);
    if (!D3D12Geometry->Build(*DirectCommandContext, BuildDesc))
    {
        DEBUG_BREAK();
        D3D12Geometry.Reset();
    }

    DirectCommandContext->FinishContext();

    TickCoreProgression();
    return D3D12Geometry.ReleaseOwnership();
}

FRHIShaderResourceView* FD3D12DeviceRHI::CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc)
{
    if (!InResource)
    {
        D3D12_ERROR("CreateShaderResourceView requires a non-null resource");
        return nullptr;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC Desc = {};
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    FRHIResource*   Resource      = nullptr;
    FD3D12Resource* D3D12Resource = nullptr;

    if (InDesc.IsBufferSRV())
    {
        D3D12_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Buffer,
            "CreateShaderResourceView: buffer view requires an FRHIBuffer resource");
            
        FD3D12BufferRHI* D3D12Buffer = FD3D12DeviceRHI::ResourceCast(static_cast<FRHIBuffer*>(InResource));
        CHECK(D3D12Buffer != nullptr);

        Resource      = D3D12Buffer;
        D3D12Resource = D3D12Buffer->GetResource();

        const auto& BufferDesc   = InDesc.Buffer;
        Desc.ViewDimension       = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = BufferDesc.FirstElement;
        Desc.Buffer.NumElements  = BufferDesc.NumElements;

        if (BufferDesc.Type != EBufferViewType::ByteAddress)
        {
            Desc.Format                     = DXGI_FORMAT_UNKNOWN;
            Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
            Desc.Buffer.StructureByteStride = D3D12Buffer->GetDesc().Stride;
        }
        else
        {
            Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
            Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;
            Desc.Buffer.StructureByteStride = 0;
        }
    }
    else if (InDesc.IsTextureSRV())
    {
        D3D12_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Texture,
            "CreateShaderResourceView: texture view requires an FRHITexture resource");

        FD3D12TextureRHI* D3D12Texture = FD3D12DeviceRHI::ResourceCast(static_cast<FRHITexture*>(InResource));
        CHECK(D3D12Texture != nullptr);
        CHECK(IsViewDimensionCompatible(D3D12Texture->GetDesc().Dimension, InDesc.ViewDimension));

        Resource      = D3D12Texture;
        D3D12Resource = D3D12Texture->GetResource();

        const bool bIsMultisampled = D3D12Texture->GetDesc().IsMultisampled();
        switch (InDesc.ViewDimension)
        {
            case EViewDimension::Texture1D:
            {
                const auto& TextureDesc            = InDesc.Texture1D;
                Desc.Format                        = D3D12CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE1D;
                Desc.Texture1D.MostDetailedMip     = TextureDesc.FirstMipLevel;
                Desc.Texture1D.MipLevels           = TextureDesc.NumMips;
                Desc.Texture1D.ResourceMinLODClamp = TextureDesc.MinLODClamp;
                break;
            }

            case EViewDimension::Texture1DArray:
            {
                const auto& TextureDesc                 = InDesc.Texture1DArray;
                Desc.Format                             = D3D12CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                Desc.Texture1DArray.MostDetailedMip     = TextureDesc.FirstMipLevel;
                Desc.Texture1DArray.MipLevels           = TextureDesc.NumMips;
                Desc.Texture1DArray.ResourceMinLODClamp = TextureDesc.MinLODClamp;
                Desc.Texture1DArray.FirstArraySlice     = TextureDesc.FirstArraySlice;
                Desc.Texture1DArray.ArraySize           = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                break;
            }

            case EViewDimension::Texture2D:
            {
                const auto& TextureDesc = InDesc.Texture2D;
                Desc.Format = D3D12CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));

                if (!bIsMultisampled)
                {
                    Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
                    Desc.Texture2D.MostDetailedMip     = TextureDesc.FirstMipLevel;
                    Desc.Texture2D.MipLevels           = TextureDesc.NumMips;
                    Desc.Texture2D.ResourceMinLODClamp = TextureDesc.MinLODClamp;
                    Desc.Texture2D.PlaneSlice          = TextureDesc.PlaneSlice;
                }
                else
                {
                    Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                }

                break;
            }

            case EViewDimension::Texture2DArray:
            {
                const auto& TextureDesc = InDesc.Texture2DArray;
                Desc.Format = D3D12CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));

                if (!bIsMultisampled)
                {
                    Desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    Desc.Texture2DArray.MostDetailedMip     = TextureDesc.FirstMipLevel;
                    Desc.Texture2DArray.MipLevels           = TextureDesc.NumMips;
                    Desc.Texture2DArray.ResourceMinLODClamp = TextureDesc.MinLODClamp;
                    Desc.Texture2DArray.FirstArraySlice     = TextureDesc.FirstArraySlice;
                    Desc.Texture2DArray.ArraySize           = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                    Desc.Texture2DArray.PlaneSlice          = TextureDesc.PlaneSlice;
                }
                else
                {
                    Desc.ViewDimension                    = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    Desc.Texture2DMSArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                    Desc.Texture2DMSArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                }

                break;
            }

            case EViewDimension::TextureCube:
            {
                const auto& TextureDesc              = InDesc.TextureCube;
                Desc.Format                          = D3D12CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
                Desc.TextureCube.MostDetailedMip     = TextureDesc.FirstMipLevel;
                Desc.TextureCube.MipLevels           = TextureDesc.NumMips;
                Desc.TextureCube.ResourceMinLODClamp = TextureDesc.MinLODClamp;
                break;
            }

            case EViewDimension::TextureCubeArray:
            {
                const auto& TextureDesc                   = InDesc.TextureCubeArray;
                Desc.Format                               = D3D12CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                Desc.TextureCubeArray.MostDetailedMip     = TextureDesc.FirstMipLevel;
                Desc.TextureCubeArray.MipLevels           = TextureDesc.NumMips;
                Desc.TextureCubeArray.ResourceMinLODClamp = TextureDesc.MinLODClamp;
                Desc.TextureCubeArray.First2DArrayFace    = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, TextureDesc.FirstCube);
                Desc.TextureCubeArray.NumCubes            = Math::Max<uint16>(TextureDesc.NumCubes, 1u);
                break;
            }

            case EViewDimension::Texture3D:
            {
                const auto& TextureDesc            = InDesc.Texture3D;
                Desc.Format                        = D3D12CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
                Desc.Texture3D.MostDetailedMip     = TextureDesc.FirstMipLevel;
                Desc.Texture3D.MipLevels           = TextureDesc.NumMips;
                Desc.Texture3D.ResourceMinLODClamp = TextureDesc.MinLODClamp;
                break;
            }

            default:
            {
                D3D12_ERROR("CreateShaderResourceView: unsupported texture ViewDimension");
                return nullptr;
            }
        }
    }
    else if (InDesc.IsAccelerationStructureSRV())
    {
        D3D12_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::SceneAccelerationStructure,
            "CreateShaderResourceView: AccelerationStructure view requires an FRHISceneAccelerationStructure resource");

        FD3D12SceneAccelerationStructureRHI* D3D12Scene = FD3D12DeviceRHI::ResourceCast(static_cast<FRHISceneAccelerationStructure*>(InResource));
        CHECK(D3D12Scene != nullptr);

        Resource      = D3D12Scene;
        D3D12Resource = nullptr;

        Desc.ViewDimension                            = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        Desc.RaytracingAccelerationStructure.Location = D3D12Scene->GetGPUVirtualAddress();
    }
    else
    {
        return nullptr;
    }

    CHECK(D3D12Resource != nullptr || InDesc.IsAccelerationStructureSRV());

    FD3D12ShaderResourceViewRHIRef D3D12View = new FD3D12ShaderResourceViewRHI(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), Resource, InDesc);
    if (!D3D12View->Initialize(D3D12Resource, Desc))
    {
        return nullptr;
    }

    return D3D12View.ReleaseOwnership();
}

FRHIUnorderedAccessView* FD3D12DeviceRHI::CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
{
    if (!InResource)
    {
        D3D12_ERROR("CreateUnorderedAccessView requires a non-null resource");
        return nullptr;
    }

    FRHIResource*   Resource      = nullptr;
    FD3D12Resource* D3D12Resource = nullptr;

    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc = {};
    if (InDesc.IsBufferUAV())
    {
        D3D12_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Buffer,
            "CreateUnorderedAccessView: buffer view requires an FRHIBuffer resource");

        FD3D12BufferRHI* D3D12Buffer = FD3D12DeviceRHI::ResourceCast(static_cast<FRHIBuffer*>(InResource));
        CHECK(D3D12Buffer != nullptr);

        Resource      = D3D12Buffer;
        D3D12Resource = D3D12Buffer->GetResource();

        const auto& BufferDesc   = InDesc.Buffer;
        Desc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = BufferDesc.FirstElement;
        Desc.Buffer.NumElements  = BufferDesc.NumElements;

        if (BufferDesc.Type != EBufferViewType::ByteAddress)
        {
            Desc.Format                     = DXGI_FORMAT_UNKNOWN;
            Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
            Desc.Buffer.StructureByteStride = D3D12Buffer->GetDesc().Stride;
        }
        else
        {
            Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
            Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_RAW;
            Desc.Buffer.StructureByteStride = 0;
        }
    }
    else if (InDesc.IsTextureUAV())
    {
        D3D12_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Texture,
            "CreateUnorderedAccessView: texture view requires an FRHITexture resource");

        FD3D12TextureRHI* D3D12Texture = FD3D12DeviceRHI::ResourceCast(static_cast<FRHITexture*>(InResource));
        CHECK(D3D12Texture != nullptr);
        CHECK(IsViewDimensionCompatible(D3D12Texture->GetDesc().Dimension, InDesc.ViewDimension));

        Resource      = D3D12Texture;
        D3D12Resource = D3D12Texture->GetResource();

        switch (InDesc.ViewDimension)
        {
            case EViewDimension::Texture1D:
            {
                const auto& TextureDesc = InDesc.Texture1D;
                Desc.Format             = ConvertFormat(TextureDesc.Format);
                Desc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
                Desc.Texture1D.MipSlice = TextureDesc.MipLevel;
                break;
            }

            case EViewDimension::Texture1DArray:
            {
                const auto& TextureDesc             = InDesc.Texture1DArray;
                Desc.Format                         = ConvertFormat(TextureDesc.Format);
                Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                Desc.Texture1DArray.MipSlice        = TextureDesc.MipLevel;
                Desc.Texture1DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                Desc.Texture1DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                break;
            }

            case EViewDimension::Texture2D:
            {
                const auto& TextureDesc   = InDesc.Texture2D;
                Desc.Format               = ConvertFormat(TextureDesc.Format);
                Desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
                Desc.Texture2D.MipSlice   = TextureDesc.MipLevel;
                Desc.Texture2D.PlaneSlice = TextureDesc.PlaneSlice;
                break;
            }

            case EViewDimension::Texture2DArray:
            {
                const auto& TextureDesc             = InDesc.Texture2DArray;
                Desc.Format                         = ConvertFormat(TextureDesc.Format);
                Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                Desc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                Desc.Texture2DArray.PlaneSlice      = TextureDesc.PlaneSlice;
                Desc.Texture2DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                Desc.Texture2DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                break;
            }

            case EViewDimension::TextureCube:
            {
                const auto& TextureDesc             = InDesc.TextureCube;
                Desc.Format                         = ConvertFormat(TextureDesc.Format);
                Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                Desc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                Desc.Texture2DArray.PlaneSlice      = 0;
                Desc.Texture2DArray.FirstArraySlice = 0;
                Desc.Texture2DArray.ArraySize       = RHI_NUM_CUBE_FACES;
                break;
            }

            case EViewDimension::TextureCubeArray:
            {
                const auto& TextureDesc             = InDesc.TextureCubeArray;
                Desc.Format                         = ConvertFormat(TextureDesc.Format);
                Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                Desc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                Desc.Texture2DArray.PlaneSlice      = 0;
                Desc.Texture2DArray.FirstArraySlice = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, TextureDesc.FirstCube);
                Desc.Texture2DArray.ArraySize       = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, Math::Max<uint16>(TextureDesc.NumCubes, 1u));
                break;
            }

            case EViewDimension::Texture3D:
            {
                const auto& TextureDesc    = InDesc.Texture3D;
                Desc.Format                = ConvertFormat(TextureDesc.Format);
                Desc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
                Desc.Texture3D.MipSlice    = TextureDesc.MipLevel;
                Desc.Texture3D.FirstWSlice = TextureDesc.FirstWSlice;
                Desc.Texture3D.WSize       = Math::Max<uint16>(TextureDesc.WSize, 1u);
                break;
            }

            default:
            {
                D3D12_ERROR("CreateUnorderedAccessView: unsupported texture ViewDimension");
                return nullptr;
            }
        }
    }
    else
    {
        return nullptr;
    }

    const D3D12_RESOURCE_DESC& ResourceDesc = D3D12Resource->GetDesc();
    if ((ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == D3D12_RESOURCE_FLAG_NONE)
    {
        String DebugName;
        D3D12Resource->GetDebugName(DebugName);
        D3D12_ERROR("Resource '%s' does not allow UnorderedAccessViews", *DebugName);
        return nullptr;
    }

    FD3D12UnorderedAccessViewRHIRef D3D12View = new FD3D12UnorderedAccessViewRHI(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), Resource, InDesc);
    if (!D3D12View->Initialize(nullptr, D3D12Resource, Desc))
    {
        return nullptr;
    }

    return D3D12View.ReleaseOwnership();
}

FRHIRenderTargetView* FD3D12DeviceRHI::CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc)
{
    if (!InResource)
    {
        D3D12_WARNING("Cannot create RenderTargetView without a valid resource");
        return nullptr;
    }

    D3D12_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Texture,
        "CreateRenderTargetView: requires an FRHITexture resource");

    FD3D12TextureRHI* D3D12Texture = FD3D12DeviceRHI::ResourceCast(static_cast<FRHITexture*>(InResource));
    if (!D3D12Texture)
    {
        D3D12_WARNING("Cannot create RenderTargetView without a valid texture");
        return nullptr;
    }
    CHECK(IsViewDimensionCompatible(D3D12Texture->GetDesc().Dimension, InDesc.ViewDimension));

    FD3D12Resource* D3D12Resource = D3D12Texture->GetResource();
    if (!D3D12Resource)
    {
        D3D12_WARNING("Texture does not have a valid D3D12Resource");
        return nullptr;
    }

    const D3D12_RESOURCE_DESC& ResourceDesc = D3D12Resource->GetDesc();
    if ((ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == D3D12_RESOURCE_FLAG_NONE)
    {
        String DebugName;
        D3D12Resource->GetDebugName(DebugName);
        D3D12_ERROR("Texture '%s' does not allow RenderTargetViews", *DebugName);
        return nullptr;
    }

    const bool bIsMultisampled = D3D12Texture->GetDesc().IsMultisampled();

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
        {
            const auto& TextureDesc    = InDesc.Texture1D;
            RTVDesc.Format             = ConvertFormat(TextureDesc.Format);
            RTVDesc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE1D;
            RTVDesc.Texture1D.MipSlice = TextureDesc.MipLevel;
            break;
        }

        case EViewDimension::Texture1DArray:
        {
            const auto& TextureDesc                = InDesc.Texture1DArray;
            RTVDesc.Format                         = ConvertFormat(TextureDesc.Format);
            RTVDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            RTVDesc.Texture1DArray.MipSlice        = TextureDesc.MipLevel;
            RTVDesc.Texture1DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
            RTVDesc.Texture1DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            break;
        }

        case EViewDimension::Texture2D:
        {
            const auto& TextureDesc = InDesc.Texture2D;
            RTVDesc.Format = ConvertFormat(TextureDesc.Format);

            if (!bIsMultisampled)
            {
                RTVDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
                RTVDesc.Texture2D.MipSlice   = TextureDesc.MipLevel;
                RTVDesc.Texture2D.PlaneSlice = TextureDesc.PlaneSlice;
            }
            else
            {
                RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            }

            break;
        }

        case EViewDimension::Texture2DArray:
        {
            const auto& TextureDesc = InDesc.Texture2DArray;
            RTVDesc.Format = ConvertFormat(TextureDesc.Format);

            if (!bIsMultisampled)
            {
                RTVDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                RTVDesc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                RTVDesc.Texture2DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                RTVDesc.Texture2DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                RTVDesc.Texture2DArray.PlaneSlice      = TextureDesc.PlaneSlice;
            }
            else
            {
                RTVDesc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                RTVDesc.Texture2DMSArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                RTVDesc.Texture2DMSArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            }

            break;
        }

        case EViewDimension::TextureCube:
        {
            const auto& TextureDesc = InDesc.TextureCube;
            RTVDesc.Format = ConvertFormat(TextureDesc.Format);

            if (!bIsMultisampled)
            {
                RTVDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                RTVDesc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                RTVDesc.Texture2DArray.FirstArraySlice = 0;
                RTVDesc.Texture2DArray.ArraySize       = RHI_NUM_CUBE_FACES;
                RTVDesc.Texture2DArray.PlaneSlice      = 0;
            }
            else
            {
                RTVDesc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                RTVDesc.Texture2DMSArray.FirstArraySlice = 0;
                RTVDesc.Texture2DMSArray.ArraySize       = RHI_NUM_CUBE_FACES;
            }

            break;
        }

        case EViewDimension::TextureCubeArray:
        {
            const auto& TextureDesc = InDesc.TextureCubeArray;
            RTVDesc.Format = ConvertFormat(TextureDesc.Format);

            const uint32 FirstLayer  = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, TextureDesc.FirstCube);
            const uint32 NumLayers   = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, Math::Max<uint16>(TextureDesc.NumCubes, 1u));

            if (!bIsMultisampled)
            {
                RTVDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                RTVDesc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                RTVDesc.Texture2DArray.FirstArraySlice = FirstLayer;
                RTVDesc.Texture2DArray.ArraySize       = NumLayers;
                RTVDesc.Texture2DArray.PlaneSlice      = 0;
            }
            else
            {
                RTVDesc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                RTVDesc.Texture2DMSArray.FirstArraySlice = FirstLayer;
                RTVDesc.Texture2DMSArray.ArraySize       = NumLayers;
            }

            break;
        }

        case EViewDimension::Texture3D:
        {
            const auto& TextureDesc       = InDesc.Texture3D;
            RTVDesc.Format                = ConvertFormat(TextureDesc.Format);
            RTVDesc.ViewDimension         = D3D12_RTV_DIMENSION_TEXTURE3D;
            RTVDesc.Texture3D.MipSlice    = TextureDesc.MipLevel;
            RTVDesc.Texture3D.FirstWSlice = TextureDesc.FirstWSlice;
            RTVDesc.Texture3D.WSize       = Math::Max<uint16>(TextureDesc.WSize, 1u);
            break;
        }

        default:
        {
            D3D12_ERROR("CreateRenderTargetView: unsupported ViewDimension");
            return nullptr;
        }
    }

    D3D12_ERROR_COND(RTVDesc.Format != DXGI_FORMAT_UNKNOWN, "Unallowed format for RenderTargetViews");

    FD3D12RenderTargetViewRHIRef D3D12View = new FD3D12RenderTargetViewRHI(GetDevice(), GetDevice()->GetRenderTargetOfflineDescriptorHeap(), D3D12Texture, InDesc);
    if (!D3D12View->Initialize(D3D12Resource, RTVDesc))
    {
        return nullptr;
    }

    D3D12View->RegisterWithResource(D3D12Texture);
    return D3D12View.ReleaseOwnership();
}

FRHIDepthStencilView* FD3D12DeviceRHI::CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc)
{
    if (!InResource)
    {
        D3D12_WARNING("Cannot create DepthStencilView without a valid resource");
        return nullptr;
    }

    D3D12_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Texture,
        "CreateDepthStencilView: requires an FRHITexture resource");

    FD3D12TextureRHI* D3D12Texture = FD3D12DeviceRHI::ResourceCast(static_cast<FRHITexture*>(InResource));
    if (!D3D12Texture)
    {
        D3D12_WARNING("Cannot create DepthStencilView without a valid texture");
        return nullptr;
    }
    CHECK(IsViewDimensionCompatible(D3D12Texture->GetDesc().Dimension, InDesc.ViewDimension));

    FD3D12Resource* D3D12Resource = D3D12Texture->GetResource();
    if (!D3D12Resource)
    {
        D3D12_WARNING("Texture does not have a valid D3D12Resource");
        return nullptr;
    }

    const D3D12_RESOURCE_DESC& ResourceDesc = D3D12Resource->GetDesc();
    if ((ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) == D3D12_RESOURCE_FLAG_NONE)
    {
        String DebugName;
        D3D12Resource->GetDebugName(DebugName);
        D3D12_ERROR("Texture '%s' does not allow DepthStencilViews", *DebugName);
        return nullptr;
    }

    const bool bIsMultisampled = D3D12Texture->GetDesc().IsMultisampled();

    D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
        {
            const auto& TextureDesc    = InDesc.Texture1D;
            DSVDesc.Format             = ConvertFormat(TextureDesc.Format);
            DSVDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE1D;
            DSVDesc.Texture1D.MipSlice = TextureDesc.MipLevel;
            break;
        }

        case EViewDimension::Texture1DArray:
        {
            const auto& TextureDesc                = InDesc.Texture1DArray;
            DSVDesc.Format                         = ConvertFormat(TextureDesc.Format);
            DSVDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            DSVDesc.Texture1DArray.MipSlice        = TextureDesc.MipLevel;
            DSVDesc.Texture1DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
            DSVDesc.Texture1DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            break;
        }

        case EViewDimension::Texture2D:
        {
            const auto& TextureDesc = InDesc.Texture2D;
            DSVDesc.Format = ConvertFormat(TextureDesc.Format);

            if (!bIsMultisampled)
            {
                DSVDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
                DSVDesc.Texture2D.MipSlice = TextureDesc.MipLevel;
            }
            else
            {
                DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            }

            break;
        }

        case EViewDimension::Texture2DArray:
        {
            const auto& TextureDesc = InDesc.Texture2DArray;
            DSVDesc.Format = ConvertFormat(TextureDesc.Format);

            if (!bIsMultisampled)
            {
                DSVDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                DSVDesc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                DSVDesc.Texture2DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                DSVDesc.Texture2DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            }
            else
            {
                DSVDesc.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                DSVDesc.Texture2DMSArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                DSVDesc.Texture2DMSArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            }

            break;
        }

        case EViewDimension::TextureCube:
        {
            const auto& TextureDesc = InDesc.TextureCube;
            DSVDesc.Format = ConvertFormat(TextureDesc.Format);

            if (!bIsMultisampled)
            {
                DSVDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                DSVDesc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                DSVDesc.Texture2DArray.FirstArraySlice = 0;
                DSVDesc.Texture2DArray.ArraySize       = RHI_NUM_CUBE_FACES;
            }
            else
            {
                DSVDesc.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                DSVDesc.Texture2DMSArray.FirstArraySlice = 0;
                DSVDesc.Texture2DMSArray.ArraySize       = RHI_NUM_CUBE_FACES;
            }

            break;
        }

        case EViewDimension::TextureCubeArray:
        {
            const auto& TextureDesc = InDesc.TextureCubeArray;
            DSVDesc.Format = ConvertFormat(TextureDesc.Format);
            
            const uint32 FirstLayer  = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, TextureDesc.FirstCube);
            const uint32 NumLayers   = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, Math::Max<uint16>(TextureDesc.NumCubes, 1u));

            if (!bIsMultisampled)
            {
                DSVDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                DSVDesc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                DSVDesc.Texture2DArray.FirstArraySlice = FirstLayer;
                DSVDesc.Texture2DArray.ArraySize       = NumLayers;
            }
            else
            {
                DSVDesc.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                DSVDesc.Texture2DMSArray.FirstArraySlice = FirstLayer;
                DSVDesc.Texture2DMSArray.ArraySize       = NumLayers;
            }

            break;
        }

        default:
        {
            D3D12_ERROR("CreateDepthStencilView: unsupported ViewDimension");
            return nullptr;
        }
    }

    if (DSVDesc.Format == DXGI_FORMAT_UNKNOWN)
    {
        D3D12_ERROR("Unallowed format for DepthStencilViews");
        return nullptr;
    }

    DSVDesc.Flags = D3D12_DSV_FLAG_NONE;
    if (IsEnumFlagSet(InDesc.Flags, EDepthStencilViewFlags::ReadOnlyDepth))
    {
        DSVDesc.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    }

    if (IsEnumFlagSet(InDesc.Flags, EDepthStencilViewFlags::ReadOnlyStencil))
    {
        DSVDesc.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;
    }

    FD3D12DepthStencilViewRHIRef D3D12View = new FD3D12DepthStencilViewRHI(GetDevice(), GetDevice()->GetDepthStencilOfflineDescriptorHeap(), D3D12Texture, InDesc);
    if (!D3D12View->Initialize(D3D12Resource, DSVDesc))
    {
        return nullptr;
    }

    D3D12View->RegisterWithResource(D3D12Texture);
    return D3D12View.ReleaseOwnership();
}

FRHIComputeShader* FD3D12DeviceRHI::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    FD3D12ComputeShaderRHIRef NewShader = new FD3D12ComputeShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIVertexShader* FD3D12DeviceRHI::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    FD3D12VertexShaderRHIRef NewShader = new FD3D12VertexShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIHullShader* FD3D12DeviceRHI::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    FD3D12HullShaderRHIRef NewShader = new FD3D12HullShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDomainShader* FD3D12DeviceRHI::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    FD3D12DomainShaderRHIRef NewShader = new FD3D12DomainShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIGeometryShader* FD3D12DeviceRHI::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    FD3D12GeometryShaderRHIRef NewShader = new FD3D12GeometryShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIMeshShader* FD3D12DeviceRHI::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    FD3D12MeshShaderRHIRef NewShader = new FD3D12MeshShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIAmplificationShader* FD3D12DeviceRHI::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    FD3D12AmplificationShaderRHIRef NewShader = new FD3D12AmplificationShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIPixelShader* FD3D12DeviceRHI::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    FD3D12PixelShaderRHIRef NewShader = new FD3D12PixelShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayGenShader* FD3D12DeviceRHI::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    FD3D12RayGenShaderRHIRef NewShader = new FD3D12RayGenShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        D3D12_ERROR_CRITICAL("[FD3D12DeviceRHI]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayAnyHitShader* FD3D12DeviceRHI::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    FD3D12RayAnyHitShaderRHIRef NewShader = new FD3D12RayAnyHitShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        D3D12_ERROR_CRITICAL("[FD3D12DeviceRHI]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayClosestHitShader* FD3D12DeviceRHI::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    FD3D12RayClosestHitShaderRHIRef NewShader = new FD3D12RayClosestHitShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        D3D12_ERROR_CRITICAL("[FD3D12DeviceRHI]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayMissShader* FD3D12DeviceRHI::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    FD3D12RayMissShaderRHIRef NewShader = new FD3D12RayMissShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        D3D12_ERROR_CRITICAL("[FD3D12DeviceRHI]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDepthStencilState* FD3D12DeviceRHI::CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
{
    return new FD3D12DepthStencilStateRHI(InDesc);
}

FRHIRasterizerState* FD3D12DeviceRHI::CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
{
    return new FD3D12RasterizerStateRHI(InDesc);
}

FRHIBlendState* FD3D12DeviceRHI::CreateBlendState(const FRHIBlendStateDesc& InDesc)
{
    return new FD3D12BlendStateRHI(InDesc);
}

FRHIInputLayout* FD3D12DeviceRHI::CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
{
    return new FD3D12InputLayoutRHI(InInputElements);
}

FRHIGraphicsPipelineState* FD3D12DeviceRHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
{
    FD3D12GraphicsPipelineStateRHIRef NewPipelineState = new FD3D12GraphicsPipelineStateRHI(GetDevice());
    if (!NewPipelineState->Initialize(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewPipelineState.ReleaseOwnership();
    }
}

FRHIComputePipelineState* FD3D12DeviceRHI::CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
{
    FD3D12ComputePipelineStateRHIRef NewPipelineState = new FD3D12ComputePipelineStateRHI(GetDevice(), MakeSharedRef<FD3D12ComputeShaderRHI>(InDesc.Shader));
    if (!NewPipelineState->Initialize(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewPipelineState.ReleaseOwnership();
    }
}

FRHIMeshletPipelineState* FD3D12DeviceRHI::CreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc& InDesc)
{
    FD3D12MeshletPipelineStateRHIRef NewPipelineState = new FD3D12MeshletPipelineStateRHI(GetDevice());
    if (!NewPipelineState->Initialize(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewPipelineState.ReleaseOwnership();
    }
}

FRHIRayTracingPipelineState* FD3D12DeviceRHI::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc)
{
    FD3D12RayTracingPipelineStateRHIRef NewPipelineState = new FD3D12RayTracingPipelineStateRHI(GetDevice());
    if (!NewPipelineState->Initialize(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewPipelineState.ReleaseOwnership();
    }
}

FRHIQuery* FD3D12DeviceRHI::CreateQuery(EQueryType InQueryType)
{
    return new FD3D12QueryRHI(GetDevice(), InQueryType);
}

FRHIFence* FD3D12DeviceRHI::CreateFence()
{
    FD3D12FenceRHIRef NewFence = new FD3D12FenceRHI(GetDevice());
    if (!NewFence->Initialize())
    {
        return nullptr;
    }

    return NewFence.ReleaseOwnership();
}

FRHISwapChain* FD3D12DeviceRHI::CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc)
{
    CHECK(InSwapChainDesc.WindowHandle != nullptr);

    FD3D12SwapChainRHIRef NewSwapChain = new FD3D12SwapChainRHI(GetDevice(), DirectCommandContext, InSwapChainDesc);
    if (!NewSwapChain->Initialize(DirectCommandContext))
    {
        return nullptr;
    }
    else
    {
        return NewSwapChain.ReleaseOwnership();
    }
}

bool FD3D12DeviceRHI::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const
{
    if (!Adapter)
    {
        return false;
    }

    const DXGI_MEMORY_SEGMENT_GROUP MemoryGroup = MemoryType == EVideoMemoryType::Local ? 
        DXGI_MEMORY_SEGMENT_GROUP_LOCAL : 
        DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL;

    DXGI_QUERY_VIDEO_MEMORY_INFO VideoMemoryInfo;
    HRESULT hr = Adapter->GetDXGIAdapter3()->QueryVideoMemoryInfo(0, MemoryGroup, &VideoMemoryInfo);
    if (FAILED(hr))
    {
        D3D12_ERROR("[FD3D12DeviceRHI] QueryVideoMemoryInfo failed");
        return false;
    }

    OutMemoryInfo.MemoryType   = MemoryType;
    OutMemoryInfo.MemoryUsage  = VideoMemoryInfo.CurrentUsage;
    OutMemoryInfo.MemoryBudget = VideoMemoryInfo.Budget;
    return true;
}

bool FD3D12DeviceRHI::QueryUAVFormatSupport(EFormat Format) const
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData = {};

    HRESULT Result = Device->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
    if (SUCCEEDED(Result))
    {
        if (FeatureData.TypedUAVLoadAdditionalFormats)
        {
            D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport =
            {
                ConvertFormat(Format),
                D3D12_FORMAT_SUPPORT1_NONE,
                D3D12_FORMAT_SUPPORT2_NONE
            };

            Result = Device->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
            if (FAILED(Result) || (FormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) == 0)
            {
                return false;
            }
        }
    }

    return true;
}

bool FD3D12DeviceRHI::GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode)
{
    FD3D12QueryRHI* D3D12Query = FD3D12DeviceRHI::ResourceCast(Query);
    if (!D3D12Query)
    {
        return false;
    }

    if (Mode == EQueryResultMode::Wait)
    {
        const FD3D12FenceSyncPoint& SyncPoint = D3D12Query->SyncPoint;
        if (!SyncPoint.IsValid())
        {
            return false;
        }

        SyncPoint.Wait();
    }

    OutResult = *D3D12Query->QueryResult;
    return true;
}

bool FD3D12DeviceRHI::GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode)
{
    FD3D12QueryRHI* D3D12Query = FD3D12DeviceRHI::ResourceCast(Query);
    if (!D3D12Query)
    {
        return false;
    }

    if (Mode == EQueryResultMode::Wait)
    {
        const FD3D12FenceSyncPoint& SyncPoint = D3D12Query->SyncPoint;
        if (!SyncPoint.IsValid())
        {
            return false;
        }

        SyncPoint.Wait();
    }

    OutResult = *reinterpret_cast<const FRHIPipelineStatistics*>(D3D12Query->QueryResult);
    return true;
}

void FD3D12DeviceRHI::EnqueueResourceDeletion(FRHIResource* Resource)
{
    if (Resource)
    {
        DeferDeletion(Resource);
    }
}

String FD3D12DeviceRHI::GetAdapterName() const 
{ 
    CHECK(Adapter != nullptr);
    return Adapter->GetDescription(); 
}

IRHICommandContext* FD3D12DeviceRHI::ObtainCommandContext()
{
    return DirectCommandContext;
}

void* FD3D12DeviceRHI::GetRHINativeAdapter() 
{
    CHECK(Adapter != nullptr);
    return reinterpret_cast<void*>(Adapter->GetDXGIAdapter());
}

void* FD3D12DeviceRHI::GetRHINativeDevice()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D12Device());
}

void* FD3D12DeviceRHI::GetRHINativeDirectCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Direct));
}

void* FD3D12DeviceRHI::GetRHINativeComputeCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Compute));
}

void* FD3D12DeviceRHI::GetRHINativeCopyCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Copy));
}

void FD3D12DeviceRHI::TickCoreProgression()
{
    Device->GetQueue(ED3D12CommandQueueType::Direct)->ProcessCommandQueue();

    if (FD3D12LinearAllocator* StagingBufferAllocator = Device->GetStagingBufferAllocator())
    {
        StagingBufferAllocator->CleanUp();
    }

    if (FD3D12DynamicConstantsAllocator* DynamicConstantsAllocator = Device->GetDynamicConstantsAllocator())
    {
        DynamicConstantsAllocator->CleanUp();
    }

    if (FD3D12UploadHeapAllocator* UploadHeapAllocator = Device->GetUploadHeapAllocator())
    {
        UploadHeapAllocator->CleanUp();
    }

    if (FD3D12BufferAllocator* BufferAllocator = Device->GetBufferAllocator())
    {
        BufferAllocator->CleanUp();
    }

    if (FD3D12TextureAllocator* TextureAllocator = Device->GetTextureAllocator())
    {
        TextureAllocator->CleanUp();
    }
}

void FD3D12DeviceRHI::FlushCompletedSubmissions()
{
    Device->GetQueue(ED3D12CommandQueueType::Direct)->ProcessCommandQueue();
}

void FD3D12DeviceRHI::FlushDeletionQueue(FD3D12Commands* Commands)
{
    CHECK(Commands != nullptr);
    if (Commands->IsEmpty())
    {
        return;
    }

    TScopedLock Lock(DeferredObjectsCS);
    Commands->DeferredObjects = Move(DeferredObjects);
}

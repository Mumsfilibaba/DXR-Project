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

IMPLEMENT_ENGINE_MODULE(FD3D12RHIModule, D3D12RHI);

static TAutoConsoleVariable<bool> CVarEnablePix(
    "D3D12RHI.EnablePIX",
    "Enables loading of PIX when creating device to capture frame's programmatically",
    false);

FD3D12RHI* FD3D12RHI::GD3D12RHI = nullptr;

FRHI* FD3D12RHIModule::CreateRHI()
{
    TUniquePtr<FD3D12RHI> NewRHI = MakeUniquePtr<FD3D12RHI>();
    if (!NewRHI->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewRHI.Release();
    }
}

FD3D12RHI::FD3D12RHI()
    : FRHI(ERHIType::D3D12)
    , Device(nullptr)
    , DirectCommandContext(nullptr)
{
    if (!GD3D12RHI)
    {
        GD3D12RHI = this;
    }
}

FD3D12RHI::~FD3D12RHI()
{
    const auto FlushDeletionQueues = [this]()
    {
        // NOTE: Objects could contain other objects, that now need to be flushed
        if (FRHICommandListExecutor::IsInitialized())
        {
            FRHICommandListExecutor::Get().FlushDeletedResources();
        }

        while (!DeletionQueue.IsEmpty())
        {
            TArray<FD3D12DeferredObject> Items;
            {
                TScopedLock Lock(DeletionQueueCS);
                Items = Move(DeletionQueue);
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

    while (!PendingSubmissions.IsEmpty())
    {
        ProcessPendingCommandSubmissions();
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

    SAFE_DELETE(Device);
    SAFE_DELETE(Adapter);

    D3D12Loader::Release();

    if (GD3D12RHI == this)
    {
        GD3D12RHI = nullptr;
    }
}

bool FD3D12RHI::Initialize()
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

bool FD3D12RHI::InitializeDeviceFeatureSupport()
{
    // -------------------------------------------------------------------------------------------
    // Baseline defaults
    // -------------------------------------------------------------------------------------------

    RHIDeviceFeatureSupport::bSupportsGeometryShaders                       = true; // Geometry Shaders are always supported
    RHIDeviceFeatureSupport::bSupportRenderTargetArrayIndexFromVertexShader = false;

    RHIDeviceFeatureSupport::bSupportsViewInstancing     = false;
    RHIDeviceFeatureSupport::MaxViewInstanceCount        = 1;

    RHIDeviceFeatureSupport::bSupportsRayTracing         = false;
    RHIDeviceFeatureSupport::RayTracingTier              = ERayTracingTier::NotSupported;
    RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = 0;

    RHIDeviceFeatureSupport::bSupportsVRS                = false;
    RHIDeviceFeatureSupport::ShadingRateTier             = EShadingRateTier::NotSupported;
    RHIDeviceFeatureSupport::ShadingRateImageTileSize    = 0;

    RHIDeviceFeatureSupport::bSupportDrawIndirect        = true;
    RHIDeviceFeatureSupport::bSupportMultiDrawIndirect   = true;
    RHIDeviceFeatureSupport::MaxDrawIndirectCount        = uint32(~0u);

    // -------------------------------------------------------------------------------------------
    // Texture / image limits (canonical D3D12 defines)
    // -------------------------------------------------------------------------------------------

    RHIDeviceFeatureSupport::MaxTexture1DSize        = D3D12_REQ_TEXTURE1D_U_DIMENSION;
    RHIDeviceFeatureSupport::MaxTexture1DArrayLayers = D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
    RHIDeviceFeatureSupport::MaxTexture2DSize        = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    RHIDeviceFeatureSupport::MaxTexture2DArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    RHIDeviceFeatureSupport::MaxTexture3DWidth       = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    RHIDeviceFeatureSupport::MaxTexture3DHeight      = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    RHIDeviceFeatureSupport::MaxTexture3DDepth       = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    RHIDeviceFeatureSupport::MaxCubeTextureSize      = D3D12_REQ_TEXTURECUBE_DIMENSION;
    RHIDeviceFeatureSupport::MaxCubeArrayCount       = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION / RHI_NUM_CUBE_FACES;

    // -------------------------------------------------------------------------------------------
    // Buffer / memory limits
    // -------------------------------------------------------------------------------------------

    RHIDeviceFeatureSupport::MaxBufferSize              = uint64(~0ull);
    RHIDeviceFeatureSupport::MaxConstantBufferSize      = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
    RHIDeviceFeatureSupport::MaxStorageBufferSize       = uint64(~0ull);
    RHIDeviceFeatureSupport::StructuredBufferMinStride  = 0;
    RHIDeviceFeatureSupport::StructuredBufferMaxStride  = uint32(~0u);
    RHIDeviceFeatureSupport::RawBufferRequiredAlignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;

    // -------------------------------------------------------------------------------------------
    // SV_RenderTargetArrayIndex from VS
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS Features = {};
        if (SUCCEEDED(GetDevice()->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &Features, sizeof(Features))))
        {
            RHIDeviceFeatureSupport::bSupportRenderTargetArrayIndexFromVertexShader = !!Features.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
        }
    }

    // -------------------------------------------------------------------------------------------
    // Ray Tracing (DXR)
    // -------------------------------------------------------------------------------------------

    if (GD3D12RayTracingTier >= D3D12_RAYTRACING_TIER_1_0)
    {
        RHIDeviceFeatureSupport::bSupportsRayTracing         = true;
        RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;

        if (GD3D12RayTracingTier == D3D12_RAYTRACING_TIER_1_1)
        {
            RHIDeviceFeatureSupport::RayTracingTier = ERayTracingTier::Tier1_1;
        }
        else
        {
            RHIDeviceFeatureSupport::RayTracingTier = ERayTracingTier::Tier1;
        }
    }
    else
    {
        RHIDeviceFeatureSupport::bSupportsRayTracing         = false;
        RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = 0;
        RHIDeviceFeatureSupport::RayTracingTier              = ERayTracingTier::NotSupported;
    }

    // -------------------------------------------------------------------------------------------
    // View Instancing
    // -------------------------------------------------------------------------------------------

    if (GD3D12ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED)
    {
        RHIDeviceFeatureSupport::bSupportsViewInstancing = true;
        RHIDeviceFeatureSupport::MaxViewInstanceCount    = D3D12_MAX_VIEW_INSTANCE_COUNT;
    }
    else
    {
        RHIDeviceFeatureSupport::bSupportsViewInstancing = false;
        RHIDeviceFeatureSupport::MaxViewInstanceCount    = 1;
    }

    // -------------------------------------------------------------------------------------------
    // Variable Rate Shading (VRS)
    // -------------------------------------------------------------------------------------------

    switch (GD3D12VariableRateShadingTier)
    {
    default:
    case D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED:
        RHIDeviceFeatureSupport::ShadingRateTier          = EShadingRateTier::NotSupported;
        RHIDeviceFeatureSupport::bSupportsVRS             = false;
        RHIDeviceFeatureSupport::ShadingRateImageTileSize = 0;
        break;

    case D3D12_VARIABLE_SHADING_RATE_TIER_1:
        RHIDeviceFeatureSupport::ShadingRateTier = EShadingRateTier::Tier1;
        RHIDeviceFeatureSupport::bSupportsVRS    = true;
        break;

    case D3D12_VARIABLE_SHADING_RATE_TIER_2:
        RHIDeviceFeatureSupport::ShadingRateTier = EShadingRateTier::Tier2;
        RHIDeviceFeatureSupport::bSupportsVRS    = true;
        break;
    }

    if (RHIDeviceFeatureSupport::bSupportsVRS)
    {
        // Tile size is not cached
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 Features6 = {};
        if (SUCCEEDED(GetDevice()->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &Features6, sizeof(Features6))))
        {
            RHIDeviceFeatureSupport::ShadingRateImageTileSize = Features6.ShadingRateImageTileSize;
        }
        else
        {
            RHIDeviceFeatureSupport::ShadingRateImageTileSize = 0;
        }
    }

    return true;
}


FRHITexture* FD3D12RHI::CreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FD3D12TextureRef NewTexture = new FD3D12Texture(GetDevice(), InTextureInfo);
    if (!NewTexture->Initialize(DirectCommandContext, InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewTexture.ReleaseOwnership();
    }
}

FRHIBuffer* FD3D12RHI::CreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData)
{
    TSharedRef<FD3D12Buffer> NewBuffer = new FD3D12Buffer(GetDevice(), InBufferInfo);
    if (!NewBuffer->Initialize(DirectCommandContext, InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

FRHISamplerState* FD3D12RHI::CreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo)
{
    TScopedLock Lock(SamplerStateMapCS);

    FD3D12SamplerStateRef Result;

    // Check if there already is an existing sampler state with this description
    if (FD3D12SamplerStateRef* ExistingSamplerState = SamplerStateMap.Find(InSamplerInfo))
    {
        Result = *ExistingSamplerState;
    }
    else
    {
        D3D12_SAMPLER_DESC Desc = {};
        Desc.AddressU       = ConvertSamplerMode(InSamplerInfo.AddressU);
        Desc.AddressV       = ConvertSamplerMode(InSamplerInfo.AddressV);
        Desc.AddressW       = ConvertSamplerMode(InSamplerInfo.AddressW);
        Desc.ComparisonFunc = ConvertComparisonFunc(InSamplerInfo.ComparisonFunc);
        Desc.Filter         = ConvertSamplerFilter(InSamplerInfo.Filter);
        Desc.MaxAnisotropy  = InSamplerInfo.MaxAnisotropy;
        Desc.MaxLOD         = InSamplerInfo.MaxLOD;
        Desc.MinLOD         = InSamplerInfo.MinLOD;
        Desc.MipLODBias     = InSamplerInfo.MipLODBias;
        
        FMemory::Memcpy(Desc.BorderColor, InSamplerInfo.BorderColor.RGBA, sizeof(Desc.BorderColor));

        Result = new FD3D12SamplerState(GetDevice(), GetDevice()->GetSamplerOfflineDescriptorHeap(), InSamplerInfo);
        if (!Result->CreateSampler(Desc))
        {
            return nullptr;
        }
        else
        {
            SamplerStateMap.Add(InSamplerInfo, Result);
        }
    }

    return Result.ReleaseOwnership();
}

FRHIRayTracingScene* FD3D12RHI::CreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo)
{
    FRayTracingSceneBuildInfo BuildInfo;
    BuildInfo.Instances    = InSceneInfo.Instances.Data();
    BuildInfo.NumInstances = InSceneInfo.Instances.Size();
    BuildInfo.bUpdate      = false;

    DirectCommandContext->StartContext();

    TSharedRef<FD3D12RayTracingScene> D3D12Scene = new FD3D12RayTracingScene(GetDevice(), InSceneInfo);
    if (!D3D12Scene->Build(*DirectCommandContext, BuildInfo))
    {
        DEBUG_BREAK();
        D3D12Scene.Reset();
    }

    DirectCommandContext->FinishContext();
    return D3D12Scene.ReleaseOwnership();
}

FRHIRayTracingGeometry* FD3D12RHI::CreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo)
{
    FRayTracingGeometryBuildInfo BuildInfo;
    BuildInfo.VertexBuffer = InGeometryInfo.VertexBuffer;
    BuildInfo.NumVertices  = InGeometryInfo.NumVertices;
    BuildInfo.IndexBuffer  = InGeometryInfo.IndexBuffer;
    BuildInfo.NumIndices   = InGeometryInfo.NumIndices;
    BuildInfo.IndexFormat  = InGeometryInfo.IndexFormat;
    BuildInfo.bUpdate      = false;

    DirectCommandContext->StartContext();

    TSharedRef<FD3D12RayTracingGeometry> D3D12Geometry = new FD3D12RayTracingGeometry(GetDevice(), InGeometryInfo);
    if (!D3D12Geometry->Build(*DirectCommandContext, BuildInfo))
    {
        DEBUG_BREAK();
        D3D12Geometry.Reset();
    }

    DirectCommandContext->FinishContext();
    return D3D12Geometry.ReleaseOwnership();
}

FRHIShaderResourceView* FD3D12RHI::CreateShaderResourceView(const FRHIShaderResourceViewInfo& InInfo)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc = {};
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    FRHIResource*   Resource      = nullptr;
    FD3D12Resource* D3D12Resource = nullptr;

    if (InInfo.IsBufferSRV())
    {
        FD3D12Buffer* D3D12Buffer = FD3D12Buffer::Cast(InInfo.BufferSRV.Buffer);
        CHECK(D3D12Buffer != nullptr);

        D3D12Resource = D3D12Buffer->GetResource();
        Resource      = D3D12Buffer;

        Desc.ViewDimension       = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = InInfo.BufferSRV.FirstElement;
        Desc.Buffer.NumElements  = InInfo.BufferSRV.NumElements;

        if (InInfo.BufferSRV.Format == EBufferSRVFormat::None)
        {
            Desc.Format                     = DXGI_FORMAT_UNKNOWN;
            Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
            Desc.Buffer.StructureByteStride = InInfo.BufferSRV.Buffer->GetInfo().Stride;
        }
        else
        {
            Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
            Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;
            Desc.Buffer.StructureByteStride = 0;
        }
    }
    else if (InInfo.IsTextureSRV())
    {
        FD3D12Texture* D3D12Texture = FD3D12Texture::Cast(InInfo.TextureSRV.Texture);
        CHECK(D3D12Texture != nullptr);

        D3D12Resource = D3D12Texture->GetResource();
        Resource      = D3D12Texture;

        Desc.Format = ConvertFormat(InInfo.TextureSRV.Format);

        const FRHITextureInfo& TextureInfo = D3D12Texture->GetInfo();
        if (TextureInfo.IsTexture2D())
        {
            if (!TextureInfo.IsMultisampled())
            {
                Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
                Desc.Texture2D.MostDetailedMip     = InInfo.TextureSRV.FirstMipLevel;
                Desc.Texture2D.MipLevels           = InInfo.TextureSRV.NumMips;
                Desc.Texture2D.ResourceMinLODClamp = InInfo.TextureSRV.MinLODClamp;
                Desc.Texture2D.PlaneSlice          = 0;
            }
            else
            {
                Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
        }
        else if (TextureInfo.IsTexture2DArray())
        {
            if (!TextureInfo.IsMultisampled())
            {
                Desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                Desc.Texture2DArray.MostDetailedMip     = InInfo.TextureSRV.FirstMipLevel;
                Desc.Texture2DArray.MipLevels           = InInfo.TextureSRV.NumMips;
                Desc.Texture2DArray.ResourceMinLODClamp = InInfo.TextureSRV.MinLODClamp;
                Desc.Texture2DArray.FirstArraySlice     = InInfo.TextureSRV.FirstArraySlice;
                Desc.Texture2DArray.ArraySize           = InInfo.TextureSRV.NumSlices;
                Desc.Texture2DArray.PlaneSlice          = 0;
            }
            else
            {
                Desc.ViewDimension                    = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                Desc.Texture2DMSArray.FirstArraySlice = InInfo.TextureSRV.FirstArraySlice;
                Desc.Texture2DMSArray.ArraySize       = InInfo.TextureSRV.NumSlices;
            }
        }
        else if (TextureInfo.IsTextureCube())
        {
            Desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
            Desc.TextureCube.MostDetailedMip     = InInfo.TextureSRV.FirstMipLevel;
            Desc.TextureCube.MipLevels           = InInfo.TextureSRV.NumMips;
            Desc.TextureCube.ResourceMinLODClamp = InInfo.TextureSRV.MinLODClamp;
        }
        else if (TextureInfo.IsTextureCubeArray())
        {
            Desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            Desc.TextureCubeArray.MostDetailedMip     = InInfo.TextureSRV.FirstMipLevel;
            Desc.TextureCubeArray.MipLevels           = InInfo.TextureSRV.NumMips;
            Desc.TextureCubeArray.ResourceMinLODClamp = InInfo.TextureSRV.MinLODClamp;
            Desc.TextureCubeArray.First2DArrayFace    = InInfo.TextureSRV.FirstArraySlice * RHI_NUM_CUBE_FACES;
            Desc.TextureCubeArray.NumCubes            = InInfo.TextureSRV.NumSlices;
        }
        else if (TextureInfo.IsTexture3D())
        {
            Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
            Desc.Texture3D.MostDetailedMip     = InInfo.TextureSRV.FirstMipLevel;
            Desc.Texture3D.MipLevels           = InInfo.TextureSRV.NumMips;
            Desc.Texture3D.ResourceMinLODClamp = InInfo.TextureSRV.MinLODClamp;
        }
    }
    else
    {
        return nullptr;
    }

    FD3D12ShaderResourceViewRef D3D12View = new FD3D12ShaderResourceView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), Resource);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    CHECK(D3D12Resource != nullptr);

    if (D3D12View->CreateView(D3D12Resource, Desc))
    {
        return D3D12View.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

FRHIUnorderedAccessView* FD3D12RHI::CreateUnorderedAccessView(const FRHIUnorderedAccessViewInfo& InInfo)
{
    FRHIResource*   Resource      = nullptr;
    FD3D12Resource* D3D12Resource = nullptr;

    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc = {};
    if (InInfo.IsBufferUAV())
    {
        FD3D12Buffer* D3D12Buffer = FD3D12Buffer::Cast(InInfo.BufferUAV.Buffer);
        CHECK(D3D12Buffer != nullptr);

        D3D12Resource = D3D12Buffer->GetResource();
        Resource      = D3D12Buffer;

        Desc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = InInfo.BufferUAV.FirstElement;
        Desc.Buffer.NumElements  = InInfo.BufferUAV.NumElements;

        if (InInfo.BufferUAV.Format == EBufferUAVFormat::None)
        {
            Desc.Format                     = DXGI_FORMAT_UNKNOWN;
            Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
            Desc.Buffer.StructureByteStride = InInfo.BufferUAV.Buffer->GetInfo().Stride;
        }
        else
        {
            Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
            Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_RAW;
            Desc.Buffer.StructureByteStride = 0;
        }
    }
    else if (InInfo.IsTextureUAV())
    {
        Desc.Format = ConvertFormat(InInfo.TextureUAV.Format);
        
        FD3D12Texture* D3D12Texture = FD3D12Texture::Cast(InInfo.TextureUAV.Texture);
        CHECK(D3D12Texture != nullptr);

        D3D12Resource = D3D12Texture->GetResource();
        Resource      = D3D12Texture;

        const FRHITextureInfo& TextureInfo = D3D12Texture->GetInfo();
        if (TextureInfo.IsTexture2D())
        {
            if (!TextureInfo.IsMultisampled())
            {
                Desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
                Desc.Texture2D.MipSlice   = InInfo.TextureUAV.MipLevel;
                Desc.Texture2D.PlaneSlice = 0;
            }
            else
            {
                D3D12_ERROR("MultiSampled Textures is not supported");
            }
        }
        else if (TextureInfo.IsTexture2DArray())
        {
            if (!TextureInfo.IsMultisampled())
            {
                Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                Desc.Texture2DArray.MipSlice        = InInfo.TextureUAV.MipLevel;
                Desc.Texture2DArray.PlaneSlice      = 0;
                Desc.Texture2DArray.FirstArraySlice = InInfo.TextureUAV.FirstArraySlice;
                Desc.Texture2DArray.ArraySize       = InInfo.TextureUAV.NumSlices;
            }
            else
            {
                D3D12_ERROR("MultiSampled Textures is not supported");
            }
        }
        else if (TextureInfo.IsTextureCube() || TextureInfo.IsTextureCubeArray())
        {
            Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = InInfo.TextureUAV.MipLevel;
            Desc.Texture2DArray.PlaneSlice      = 0;
            Desc.Texture2DArray.FirstArraySlice = InInfo.TextureUAV.FirstArraySlice * RHI_NUM_CUBE_FACES;
            Desc.Texture2DArray.ArraySize       = InInfo.TextureUAV.NumSlices * RHI_NUM_CUBE_FACES;
        }
        else if (TextureInfo.IsTexture3D())
        {
            Desc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
            Desc.Texture3D.FirstWSlice = InInfo.TextureUAV.FirstArraySlice;
            Desc.Texture3D.WSize       = InInfo.TextureUAV.NumSlices;
            Desc.Texture3D.MipSlice    = InInfo.TextureUAV.MipLevel;
        }
    }

    FD3D12UnorderedAccessViewRef D3D12View = new FD3D12UnorderedAccessView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), Resource);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    if (D3D12View->CreateView(nullptr, D3D12Resource, Desc))
    {
        return D3D12View.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

FRHIComputeShader* FD3D12RHI::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12ComputeShader> NewShader = new FD3D12ComputeShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIVertexShader* FD3D12RHI::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12VertexShader> NewShader = new FD3D12VertexShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIHullShader* FD3D12RHI::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12HullShader> NewShader = new FD3D12HullShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDomainShader* FD3D12RHI::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12DomainShader> NewShader = new FD3D12DomainShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIGeometryShader* FD3D12RHI::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12GeometryShader> NewShader = new FD3D12GeometryShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIMeshShader* FD3D12RHI::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIAmplificationShader* FD3D12RHI::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIPixelShader* FD3D12RHI::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12PixelShader> NewShader = new FD3D12PixelShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayGenShader* FD3D12RHI::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12RayGenShader> NewShader = new FD3D12RayGenShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        D3D12_ERROR_CRITICAL("[FD3D12RHI]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayAnyHitShader* FD3D12RHI::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12RayAnyHitShader> NewShader = new FD3D12RayAnyHitShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        D3D12_ERROR_CRITICAL("[FD3D12RHI]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayClosestHitShader* FD3D12RHI::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12RayClosestHitShader> NewShader = new FD3D12RayClosestHitShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        D3D12_ERROR_CRITICAL("[FD3D12RHI]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayMissShader* FD3D12RHI::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12RayMissShader> NewShader = new FD3D12RayMissShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        D3D12_ERROR_CRITICAL("[FD3D12RHI]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDepthStencilState* FD3D12RHI::CreateDepthStencilState(const FRHIDepthStencilStateInfo& InInfo)
{
    return new FD3D12DepthStencilState(InInfo);
}

FRHIRasterizerState* FD3D12RHI::CreateRasterizerState(const FRHIRasterizerStateInfo& InInfo)
{
    return new FD3D12RasterizerState(InInfo);
}

FRHIBlendState* FD3D12RHI::CreateBlendState(const FRHIBlendStateInfo& InInfo)
{
    return new FD3D12BlendState(InInfo);
}

FRHIInputLayout* FD3D12RHI::CreateInputLayout(const TArray<FRHIInputElementInfo>& InInputElements)
{
    return new FD3D12InputLayout(InInputElements);
}

FRHIGraphicsPipelineState* FD3D12RHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInfo& InInfo)
{
    FD3D12GraphicsPipelineStateRef NewPipelineState = new FD3D12GraphicsPipelineState(GetDevice());
    if (!NewPipelineState->Initialize(InInfo))
    {
        return nullptr;
    }
    else
    {
        return NewPipelineState.ReleaseOwnership();
    }
}

FRHIComputePipelineState* FD3D12RHI::CreateComputePipelineState(const FRHIComputePipelineStateInfo& InInfo)
{
    FD3D12ComputePipelineStateRef NewPipelineState = new FD3D12ComputePipelineState(GetDevice(), MakeSharedRef<FD3D12ComputeShader>(InInfo.Shader));
    if (!NewPipelineState->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewPipelineState.ReleaseOwnership();
    }
}

FRHIRayTracingPipelineState* FD3D12RHI::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& InInitializer)
{
    FD3D12RayTracingPipelineStateRef NewPipelineState = new FD3D12RayTracingPipelineState(GetDevice());
    if (!NewPipelineState->Initialize(InInitializer))
    {
        return nullptr;
    }
    else
    {
        return NewPipelineState.ReleaseOwnership();
    }
}

FRHIQuery* FD3D12RHI::CreateQuery(EQueryType InQueryType)
{
    return new FD3D12Query(GetDevice(), InQueryType);
}

FRHIGpuFence* FD3D12RHI::CreateFence()
{
    FD3D12GpuFenceRef NewFence = new FD3D12GpuFence(GetDevice());
    if (!NewFence->Initialize())
    {
        return nullptr;
    }

    return NewFence.ReleaseOwnership();
}

FRHISwapChain* FD3D12RHI::CreateSwapChain(const FRHISwapChainInfo& InSwapChainInfo)
{
    CHECK(InSwapChainInfo.WindowHandle != nullptr);

    FD3D12SwapChainRef NewSwapChain = new FD3D12SwapChain(GetDevice(), DirectCommandContext, InSwapChainInfo);
    if (!NewSwapChain->Initialize(DirectCommandContext))
    {
        return nullptr;
    }
    else
    {
        return NewSwapChain.ReleaseOwnership();
    }
}

bool FD3D12RHI::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryStats) const
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
        D3D12_ERROR("[FD3D12RHI] QueryVideoMemoryInfo failed");
        return false;
    }

    OutMemoryStats.MemoryType   = MemoryType;
    OutMemoryStats.MemoryUsage  = VideoMemoryInfo.CurrentUsage;
    OutMemoryStats.MemoryBudget = VideoMemoryInfo.Budget;
    return true;
}

bool FD3D12RHI::QueryUAVFormatSupport(EFormat Format) const
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

bool FD3D12RHI::GetQueryResult(FRHIQuery* Query, uint64& OutResult)
{
    FD3D12Query* D3D12Result = static_cast<FD3D12Query*>(Query);
    if (!D3D12Result)
    {
        return false;
    }

    OutResult = D3D12Result->Result;
    return true;
}

void FD3D12RHI::EnqueueResourceDeletion(FRHIResource* Resource)
{
    if (Resource)
    {
        DeferDeletion(Resource);
    }
}


FString FD3D12RHI::GetAdapterName() const 
{ 
    CHECK(Adapter != nullptr);
    return Adapter->GetDescription(); 
}

IRHICommandContext* FD3D12RHI::ObtainCommandContext()
{
    return DirectCommandContext;
}

void* FD3D12RHI::GetNativeAdapter() 
{
    CHECK(Adapter != nullptr);
    return reinterpret_cast<void*>(Adapter->GetDXGIAdapter());
}

void* FD3D12RHI::GetNativeDevice()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D12Device());
}

void* FD3D12RHI::GetNativeDirectCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Direct));
}

void* FD3D12RHI::GetNativeComputeCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Compute));
}

void* FD3D12RHI::GetNativeCopyCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Copy));
}

void FD3D12RHI::ProcessPendingCommandSubmissions()
{
    bool bProcess = true;
    while (bProcess)
    {
        FD3D12CommandSubmission* CommandSubmission = nullptr;
        if (PendingSubmissions.Peek(CommandSubmission))
        {
            CHECK(CommandSubmission != nullptr);
            if (!CommandSubmission->SyncPoint.IsReached())
            {
                bProcess = false;
                break;
            }
            else
            {
                // If we are finished we remove the item from the queue
                PendingSubmissions.Dequeue();
                CommandSubmission->Finish();
            }
        }
        else
        {
            bProcess = false;
        }
    }
}

void FD3D12RHI::SubmitCommands(FD3D12CommandSubmission* CommandSubmission, bool bFlushDeletionQueue)
{
    CHECK(CommandSubmission != nullptr);

    if (!CommandSubmission->IsEmpty())
    {
        if (bFlushDeletionQueue)
        {
            TScopedLock Lock(DeletionQueueCS);
            CommandSubmission->DeletionQueue = Move(DeletionQueue);
        }

        CommandSubmission->SyncPoint = CommandSubmission->Queue->ExecuteCommandLists(CommandSubmission->CommandLists.Data(), CommandSubmission->CommandLists.Size(), false);
        PendingSubmissions.Enqueue(CommandSubmission);
    }
}

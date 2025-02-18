#include "Core/Misc/ConsoleManager.h"
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
#include "D3D12RHI/D3D12Viewport.h"
#include "D3D12RHI/D3D12RHIShaderCompiler.h"
#include "D3D12RHI/D3D12Query.h"
#include "D3D12RHI/DynamicD3D12.h"

IMPLEMENT_ENGINE_MODULE(FD3D12RHIModule, D3D12RHI);

static TAutoConsoleVariable<bool> CVarEnablePix(
    "D3D12RHI.EnablePIX",
    "Enables loading of PIX when creating device to capture frame's programmatically",
    false);

FD3D12RHI* FD3D12RHI::GD3D12RHI = nullptr;

FRHI* FD3D12RHIModule::CreateRHI()
{
    return new FD3D12RHI();
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
        FRHICommandListExecutor::Get().FlushDeletedResources();

        while (!DeletionQueue.IsEmpty())
        {
            TArray<FD3D12DeferredObject> Items;
            {
                TScopedLock Lock(DeletionQueueCS);
                Items = Move(DeletionQueue);
            }

            FD3D12DeferredObject::ProcessItems(Items);

            // NOTE: Objects could contain other objects, that now need to be flushed
            FRHICommandListExecutor::Get().FlushDeletedResources();
        }
    };

    // Flush the default context before flushing the submission queue
    if (DirectCommandContext)
    {
        DirectCommandContext->RHIFlush();
    }

    while (!PendingSubmissions.IsEmpty())
    {
        ProcessPendingCommands();
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

    // Remove GenerateMips PSOs, these will be added to the deferred resources so reset them here
    GenerateMipsTex2D_PSO.Reset();
    GenerateMipsTexCube_PSO.Reset();

    // ... Finally, delete all remaining resources
    FlushDeletionQueues();

    SAFE_DELETE(Device);
    SAFE_DELETE(Adapter);

    FDynamicD3D12::Release();

    if (GD3D12RHI == this)
    {
        GD3D12RHI = nullptr;
    }
}

bool FD3D12RHI::Initialize()
{
    const bool bEnablePIX = CVarEnablePix.GetValue();

    // Load Library and Function-Pointers etc.
    const bool bResult = FDynamicD3D12::Initialize(bEnablePIX);
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

    // Initialize shader compiler
    GD3D12ShaderCompiler = new FD3D12ShaderCompiler();
    if (!GD3D12ShaderCompiler->Initialize())
    {
        return false;
    }

    // Initialize GenerateMips Shaders and pipeline states 
    TArray<uint8> Code;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, TArrayView<FShaderDefine>(), EShaderOutputLanguage::HLSL);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/GenerateMipsTex2D.hlsl", CompileInfo, Code))
    {
        D3D12_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTex2D Shader");
        return false;
    }

    TSharedRef<FD3D12ComputeShader> Shader = new FD3D12ComputeShader(GetDevice(), Code);
    if (!Shader->Initialize())
    {
        D3D12_ERROR("[D3D12CommandContext]: Failed to Create ComputeShader");
        return false;
    }

    GenerateMipsTex2D_PSO = new FD3D12ComputePipelineState(GetDevice(), Shader);
    if (!GenerateMipsTex2D_PSO->Initialize())
    {
        D3D12_ERROR("[D3D12CommandContext]: Failed to create GenerateMipsTex2D PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTex2D_PSO->SetDebugName("GenerateMipsTex2D Gen PSO");
    }

    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, TArrayView<FShaderDefine>(), EShaderOutputLanguage::HLSL);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/GenerateMipsTexCube.hlsl", CompileInfo, Code))
    {
        D3D12_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTexCube Shader");
        return false;
    }

    Shader = new FD3D12ComputeShader(GetDevice(), Code);
    if (!Shader->Initialize())
    {
        DEBUG_BREAK();
        return false;
    }

    GenerateMipsTexCube_PSO = new FD3D12ComputePipelineState(GetDevice(), Shader);
    if (!GenerateMipsTexCube_PSO->Initialize())
    {
        D3D12_ERROR("[D3D12CommandContext]: Failed to create GenerateMipsTexCube PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTexCube_PSO->SetDebugName("GenerateMipsTexCube Gen PSO");
    }

    // Initialize context
    DirectCommandContext = new FD3D12CommandContext(GetDevice(), ED3D12CommandQueueType::Direct);
    if (!(DirectCommandContext && DirectCommandContext->Initialize()))
    {
        return false;
    }

    // RenderTargetArrayIndex from vertex-shader Support
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS Features;
        FMemory::Memzero(&Features);

        HRESULT Result = GetDevice()->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &Features, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
        if (SUCCEEDED(Result))
        {
            if (Features.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
            {
                FRHIDeviceInfo::SupportRenderTargetArrayIndexFromVertexShader = true;
            }
            else
            {
                FRHIDeviceInfo::SupportRenderTargetArrayIndexFromVertexShader = false;
            }
        }
    }

    // RayTracing Support
    if (GD3D12RayTracingTier >= D3D12_RAYTRACING_TIER_1_0)
    {
        if (GD3D12RayTracingTier == D3D12_RAYTRACING_TIER_1_1)
        {
            FRHIDeviceInfo::RayTracingTier = ERayTracingTier::Tier1_1;
        }
        else if (GD3D12RayTracingTier == D3D12_RAYTRACING_TIER_1_0)
        {
            FRHIDeviceInfo::RayTracingTier = ERayTracingTier::Tier1;
        }

        FRHIDeviceInfo::RayTracingMaxRecursionDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
    }
    else
    {
        FRHIDeviceInfo::RayTracingTier = ERayTracingTier::NotSupported;
    }
    
    FRHIDeviceInfo::SupportsRayTracing = FRHIDeviceInfo::RayTracingTier != ERayTracingTier::NotSupported;

    // View-Instancing Support
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS3 Features3;
        FMemory::Memzero(&Features3);

        HRESULT Result = GetDevice()->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &Features3, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS3));
        if (SUCCEEDED(Result))
        {
            if (Features3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED)
            {
                FRHIDeviceInfo::SupportsViewInstancing = true;
                FRHIDeviceInfo::MaxViewInstanceCount = D3D12_MAX_VIEW_INSTANCE_COUNT;
            }
        }
        else
        {
            FRHIDeviceInfo::SupportsViewInstancing = false;
            FRHIDeviceInfo::MaxViewInstanceCount = 0;
        }
    }

    // Variable-Rate-Shading Support
    switch (GD3D12VariableRateShadingTier)
    {
        case D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED:
        {
            FRHIDeviceInfo::ShadingRateTier = EShadingRateTier::NotSupported;
            break;
        }
        case D3D12_VARIABLE_SHADING_RATE_TIER_1:
        {
            FRHIDeviceInfo::ShadingRateTier = EShadingRateTier::Tier1;
            break;
        }
        case D3D12_VARIABLE_SHADING_RATE_TIER_2:
        {
            FRHIDeviceInfo::ShadingRateTier = EShadingRateTier::Tier2;
            break;
        }
    }

    FRHIDeviceInfo::SupportsVRS = FRHIDeviceInfo::ShadingRateTier != EShadingRateTier::NotSupported;
    if (FRHIDeviceInfo::SupportsVRS)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 Features6;
        FMemory::Memzero(&Features6);

        HRESULT Result = GetDevice()->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &Features6, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));
        if (SUCCEEDED(Result))
        {
            FRHIDeviceInfo::ShadingRateImageTileSize = Features6.ShadingRateImageTileSize;
        }
    }
    else
    {
        FRHIDeviceInfo::ShadingRateImageTileSize = 0;
    }

    // GeometryShaders Support
    FRHIDeviceInfo::SupportsGeometryShaders = true;
    return true;
}

FRHITexture* FD3D12RHI::RHICreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
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

FRHIBuffer* FD3D12RHI::RHICreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData)
{
    TSharedRef<FD3D12Buffer>  NewBuffer = new FD3D12Buffer(GetDevice(), InBufferInfo);
    if (!NewBuffer->Initialize(DirectCommandContext, InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

FRHISamplerState* FD3D12RHI::RHICreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo)
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
        D3D12_SAMPLER_DESC Desc;
        FMemory::Memzero(&Desc);

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

FRHIRayTracingScene* FD3D12RHI::RHICreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo)
{
    FRayTracingSceneBuildInfo BuildInfo;
    BuildInfo.Instances    = InSceneInfo.Instances.Data();
    BuildInfo.NumInstances = InSceneInfo.Instances.Size();
    BuildInfo.bUpdate      = false;

    DirectCommandContext->RHIStartContext();

    TSharedRef<FD3D12RayTracingScene> D3D12Scene = new FD3D12RayTracingScene(GetDevice(), InSceneInfo);
    if (!D3D12Scene->Build(*DirectCommandContext, BuildInfo))
    {
        DEBUG_BREAK();
        D3D12Scene.Reset();
    }

    DirectCommandContext->RHIFinishContext();
    return D3D12Scene.ReleaseOwnership();
}

FRHIRayTracingGeometry* FD3D12RHI::RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo)
{
    FRayTracingGeometryBuildInfo BuildInfo;
    BuildInfo.VertexBuffer = InGeometryInfo.VertexBuffer;
    BuildInfo.NumVertices  = InGeometryInfo.NumVertices;
    BuildInfo.IndexBuffer  = InGeometryInfo.IndexBuffer;
    BuildInfo.NumIndices   = InGeometryInfo.NumIndices;
    BuildInfo.IndexFormat  = InGeometryInfo.IndexFormat;
    BuildInfo.bUpdate      = false;

    DirectCommandContext->RHIStartContext();

    TSharedRef<FD3D12RayTracingGeometry> D3D12Geometry = new FD3D12RayTracingGeometry(GetDevice(), InGeometryInfo);
    if (!D3D12Geometry->Build(*DirectCommandContext, BuildInfo))
    {
        DEBUG_BREAK();
        D3D12Geometry.Reset();
    }

    DirectCommandContext->RHIFinishContext();
    return D3D12Geometry.ReleaseOwnership();
}

FRHIShaderResourceView* FD3D12RHI::RHICreateShaderResourceView(const FRHITextureSRVInfo& InInfo)
{
    FD3D12Texture* D3D12Texture = GetD3D12Texture(InInfo.Texture);
    if (!D3D12Texture)
    {
        D3D12_ERROR("Texture cannot be nullptr");
        return nullptr;
    }

    const FRHITextureInfo& TextureInfo = D3D12Texture->GetInfo();
    CHECK(TextureInfo.IsShaderResource());
    CHECK(InInfo.Format != EFormat::Unknown);
    CHECK(IsTypelessFormat(InInfo.Format) == false);
    
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Format                  = ConvertFormat(InInfo.Format);
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    
    if (TextureInfo.IsTexture2D())
    {
        if (!TextureInfo.IsMultisampled())
        {
            Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MostDetailedMip     = InInfo.FirstMipLevel;
            Desc.Texture2D.MipLevels           = InInfo.NumMips;
            Desc.Texture2D.ResourceMinLODClamp = InInfo.MinLODClamp;
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
            Desc.Texture2DArray.MostDetailedMip     = InInfo.FirstMipLevel;
            Desc.Texture2DArray.MipLevels           = InInfo.NumMips;
            Desc.Texture2DArray.ResourceMinLODClamp = InInfo.MinLODClamp;
            Desc.Texture2DArray.FirstArraySlice     = InInfo.FirstArraySlice;
            Desc.Texture2DArray.ArraySize           = InInfo.NumSlices;
            Desc.Texture2DArray.PlaneSlice          = 0;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.FirstArraySlice = InInfo.FirstArraySlice;
            Desc.Texture2DMSArray.ArraySize       = InInfo.NumSlices;
        }
    }
    else if (TextureInfo.IsTextureCube())
    {
        Desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        Desc.TextureCube.MostDetailedMip     = InInfo.FirstMipLevel;
        Desc.TextureCube.MipLevels           = InInfo.NumMips;
        Desc.TextureCube.ResourceMinLODClamp = InInfo.MinLODClamp;
    }
    else if (TextureInfo.IsTextureCubeArray())
    {
        Desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        Desc.TextureCubeArray.MostDetailedMip     = InInfo.FirstMipLevel;
        Desc.TextureCubeArray.MipLevels           = InInfo.NumMips;
        Desc.TextureCubeArray.ResourceMinLODClamp = InInfo.MinLODClamp;
        Desc.TextureCubeArray.First2DArrayFace    = InInfo.FirstArraySlice * RHI_NUM_CUBE_FACES;
        Desc.TextureCubeArray.NumCubes            = InInfo.NumSlices;
    }
    else if (TextureInfo.IsTexture3D())
    {
        Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MostDetailedMip     = InInfo.FirstMipLevel;
        Desc.Texture3D.MipLevels           = InInfo.NumMips;
        Desc.Texture3D.ResourceMinLODClamp = InInfo.MinLODClamp;
    }

    FD3D12ShaderResourceViewRef D3D12View = new FD3D12ShaderResourceView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), D3D12Texture);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    FD3D12Resource* D3D12Resource = D3D12Texture->GetD3D12Resource();
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

FRHIShaderResourceView* FD3D12RHI::RHICreateShaderResourceView(const FRHIBufferSRVInfo& InInfo)
{
    FD3D12Buffer* D3D12Buffer = GetD3D12Buffer(InInfo.Buffer);
    if (!D3D12Buffer)
    {
        D3D12_ERROR("Cannot create a ShaderResourceView from a nullptr Buffer");
        return nullptr;
    }

    const FRHIBufferInfo& BufferInfo = D3D12Buffer->GetInfo();
    CHECK(BufferInfo.IsShaderResource());

    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    Desc.Buffer.FirstElement     = InInfo.FirstElement;
    Desc.Buffer.NumElements      = InInfo.NumElements;

    if (InInfo.Format == EBufferSRVFormat::None)
    {
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = InInfo.Buffer->GetStride();
    }
    else
    {
        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }

    FD3D12ShaderResourceViewRef D3D12View = new FD3D12ShaderResourceView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), InInfo.Buffer);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    FD3D12Resource* D3D12Resource = D3D12Buffer->GetD3D12Resource();
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

FRHIUnorderedAccessView* FD3D12RHI::RHICreateUnorderedAccessView(const FRHITextureUAVInfo& InInfo)
{
    FD3D12Texture* Texture = GetD3D12Texture(InInfo.Texture);
    if (!Texture)
    {
        D3D12_ERROR("Texture cannot be nullptr");
        return nullptr;
    }

    const FRHITextureInfo& TextureInfo = Texture->GetInfo();
    CHECK(TextureInfo.IsUnorderedAccess());
    CHECK(InInfo.Format != EFormat::Unknown);
    CHECK(IsTypelessFormat(InInfo.Format) == false);

    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Format = ConvertFormat(InInfo.Format);

    if (TextureInfo.IsTexture2D())
    {
        if (!TextureInfo.IsMultisampled())
        {
            Desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice   = InInfo.MipLevel;
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
            Desc.Texture2DArray.MipSlice        = InInfo.MipLevel;
            Desc.Texture2DArray.PlaneSlice      = 0;
            Desc.Texture2DArray.FirstArraySlice = InInfo.FirstArraySlice;
            Desc.Texture2DArray.ArraySize       = InInfo.NumSlices;
        }
        else
        {
            D3D12_ERROR("MultiSampled Textures is not supported");
        }
    }
    else if (TextureInfo.IsTextureCube() || TextureInfo.IsTextureCubeArray())
    {
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = InInfo.MipLevel;
        Desc.Texture2DArray.PlaneSlice      = 0;
        Desc.Texture2DArray.FirstArraySlice = InInfo.FirstArraySlice * RHI_NUM_CUBE_FACES;
        Desc.Texture2DArray.ArraySize       = InInfo.NumSlices * RHI_NUM_CUBE_FACES;
    }
    else if (TextureInfo.IsTexture3D())
    {
        Desc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.FirstWSlice = InInfo.FirstArraySlice;
        Desc.Texture3D.WSize       = InInfo.NumSlices;
        Desc.Texture3D.MipSlice    = InInfo.MipLevel;
    }

    FD3D12UnorderedAccessViewRef D3D12View = new FD3D12UnorderedAccessView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), InInfo.Texture);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    FD3D12Resource* D3D12Resource = GetD3D12Resource(InInfo.Texture);
    CHECK(D3D12Resource != nullptr);

    if (D3D12View->CreateView(nullptr, D3D12Resource, Desc))
    {
        return D3D12View.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

FRHIUnorderedAccessView* FD3D12RHI::RHICreateUnorderedAccessView(const FRHIBufferUAVInfo& InInfo)
{
    FD3D12Buffer* D3D12Buffer = GetD3D12Buffer(InInfo.Buffer);
    if (!D3D12Buffer)
    {
        D3D12_ERROR("Cannot create a UnorderedAccessView from a nullptr Buffer");
        return nullptr;
    }

    const FRHIBufferInfo& BufferInfo = D3D12Buffer->GetInfo();
    CHECK(BufferInfo.IsUnorderedAccess());

    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
    Desc.Buffer.FirstElement = InInfo.FirstElement;
    Desc.Buffer.NumElements  = InInfo.NumElements;

    if (InInfo.Format == EBufferUAVFormat::None)
    {
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = InInfo.Buffer->GetStride();
    }
    else
    {
        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }

    FD3D12UnorderedAccessViewRef D3D12View = new FD3D12UnorderedAccessView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), InInfo.Buffer);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    FD3D12Resource* D3D12Resource = D3D12Buffer->GetD3D12Resource();
    CHECK(D3D12Resource != nullptr);

    if (D3D12View->CreateView(nullptr, D3D12Resource, Desc))
    {
        return D3D12View.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

FRHIComputeShader* FD3D12RHI::RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12ComputeShader> NewShader = new FD3D12ComputeShader(GetDevice(), ShaderCode);
    if (!NewShader->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIVertexShader* FD3D12RHI::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12VertexShader> NewShader = new FD3D12VertexShader(GetDevice(), ShaderCode);
    if (!FD3D12Shader::GetShaderReflection(NewShader.Get()))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIHullShader* FD3D12RHI::RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12HullShader> NewShader = new FD3D12HullShader(GetDevice(), ShaderCode);
    if (!FD3D12Shader::GetShaderReflection(NewShader.Get()))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDomainShader* FD3D12RHI::RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12DomainShader> NewShader = new FD3D12DomainShader(GetDevice(), ShaderCode);
    if (!FD3D12Shader::GetShaderReflection(NewShader.Get()))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIGeometryShader* FD3D12RHI::RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12GeometryShader> NewShader = new FD3D12GeometryShader(GetDevice(), ShaderCode);
    if (!FD3D12Shader::GetShaderReflection(NewShader.Get()))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIMeshShader* FD3D12RHI::RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIAmplificationShader* FD3D12RHI::RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIPixelShader* FD3D12RHI::RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12PixelShader> NewShader = new FD3D12PixelShader(GetDevice(), ShaderCode);
    if (!FD3D12Shader::GetShaderReflection(NewShader.Get()))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayGenShader* FD3D12RHI::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12RayGenShader> NewShader = new FD3D12RayGenShader(GetDevice(), ShaderCode);
    if (!FD3D12RayTracingShader::GetRayTracingShaderReflection(NewShader.Get()))
    {
        D3D12_ERROR("[FD3D12RHI]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayAnyHitShader* FD3D12RHI::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12RayAnyHitShader> NewShader = new FD3D12RayAnyHitShader(GetDevice(), ShaderCode);
    if (!FD3D12RayTracingShader::GetRayTracingShaderReflection(NewShader.Get()))
    {
        D3D12_ERROR("[FD3D12RHI]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayClosestHitShader* FD3D12RHI::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12RayClosestHitShader> NewShader = new FD3D12RayClosestHitShader(GetDevice(), ShaderCode);
    if (!FD3D12RayTracingShader::GetRayTracingShaderReflection(NewShader.Get()))
    {
        D3D12_ERROR("[FD3D12RHI]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayMissShader* FD3D12RHI::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12RayMissShader> NewShader = new FD3D12RayMissShader(GetDevice(), ShaderCode);
    if (!FD3D12RayTracingShader::GetRayTracingShaderReflection(NewShader.Get()))
    {
        D3D12_ERROR("[FD3D12RHI]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDepthStencilState* FD3D12RHI::RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer)
{
    return new FD3D12DepthStencilState(InInitializer);
}

FRHIRasterizerState* FD3D12RHI::RHICreateRasterizerState(const FRHIRasterizerStateInitializer& InInitializer)
{
    return new FD3D12RasterizerState(InInitializer);
}

FRHIBlendState* FD3D12RHI::RHICreateBlendState(const FRHIBlendStateInitializer& InInitializer)
{
    return new FD3D12BlendState(InInitializer);
}

FRHIVertexLayout* FD3D12RHI::RHICreateVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList)
{
    return new FD3D12VertexLayout(InInitializerList);
}

FRHIGraphicsPipelineState* FD3D12RHI::RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer)
{
    FD3D12GraphicsPipelineStateRef NewPipelineState = new FD3D12GraphicsPipelineState(GetDevice());
    if (!NewPipelineState->Initialize(InInitializer))
    {
        return nullptr;
    }
    else
    {
        return NewPipelineState.ReleaseOwnership();
    }
}

FRHIComputePipelineState* FD3D12RHI::RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& InInitializer)
{
    FD3D12ComputePipelineStateRef NewPipelineState = new FD3D12ComputePipelineState(GetDevice(), MakeSharedRef<FD3D12ComputeShader>(InInitializer.Shader));
    if (!NewPipelineState->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewPipelineState.ReleaseOwnership();
    }
}

FRHIRayTracingPipelineState* FD3D12RHI::RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& InInitializer)
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

FRHIQuery* FD3D12RHI::RHICreateQuery(EQueryType InQueryType)
{
    return new FD3D12Query(GetDevice(), InQueryType);
}

FRHIViewport* FD3D12RHI::RHICreateViewport(const FRHIViewportInfo& InViewportInfo)
{
    FD3D12ViewportRef Viewport = new FD3D12Viewport(GetDevice(), DirectCommandContext, InViewportInfo);
    if (!Viewport->Initialize(DirectCommandContext))
    {
        return nullptr;
    }
    else
    {
        return Viewport.ReleaseOwnership();
    }
}

bool FD3D12RHI::RHIQueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryStats) const
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

bool FD3D12RHI::RHIQueryUAVFormatSupport(EFormat Format) const
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData;
    FMemory::Memzero(&FeatureData);

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

bool FD3D12RHI::RHIGetQueryResult(FRHIQuery* Query, uint64& OutResult)
{
    FD3D12Query* D3D12Result = static_cast<FD3D12Query*>(Query);
    if (!D3D12Result)
    {
        return false;
    }

    OutResult = D3D12Result->Result;
    return true;
}

void FD3D12RHI::RHIEnqueueResourceDeletion(FRHIResource* Resource)
{
    if (Resource)
    {
        DeferDeletion(Resource);
    }
}


FString FD3D12RHI::RHIGetAdapterName() const 
{ 
    CHECK(Adapter != nullptr);
    return Adapter->GetDescription(); 
}

IRHICommandContext* FD3D12RHI::RHIObtainCommandContext()
{
    return DirectCommandContext;
}

void* FD3D12RHI::RHIGetAdapter() 
{
    CHECK(Adapter != nullptr);
    return reinterpret_cast<void*>(Adapter->GetDXGIAdapter());
}

void* FD3D12RHI::RHIGetDevice()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D12Device());
}

void* FD3D12RHI::RHIGetDirectCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Direct));
}

void* FD3D12RHI::RHIGetComputeCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Compute));
}

void* FD3D12RHI::RHIGetCopyCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Copy));
}

void FD3D12RHI::ProcessPendingCommands()
{
    bool bProcess = true;
    while (bProcess)
    {
        FD3D12CommandPayload* CommandPayload = nullptr;
        if (PendingSubmissions.Peek(CommandPayload))
        {
            CHECK(CommandPayload != nullptr);
            if (!CommandPayload->SyncPoint.IsReached())
            {
                bProcess = false;
                break;
            }
            else
            {
                // If we are finished we remove the item from the queue
                PendingSubmissions.Dequeue();
                CommandPayload->Finish();
            }
        }
        else
        {
            bProcess = false;
        }
    }
}

void FD3D12RHI::SubmitCommands(FD3D12CommandPayload* CommandPayload, bool bFlushDeletionQueue)
{
    CHECK(CommandPayload != nullptr);

    if (!CommandPayload->IsEmpty())
    {
        if (bFlushDeletionQueue)
        {
            TScopedLock Lock(DeletionQueueCS);
            CommandPayload->DeletionQueue = Move(DeletionQueue);
        }

        CommandPayload->SyncPoint = CommandPayload->Queue->ExecuteCommandLists(CommandPayload->CommandLists.Data(), CommandPayload->CommandLists.Size(), false);
        PendingSubmissions.Enqueue(CommandPayload);
    }
}
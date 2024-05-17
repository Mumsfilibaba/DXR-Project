#include "D3D12CommandList.h"
#include "D3D12Fence.h"
#include "D3D12RootSignature.h"
#include "D3D12Core.h"
#include "D3D12RHI.h"
#include "D3D12ResourceViews.h"
#include "D3D12RayTracing.h"
#include "D3D12PipelineState.h"
#include "D3D12Texture.h"
#include "D3D12Buffer.h"
#include "D3D12SamplerState.h"
#include "D3D12Viewport.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12Query.h"
#include "DynamicD3D12.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Threading/ScopedLock.h"
#include "CoreApplication/Windows/WindowsWindow.h"

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
        GRHICommandExecutor.FlushGarbageCollection();

        while (!DeletionQueue.IsEmpty())
        {
            TArray<FD3D12DeferredObject> Items;
            {
                TScopedLock Lock(DeletionQueueCS);
                Items = Move(DeletionQueue);
            }

            FD3D12DeferredObject::ProcessItems(Items);

            // NOTE: Objects could contain other objects, that now need to be flushed
            GRHICommandExecutor.FlushGarbageCollection();
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
    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, TArrayView<FShaderDefine>(), EShaderOutputLanguage::HLSL);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/GenerateMipsTex2D.hlsl", CompileInfo, Code))
        {
            D3D12_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTex2D Shader");
            return false;
        }
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

    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, TArrayView<FShaderDefine>(), EShaderOutputLanguage::HLSL);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/GenerateMipsTexCube.hlsl", CompileInfo, Code))
        {
            D3D12_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTexCube Shader");
            return false;
        }
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

    // Initialize Hardware Support globals
    if (GD3D12RayTracingTier >= D3D12_RAYTRACING_TIER_1_0)
    {
        if (GD3D12RayTracingTier == D3D12_RAYTRACING_TIER_1_1)
        {
            GRHIRayTracingTier = ERayTracingTier::Tier1_1;
        }
        else if (GD3D12RayTracingTier == D3D12_RAYTRACING_TIER_1_0)
        {
            GRHIRayTracingTier = ERayTracingTier::Tier1;
        }

        GRHIRayTracingMaxRecursionDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
    }
    else
    {
        GRHIRayTracingTier = ERayTracingTier::NotSupported;
    }
    
    GRHISupportsRayTracing = GRHIRayTracingTier != ERayTracingTier::NotSupported;

    switch (GD3D12VariableRateShadingTier)
    {
        case D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED: GRHIShadingRateTier = EShadingRateTier::NotSupported; break;
        case D3D12_VARIABLE_SHADING_RATE_TIER_1:             GRHIShadingRateTier = EShadingRateTier::Tier1;        break;
        case D3D12_VARIABLE_SHADING_RATE_TIER_2:             GRHIShadingRateTier = EShadingRateTier::Tier2;        break;
    }

    GRHISupportsVRS = GRHIShadingRateTier != EShadingRateTier::NotSupported;
    if (GRHISupportsVRS)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 Features6;
        FMemory::Memzero(&Features6);

        HRESULT Result = GetDevice()->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &Features6, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));
        if (SUCCEEDED(Result))
        {
            GRHIShadingRateImageTileSize = Features6.ShadingRateImageTileSize;
        }
    }
    else
    {
        GRHIShadingRateImageTileSize = 0;
    }

    // GeometryShaders support
    GRHISupportsGeometryShaders = true;

    // View-Instancing
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS3 Features3;
        FMemory::Memzero(&Features3);

        HRESULT Result = GetDevice()->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &Features3, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS3));
        if (SUCCEEDED(Result))
        {
            if (Features3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED)
            {
                GRHISupportsViewInstancing = true;
                GRHIMaxViewInstanceCount = D3D12_MAX_VIEW_INSTANCE_COUNT;
            }
        }
        else
        {
            GRHISupportsViewInstancing = false;
            GRHIMaxViewInstanceCount = 0;
        }
    }

    return true;
}

FRHITexture* FD3D12RHI::RHICreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FD3D12TextureRef NewTexture = new FD3D12Texture(GetDevice(), InTextureInfo);
    if (!NewTexture->Initialize(InInitialState, InInitialData))
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
    if (!NewBuffer->Initialize(InInitialState, InInitialData))
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
        
        FMemory::Memcpy(Desc.BorderColor, &InSamplerInfo.BorderColor.r, sizeof(Desc.BorderColor));

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

FRHIRayTracingScene* FD3D12RHI::RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& InDesc)
{
    FRayTracingSceneBuildInfo BuildInfo;
    BuildInfo.Instances    = InDesc.Instances.Data();
    BuildInfo.NumInstances = InDesc.Instances.Size();
    BuildInfo.bUpdate      = false;

    DirectCommandContext->RHIStartContext();

    TSharedRef<FD3D12RayTracingScene> D3D12Scene = new FD3D12RayTracingScene(GetDevice(), InDesc);
    if (!D3D12Scene->Build(*DirectCommandContext, BuildInfo))
    {
        DEBUG_BREAK();
        D3D12Scene.Reset();
    }

    DirectCommandContext->RHIFinishContext();
    return D3D12Scene.ReleaseOwnership();
}

FRHIRayTracingGeometry* FD3D12RHI::RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& InDesc)
{
    FRayTracingGeometryBuildInfo BuildInfo;
    BuildInfo.VertexBuffer = InDesc.VertexBuffer;
    BuildInfo.NumVertices  = InDesc.NumVertices;
    BuildInfo.IndexBuffer  = InDesc.IndexBuffer;
    BuildInfo.NumIndices   = InDesc.NumIndices;
    BuildInfo.IndexFormat  = InDesc.IndexFormat;
    BuildInfo.bUpdate      = false;

    DirectCommandContext->RHIStartContext();

    TSharedRef<FD3D12RayTracingGeometry> D3D12Geometry = new FD3D12RayTracingGeometry(GetDevice(), InDesc);
    if (!D3D12Geometry->Build(*DirectCommandContext, BuildInfo))
    {
        DEBUG_BREAK();
        D3D12Geometry.Reset();
    }

    DirectCommandContext->RHIFinishContext();
    return D3D12Geometry.ReleaseOwnership();
}

FRHIShaderResourceView* FD3D12RHI::RHICreateShaderResourceView(const FRHITextureSRVDesc& InDesc)
{
    FD3D12Texture* D3D12Texture = GetD3D12Texture(InDesc.Texture);
    if (!D3D12Texture)
    {
        D3D12_ERROR("Texture cannot be nullptr");
        return nullptr;
    }

    const FRHITextureInfo& TextureInfo = D3D12Texture->GetInfo();
    CHECK(TextureInfo.IsShaderResource());
    CHECK(InDesc.Format != EFormat::Unknown);
    CHECK(IsTypelessFormat(InDesc.Format) == false);
    
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Format                  = ConvertFormat(InDesc.Format);
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    
    if (TextureInfo.IsTexture2D())
    {
        if (!TextureInfo.IsMultisampled())
        {
            Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MostDetailedMip     = InDesc.FirstMipLevel;
            Desc.Texture2D.MipLevels           = InDesc.NumMips;
            Desc.Texture2D.ResourceMinLODClamp = InDesc.MinLODClamp;
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
            Desc.Texture2DArray.MostDetailedMip     = InDesc.FirstMipLevel;
            Desc.Texture2DArray.MipLevels           = InDesc.NumMips;
            Desc.Texture2DArray.ResourceMinLODClamp = InDesc.MinLODClamp;
            Desc.Texture2DArray.FirstArraySlice     = InDesc.FirstArraySlice;
            Desc.Texture2DArray.ArraySize           = InDesc.NumSlices;
            Desc.Texture2DArray.PlaneSlice          = 0;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.FirstArraySlice = InDesc.FirstArraySlice;
            Desc.Texture2DMSArray.ArraySize       = InDesc.NumSlices;
        }
    }
    else if (TextureInfo.IsTextureCube())
    {
        Desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        Desc.TextureCube.MostDetailedMip     = InDesc.FirstMipLevel;
        Desc.TextureCube.MipLevels           = InDesc.NumMips;
        Desc.TextureCube.ResourceMinLODClamp = InDesc.MinLODClamp;
    }
    else if (TextureInfo.IsTextureCubeArray())
    {
        Desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        Desc.TextureCubeArray.MostDetailedMip     = InDesc.FirstMipLevel;
        Desc.TextureCubeArray.MipLevels           = InDesc.NumMips;
        Desc.TextureCubeArray.ResourceMinLODClamp = InDesc.MinLODClamp;
        Desc.TextureCubeArray.First2DArrayFace    = InDesc.FirstArraySlice * RHI_NUM_CUBE_FACES;
        Desc.TextureCubeArray.NumCubes            = InDesc.NumSlices;
    }
    else if (TextureInfo.IsTexture3D())
    {
        Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MostDetailedMip     = InDesc.FirstMipLevel;
        Desc.Texture3D.MipLevels           = InDesc.NumMips;
        Desc.Texture3D.ResourceMinLODClamp = InDesc.MinLODClamp;
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

FRHIShaderResourceView* FD3D12RHI::RHICreateShaderResourceView(const FRHIBufferSRVDesc& InDesc)
{
    FD3D12Buffer* D3D12Buffer = GetD3D12Buffer(InDesc.Buffer);
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
    Desc.Buffer.FirstElement     = InDesc.FirstElement;
    Desc.Buffer.NumElements      = InDesc.NumElements;

    if (InDesc.Format == EBufferSRVFormat::None)
    {
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = InDesc.Buffer->GetStride();
    }
    else
    {
        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }

    FD3D12ShaderResourceViewRef D3D12View = new FD3D12ShaderResourceView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), InDesc.Buffer);
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

FRHIUnorderedAccessView* FD3D12RHI::RHICreateUnorderedAccessView(const FRHITextureUAVDesc& InDesc)
{
    FD3D12Texture* Texture = GetD3D12Texture(InDesc.Texture);
    if (!Texture)
    {
        D3D12_ERROR("Texture cannot be nullptr");
        return nullptr;
    }

    const FRHITextureInfo& TextureInfo = Texture->GetInfo();
    CHECK(TextureInfo.IsUnorderedAccess());
    CHECK(InDesc.Format != EFormat::Unknown);
    CHECK(IsTypelessFormat(InDesc.Format) == false);

    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Format = ConvertFormat(InDesc.Format);

    if (TextureInfo.IsTexture2D())
    {
        if (!TextureInfo.IsMultisampled())
        {
            Desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice   = InDesc.MipLevel;
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
            Desc.Texture2DArray.MipSlice        = InDesc.MipLevel;
            Desc.Texture2DArray.PlaneSlice      = 0;
            Desc.Texture2DArray.FirstArraySlice = InDesc.FirstArraySlice;
            Desc.Texture2DArray.ArraySize       = InDesc.NumSlices;
        }
        else
        {
            D3D12_ERROR("MultiSampled Textures is not supported");
        }
    }
    else if (TextureInfo.IsTextureCube() || TextureInfo.IsTextureCubeArray())
    {
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = InDesc.MipLevel;
        Desc.Texture2DArray.PlaneSlice      = 0;
        Desc.Texture2DArray.FirstArraySlice = InDesc.FirstArraySlice * RHI_NUM_CUBE_FACES;
        Desc.Texture2DArray.ArraySize       = InDesc.NumSlices * RHI_NUM_CUBE_FACES;
    }
    else if (TextureInfo.IsTexture3D())
    {
        Desc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.FirstWSlice = InDesc.FirstArraySlice;
        Desc.Texture3D.WSize       = InDesc.NumSlices;
        Desc.Texture3D.MipSlice    = InDesc.MipLevel;
    }

    FD3D12UnorderedAccessViewRef D3D12View = new FD3D12UnorderedAccessView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), InDesc.Texture);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    FD3D12Resource* D3D12Resource = GetD3D12Resource(InDesc.Texture);
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

FRHIUnorderedAccessView* FD3D12RHI::RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& InDesc)
{
    FD3D12Buffer* D3D12Buffer = GetD3D12Buffer(InDesc.Buffer);
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
    Desc.Buffer.FirstElement = InDesc.FirstElement;
    Desc.Buffer.NumElements  = InDesc.NumElements;

    if (InDesc.Format == EBufferUAVFormat::None)
    {
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = InDesc.Buffer->GetStride();
    }
    else
    {
        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }

    FD3D12UnorderedAccessViewRef D3D12View = new FD3D12UnorderedAccessView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), InDesc.Buffer);
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

FRHIVertexInputLayout* FD3D12RHI::RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& InInitializer)
{
    return new FD3D12VertexInputLayout(InInitializer);
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
    if (!Viewport->Initialize())
    {
        return nullptr;
    }
    else
    {
        return Viewport.ReleaseOwnership();
    }
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
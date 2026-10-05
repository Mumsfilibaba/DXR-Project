#include "Core/Containers/UniquePtr.h"
#include "Core/Math/Math.h"
#include "Core/Threading/ScopedLock.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIStats.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalCapabilities.h"
#include "MetalRHI/MetalDeviceDebug.h"
#include "MetalRHI/MetalQueue.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

IMPLEMENT_ENGINE_MODULE(FMetalModuleRHI, MetalRHI);

FMetalDeviceRHI* FMetalDeviceRHI::MetalDeviceRHI = nullptr;

FRHIDevice* FMetalModuleRHI::CreateDevice()
{
    TUniquePtr<FMetalDeviceRHI> NewRHI = MakeUniquePtr<FMetalDeviceRHI>();

    if (!NewRHI->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewRHI.Release();
    }
}

ERHIType FMetalDeviceRHI::GetRHIType() const
{
    return ERHIType::Metal;
}

FMetalDeviceRHI::FMetalDeviceRHI()
    : FRHIDevice()
    , Device(nullptr)
    , CommandContext(nullptr)
{
    if (!MetalDeviceRHI)
    {
        MetalDeviceRHI = this;
    }
}

void FMetalDeviceRHI::FlushDeferredDeletions()
{
    if (FRHICommandListExecutor::IsInitialized())
    {
        FRHICommandListExecutor::Get().FlushDeletedResources();
    }

    while (!DeferredObjects.IsEmpty())
    {
        TArray<FMetalDeferredObject> Items;
        {
            TScopedLock Lock(DeferredObjectsCS);
            Items = Move(DeferredObjects);
        }

        FMetalDeferredObject::ProcessItems(Items);

        if (FRHICommandListExecutor::IsInitialized())
        {
            FRHICommandListExecutor::Get().FlushDeletedResources();
        }
    }
}

void FMetalDeviceRHI::FlushDeletionQueue(FMetalCommands* Commands)
{
    TScopedLock Lock(DeferredObjectsCS);

    if (!Commands)
    {
        return;
    }

    if (Commands->DeferredObjects.IsEmpty())
    {
        Commands->DeferredObjects = Move(DeferredObjects);
    }
    else if (!DeferredObjects.IsEmpty())
    {
        Commands->DeferredObjects.Append(DeferredObjects);
        DeferredObjects.Clear();
    }
}

FMetalDeviceRHI::~FMetalDeviceRHI()
{
    if (Device)
    {
        Device->WaitForGPU();
    }

    FlushDeferredDeletions();
    UAVClearPipelines.Release();

    if (CommandContext && Device)
    {
        Device->GetQueue(EMetalQueueType::Direct)->ReleaseCommandContext(CommandContext);
        CommandContext = nullptr;
    }

    {
        TScopedLock Lock(SamplerStateMapCS);
        SamplerStateMap.Clear();
    }

    FlushDeferredDeletions();

    SAFE_DELETE(Device);

    FlushDeferredDeletions();

#if METAL_ENABLE_DEBUG_LAYER
    MetalStopValidationCapture();
#endif

    if (MetalDeviceRHI == this)
    {
        MetalDeviceRHI = nullptr;
    }
}

bool FMetalDeviceRHI::InitializeDeviceFeatureSupport()
{
    RHI::bSupportsGeometryShaders                       = false;
    RHI::bSupportsTessellation                          = false;
    RHI::MaxPatchControlPoints                          = 0;
    RHI::bSupportRenderTargetArrayIndexFromVertexShader = true;
    RHI::MaxShaderModel                                 = EShaderModel::SM_6_6;
    RHI::bSupportsBindless                              = GMetalSupportsBindless;

    // The GPU can amplify vertices (GMetalFeatures.MaxVertexAmplificationCount), but SV_ViewID never reaches
    // the shader: SPIRV-Cross cannot emit [[amplification_id]], and without its multiview option it compiles
    // SV_ViewID to a constant 0, so every amplified view would render view 0. Re-enabling this needs SPIRV-Cross
    // multiview emulation (multiview plus multiview_layered_rendering and a view-mask buffer, as MoltenVK does),
    // which derives the view from the instance index and is not capped at the amplification count.
    RHI::bSupportsViewInstancing = false;
    RHI::MaxViewInstanceCount    = 1;

    RHI::bSupportsRayTracing                                       = GMetalFeatures.bRayTracing;
    RHI::bSupportsRayTracingPipeline                               = false;
    RHI::RayTracingTier                                            = ERayTracingTier::NotSupported;
    RHI::RayTracingMaxRecursionDepth                               = 0;
    RHI::bSupportsInlineRayTracing                                 = GMetalFeatures.bRayTracing;
    RHI::bSupportsOpacityMicromap                                  = false;
    RHI::bSupportsShaderExecutionReordering                        = false;
    RHI::bShaderExecutionReorderingActuallyReorders                = false;
    RHI::bSupportsRayTracingPipelineAdditions                      = false;
    RHI::bSupportsClustersAndPartitionedSceneAccelerationStructure = false;
    RHI::bSupportsIndirectAccelerationStructureOperations          = false;
    RHI::bSupportsDispatchRaysIndirect                             = false;
    RHI::RayTracingMaxTrianglesPerCluster                          = 0;
    RHI::RayTracingMaxVerticesPerCluster                           = 0;
    RHI::RayTracingMaxPartitionedInstanceCount                     = 0;
    RHI::bSupportsShaderBindingTableDescriptors                    = false;
    RHI::bSupportsToolsVisualization                               = false;

    RHI::bSupportsTransparentSwapChain = true;

    RHI::bSupportsVRS             = false;
    RHI::ShadingRateTier          = EShadingRateTier::NotSupported;
    RHI::ShadingRateImageTileSize = 0;

    RHI::bSupportsSamplerFeedback = false;
    RHI::SamplerFeedbackTier      = ESamplerFeedbackTier::NotSupported;

    RHI::bSupportsProgrammableSamplePositions = GMetalSupportsProgrammableSamplePositions;
    RHI::SamplePositionsTier                  = GMetalSupportsProgrammableSamplePositions
        ? ESamplePositionsTier::Tier1
        : ESamplePositionsTier::NotSupported;
    RHI::MaxSamplePositionGridWidth           = GMetalSupportsProgrammableSamplePositions ? 1u : 0u;
    RHI::MaxSamplePositionGridHeight          = GMetalSupportsProgrammableSamplePositions ? 1u : 0u;
    RHI::SupportedSamplePositionSampleCounts  = 0;

    if (GMetalSupportsProgrammableSamplePositions)
    {
        uint32 SampleCounts = 0;

        if (QuerySupportedSampleCounts(EFormat::B8G8R8A8_Unorm, SampleCounts))
        {
            RHI::SupportedSamplePositionSampleCounts = SampleCounts;
        }
    }

    RHI::bSupportsDrawIndirect               = true;
    RHI::bSupportsDrawIndirectCount          = false;
    RHI::bSupportsDispatchIndirect           = true;
    RHI::bSupportsDispatchMeshIndirect       = GMetalSupportsMeshShaders;
    RHI::bSupportsDispatchMeshIndirectCount  = false;
    RHI::MaxDrawIndirectCommandCount         = uint32(~0u);
    RHI::MaxDispatchMeshIndirectCommandCount = GMetalSupportsMeshShaders ? uint32(~0u) : 0;

    RHI::MaxTexture1DSize        = GMetalMaxTexture2DSize;
    RHI::MaxTexture1DArrayLayers = GMetalMaxTextureArrayLayers;
    RHI::MaxTexture2DSize        = GMetalMaxTexture2DSize;
    RHI::MaxTexture2DArrayLayers = GMetalMaxTextureArrayLayers;
    RHI::MaxTexture3DWidth       = GMetalMaxTexture3DSize;
    RHI::MaxTexture3DHeight      = GMetalMaxTexture3DSize;
    RHI::MaxTexture3DDepth       = GMetalMaxTexture3DSize;
    RHI::MaxCubeTextureSize      = GMetalMaxTexture2DSize;
    RHI::MaxCubeArrayCount       = GMetalMaxTextureArrayLayers / RHI_NUM_CUBE_FACES;

    RHI::MaxBufferSize                        = GMetalMaxBufferLength;
    RHI::MaxConstantBufferSize                = 64 * 1024;
    RHI::MaxStorageBufferSize                 = GMetalMaxBufferLength;
    RHI::StructuredBufferMinStride            = 4;
    RHI::StructuredBufferMaxStride            = 2048;
    RHI::RawBufferRequiredAlignment           = 4;
    RHI::AccelerationStructureBufferAlignment = 256;

    RHI::bSupportsDynamicDepthBias = true;
    RHI::bSupportsDepthBoundsTest  = false;
    RHI::bSupportsStreamOutput     = false;

    RHI::bSupportsTimestampQueries           = GMetalSupportsTimestampQueries;
    RHI::bSupportsPipelineStatisticsQueries  = false;
    RHI::bSupportsGPUTimestampBubblesRemoval = false;

    RHI::DefaultSwapChainFormat = EFormat::B8G8R8A8_Unorm;

    // A feature reported to the renderer without the hardware behind it would otherwise only fail inside a pass
    CHECK(!RHI::bSupportsBindless || (MetalRHI::SupportsMetal3() && GMetalFeatures.ArgumentBuffersTier == MTLArgumentBuffersTier2));
    CHECK(!RHI::bSupportsDispatchMeshIndirect || MetalRHI::SupportsMetal3());
    CHECK(!RHI::bSupportsProgrammableSamplePositions || GMetalSupportsProgrammableSamplePositions);
    CHECK(!RHI::bSupportsTimestampQueries || GMetalSupportsCounterSampling);
    CHECK(!RHI::bSupportsViewInstancing || GMetalFeatures.MaxVertexAmplificationCount >= RHI::MaxViewInstanceCount);
    CHECK(!RHI::bSupportsRayTracing || GMetalFeatures.bRayTracing);
    CHECK(!RHI::bSupportsInlineRayTracing || GMetalFeatures.bRayTracing);

    return true;
}

bool FMetalDeviceRHI::Initialize()
{
#if METAL_ENABLE_DEBUG_LAYER
    MetalEnableDebugLayer();
    MetalStartValidationCapture();
#endif

    Device = new FMetalDevice();

    if (!Device->Initialize())
    {
        METAL_ERROR("Failed to initialize FMetalDevice");
        return false;
    }

    METAL_INFO("Created FMetalDevice");

    CommandContext = Device->GetQueue(EMetalQueueType::Direct)->ObtainCommandContext();

    if (!CommandContext)
    {
        METAL_ERROR("Failed to initialize FMetalCommandContext");
        return false;
    }

    if (!InitializeDeviceFeatureSupport())
    {
        METAL_ERROR("Failed to initialize device feature support");
        return false;
    }

    DumpMetalCapabilities();
    return true;
}

FRHITexture* FMetalDeviceRHI::CreateTexture(const FRHITextureDesc& InTextureDesc, ERHIResourceState InInitialState, const IRHITextureData* InInitialData)
{
    FMetalTextureRef NewTexture = new FMetalTextureRHI(GetMetalDevice(), InTextureDesc);

    if (!NewTexture->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewTexture.ReleaseOwnership();
    }
}

FRHIBuffer* FMetalDeviceRHI::CreateBuffer(const FRHIBufferDesc& InBufferDesc, ERHIResourceState InInitialState, const void* InInitialData)
{
    FMetalBufferRef NewBuffer = new FMetalBufferRHI(GetMetalDevice(), InBufferDesc);

    if (!NewBuffer->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

FRHISamplerState* FMetalDeviceRHI::CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc)
{
    TScopedLock Lock(SamplerStateMapCS);

    TSharedRef<FMetalSamplerStateRHI> Result;

    if (TSharedRef<FMetalSamplerStateRHI>* ExistingSamplerState = SamplerStateMap.Find(InSamplerDesc))
    {
        Result = *ExistingSamplerState;
    }
    else
    {
        Result = new FMetalSamplerStateRHI(GetMetalDevice(), InSamplerDesc);

        if (!Result->Initialize())
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

FRHISceneAccelerationStructure* FMetalDeviceRHI::CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc)
{
    if (!RHI::bSupportsRayTracing)
    {
        return nullptr;
    }

    FRHISceneAccelerationStructureBuildDesc BuildDesc;
    BuildDesc.Instances    = InSceneDesc.Instances.Data();
    BuildDesc.NumInstances = InSceneDesc.Instances.Size();
    BuildDesc.bUpdate      = false;

    FMetalSceneAccelerationStructureRHIRef MetalScene = new FMetalSceneAccelerationStructureRHI(GetMetalDevice(), InSceneDesc);

    {
        FMetalScopedCommandContext BuildContext(*GetMetalDevice()->GetQueue(EMetalQueueType::Direct));
        if (!MetalScene->Build(*BuildContext, BuildDesc))
        {
            METAL_ERROR("Failed to build the scene acceleration structure");
            MetalScene.Reset();
        }
    }

    return MetalScene.ReleaseOwnership();
}

FRHIGeometryAccelerationStructure* FMetalDeviceRHI::CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
{
    if (!RHI::bSupportsRayTracing)
    {
        return nullptr;
    }

    FMetalGeometryAccelerationStructureRHIRef MetalGeometry = new FMetalGeometryAccelerationStructureRHI(GetMetalDevice(), InGeometryDesc);

    if (InGeometryDesc.GeometryType == ERayTracingGeometryType::ProceduralAABBs)
    {
        return MetalGeometry.ReleaseOwnership();
    }

    FRHIGeometryAccelerationStructureBuildDesc BuildDesc;
    BuildDesc.VertexBuffer = InGeometryDesc.VertexBuffer;
    BuildDesc.NumVertices  = InGeometryDesc.NumVertices;
    BuildDesc.IndexBuffer  = InGeometryDesc.IndexBuffer;
    BuildDesc.NumIndices   = InGeometryDesc.NumIndices;
    BuildDesc.IndexFormat  = InGeometryDesc.IndexFormat;
    BuildDesc.bUpdate      = false;

    {
        FMetalScopedCommandContext BuildContext(*GetMetalDevice()->GetQueue(EMetalQueueType::Direct));
        if (!MetalGeometry->Build(*BuildContext, BuildDesc))
        {
            METAL_ERROR("Failed to build the geometry acceleration structure");
            MetalGeometry.Reset();
        }
    }

    return MetalGeometry.ReleaseOwnership();
}

FRHIShaderResourceView* FMetalDeviceRHI::CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc)
{
    if (!InResource)
    {
        return nullptr;
    }

    if (!InDesc.IsBufferSRV() && !InDesc.IsTextureSRV() && !InDesc.IsAccelerationStructureSRV())
    {
        return nullptr;
    }

    TSharedRef<FMetalShaderResourceViewRHI> NewView = new FMetalShaderResourceViewRHI(GetMetalDevice(), InResource, InDesc);

    if (!NewView->Initialize())
    {
        return nullptr;
    }

    return NewView.ReleaseOwnership();
}

FRHIRenderTargetView* FMetalDeviceRHI::CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc)
{
    if (!InResource || InResource->GetResourceType() != ERHIResourceType::Texture)
    {
        return nullptr;
    }

    FRHITexture* Texture = static_cast<FRHITexture*>(InResource);

    TSharedRef<FMetalRenderTargetViewRHI> NewView = new FMetalRenderTargetViewRHI(GetMetalDevice(), Texture, InDesc);

    if (!NewView->Initialize())
    {
        return nullptr;
    }

    return NewView.ReleaseOwnership();
}

FRHIDepthStencilView* FMetalDeviceRHI::CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc)
{
    if (!InResource || InResource->GetResourceType() != ERHIResourceType::Texture)
    {
        return nullptr;
    }
    
    FRHITexture* Texture = static_cast<FRHITexture*>(InResource);

    TSharedRef<FMetalDepthStencilViewRHI> NewView = new FMetalDepthStencilViewRHI(GetMetalDevice(), Texture, InDesc);

    if (!NewView->Initialize())
    {
        return nullptr;
    }

    return NewView.ReleaseOwnership();
}

FRHIUnorderedAccessView* FMetalDeviceRHI::CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
{
    if (!InResource)
    {
        return nullptr;
    }

    if (!InDesc.IsBufferUAV() && !InDesc.IsTextureUAV())
    {
        return nullptr;
    }

    TSharedRef<FMetalUnorderedAccessViewRHI> NewView = new FMetalUnorderedAccessViewRHI(GetMetalDevice(), InResource, InDesc);

    if (!NewView->Initialize())
    {
        return nullptr;
    }

    return NewView.ReleaseOwnership();
}

FRHIUnorderedAccessView* FMetalDeviceRHI::CreateSamplerFeedbackUnorderedAccessView(FRHITexture* /* InFeedbackTexture */, FRHITexture* /* InTargetedTexture */)
{
    METAL_ERROR("CreateSamplerFeedbackUnorderedAccessView: sampler feedback is not supported by the Metal backend");
    return nullptr;
}

template<typename MetalShaderType>
MetalShaderType* FMetalDeviceRHI::CreateShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<MetalShaderType> NewShader = new MetalShaderType(GetMetalDevice());
    return NewShader->Initialize(ShaderCode) ? NewShader.ReleaseOwnership() : nullptr;
}

FRHIComputeShader* FMetalDeviceRHI::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return CreateShader<FMetalComputeShaderRHI>(ShaderCode);
}

FRHIVertexShader* FMetalDeviceRHI::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return CreateShader<FMetalVertexShaderRHI>(ShaderCode);
}

FRHIHullShader* FMetalDeviceRHI::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIDomainShader* FMetalDeviceRHI::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIGeometryShader* FMetalDeviceRHI::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIMeshShader* FMetalDeviceRHI::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    if (!GMetalSupportsMeshShaders)
    {
        return nullptr;
    }

    return CreateShader<FMetalMeshShaderRHI>(ShaderCode);
}

FRHIAmplificationShader* FMetalDeviceRHI::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    if (!GMetalSupportsMeshShaders)
    {
        return nullptr;
    }

    return CreateShader<FMetalAmplificationShaderRHI>(ShaderCode);
}

FRHIPixelShader* FMetalDeviceRHI::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return CreateShader<FMetalPixelShaderRHI>(ShaderCode);
}

FRHIRayGenShader* FMetalDeviceRHI::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return RHI::bSupportsRayTracingPipeline ? CreateShader<FMetalRayGenShaderRHI>(ShaderCode) : nullptr;
}

FRHIRayAnyHitShader* FMetalDeviceRHI::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return RHI::bSupportsRayTracingPipeline ? CreateShader<FMetalRayAnyHitShaderRHI>(ShaderCode) : nullptr;
}

FRHIRayClosestHitShader* FMetalDeviceRHI::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return RHI::bSupportsRayTracingPipeline ? CreateShader<FMetalRayClosestHitShaderRHI>(ShaderCode) : nullptr;
}

FRHIRayMissShader* FMetalDeviceRHI::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return RHI::bSupportsRayTracingPipeline ? CreateShader<FMetalRayMissShaderRHI>(ShaderCode) : nullptr;
}

FRHIRayIntersectionShader* FMetalDeviceRHI::CreateRayIntersectionShader(const TArray<uint8>& ShaderCode)
{
    return RHI::bSupportsRayTracingPipeline ? CreateShader<FMetalRayIntersectionShaderRHI>(ShaderCode) : nullptr;
}

FRHIRayCallableShader* FMetalDeviceRHI::CreateRayCallableShader(const TArray<uint8>& ShaderCode)
{
    return RHI::bSupportsRayTracingPipeline ? CreateShader<FMetalRayCallableShaderRHI>(ShaderCode) : nullptr;
}

FRHIDepthStencilState* FMetalDeviceRHI::CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
{
    return new FMetalDepthStencilStateRHI(InDesc);
}

FRHIRasterizerState* FMetalDeviceRHI::CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
{
    return new FMetalRasterizerStateRHI(InDesc);
}

FRHIBlendState* FMetalDeviceRHI::CreateBlendState(const FRHIBlendStateDesc& InDesc)
{
    return new FMetalBlendStateRHI(InDesc);
}

FRHIInputLayout* FMetalDeviceRHI::CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
{
    return new FMetalInputLayoutRHI(InInputElements);
}

FRHIGraphicsPipelineState* FMetalDeviceRHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
{
    FMetalGraphicsPipelineStateRef NewPipelineState = new FMetalGraphicsPipelineStateRHI(GetMetalDevice(), InDesc);

    if (!NewPipelineState->Initialize())
    {
        return nullptr;
    }
    
    return NewPipelineState.ReleaseOwnership();
}

FRHIComputePipelineState* FMetalDeviceRHI::CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
{
    FMetalComputePipelineStateRef NewPipelineState = new FMetalComputePipelineStateRHI(GetMetalDevice(), InDesc);

    if (!NewPipelineState->Initialize())
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

FRHIMeshletPipelineState* FMetalDeviceRHI::CreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc& InDesc)
{
    if (!GMetalSupportsMeshShaders)
    {
        return nullptr;
    }

    FMetalMeshletPipelineStateRef NewPipelineState = new FMetalMeshletPipelineStateRHI(GetMetalDevice(), InDesc);

    if (!NewPipelineState->Initialize())
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

FRHIRayTracingPipelineState* FMetalDeviceRHI::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& Desc)
{
    if (!RHI::bSupportsRayTracingPipeline)
    {
        return nullptr;
    }

    FMetalRayTracingPipelineStateRef NewPipelineState = new FMetalRayTracingPipelineStateRHI(GetMetalDevice(), Desc);

    if (!NewPipelineState->Initialize())
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

FRHIQuery* FMetalDeviceRHI::CreateQuery(EQueryType InQueryType)
{
    return new FMetalQueryRHI(GetMetalDevice(), InQueryType);
}

FRHISwapChain* FMetalDeviceRHI::CreateSwapChain(const FRHISwapChainDesc& SwapChainDesc)
{
    FCocoaWindow* Window = reinterpret_cast<FCocoaWindow*>(SwapChainDesc.WindowHandle);

    if (!Window)
    {
        return nullptr;
    }

    FRHISwapChainDesc NewViewportDesc(SwapChainDesc);

    if (SwapChainDesc.Width == 0 || SwapChainDesc.Height == 0)
    {
        __block NSRect Frame;
        __block NSRect ContentRect;
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            Frame       = Window.frame;
            ContentRect = [Window contentRectForFrameRect:Window.frame];
        }, NSDefaultRunLoopMode, true);
        
        NewViewportDesc.Width  = ContentRect.size.width;
        NewViewportDesc.Height = ContentRect.size.height;
    }
    
    FMetalSwapChainRef NewSwapChain = new FMetalSwapChainRHI(GetMetalDevice(), NewViewportDesc);

    if (!NewSwapChain->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewSwapChain.ReleaseOwnership();
    }
}

bool FMetalDeviceRHI::QueryUAVFormatSupport(EFormat Format) const
{
    const MTLPixelFormat PixelFormat = MetalRHI::ConvertFormat(Format);
    return MetalRHI::FormatSupportsShaderWrite(PixelFormat);
}

bool FMetalDeviceRHI::QuerySupportedSampleCounts(EFormat Format, uint32& OutSampleCounts) const
{
    OutSampleCounts = 0;

    if (!MetalRHI::FormatIsRenderable(MetalRHI::ConvertFormat(Format)))
    {
        return false;
    }

    OutSampleCounts = GMetalFeatures.SupportedSampleCounts;
    return OutSampleCounts != 0;
}

bool FMetalDeviceRHI::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const
{
    CHECK(Device != nullptr);
    return Device->QueryVideoMemoryInfo(MemoryType, OutMemoryInfo);
}

bool FMetalDeviceRHI::GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode)
{
    OutResult = FRHIPipelineStatistics();
    return false;
}

void FMetalDeviceRHI::BeginFrame()
{
    SCOPED_AUTORELEASE_POOL();

    Device->BeginFrame();
}

void FMetalDeviceRHI::EndFrame()
{
    SCOPED_AUTORELEASE_POOL();

    Device->EndFrame();

#if METAL_ENABLE_STATS
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
#endif
}

FRHIFence* FMetalDeviceRHI::CreateFence()
{
    return new FMetalFenceRHI(Device->GetMTLDevice());
}

IRHICommandContext* FMetalDeviceRHI::ObtainCommandContext()
{
    return CommandContext;
}

bool FMetalDeviceRHI::GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode)
{
    OutResult = 0;

    FMetalQueryRHI* MetalQuery = ResourceCast(Query);

    if (!MetalQuery || !MetalQuery->QueryResult)
    {
        return false;
    }

    if (MetalQuery->GetType() == EQueryType::PipelineStatistics)
    {
        return false;
    }

    if (!MetalQuery->bResolved)
    {
        FMetalQueue* Queue = MetalQuery->SubmittedQueue ? MetalQuery->SubmittedQueue : Device->GetQueue(EMetalQueueType::Direct);

        if (Mode == EQueryResultMode::Wait)
        {
            if (MetalQuery->SubmissionValue != 0)
            {
                Queue->WaitForValue(MetalQuery->SubmissionValue);
            }
            else
            {
                Queue->WaitForCompletion();
            }

            if (!MetalQuery->bResolved)
            {
                MetalQuery->Resolve();
            }
        }
        else
        {
            Queue->ProcessCommandQueue();
        }
    }

    if (!MetalQuery->bResolved)
    {
        return false;
    }

    OutResult = *MetalQuery->QueryResult;
    return true;
}

void FMetalDeviceRHI::EnqueueResourceDeletion(FRHIResource* Resource)
{
    DeferDeletion(Resource);
}

void* FMetalDeviceRHI::GetRHINativeAdapter()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetMTLDevice());
}

void* FMetalDeviceRHI::GetRHINativeDevice()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetMTLDevice());
}

void* FMetalDeviceRHI::GetRHINativeDirectCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetQueue(EMetalQueueType::Direct)->GetMTLCommandQueue());
}

void* FMetalDeviceRHI::GetRHINativeComputeCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetQueue(EMetalQueueType::Compute)->GetMTLCommandQueue());
}

void* FMetalDeviceRHI::GetRHINativeCopyCommandQueue()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetQueue(EMetalQueueType::Copy)->GetMTLCommandQueue());
}

String FMetalDeviceRHI::GetAdapterName() const
{
    CHECK(Device != nullptr);
    return Device->GetProperties().Name;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

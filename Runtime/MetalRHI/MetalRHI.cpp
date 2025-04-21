#include "MetalRHI.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

IMPLEMENT_ENGINE_MODULE(FMetalRHIModule, MetalRHI);

FRHI* FMetalRHIModule::CreateRHI()
{
    TUniquePtr<FMetalRHI> NewRHI = MakeUniquePtr<FMetalRHI>();
    if (NewRHI->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewRHI.Release();
    }
}

FMetalRHI* FMetalRHI::GMetalRHI = nullptr;

FMetalRHI::FMetalRHI()
    : FRHI(ERHIType::Metal)
    , CommandContext()
{
    if (!GMetalRHI)
    {
        GMetalRHI = this;
    }
}

FMetalRHI::~FMetalRHI()
{
    SAFE_DELETE(CommandContext);
    SAFE_DELETE(DeviceContext);

    if (GMetalRHI == this)
    {
        GMetalRHI = nullptr;
    }
}

bool FMetalRHI::Initialize()
{
    DeviceContext = FMetalDeviceContext::CreateContext();
    if (!DeviceContext)
    {
        METAL_ERROR("Failed to create DeviceContext");
        return false;
    }
    
    METAL_INFO("Created DeviceContext");
    
    CommandContext = FMetalCommandContext::CreateMetalContext(GetDeviceContext());
    if (!CommandContext)
    {
        METAL_ERROR("Failed to create CommandContext");
        return false;
    }
    
    return true;
}

FRHITexture* FMetalRHI::CreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FMetalTextureRef NewTexture = new FMetalTexture(GetDeviceContext(), InTextureInfo);
    if (!NewTexture->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewTexture.ReleaseOwnership();
    }
}

FRHIBuffer* FMetalRHI::CreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData)
{
    FMetalBufferRef NewBuffer = new FMetalBuffer(GetDeviceContext(), InBufferInfo);
    if (!NewBuffer->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

FRHISamplerState* FMetalRHI::CreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo)
{
    FMetalSamplerStateRef NewSamplerState = new FMetalSamplerState(GetDeviceContext(), InSamplerInfo);
    if (!NewSamplerState->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewSamplerState.ReleaseOwnership();
    }
}

FRHIRayTracingScene* FMetalRHI::CreateRayTracingScene(const FRHIRayTracingSceneInfo& Desc)
{
    return new FMetalRayTracingScene(GetDeviceContext(), Desc);
}

FRHIRayTracingGeometry* FMetalRHI::CreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo)
{
    return new FMetalRayTracingGeometry(InGeometryInfo);
}

FRHIShaderResourceView* FMetalRHI::CreateShaderResourceView(const FRHITextureSRVInfo& Info)
{
    return new FMetalShaderResourceView(GetDeviceContext(), Info.Texture);
}

FRHIShaderResourceView* FMetalRHI::CreateShaderResourceView(const FRHIBufferSRVInfo& Info)
{
    return new FMetalShaderResourceView(GetDeviceContext(), Info.Buffer);
}

FRHIUnorderedAccessView* FMetalRHI::CreateUnorderedAccessView(const FRHITextureUAVInfo& Info)
{
    return new FMetalUnorderedAccessView(GetDeviceContext(), Info.Texture);
}

FRHIUnorderedAccessView* FMetalRHI::CreateUnorderedAccessView(const FRHIBufferUAVInfo& Info)
{
    return new FMetalUnorderedAccessView(GetDeviceContext(), Info.Buffer);
}

FRHIComputeShader* FMetalRHI::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    FMetalComputeShaderRef NewShader = new FMetalComputeShader(GetDeviceContext());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIVertexShader* FMetalRHI::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    FMetalVertexShaderRef NewShader = new FMetalVertexShader(GetDeviceContext());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIHullShader* FMetalRHI::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIDomainShader* FMetalRHI::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIGeometryShader* FMetalRHI::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIMeshShader* FMetalRHI::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIAmplificationShader* FMetalRHI::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIPixelShader* FMetalRHI::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    FMetalPixelShaderRef NewShader = new FMetalPixelShader(GetDeviceContext());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayGenShader* FMetalRHI::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    FMetalRayGenShaderRef NewShader = new FMetalRayGenShader(GetDeviceContext());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayAnyHitShader* FMetalRHI::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    FMetalRayAnyHitShaderRef NewShader = new FMetalRayAnyHitShader(GetDeviceContext());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayClosestHitShader* FMetalRHI::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    FMetalRayClosestHitShaderRef NewShader = new FMetalRayClosestHitShader(GetDeviceContext());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayMissShader* FMetalRHI::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    FMetalRayMissShaderRef NewShader = new FMetalRayMissShader(GetDeviceContext());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDepthStencilState* FMetalRHI::CreateDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer)
{
    FMetalDepthStencilStateRef NewDepthStencilState = new FMetalDepthStencilState(GetDeviceContext(), InInitializer);
    if (!NewDepthStencilState->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewDepthStencilState.ReleaseOwnership();
    }
}

FRHIRasterizerState* FMetalRHI::CreateRasterizerState(const FRHIRasterizerStateInitializer& InInitializer)
{
    return new FMetalRasterizerState(InInitializer);
}

FRHIBlendState* FMetalRHI::CreateBlendState(const FRHIBlendStateInitializer& InInitializer)
{
    return new FMetalBlendState(InInitializer);
}

FRHIVertexLayout* FMetalRHI::CreateVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList)
{
    return new FMetalVertexLayout(InInitializerList);
}

FRHIGraphicsPipelineState* FMetalRHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer)
{
    return new FMetalGraphicsPipelineState(GetDeviceContext(), InInitializer);
}

FRHIComputePipelineState* FMetalRHI::CreateComputePipelineState(const FRHIComputePipelineStateInitializer& Desc)
{
    return new FMetalComputePipelineState();
}

FRHIRayTracingPipelineState* FMetalRHI::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Desc)
{
    return new FMetalRayTracingPipelineState();
}

FRHIQuery* FMetalRHI::CreateQuery(EQueryType InQueryType)
{
    return new FMetalQuery(InQueryType);
}

FRHIViewport* FMetalRHI::CreateViewport(const FRHIViewportInfo& ViewportInfo)
{
    FCocoaWindow* Window = reinterpret_cast<FCocoaWindow*>(ViewportInfo.WindowHandle);
    if (!Window)
    {
        return nullptr;
    }

    FRHIViewportInfo NewViewportInfo(ViewportInfo);
    if (ViewportInfo.Width == 0 || ViewportInfo.Height == 0)
    {
        __block NSRect Frame;
        __block NSRect ContentRect;
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            Frame       = Window.frame;
            ContentRect = [Window contentRectForFrameRect:Window.frame];
        }, NSDefaultRunLoopMode, true);
        
        NewViewportInfo.Width  = ContentRect.size.width;
        NewViewportInfo.Height = ContentRect.size.height;
    }
    
    FMetalViewportRef NewViewport = new FMetalViewport(GetDeviceContext(), NewViewportInfo);
    if (!NewViewport->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewViewport.ReleaseOwnership();
    }
}

bool FMetalRHI::QueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

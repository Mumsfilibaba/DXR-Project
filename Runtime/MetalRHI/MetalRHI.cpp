#include "MetalRHI.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

IMPLEMENT_ENGINE_MODULE(FMetalRHIModule, MetalRHI);

FRHI* FMetalRHIModule::CreateRHI()
{
    return new FMetalRHI();
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

FRHITexture* FMetalRHI::RHICreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FMetalTextureRef NewTexture = new FMetalTexture(GetDeviceContext(), InDesc);
    if (!NewTexture->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewTexture.ReleaseOwnership();
    }
}

FRHIBuffer* FMetalRHI::RHICreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData)
{
    FMetalBufferRef NewBuffer = new FMetalBuffer(GetDeviceContext(), InDesc);
    if (!NewBuffer->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

FRHISamplerState* FMetalRHI::RHICreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo)
{
    FMetalSamplerStateRef NewSamplerState = new FMetalSamplerState(GetDeviceContext(), Desc);
    if (!NewSamplerState->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewSamplerState.ReleaseOwnership();
    }
}

FRHIRayTracingScene* FMetalRHI::RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& Desc)
{
    return new FMetalRayTracingScene(GetDeviceContext(), Desc);
}

FRHIRayTracingGeometry* FMetalRHI::RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& Desc)
{
    return new FMetalRayTracingGeometry(Desc);
}

FRHIShaderResourceView* FMetalRHI::RHICreateShaderResourceView(const FRHITextureSRVDesc& Desc)
{
    return new FMetalShaderResourceView(GetDeviceContext(), Desc.Texture);
}

FRHIShaderResourceView* FMetalRHI::RHICreateShaderResourceView(const FRHIBufferSRVDesc& Desc)
{
    return new FMetalShaderResourceView(GetDeviceContext(), Desc.Buffer);
}

FRHIUnorderedAccessView* FMetalRHI::RHICreateUnorderedAccessView(const FRHITextureUAVDesc& Desc)
{
    return new FMetalUnorderedAccessView(GetDeviceContext(), Desc.Texture);
}

FRHIUnorderedAccessView* FMetalRHI::RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& Desc)
{
    return new FMetalUnorderedAccessView(GetDeviceContext(), Desc.Buffer);
}

FRHIComputeShader* FMetalRHI::RHICreateComputeShader(const TArray<uint8>& ShaderCode)
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

FRHIVertexShader* FMetalRHI::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
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

FRHIHullShader* FMetalRHI::RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIDomainShader* FMetalRHI::RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIGeometryShader* FMetalRHI::RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIMeshShader* FMetalRHI::RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIAmplificationShader* FMetalRHI::RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIPixelShader* FMetalRHI::RHICreatePixelShader(const TArray<uint8>& ShaderCode)
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

FRHIRayGenShader* FMetalRHI::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
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

FRHIRayAnyHitShader* FMetalRHI::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
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

FRHIRayClosestHitShader* FMetalRHI::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
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

FRHIRayMissShader* FMetalRHI::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
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

FRHIDepthStencilState* FMetalRHI::RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer)
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

FRHIRasterizerState* FMetalRHI::RHICreateRasterizerState(const FRHIRasterizerStateInitializer& InInitializer)
{
    return new FMetalRasterizerState(InInitializer);
}

FRHIBlendState* FMetalRHI::RHICreateBlendState(const FRHIBlendStateInitializer& InInitializer)
{
    return new FMetalBlendState(InInitializer);
}

FRHIVertexInputLayout* FMetalRHI::RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& InInitializer)
{
    return new FMetalVertexInputLayout(InInitializer);
}

FRHIGraphicsPipelineState* FMetalRHI::RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer)
{
    return new FMetalGraphicsPipelineState(GetDeviceContext(), InInitializer);
}

FRHIComputePipelineState* FMetalRHI::RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& Desc)
{
    return new FMetalComputePipelineState();
}

FRHIRayTracingPipelineState* FMetalRHI::RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Desc)
{
    return new FMetalRayTracingPipelineState();
}

FRHIQuery* FMetalRHI::RHICreateQuery()
{
    return new FMetalQuery();
}

FRHIViewport* FMetalRHI::RHICreateViewport(const FRHIViewportInfo& ViewportInfo)
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
        ExecuteOnMainThread(^
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

bool FMetalRHI::RHIQueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

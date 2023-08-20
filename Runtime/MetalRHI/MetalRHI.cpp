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

FRHITexture* FMetalRHI::RHICreateTexture(const FRHITextureDesc& InDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FMetalTextureRef NewTexture = new FMetalTexture(GetDeviceContext(), InDesc);
    if (!NewTexture->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }

    return NewTexture.ReleaseOwnership();
}

FRHISamplerState* FMetalRHI::RHICreateSamplerState(const FRHISamplerStateDesc& Desc)
{
    return new FMetalSamplerState(Desc);
}

FRHIBuffer* FMetalRHI::RHICreateBuffer(const FRHIBufferDesc& InDesc, EResourceAccess InInitialState, const void* InInitialData)
{
    TSharedRef<FMetalBuffer> NewBuffer = new FMetalBuffer(GetDeviceContext(), InDesc);
    if (!NewBuffer->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }

    return NewBuffer.ReleaseOwnership();
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
    return new FMetalComputeShader(GetDeviceContext(), ShaderCode);
}

FRHIVertexShader* FMetalRHI::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return new FMetalVertexShader(GetDeviceContext(), ShaderCode);
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
    return new FMetalPixelShader(GetDeviceContext(), ShaderCode);
}

FRHIRayGenShader* FMetalRHI::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return new FMetalRayGenShader(GetDeviceContext(), ShaderCode);
}

FRHIRayAnyHitShader* FMetalRHI::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return new FMetalRayAnyHitShader(GetDeviceContext(), ShaderCode);
}

FRHIRayClosestHitShader* FMetalRHI::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return new FMetalRayClosestHitShader(GetDeviceContext(), ShaderCode);
}

FRHIRayMissShader* FMetalRHI::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return new FMetalRayMissShader(GetDeviceContext(), ShaderCode);
}

FRHIDepthStencilState* FMetalRHI::RHICreateDepthStencilState(const FRHIDepthStencilStateDesc& Desc)
{
    return new FMetalDepthStencilState(GetDeviceContext(), Desc);
}

FRHIRasterizerState* FMetalRHI::RHICreateRasterizerState(const FRHIRasterizerStateDesc& Desc)
{
    return new FMetalRasterizerState(GetDeviceContext(), Desc);
}

FRHIBlendState* FMetalRHI::RHICreateBlendState(const FRHIBlendStateDesc& Desc)
{
    return new FMetalBlendState(GetDeviceContext(), Desc);
}

FRHIVertexInputLayout* FMetalRHI::RHICreateVertexInputLayout(const FRHIVertexInputLayoutDesc& Desc)
{
    return new FMetalInputLayoutState(GetDeviceContext(), Desc);
}

FRHIGraphicsPipelineState* FMetalRHI::RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& Desc)
{
    return new FMetalGraphicsPipelineState(GetDeviceContext(), Desc);
}

FRHIComputePipelineState* FMetalRHI::RHICreateComputePipelineState(const FRHIComputePipelineStateDesc& Desc)
{
    return new FMetalComputePipelineState();
}

FRHIRayTracingPipelineState* FMetalRHI::RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& Desc)
{
    return new FMetalRayTracingPipelineState();
}

FRHITimestampQuery* FMetalRHI::RHICreateTimestampQuery()
{
    return new FMetalTimestampQuery();
}

FRHIViewport* FMetalRHI::RHICreateViewport(const FRHIViewportDesc& Desc)
{
    FCocoaWindow* Window = reinterpret_cast<FCocoaWindow*>(Desc.WindowHandle);
    
    __block NSRect Frame;
    __block NSRect ContentRect;
    ExecuteOnMainThread(^
    {
        Frame       = Window.frame;
        ContentRect = [Window contentRectForFrameRect:Window.frame];
    }, NSDefaultRunLoopMode, true);
    
    FRHIViewportDesc NewDesc(Desc);
    NewDesc.Width  = ContentRect.size.width;
    NewDesc.Height = ContentRect.size.height;
    
    return new FMetalViewport(GetDeviceContext(), NewDesc);
}

void FMetalRHI::RHIQueryRayTracingSupport(FRHIRayTracingSupport& OutSupport) const
{
    OutSupport = FRHIRayTracingSupport();
}

void FMetalRHI::RHIQueryShadingRateSupport(FRHIShadingRateSupport& OutSupport) const
{
    OutSupport = FRHIShadingRateSupport();
}

bool FMetalRHI::RHIQueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

#include "MetalInterface.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

IMPLEMENT_ENGINE_MODULE(FMetalInterfaceModule, MetalRHI);

FRHIInterface* FMetalInterfaceModule::CreateInterface()
{
    return new FMetalInterface();
}


FMetalInterface* FMetalInterface::GMetalInterface = nullptr;

FMetalInterface::FMetalInterface()
    : FRHIInterface(ERHIInstanceType::Metal)
    , CommandContext()
{
    if (!GMetalInterface)
    {
        GMetalInterface = this;
    }
}

FMetalInterface::~FMetalInterface()
{
    SAFE_DELETE(CommandContext);
    SAFE_DELETE(DeviceContext);

    if (GMetalInterface == this)
    {
        GMetalInterface = nullptr;
    }
}

bool FMetalInterface::Initialize()
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

FRHITexture* FMetalInterface::RHICreateTexture(const FRHITextureDesc& InDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FMetalTextureRef NewTexture = new FMetalTexture(GetDeviceContext(), InDesc);
    if (!NewTexture->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }

    return NewTexture.ReleaseOwnership();
}

FRHISamplerState* FMetalInterface::RHICreateSamplerState(const FRHISamplerStateDesc& Desc)
{
    return new FMetalSamplerState(Desc);
}

FRHIBuffer* FMetalInterface::RHICreateBuffer(const FRHIBufferDesc& InDesc, EResourceAccess InInitialState, const void* InInitialData)
{
    TSharedRef<FMetalBuffer> NewBuffer = new FMetalBuffer(GetDeviceContext(), InDesc);
    if (!NewBuffer->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }

    return NewBuffer.ReleaseOwnership();
}

FRHIRayTracingScene* FMetalInterface::RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& Desc)
{
    return new FMetalRayTracingScene(GetDeviceContext(), Desc);
}

FRHIRayTracingGeometry* FMetalInterface::RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& Desc)
{
    return new FMetalRayTracingGeometry(Desc);
}

FRHIShaderResourceView* FMetalInterface::RHICreateShaderResourceView(const FRHITextureSRVDesc& Desc)
{
    return new FMetalShaderResourceView(GetDeviceContext(), Desc.Texture);
}

FRHIShaderResourceView* FMetalInterface::RHICreateShaderResourceView(const FRHIBufferSRVDesc& Desc)
{
    return new FMetalShaderResourceView(GetDeviceContext(), Desc.Buffer);
}

FRHIUnorderedAccessView* FMetalInterface::RHICreateUnorderedAccessView(const FRHITextureUAVDesc& Desc)
{
    return new FMetalUnorderedAccessView(GetDeviceContext(), Desc.Texture);
}

FRHIUnorderedAccessView* FMetalInterface::RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& Desc)
{
    return new FMetalUnorderedAccessView(GetDeviceContext(), Desc.Buffer);
}

FRHIComputeShader* FMetalInterface::RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return new FMetalComputeShader(GetDeviceContext(), ShaderCode);
}

FRHIVertexShader* FMetalInterface::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return new FMetalVertexShader(GetDeviceContext(), ShaderCode);
}

FRHIHullShader* FMetalInterface::RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIDomainShader* FMetalInterface::RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIGeometryShader* FMetalInterface::RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIMeshShader* FMetalInterface::RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIAmplificationShader* FMetalInterface::RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return nullptr;
}

FRHIPixelShader* FMetalInterface::RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return new FMetalPixelShader(GetDeviceContext(), ShaderCode);
}

FRHIRayGenShader* FMetalInterface::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return new FMetalRayGenShader(GetDeviceContext(), ShaderCode);
}

FRHIRayAnyHitShader* FMetalInterface::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return new FMetalRayAnyHitShader(GetDeviceContext(), ShaderCode);
}

FRHIRayClosestHitShader* FMetalInterface::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return new FMetalRayClosestHitShader(GetDeviceContext(), ShaderCode);
}

FRHIRayMissShader* FMetalInterface::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return new FMetalRayMissShader(GetDeviceContext(), ShaderCode);
}

FRHIDepthStencilState* FMetalInterface::RHICreateDepthStencilState(const FRHIDepthStencilStateDesc& Desc)
{
    return new FMetalDepthStencilState(GetDeviceContext(), Desc);
}

FRHIRasterizerState* FMetalInterface::RHICreateRasterizerState(const FRHIRasterizerStateDesc& Desc)
{
    return new FMetalRasterizerState(GetDeviceContext(), Desc);
}

FRHIBlendState* FMetalInterface::RHICreateBlendState(const FRHIBlendStateDesc& Desc)
{
    return new FMetalBlendState(GetDeviceContext(), Desc);
}

FRHIVertexInputLayout* FMetalInterface::RHICreateVertexInputLayout(const FRHIVertexInputLayoutDesc& Desc)
{
    return new FMetalInputLayoutState(GetDeviceContext(), Desc);
}

FRHIGraphicsPipelineState* FMetalInterface::RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& Desc)
{
    return new FMetalGraphicsPipelineState(GetDeviceContext(), Desc);
}

FRHIComputePipelineState* FMetalInterface::RHICreateComputePipelineState(const FRHIComputePipelineStateDesc& Desc)
{
    return new FMetalComputePipelineState();
}

FRHIRayTracingPipelineState* FMetalInterface::RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& Desc)
{
    return new FMetalRayTracingPipelineState();
}

FRHITimestampQuery* FMetalInterface::RHICreateTimestampQuery()
{
    return new FMetalTimestampQuery();
}

FRHIViewport* FMetalInterface::RHICreateViewport(const FRHIViewportDesc& Desc)
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

void FMetalInterface::RHIQueryRayTracingSupport(FRHIRayTracingSupport& OutSupport) const
{
    OutSupport = FRHIRayTracingSupport();
}

void FMetalInterface::RHIQueryShadingRateSupport(FRHIShadingRateSupport& OutSupport) const
{
    OutSupport = FRHIShadingRateSupport();
}

bool FMetalInterface::RHIQueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

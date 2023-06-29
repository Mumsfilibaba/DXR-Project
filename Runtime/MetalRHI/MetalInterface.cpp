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

FRHIRayTracingScene* FMetalInterface::RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& Initializer)
{
    return new FMetalRayTracingScene(GetDeviceContext(), Initializer);
}

FRHIRayTracingGeometry* FMetalInterface::RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& Initializer)
{
    return new FMetalRayTracingGeometry(Initializer);
}

FRHIShaderResourceView* FMetalInterface::RHICreateShaderResourceView(const FRHITextureSRVDesc& Initializer)
{
    return new FMetalShaderResourceView(GetDeviceContext(), Initializer.Texture);
}

FRHIShaderResourceView* FMetalInterface::RHICreateShaderResourceView(const FRHIBufferSRVDesc& Initializer)
{
    return new FMetalShaderResourceView(GetDeviceContext(), Initializer.Buffer);
}

FRHIUnorderedAccessView* FMetalInterface::RHICreateUnorderedAccessView(const FRHITextureUAVDesc& Initializer)
{
    return new FMetalUnorderedAccessView(GetDeviceContext(), Initializer.Texture);
}

FRHIUnorderedAccessView* FMetalInterface::RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& Initializer)
{
    return new FMetalUnorderedAccessView(GetDeviceContext(), Initializer.Buffer);
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

FRHIDepthStencilState* FMetalInterface::RHICreateDepthStencilState(const FRHIDepthStencilStateDesc& Initializer)
{
    return new FMetalDepthStencilState(GetDeviceContext(), Initializer);
}

FRHIRasterizerState* FMetalInterface::RHICreateRasterizerState(const FRHIRasterizerStateDesc& Initializer)
{
    return new FMetalRasterizerState(GetDeviceContext(), Initializer);
}

FRHIBlendState* FMetalInterface::RHICreateBlendState(const FRHIBlendStateDesc& Initializer)
{
    return new FMetalBlendState();
}

FRHIVertexInputLayout* FMetalInterface::RHICreateVertexInputLayout(const FRHIVertexInputLayoutDesc& Initializer)
{
    return new FMetalInputLayoutState(GetDeviceContext(), Initializer);
}

FRHIGraphicsPipelineState* FMetalInterface::RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& Initializer)
{
    return new FMetalGraphicsPipelineState(GetDeviceContext(), Initializer);
}

FRHIComputePipelineState* FMetalInterface::RHICreateComputePipelineState(const FRHIComputePipelineStateDesc& Initializer)
{
    return new FMetalComputePipelineState();
}

FRHIRayTracingPipelineState* FMetalInterface::RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& Initializer)
{
    return new FMetalRayTracingPipelineState();
}

FRHITimestampQuery* FMetalInterface::RHICreateTimestampQuery()
{
    return new FMetalTimestampQuery();
}

FRHIViewport* FMetalInterface::RHICreateViewport(const FRHIViewportInitializer& Initializer)
{
    FCocoaWindow* Window = (FCocoaWindow*)Initializer.WindowHandle;
    
    __block NSRect Frame;
    __block NSRect ContentRect;
    ExecuteOnMainThread(^
    {
        Frame       = Window.frame;
        ContentRect = [Window contentRectForFrameRect:Window.frame];
    }, NSDefaultRunLoopMode, true);
    
    FRHIViewportInitializer NewInitializer(Initializer);
    NewInitializer.Width  = ContentRect.size.width;
    NewInitializer.Height = ContentRect.size.height;
    
    return new FMetalViewport(GetDeviceContext(), NewInitializer);
}

IRHICommandContext* FMetalInterface::RHIGetDefaultCommandContext()
{
    return CommandContext;
}

FString FMetalInterface::GetAdapterDescription() const
{
    return FString();
}

void FMetalInterface::RHIQueryRayTracingSupport(FRayTracingSupport& OutSupport) const
{
    OutSupport = FRayTracingSupport();
}

void FMetalInterface::RHIQueryShadingRateSupport(FShadingRateSupport& OutSupport) const
{
    OutSupport = FShadingRateSupport();
}

bool FMetalInterface::RHIQueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

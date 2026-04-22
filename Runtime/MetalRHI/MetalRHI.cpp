#include "MetalRHI/MetalRHI.h"

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

FRHITexture* FMetalRHI::CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FMetalTextureRef NewTexture = new FMetalTexture(GetDeviceContext(), InTextureDesc);
    if (!NewTexture->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewTexture.ReleaseOwnership();
    }
}

FRHIBuffer* FMetalRHI::CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState, const void* InInitialData)
{
    FMetalBufferRef NewBuffer = new FMetalBuffer(GetDeviceContext(), InBufferDesc);
    if (!NewBuffer->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

FRHISamplerState* FMetalRHI::CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc)
{
    FMetalSamplerStateRef NewSamplerState = new FMetalSamplerState(GetDeviceContext(), InSamplerDesc);
    if (!NewSamplerState->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewSamplerState.ReleaseOwnership();
    }
}

FRHISceneAccelerationStructure* FMetalRHI::CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& Desc)
{
    return new FMetalRayTracingScene(GetDeviceContext(), Desc);
}

FRHIGeometryAccelerationStructure* FMetalRHI::CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
{
    return new FMetalRayTracingGeometry(InGeometryDesc);
}

FRHIShaderResourceView* FMetalRHI::CreateShaderResourceView(const FRHIShaderResourceViewDesc& InDesc)
{
    if (InDesc.IsBufferSRV())
    {
        return new FMetalShaderResourceView(GetDeviceContext(), InDesc.BufferSRV.Buffer);
    }
    else if (InDesc.IsTextureSRV())
    {
        return new FMetalShaderResourceView(GetDeviceContext(), InDesc.TextureSRV.Texture);
    }
    else
    {
        return nullptr;
    }
}

FRHIUnorderedAccessView* FMetalRHI::CreateUnorderedAccessView(const FRHIUnorderedAccessViewDesc& InDesc)
{
    if (InDesc.IsBufferUAV())
    {
        return new FMetalUnorderedAccessView(GetDeviceContext(), InDesc.BufferUAV.Buffer);
    }
    else if (InDesc.IsTextureUAV())
    {
        return new FMetalUnorderedAccessView(GetDeviceContext(), InDesc.TextureUAV.Texture);
    }
    else
    {
        return nullptr;
    }
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

FRHIDepthStencilState* FMetalRHI::CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
{
    FMetalDepthStencilStateRef NewDepthStencilState = new FMetalDepthStencilState(GetDeviceContext(), InDesc);
    if (!NewDepthStencilState->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewDepthStencilState.ReleaseOwnership();
    }
}

FRHIRasterizerState* FMetalRHI::CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
{
    return new FMetalRasterizerState(InDesc);
}

FRHIBlendState* FMetalRHI::CreateBlendState(const FRHIBlendStateDesc& InDesc)
{
    return new FMetalBlendState(InDesc);
}

FRHIInputLayout* FMetalRHI::CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
{
    return new FMetalInputLayout(InInputElements);
}

FRHIGraphicsPipelineState* FMetalRHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
{
    return new FMetalGraphicsPipelineState(GetDeviceContext(), InDesc);
}

FRHIComputePipelineState* FMetalRHI::CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
{
    return new FMetalComputePipelineState();
}

FRHIRayTracingPipelineState* FMetalRHI::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& Desc)
{
    return new FMetalRayTracingPipelineState();
}

FRHIQuery* FMetalRHI::CreateQuery(EQueryType InQueryType)
{
    return new FMetalQuery(InQueryType);
}

FRHISwapChain* FMetalRHI::CreateSwapChain(const FRHISwapChainDesc& SwapChainDesc)
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
    
    FMetalSwapChainRef NewSwapChain = new FMetalSwapChain(GetDeviceContext(), NewViewportDesc);
    if (!NewSwapChain->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewSwapChain.ReleaseOwnership();
    }
}

bool FMetalRHI::QueryUAVFormatSupport(EFormat Format) const
{
    return true;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

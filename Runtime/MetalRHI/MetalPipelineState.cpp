#include "MetalRHI/MetalPipelineState.h"

FMetalInputLayoutRHI::FMetalInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements)
    : FRHIInputLayout()
    , VertexDescriptor(nullptr)
{
    VertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
    for (int32 Index = 0; Index < InInputElements.Size(); ++Index)
    {
        const auto& Element = InInputElements[Index];
        VertexDescriptor.attributes[Index].format      = ConvertVertexFormat(Element.Format);
        VertexDescriptor.attributes[Index].offset      = Element.ByteOffset;
        VertexDescriptor.attributes[Index].bufferIndex = Element.InputSlot;
        
        VertexDescriptor.layouts[Element.InputSlot].stride       = Element.VertexStride;
        VertexDescriptor.layouts[Element.InputSlot].stepFunction = ConvertVertexInputClass(Element.InputClass);
        VertexDescriptor.layouts[Element.InputSlot].stepRate     = Element.InputClass == EVertexInputClass::Vertex ? 1 : Element.InstanceStepRate;
    }
}

FMetalInputLayoutRHI::~FMetalInputLayoutRHI()
{
}

const FRHIInputElementDesc* FMetalInputLayoutRHI::GetInputElementDesc(uint32 Index) const
{
    return &InputElements[Index];
}

uint32 FMetalInputLayoutRHI::GetNumInputElementDescs() const
{
    return InputElements.Size();
}

void* FMetalInputLayoutRHI::GetRHINativeState() const
{
    return nullptr;
}

FMetalDepthStencilStateRHI::FMetalDepthStencilStateRHI(FMetalDevice* InDevice, const FRHIDepthStencilStateDesc& InDesc)
    : FRHIDepthStencilState(InDesc)
    , FMetalDeviceChild(InDevice)
    , DepthStencilState(nullptr)
{
}

FMetalDepthStencilStateRHI::~FMetalDepthStencilStateRHI()
{
    [DepthStencilState release];
}

void* FMetalDepthStencilStateRHI::GetRHINativeState() const
{
    return nullptr;
}

bool FMetalDepthStencilStateRHI::Initialize()
{
    SCOPED_AUTORELEASE_POOL();
    
    MTLDepthStencilDescriptor* Descriptor = [[MTLDepthStencilDescriptor new] autorelease];
    Descriptor.depthWriteEnabled    = Desc.bDepthEnable;
    Descriptor.depthCompareFunction = ConvertCompareFunction(Desc.DepthFunc);
    
    if (Desc.bStencilEnable)
    {
        Descriptor.backFaceStencil                            = [[MTLStencilDescriptor new] autorelease];
        Descriptor.backFaceStencil.stencilCompareFunction     = ConvertCompareFunction(Desc.BackFace.StencilFunc);
        Descriptor.backFaceStencil.stencilFailureOperation    = ConvertStencilOp(Desc.BackFace.StencilFailOp);
        Descriptor.backFaceStencil.depthFailureOperation      = ConvertStencilOp(Desc.BackFace.StencilDepthFailOp);
        Descriptor.backFaceStencil.depthStencilPassOperation  = ConvertStencilOp(Desc.BackFace.StencilDepthPassOp);
        Descriptor.backFaceStencil.readMask                   = Desc.StencilReadMask;
        Descriptor.backFaceStencil.writeMask                  = Desc.StencilWriteMask;
        
        Descriptor.frontFaceStencil                           = [[MTLStencilDescriptor new] autorelease];
        Descriptor.frontFaceStencil.stencilCompareFunction    = ConvertCompareFunction(Desc.FrontFace.StencilFunc);
        Descriptor.frontFaceStencil.stencilFailureOperation   = ConvertStencilOp(Desc.FrontFace.StencilFailOp);
        Descriptor.frontFaceStencil.depthFailureOperation     = ConvertStencilOp(Desc.FrontFace.StencilDepthFailOp);
        Descriptor.frontFaceStencil.depthStencilPassOperation = ConvertStencilOp(Desc.FrontFace.StencilDepthPassOp);
        Descriptor.frontFaceStencil.readMask                  = Desc.StencilReadMask;
        Descriptor.frontFaceStencil.writeMask                 = Desc.StencilWriteMask;
    }
    else
    {
        Descriptor.backFaceStencil  = nullptr;
        Descriptor.frontFaceStencil = nullptr;
    }

    id<MTLDevice> Device = GetDevice()->GetMTLDevice();
    CHECK(Device != nil);

    DepthStencilState = [Device newDepthStencilStateWithDescriptor:Descriptor];
    if (!DepthStencilState)
    {
        LOG_ERROR("Failed to create DepthStencilState");
        return false;
    }

    return true;
}

FMetalRasterizerStateRHI::FMetalRasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc)
    : FRHIRasterizerState(InDesc)
    , FillMode(ConvertFillMode(InDesc.FillMode))
    , FrontFaceWinding(InDesc.bFrontCounterClockwise ? MTLWindingCounterClockwise : MTLWindingClockwise)
{
}

FMetalRasterizerStateRHI::~FMetalRasterizerStateRHI()
{
}

void* FMetalRasterizerStateRHI::GetRHINativeState() const
{
    return nullptr;
}

FMetalBlendStateRHI::FMetalBlendStateRHI(const FRHIBlendStateDesc& InDesc)
    : FRHIBlendState(InDesc)
{
    for (int32 Index = 0; Index < InDesc.NumRenderTargets; Index++)
    {
        ColorAttachments[Index].bBlendingEnabled            = InDesc.RenderTargets[Index].bBlendEnable ? YES : NO;
        ColorAttachments[Index].SourceColorBlendFactor      = ConvertBlend(InDesc.RenderTargets[Index].SrcBlend);
        ColorAttachments[Index].DestinationColorBlendFactor = ConvertBlend(InDesc.RenderTargets[Index].DstBlend);
        ColorAttachments[Index].ColorBlendOperation         = ConvertBlendOp(InDesc.RenderTargets[Index].BlendOp);
        ColorAttachments[Index].SourceAlphaBlendFactor      = ConvertBlend(InDesc.RenderTargets[Index].SrcBlendAlpha);
        ColorAttachments[Index].DestinationAlphaBlendFactor = ConvertBlend(InDesc.RenderTargets[Index].DstBlendAlpha);
        ColorAttachments[Index].AlphaBlendOperation         = ConvertBlendOp(InDesc.RenderTargets[Index].BlendOpAlpha);
        ColorAttachments[Index].WriteMask                   = ConvertColorWriteFlags(InDesc.RenderTargets[Index].ColorWriteMask);
    }
}

FMetalBlendStateRHI::~FMetalBlendStateRHI()
{
}

void* FMetalBlendStateRHI::GetRHINativeState() const
{
    return nullptr;
}

FMetalGraphicsPipelineStateRHI::FMetalGraphicsPipelineStateRHI(FMetalDevice* InDevice, const FRHIGraphicsPipelineStateDesc& InDesc)
    : FRHIGraphicsPipelineState()
    , FMetalDeviceChild(InDevice)
    , Desc(InDesc)
    , BlendState(nullptr)
    , DepthStencilState(nullptr)
    , RasterizerState(nullptr)
    , PipelineState(nil)
{
    NumBuffers.Memzero();

    for (EShaderVisibility::Type ShaderStage = EShaderVisibility::Compute; ShaderStage < EShaderVisibility::Count; ShaderStage = static_cast<EShaderVisibility::Type>(ShaderStage + 1))
    {
        BufferBindings[ShaderStage].Memzero();
        TextureBindings[ShaderStage].Fill(FMetalResourceBinding(0));
        SamplerBindings[ShaderStage].Fill(FMetalResourceBinding(0));
    }
}

FMetalGraphicsPipelineStateRHI::~FMetalGraphicsPipelineStateRHI()
{
    [PipelineState release];
}

bool FMetalGraphicsPipelineStateRHI::Initialize()
{
    SCOPED_AUTORELEASE_POOL();

    DepthStencilState = MakeSharedRef<FMetalDepthStencilStateRHI>(GetDevice(), Desc.DepthStencilState);
    if (!DepthStencilState || !DepthStencilState->Initialize())
    {
        METAL_ERROR("Failed to create DepthStencilState for graphics PSO");
        return false;
    }

    RasterizerState = MakeSharedRef<FMetalRasterizerStateRHI>(Desc.RasterizerState);
    if (!RasterizerState)
    {
        METAL_ERROR("Failed to create RasterizerState for graphics PSO");
        return false;
    }

    MTLRenderPipelineDescriptor* Descriptor = [MTLRenderPipelineDescriptor new];
    if (FMetalShader* VertexShader = GetMetalShader(Desc.VertexShader))
    {
        Descriptor.vertexFunction = VertexShader->GetMTLFunction();
    }

    if (FMetalShader* PixelShader = GetMetalShader(Desc.PixelShader))
    {
        Descriptor.fragmentFunction = PixelShader->GetMTLFunction();
    }

    for (uint32 Index = 0; Index < Desc.RasterizerOutputFormats.NumRenderTargets; ++Index)
    {
        Descriptor.colorAttachments[Index].pixelFormat = ConvertFormat(Desc.RasterizerOutputFormats.RenderTargetFormats[Index]);
    }

    Descriptor.depthAttachmentPixelFormat = ConvertFormat(Desc.RasterizerOutputFormats.DepthStencilFormat);

    FMetalInputLayoutRHI* InputLayout = static_cast<FMetalInputLayoutRHI*>(Desc.InputLayout);
    Descriptor.vertexDescriptor = InputLayout ? InputLayout->GetMTLVertexDescriptor() : nil;

    NSError* Error = nil;
    MTLRenderPipelineReflection* PipelineReflection = nil;
    PipelineState = [GetDevice()->GetMTLDevice() newRenderPipelineStateWithDescriptor:Descriptor
                                                                              options:MTLPipelineOptionArgumentInfo
                                                                           reflection:&PipelineReflection
                                                                                error:&Error];
    [Descriptor release];

    if (PipelineState == nil)
    {
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR("Failed to create pipeline state, error %s", *ErrorString);
        return false;
    }

    // Vertex function resources
    for (MTLArgument* Argument in PipelineReflection.vertexArguments)
    {
        if (!Argument.active)
        {
            continue;
        }

        if (Argument.type == MTLArgumentTypeBuffer)
        {
            // NOTE: Might not be the best way, but for now it works since all shaders will have this name of vertexbuffers
            if ([Argument.name containsString:@"vertexBuffer."])
            {
                VertexBuffers.Emplace(static_cast<uint8>(Argument.index));
            }
            else
            {
                const auto Index = NumBuffers[EShaderVisibility::Vertex]++;
                CHECK(Index < BufferBindings[EShaderVisibility::Vertex].Size());

                BufferBindings[EShaderVisibility::Vertex][Index] = static_cast<uint8>(Argument.index);
            }
        }
        else if (Argument.type == MTLArgumentTypeTexture)
        {
            TextureBindings[EShaderVisibility::Vertex].Emplace(static_cast<uint8>(Argument.index));
        }
        else if (Argument.type == MTLArgumentTypeSampler)
        {
            SamplerBindings[EShaderVisibility::Vertex].Emplace(static_cast<uint8>(Argument.index));
        }
    }

    VertexBuffers.Shrink();
    TextureBindings[EShaderVisibility::Vertex].Shrink();
    SamplerBindings[EShaderVisibility::Vertex].Shrink();

    // Pixel function resources
    for (MTLArgument* Argument in PipelineReflection.fragmentArguments)
    {
        if (!Argument.active)
        {
            continue;
        }

        if (Argument.type == MTLArgumentTypeBuffer)
        {
            const auto Index = NumBuffers[EShaderVisibility::Pixel]++;
            CHECK(Index < BufferBindings[EShaderVisibility::Pixel].Size());

            BufferBindings[EShaderVisibility::Pixel][Index] = static_cast<uint8>(Argument.index);
        }
        else if (Argument.type == MTLArgumentTypeTexture)
        {
            TextureBindings[EShaderVisibility::Pixel].Emplace(static_cast<uint8>(Argument.index));
        }
        else if (Argument.type == MTLArgumentTypeSampler)
        {
            SamplerBindings[EShaderVisibility::Pixel].Emplace(static_cast<uint8>(Argument.index));
        }
    }

    TextureBindings[EShaderVisibility::Pixel].Shrink();
    SamplerBindings[EShaderVisibility::Pixel].Shrink();

    return true;
}

void FMetalGraphicsPipelineStateRHI::SetDebugName(const String& InName)
{
}

void FMetalGraphicsPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName.Clear();
}

void* FMetalGraphicsPipelineStateRHI::GetRHINativeState() const
{
    return (__bridge void*)PipelineState;
}

void FMetalComputePipelineStateRHI::SetDebugName(const String& InName)
{
}

void FMetalComputePipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName.Clear();
}

void* FMetalComputePipelineStateRHI::GetRHINativeState() const
{
    return nullptr;
}

void FMetalRayTracingPipelineStateRHI::SetDebugName(const String& InName)
{
}

void FMetalRayTracingPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName.Clear();
}

void* FMetalRayTracingPipelineStateRHI::GetRHINativeState() const
{
    return nullptr;
}

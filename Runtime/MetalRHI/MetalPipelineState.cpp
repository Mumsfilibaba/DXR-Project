#include "MetalRHI/MetalPipelineState.h"

FMetalInputLayout::FMetalInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
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

FMetalInputLayout::~FMetalInputLayout()
{
}

FMetalDepthStencilState::FMetalDepthStencilState(FMetalDeviceContext* DeviceContext, const FRHIDepthStencilStateDesc& InDesc)
    : FRHIDepthStencilState()
    , FMetalDeviceChild(DeviceContext)
    , DepthStencilState(nullptr)
    , Desc(InDesc)
{
}

FMetalDepthStencilState::~FMetalDepthStencilState()
{
    [DepthStencilState release];
}

bool FMetalDepthStencilState::Initialize()
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

    id<MTLDevice> Device = GetDeviceContext()->GetMTLDevice();
    CHECK(Device != nil);

    DepthStencilState = [Device newDepthStencilStateWithDescriptor:Descriptor];
    if (!DepthStencilState)
    {
        LOG_ERROR("Failed to create DepthStencilState");
        return false;
    }

    return true;
}

FMetalRasterizerState::FMetalRasterizerState(const FRHIRasterizerStateDesc& InDesc)
    : FRHIRasterizerState()
    , FillMode(ConvertFillMode(InDesc.FillMode))
    , FrontFaceWinding(InDesc.bFrontCounterClockwise ? MTLWindingCounterClockwise : MTLWindingClockwise)
    , Desc(InDesc)
{
}

FMetalRasterizerState::~FMetalRasterizerState()
{
}

FMetalBlendState::FMetalBlendState(const FRHIBlendStateDesc& InDesc)
    : FRHIBlendState()
    , Desc(InDesc)
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

FMetalBlendState::~FMetalBlendState()
{
}

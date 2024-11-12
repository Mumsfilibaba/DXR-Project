#include "MetalPipelineState.h"

FMetalVertexLayout::FMetalVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList)
    : FRHIVertexLayout()
    , VertexDescriptor(nullptr)
{
    VertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
    for (int32 Index = 0; Index < InInitializerList.Size(); ++Index)
    {
        const auto& Element = InInitializerList[Index];
        VertexDescriptor.attributes[Index].format      = ConvertVertexFormat(Element.Format);
        VertexDescriptor.attributes[Index].offset      = Element.ByteOffset;
        VertexDescriptor.attributes[Index].bufferIndex = Element.InputSlot;
        
        VertexDescriptor.layouts[Element.InputSlot].stride       = Element.VertexStride;
        VertexDescriptor.layouts[Element.InputSlot].stepFunction = ConvertVertexInputClass(Element.InputClass);
        VertexDescriptor.layouts[Element.InputSlot].stepRate     = Element.InputClass == EVertexInputClass::Vertex ? 1 : Element.InstanceStepRate;
    }
}

FMetalVertexLayout::~FMetalVertexLayout()
{
}

FMetalDepthStencilState::FMetalDepthStencilState(FMetalDeviceContext* DeviceContext, const FRHIDepthStencilStateInitializer& InInitializer)
    : FRHIDepthStencilState()
    , FMetalDeviceChild(DeviceContext)
    , DepthStencilState(nullptr)
    , Initializer(InInitializer)
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
    Descriptor.depthWriteEnabled    = Initializer.bDepthEnable;
    Descriptor.depthCompareFunction = ConvertCompareFunction(Initializer.DepthFunc);
    
    if (Initializer.bStencilEnable)
    {
        Descriptor.backFaceStencil                            = [[MTLStencilDescriptor new] autorelease];
        Descriptor.backFaceStencil.stencilCompareFunction     = ConvertCompareFunction(Initializer.BackFace.StencilFunc);
        Descriptor.backFaceStencil.stencilFailureOperation    = ConvertStencilOp(Initializer.BackFace.StencilFailOp);
        Descriptor.backFaceStencil.depthFailureOperation      = ConvertStencilOp(Initializer.BackFace.StencilDepthFailOp);
        Descriptor.backFaceStencil.depthStencilPassOperation  = ConvertStencilOp(Initializer.BackFace.StencilDepthPassOp);
        Descriptor.backFaceStencil.readMask                   = Initializer.StencilReadMask;
        Descriptor.backFaceStencil.writeMask                  = Initializer.StencilWriteMask;
        
        Descriptor.frontFaceStencil                           = [[MTLStencilDescriptor new] autorelease];
        Descriptor.frontFaceStencil.stencilCompareFunction    = ConvertCompareFunction(Initializer.FrontFace.StencilFunc);
        Descriptor.frontFaceStencil.stencilFailureOperation   = ConvertStencilOp(Initializer.FrontFace.StencilFailOp);
        Descriptor.frontFaceStencil.depthFailureOperation     = ConvertStencilOp(Initializer.FrontFace.StencilDepthFailOp);
        Descriptor.frontFaceStencil.depthStencilPassOperation = ConvertStencilOp(Initializer.FrontFace.StencilDepthPassOp);
        Descriptor.frontFaceStencil.readMask                  = Initializer.StencilReadMask;
        Descriptor.frontFaceStencil.writeMask                 = Initializer.StencilWriteMask;
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

FMetalRasterizerState::FMetalRasterizerState(const FRHIRasterizerStateInitializer& InInitializer)
    : FRHIRasterizerState()
    , FillMode(ConvertFillMode(InInitializer.FillMode))
    , FrontFaceWinding(InInitializer.bFrontCounterClockwise ? MTLWindingCounterClockwise : MTLWindingClockwise)
    , Initializer(InInitializer)
{
}

FMetalRasterizerState::~FMetalRasterizerState()
{
}

FMetalBlendState::FMetalBlendState(const FRHIBlendStateInitializer& InInitializer)
    : FRHIBlendState()
    , Initializer(InInitializer)
{
    for (int32 Index = 0; Index < InInitializer.NumRenderTargets; Index++)
    {
        ColorAttachments[Index].bBlendingEnabled            = InInitializer.RenderTargets[Index].bBlendEnable ? YES : NO;
        ColorAttachments[Index].SourceColorBlendFactor      = ConvertBlend(InInitializer.RenderTargets[Index].SrcBlend);
        ColorAttachments[Index].DestinationColorBlendFactor = ConvertBlend(InInitializer.RenderTargets[Index].DstBlend);
        ColorAttachments[Index].ColorBlendOperation         = ConvertBlendOp(InInitializer.RenderTargets[Index].BlendOp);
        ColorAttachments[Index].SourceAlphaBlendFactor      = ConvertBlend(InInitializer.RenderTargets[Index].SrcBlendAlpha);
        ColorAttachments[Index].DestinationAlphaBlendFactor = ConvertBlend(InInitializer.RenderTargets[Index].DstBlendAlpha);
        ColorAttachments[Index].AlphaBlendOperation         = ConvertBlendOp(InInitializer.RenderTargets[Index].BlendOpAlpha);
        ColorAttachments[Index].WriteMask                   = ConvertColorWriteFlags(InInitializer.RenderTargets[Index].ColorWriteMask);
    }
}

FMetalBlendState::~FMetalBlendState()
{
}

#pragma once
#include "MetalDeviceContext.h"
#include "MetalShader.h"

#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalInputLayoutState

class CMetalInputLayoutState : public FRHIVertexInputLayout, public CMetalObject
{
public:

    CMetalInputLayoutState(CMetalDeviceContext* DeviceContext, const CRHIVertexInputLayoutInitializer& Initializer)
        : CMetalObject(DeviceContext)
        , VertexDescriptor(nil)
    {
        VertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
        for (int32 Index = 0; Index < Initializer.Elements.Size(); ++Index)
        {
            const auto& Element = Initializer.Elements[Index];
            VertexDescriptor.attributes[Index].format      = ConvertVertexFormat(Element.Format);
            VertexDescriptor.attributes[Index].offset      = Element.ByteOffset;
            VertexDescriptor.attributes[Index].bufferIndex = Element.InputSlot;
            
            VertexDescriptor.layouts[Element.InputSlot].stride       = Element.VertexStride;
            VertexDescriptor.layouts[Element.InputSlot].stepFunction = ConvertVertexInputClass(Element.InputClass);
            VertexDescriptor.layouts[Element.InputSlot].stepRate     = (Element.InputClass == EVertexInputClass::Vertex) ? 1 : Element.InstanceStepRate;
        }
    }
    
    ~CMetalInputLayoutState() = default;
    
public:
    MTLVertexDescriptor* GetMTLVertexDescriptor() const { return VertexDescriptor; }
    
private:

    MTLVertexDescriptor* VertexDescriptor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalDepthStencilState

class CMetalDepthStencilState : public FRHIDepthStencilState, public CMetalObject
{
public:
    
    CMetalDepthStencilState(CMetalDeviceContext* DeviceContext, const CRHIDepthStencilStateInitializer& Initializer)
        : CMetalObject(DeviceContext)
        , DepthStencilState()
    {
        SCOPED_AUTORELEASE_POOL();
        
        MTLDepthStencilDescriptor* Descriptor = [MTLDepthStencilDescriptor new];
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
            Descriptor.backFaceStencil  = nil;
            Descriptor.frontFaceStencil = nil;
        }

        DepthStencilState = [DeviceContext->GetMTLDevice() newDepthStencilStateWithDescriptor:Descriptor];
        Check(DepthStencilState != nil);
        
        NSRelease(Descriptor);
    }
    
    ~CMetalDepthStencilState()
    {
        NSSafeRelease(DepthStencilState);
    }
    
public:
    
    id<MTLDepthStencilState> GetMTLDepthStencilState() const { return DepthStencilState; }
    
private:
    id<MTLDepthStencilState> DepthStencilState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRasterizerState

class CMetalRasterizerState : public FRHIRasterizerState, public CMetalObject
{
public:

    CMetalRasterizerState(CMetalDeviceContext* DeviceContext, const CRHIRasterizerStateInitializer& Initializer)
        : CMetalObject(DeviceContext)
        , FillMode(ConvertFillMode(Initializer.FillMode))
        , FrontFaceWinding(Initializer.bFrontCounterClockwise ? MTLWindingCounterClockwise : MTLWindingClockwise)
    {
    }

    ~CMetalRasterizerState() = default;

public:

    MTLTriangleFillMode GetMTLFillMode() const { return FillMode; }
    
    MTLWinding GetMTLFrontFaceWinding() const { return FrontFaceWinding; }

private:
    MTLTriangleFillMode FillMode;
    MTLWinding          FrontFaceWinding;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalBlendState

class CMetalBlendState : public FRHIBlendState
{
public:

    CMetalBlendState()  = default;
    ~CMetalBlendState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalGraphicsPipelineState

class CMetalGraphicsPipelineState : public FRHIGraphicsPipelineState, public CMetalObject
{
public:

    CMetalGraphicsPipelineState(CMetalDeviceContext* DeviceContext, const CRHIGraphicsPipelineStateInitializer& Initializer)
        : CMetalObject(DeviceContext)
        , BlendState(nullptr)
        , DepthStencilState(nullptr)
        , RasterizerState(nullptr)
        , PipelineState(nil)
    {
        SCOPED_AUTORELEASE_POOL();
        
        DepthStencilState = MakeSharedRef<CMetalDepthStencilState>(Initializer.DepthStencilState);
        Check(DepthStencilState != nullptr);
        
        RasterizerState = MakeSharedRef<CMetalRasterizerState>(Initializer.RasterizerState);
        Check(RasterizerState != nullptr);
        
        MTLRenderPipelineDescriptor* Descriptor = [MTLRenderPipelineDescriptor new];
        if (CMetalShader* VertexShader = GetMetalShader(Initializer.ShaderState.VertexShader))
        {
            Descriptor.vertexFunction = VertexShader->GetMTLFunction();
        }

        if (CMetalShader* PixelShader = GetMetalShader(Initializer.ShaderState.PixelShader))
        {
            Descriptor.fragmentFunction = PixelShader->GetMTLFunction();
        }
        
        for (uint32 Index = 0; Index < Initializer.PipelineFormats.NumRenderTargets; ++Index)
        {
            Descriptor.colorAttachments[Index].pixelFormat = ConvertFormat(Initializer.PipelineFormats.RenderTargetFormats[Index]);
        }
        
        Descriptor.depthAttachmentPixelFormat = ConvertFormat(Initializer.PipelineFormats.DepthStencilFormat);
        
        CMetalInputLayoutState* InputLayout = static_cast<CMetalInputLayoutState*>(Initializer.VertexInputLayout);
        Descriptor.vertexDescriptor = InputLayout ? InputLayout->GetMTLVertexDescriptor() : nil;

        NSError* Error = nil;
        PipelineState = [DeviceContext->GetMTLDevice() newRenderPipelineStateWithDescriptor:Descriptor
                                                                                    options:MTLPipelineOptionArgumentInfo
                                                                                 reflection:&PipelineReflection
                                                                                      error:&Error];
        
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR_COND(PipelineState != nil, "[MetalRHI] Failed to created pipeline state, error %s", ErrorString.CStr());
        
        NSRelease(Descriptor);
    }
    
    ~CMetalGraphicsPipelineState()
    {
        NSSafeRelease(PipelineState);
        NSSafeRelease(PipelineReflection);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIPipelineState Interface

    virtual void SetName(const String& InName) override final { }

    virtual String GetName() const override final { return ""; }
    
public:
    
    CMetalBlendState* GetMetalBlendState() const { return BlendState.Get(); }
    
    CMetalDepthStencilState* GetMetalDepthStencilState() const { return DepthStencilState.Get(); }
    
    CMetalRasterizerState* GetMetalRasterizerState() const { return RasterizerState.Get(); }
    
    id<MTLRenderPipelineState> GetMTLPipelineState() const { return PipelineState; }
    
private:
    TSharedRef<CMetalBlendState>        BlendState;
    TSharedRef<CMetalDepthStencilState> DepthStencilState;
    TSharedRef<CMetalRasterizerState>   RasterizerState;
    
    id<MTLRenderPipelineState>          PipelineState;
    MTLRenderPipelineReflection*        PipelineReflection;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalComputePipelineState

class CMetalComputePipelineState : public FRHIComputePipelineState
{
public:

    CMetalComputePipelineState()  = default;
    ~CMetalComputePipelineState() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIPipelineState Interface

    virtual void SetName(const String& InName) override final { }

    virtual String GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRayTracingPipelineState

class CMetalRayTracingPipelineState : public FRHIRayTracingPipelineState
{
public:

    CMetalRayTracingPipelineState()  = default;
    ~CMetalRayTracingPipelineState() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIPipelineState Interface

    virtual void SetName(const String& InName) override final { }

    virtual String GetName() const override final { return ""; }
};

#pragma clang diagnostic pop

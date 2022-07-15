#pragma once
#include "MetalDeviceContext.h"
#include "MetalShader.h"

#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalInputLayoutState

class FMetalInputLayoutState : public FRHIVertexInputLayout, public FMetalObject
{
public:

    FMetalInputLayoutState(FMetalDeviceContext* DeviceContext, const CRHIVertexInputLayoutInitializer& Initializer)
        : FMetalObject(DeviceContext)
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
    
    ~FMetalInputLayoutState() = default;
    
public:
    MTLVertexDescriptor* GetMTLVertexDescriptor() const { return VertexDescriptor; }
    
private:

    MTLVertexDescriptor* VertexDescriptor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalDepthStencilState

class FMetalDepthStencilState : public FRHIDepthStencilState, public FMetalObject
{
public:
    
    FMetalDepthStencilState(FMetalDeviceContext* DeviceContext, const FRHIDepthStencilStateInitializer& Initializer)
        : FMetalObject(DeviceContext)
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
    
    ~FMetalDepthStencilState()
    {
        NSSafeRelease(DepthStencilState);
    }
    
public:
    
    id<MTLDepthStencilState> GetMTLDepthStencilState() const { return DepthStencilState; }
    
private:
    id<MTLDepthStencilState> DepthStencilState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalRasterizerState

class FMetalRasterizerState : public FRHIRasterizerState, public FMetalObject
{
public:

    FMetalRasterizerState(FMetalDeviceContext* DeviceContext, const FRHIRasterizerStateInitializer& Initializer)
        : FMetalObject(DeviceContext)
        , FillMode(ConvertFillMode(Initializer.FillMode))
        , FrontFaceWinding(Initializer.bFrontCounterClockwise ? MTLWindingCounterClockwise : MTLWindingClockwise)
    {
    }

    ~FMetalRasterizerState() = default;

public:

    MTLTriangleFillMode GetMTLFillMode() const { return FillMode; }
    
    MTLWinding GetMTLFrontFaceWinding() const { return FrontFaceWinding; }

private:
    MTLTriangleFillMode FillMode;
    MTLWinding          FrontFaceWinding;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalBlendState

class FMetalBlendState : public FRHIBlendState
{
public:

    FMetalBlendState()  = default;
    ~FMetalBlendState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalGraphicsPipelineState

class FMetalGraphicsPipelineState : public FRHIGraphicsPipelineState, public FMetalObject
{
public:

    FMetalGraphicsPipelineState(FMetalDeviceContext* DeviceContext, const CRHIGraphicsPipelineStateInitializer& Initializer)
        : FMetalObject(DeviceContext)
        , BlendState(nullptr)
        , DepthStencilState(nullptr)
        , RasterizerState(nullptr)
        , PipelineState(nil)
    {
        SCOPED_AUTORELEASE_POOL();
        
        DepthStencilState = MakeSharedRef<FMetalDepthStencilState>(Initializer.DepthStencilState);
        Check(DepthStencilState != nullptr);
        
        RasterizerState = MakeSharedRef<FMetalRasterizerState>(Initializer.RasterizerState);
        Check(RasterizerState != nullptr);
        
        MTLRenderPipelineDescriptor* Descriptor = [MTLRenderPipelineDescriptor new];
        if (FMetalShader* VertexShader = GetMetalShader(Initializer.ShaderState.VertexShader))
        {
            Descriptor.vertexFunction = VertexShader->GetMTLFunction();
        }

        if (FMetalShader* PixelShader = GetMetalShader(Initializer.ShaderState.PixelShader))
        {
            Descriptor.fragmentFunction = PixelShader->GetMTLFunction();
        }
        
        for (uint32 Index = 0; Index < Initializer.PipelineFormats.NumRenderTargets; ++Index)
        {
            Descriptor.colorAttachments[Index].pixelFormat = ConvertFormat(Initializer.PipelineFormats.RenderTargetFormats[Index]);
        }
        
        Descriptor.depthAttachmentPixelFormat = ConvertFormat(Initializer.PipelineFormats.DepthStencilFormat);
        
        FMetalInputLayoutState* InputLayout = static_cast<FMetalInputLayoutState*>(Initializer.VertexInputLayout);
        Descriptor.vertexDescriptor = InputLayout ? InputLayout->GetMTLVertexDescriptor() : nil;

        NSError* Error = nil;
        PipelineState = [DeviceContext->GetMTLDevice() newRenderPipelineStateWithDescriptor:Descriptor
                                                                                    options:MTLPipelineOptionArgumentInfo
                                                                                 reflection:&PipelineReflection
                                                                                      error:&Error];
        
        const FString ErrorString([Error localizedDescription]);
        METAL_ERROR_COND(PipelineState != nil, "[MetalRHI] Failed to created pipeline state, error %s", ErrorString.CStr());
        
        NSRelease(Descriptor);
    }
    
    ~FMetalGraphicsPipelineState()
    {
        NSSafeRelease(PipelineState);
        NSSafeRelease(PipelineReflection);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIPipelineState Interface

    virtual void SetName(const FString& InName) override final { }

    virtual FString GetName() const override final { return ""; }
    
public:
    
    FMetalBlendState*        GetMetalBlendState()        const { return BlendState.Get(); }
    FMetalDepthStencilState* GetMetalDepthStencilState() const { return DepthStencilState.Get(); }
    FMetalRasterizerState*   GetMetalRasterizerState()   const { return RasterizerState.Get(); }
    
    id<MTLRenderPipelineState> GetMTLPipelineState() const { return PipelineState; }
    
private:
    TSharedRef<FMetalBlendState>        BlendState;
    TSharedRef<FMetalDepthStencilState> DepthStencilState;
    TSharedRef<FMetalRasterizerState>   RasterizerState;
    
    id<MTLRenderPipelineState>          PipelineState;
    MTLRenderPipelineReflection*        PipelineReflection;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalComputePipelineState

class FMetalComputePipelineState : public FRHIComputePipelineState
{
public:

    FMetalComputePipelineState()  = default;
    ~FMetalComputePipelineState() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIPipelineState Interface

    virtual void SetName(const FString& InName) override final { }

    virtual FString GetName() const override final { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalRayTracingPipelineState

class FMetalRayTracingPipelineState : public FRHIRayTracingPipelineState
{
public:

    FMetalRayTracingPipelineState()  = default;
    ~FMetalRayTracingPipelineState() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIPipelineState Interface

    virtual void SetName(const FString& InName) override final { }

    virtual FString GetName() const override final { return ""; }
};

#pragma clang diagnostic pop

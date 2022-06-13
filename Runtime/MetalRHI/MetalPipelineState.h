#pragma once
#include "MetalObject.h"

#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalInputLayoutState

class CMetalInputLayoutState : public CRHIVertexInputLayout, public CMetalObject
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
            VertexDescriptor.layouts[Index].stride         = GetByteStrideFromFormat(Element.Format);
            VertexDescriptor.layouts[Index].stepFunction   = ConvertVertexInputClass(Element.InputClass);
            VertexDescriptor.layouts[Index].stepRate       = Element.InstanceStepRate;
        }
    }
    
    ~CMetalInputLayoutState() = default;
    
public:
    MTLVertexDescriptor* GetVertexDescriptor() const { return VertexDescriptor; }
    
private:

    MTLVertexDescriptor* VertexDescriptor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalDepthStencilState

class CMetalDepthStencilState : public CRHIDepthStencilState, public CMetalObject
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

class CMetalRasterizerState : public CRHIRasterizerState, public CMetalObject
{
public:

    CMetalRasterizerState(CMetalDeviceContext* DeviceContext, const CRHIRasterizerStateInitializer& Initializer)
        : CMetalObject(DeviceContext)
    {
    }

    ~CMetalRasterizerState() = default;

public:

    MTLTriangleFillMode GetMTLFillMode() const { return FillMode; }

private:
    MTLTriangleFillMode FillMode;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalBlendState

class CMetalBlendState : public CRHIBlendState
{
public:

    CMetalBlendState()  = default;
    ~CMetalBlendState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalGraphicsPipelineState

class CMetalGraphicsPipelineState : public CRHIGraphicsPipelineState, public CMetalObject
{
public:

    CMetalGraphicsPipelineState(CMetalDeviceContext* DeviceContext, const CRHIGraphicsPipelineStateInitializer& Initializer)
        : CMetalObject(DeviceContext)
        , BlendState(nullptr)
        , DepthStencilState(nullptr)
        , RasterizerState(nullptr)
    {
        DepthStencilState = MakeSharedRef<CMetalDepthStencilState>(Initializer.DepthStencilState);
        Check(DepthStencilState != nullptr);
    }
    
    ~CMetalGraphicsPipelineState() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIPipelineState Interface

    virtual void SetName(const String& InName) override final { }

    virtual String GetName() const override final { return ""; }
    
public:
    
    CMetalBlendState* GetMetalBlendState() const { return BlendState.Get(); }
    
    CMetalDepthStencilState* GetMetalDepthStencilState() const { return DepthStencilState.Get(); }
    
    CMetalRasterizerState* GetMetalRasterizerState() const { return RasterizerState.Get(); }
    
private:
    TSharedRef<CMetalBlendState>        BlendState;
    TSharedRef<CMetalDepthStencilState> DepthStencilState;
    TSharedRef<CMetalRasterizerState>   RasterizerState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalComputePipelineState

class CMetalComputePipelineState : public CRHIComputePipelineState
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

class CMetalRayTracingPipelineState : public CRHIRayTracingPipelineState
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

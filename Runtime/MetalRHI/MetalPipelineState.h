#pragma once
#include "MetalDeviceContext.h"
#include "MetalShader.h"

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

class CMetalBlendState : public CRHIBlendState
{
public:

    CMetalBlendState()  = default;
    ~CMetalBlendState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SMetalResourceBinding

struct SMetalResourceBinding
{
    SMetalResourceBinding() = default;
    
    SMetalResourceBinding(uint8 InBinding)
        : Binding(InBinding)
    { }

    uint8 Binding = 0;
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
        , PipelineState(nil)
    {
        SCOPED_AUTORELEASE_POOL();
        
        NumBuffers.Memzero();
        
        for (EShaderVisibility ShaderStage = ShaderVisibility_Compute; ShaderStage < ShaderVisibility_Count; ShaderStage = EShaderVisibility(ShaderStage + 1))
        {
            BufferBindings[ShaderStage].Memzero();
            TextureBindings[ShaderStage].Fill(SMetalResourceBinding(0));
            SamplerBindings[ShaderStage].Fill(SMetalResourceBinding(0));
        }
        
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
        MTLRenderPipelineReflection* PipelineReflection = nil;
        PipelineState = [DeviceContext->GetMTLDevice() newRenderPipelineStateWithDescriptor:Descriptor
                                                                                    options:MTLPipelineOptionArgumentInfo
                                                                                 reflection:&PipelineReflection
                                                                                      error:&Error];
        
        const String ErrorString([Error localizedDescription]);
        METAL_ERROR_COND(PipelineState != nil, "[MetalRHI] Failed to created pipeline state, error %s", ErrorString.CStr());
        
        // Vertex- Function Resources
        for (MTLArgument* Argument in PipelineReflection.vertexArguments)
        {
            if (!Argument.active)
            {
                continue;
            }
            
            if (Argument.type == MTLArgumentTypeBuffer)
            {
                // SetConstantBuffer(Shader*, Index = n) -> Maps to Buffer(n)
                // SetShaderResourceView(Shader*, Index = 5) -> Maps to Buffer(?) Texture(?)
                // SetUnorderedAccessView(Shader*, Index = 5)
                
                // NOTE: Might not be the best way, but for now it works since all shaders will have this name of vertexbuffers
                if ([Argument.name containsString:@"vertexBuffer."])
                {
                    VertexBuffers.Emplace(static_cast<uint8>(Argument.index));
                }
                else
                {
                    const auto Index = NumBuffers[ShaderVisibility_Vertex]++;
                    Check(Index < BufferBindings[ShaderVisibility_Vertex].Size());
                    
                    BufferBindings[ShaderVisibility_Vertex][Index] = static_cast<uint8>(Argument.index);
                }
            }
            else if (Argument.type == MTLArgumentTypeTexture)
            {
                TextureBindings[ShaderVisibility_Vertex].Emplace(static_cast<uint8>(Argument.index));
            }
            else if (Argument.type == MTLArgumentTypeSampler)
            {
                SamplerBindings[ShaderVisibility_Vertex].Emplace(static_cast<uint8>(Argument.index));
            }
        }
        
        VertexBuffers.ShrinkToFit();
        TextureBindings[ShaderVisibility_Vertex].ShrinkToFit();
        SamplerBindings[ShaderVisibility_Vertex].ShrinkToFit();
        
        // Pixel- Function Resources
        for (MTLArgument* Argument in PipelineReflection.fragmentArguments)
        {
            if (!Argument.active)
            {
                continue;
            }
            
            if (Argument.type == MTLArgumentTypeBuffer)
            {
                const auto Index = NumBuffers[ShaderVisibility_Pixel]++;
                Check(Index < BufferBindings[ShaderVisibility_Pixel].Size());
                
                BufferBindings[ShaderVisibility_Pixel][Index] = static_cast<uint8>(Argument.index);
            }
            else if (Argument.type == MTLArgumentTypeTexture)
            {
                TextureBindings[ShaderVisibility_Pixel].Emplace(static_cast<uint8>(Argument.index));
            }
            else if (Argument.type == MTLArgumentTypeSampler)
            {
                SamplerBindings[ShaderVisibility_Pixel].Emplace(static_cast<uint8>(Argument.index));
            }
        }
        
        TextureBindings[ShaderVisibility_Pixel].ShrinkToFit();
        SamplerBindings[ShaderVisibility_Pixel].ShrinkToFit();
        
        NSSafeRelease(Descriptor);
    }
    
    ~CMetalGraphicsPipelineState()
    {
        NSSafeRelease(PipelineState);
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
    
    uint32 GetNumBuffers(EShaderVisibility ShaderVisibility) const { return NumBuffers[ShaderVisibility]; }
    
    uint32 GetBufferBinding(EShaderVisibility ShaderVisibility, uint32 BufferIndex) const { return BufferBindings[ShaderVisibility][BufferIndex]; }
    
private:
    TSharedRef<CMetalBlendState>        BlendState;
    TSharedRef<CMetalDepthStencilState> DepthStencilState;
    TSharedRef<CMetalRasterizerState>   RasterizerState;
    
    id<MTLRenderPipelineState>          PipelineState;
    
    TArray<SMetalResourceBinding>       VertexBuffers;
    
    TStaticArray<uint8, kMaxConstantBuffers>    BufferBindings[ShaderVisibility_Count];
    TStaticArray<uint8, ShaderVisibility_Count> NumBuffers;
    
    TArray<SMetalResourceBinding>       TextureBindings[ShaderVisibility_Count];
    TArray<SMetalResourceBinding>       SamplerBindings[ShaderVisibility_Count];
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

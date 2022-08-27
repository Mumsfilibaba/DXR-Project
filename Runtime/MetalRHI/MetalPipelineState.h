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
    FMetalInputLayoutState(FMetalDeviceContext* DeviceContext, const FRHIVertexInputLayoutInitializer& Initializer)
        : FMetalObject(DeviceContext)
        , VertexDescriptor(nil)
    {
        VertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
        for (int32 Index = 0; Index < Initializer.Elements.GetSize(); ++Index)
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
// FMetalResourceBinding

struct FMetalResourceBinding
{
    FMetalResourceBinding() = default;
    
    FMetalResourceBinding(uint8 InBinding)
        : Binding(InBinding)
    { }

    uint8 Binding = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalGraphicsPipelineState

class FMetalGraphicsPipelineState : public FRHIGraphicsPipelineState, public FMetalObject
{
public:
    FMetalGraphicsPipelineState(FMetalDeviceContext* DeviceContext, const FRHIGraphicsPipelineStateInitializer& Initializer)
        : FMetalObject(DeviceContext)
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
            TextureBindings[ShaderStage].Fill(FMetalResourceBinding(0));
            SamplerBindings[ShaderStage].Fill(FMetalResourceBinding(0));
        }
        
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
        MTLRenderPipelineReflection* PipelineReflection = nil;
        PipelineState = [DeviceContext->GetMTLDevice() newRenderPipelineStateWithDescriptor:Descriptor
                                                                                    options:MTLPipelineOptionArgumentInfo
                                                                                 reflection:&PipelineReflection
                                                                                      error:&Error];
        
        const FString ErrorString([Error localizedDescription]);
        METAL_ERROR_COND(PipelineState != nil, "[MetalRHI] Failed to created pipeline state, error %s", ErrorString.GetCString());
        
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
                    Check(Index < BufferBindings[ShaderVisibility_Vertex].GetSize());
                    
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
        
        VertexBuffers.Shrink();
        TextureBindings[ShaderVisibility_Vertex].Shrink();
        SamplerBindings[ShaderVisibility_Vertex].Shrink();
        
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
                Check(Index < BufferBindings[ShaderVisibility_Pixel].GetSize());
                
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
        
        TextureBindings[ShaderVisibility_Pixel].Shrink();
        SamplerBindings[ShaderVisibility_Pixel].Shrink();
        
        NSSafeRelease(Descriptor);
    }
    
    ~FMetalGraphicsPipelineState()
    {
        NSSafeRelease(PipelineState);
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
    
    uint32 GetNumBuffers(EShaderVisibility ShaderVisibility) const { return NumBuffers[ShaderVisibility]; }
    
    uint32 GetBufferBinding(EShaderVisibility ShaderVisibility, uint32 BufferIndex) const { return BufferBindings[ShaderVisibility][BufferIndex]; }
    
private:
    TSharedRef<FMetalBlendState>        BlendState;
    TSharedRef<FMetalDepthStencilState> DepthStencilState;
    TSharedRef<FMetalRasterizerState>   RasterizerState;
    
    id<MTLRenderPipelineState>          PipelineState;
    
    TArray<FMetalResourceBinding>       VertexBuffers;
    
    TStaticArray<uint8, kMaxConstantBuffers>    BufferBindings[ShaderVisibility_Count];
    TStaticArray<uint8, ShaderVisibility_Count> NumBuffers;
    
    TArray<FMetalResourceBinding>       TextureBindings[ShaderVisibility_Count];
    TArray<FMetalResourceBinding>       SamplerBindings[ShaderVisibility_Count];
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

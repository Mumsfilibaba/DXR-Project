#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalDeviceContext.h"
#include "MetalRHI/MetalShader.h"
#include "MetalRHI/MetalRefCounted.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalInputLayout>            FMetalVertexInputLayoutRef;
typedef TSharedRef<class FMetalDepthStencilState>       FMetalDepthStencilStateRef;
typedef TSharedRef<class FMetalGraphicsPipelineState>   FMetalGraphicsPipelineStateRef;
typedef TSharedRef<class FMetalComputePipelineState>    FMetalComputePipelineStateRef;
typedef TSharedRef<class FMetalRayTracingPipelineState> FMetalRayTracingPipelineStateRef;

class FMetalInputLayout : public FRHIInputLayout
{
public:
    FMetalInputLayout(const TArray<FRHIInputElementInfo>& InInputElements);
    virtual ~FMetalInputLayout();

    virtual const FRHIInputElementInfo* GetInputElementInfo(uint32 Index) const override final
    {
        return &InputElements[Index];
    }

    virtual uint32 GetNumInputElementInfos() const override final
    {
        return InputElements.Size();
    }

    MTLVertexDescriptor* GetMTLVertexDescriptor() const 
    { 
        return VertexDescriptor;
    }

private:
    TArray<FRHIInputElementInfo> InputElements;
    MTLVertexDescriptor*         VertexDescriptor;
};

class FMetalDepthStencilState : public FRHIDepthStencilState, public FMetalDeviceChild
{
public:
    FMetalDepthStencilState(FMetalDeviceContext* DeviceContext, const FRHIDepthStencilStateInfo& InInfo);
    virtual ~FMetalDepthStencilState();

    bool Initialize();

    virtual FRHIDepthStencilStateInfo GetInfo() const override final
    {
        return Info;
    }
    
    id<MTLDepthStencilState> GetMTLDepthStencilState() const 
    { 
        return DepthStencilState; 
    }
    
private:
    id<MTLDepthStencilState>  DepthStencilState;
    FRHIDepthStencilStateInfo Info;
};

class FMetalRasterizerState : public FRHIRasterizerState
{
public:
    FMetalRasterizerState(const FRHIRasterizerStateInfo& InInfo);
    virtual ~FMetalRasterizerState();

    virtual FRHIRasterizerStateInfo GetInfo() const override final
    {
        return Info;
    }

    MTLTriangleFillMode FillMode;
    MTLWinding          FrontFaceWinding;

    const FRHIRasterizerStateInfo Info;
};

class FMetalBlendState : public FRHIBlendState
{
public:
    FMetalBlendState(const FRHIBlendStateInfo& InInfo);
    virtual ~FMetalBlendState();

    virtual FRHIBlendStateInfo GetInfo() const
    {
        return Info;
    }

    struct FBlendAttachment
    {
        MTLPixelFormat    PixelFormat;
        MTLColorWriteMask WriteMask;
        BOOL              bBlendingEnabled;
        MTLBlendOperation AlphaBlendOperation;
        MTLBlendOperation ColorBlendOperation;
        MTLBlendFactor    DestinationAlphaBlendFactor;
        MTLBlendFactor    DestinationColorBlendFactor;
        MTLBlendFactor    SourceAlphaBlendFactor;
        MTLBlendFactor    SourceColorBlendFactor;
    };

    FBlendAttachment ColorAttachments[RHI_MAX_RENDER_TARGETS];
    const FRHIBlendStateInfo Info;
};

struct FMetalResourceBinding
{
    FMetalResourceBinding() = default;
    
    FMetalResourceBinding(uint8 InBinding)
        : Binding(InBinding)
    {
    }

    uint8 Binding = 0;
};

class FMetalGraphicsPipelineState : public FRHIGraphicsPipelineState, public FMetalDeviceChild
{
public:
    FMetalGraphicsPipelineState(FMetalDeviceContext* DeviceContext, const FRHIGraphicsPipelineStateInfo& Info)
        : FMetalDeviceChild(DeviceContext)
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
        
        DepthStencilState = MakeSharedRef<FMetalDepthStencilState>(Info.DepthStencilState);
        CHECK(DepthStencilState != nullptr);
        
        RasterizerState = MakeSharedRef<FMetalRasterizerState>(Info.RasterizerState);
        CHECK(RasterizerState != nullptr);
        
        MTLRenderPipelineDescriptor* Descriptor = [MTLRenderPipelineDescriptor new];
        if (FMetalShader* VertexShader = GetMetalShader(Info.VertexShader))
        {
            Descriptor.vertexFunction = VertexShader->GetMTLFunction();
        }

        if (FMetalShader* PixelShader = GetMetalShader(Info.PixelShader))
        {
            Descriptor.fragmentFunction = PixelShader->GetMTLFunction();
        }
        
        for (uint32 Index = 0; Index < Info.RasterizerOutputFormats.NumRenderTargets; ++Index)
        {
            Descriptor.colorAttachments[Index].pixelFormat = ConvertFormat(Info.RasterizerOutputFormats.RenderTargetFormats[Index]);
        }
        
        Descriptor.depthAttachmentPixelFormat = ConvertFormat(Info.RasterizerOutputFormats.DepthStencilFormat);
        
        FMetalInputLayout* InputLayout = static_cast<FMetalInputLayout*>(Info.InputLayout);
        Descriptor.vertexDescriptor = InputLayout ? InputLayout->GetMTLVertexDescriptor() : nil;

        NSError* Error = nil;
        MTLRenderPipelineReflection* PipelineReflection = nil;
        PipelineState = [DeviceContext->GetMTLDevice() newRenderPipelineStateWithDescriptor:Descriptor
                                                                                    options:MTLPipelineOptionArgumentInfo
                                                                                 reflection:&PipelineReflection
                                                                                      error:&Error];
        
        const FString ErrorString([Error localizedDescription]);
        METAL_ERROR_COND(PipelineState != nil, "[MetalRHI] Failed to created pipeline state, error %s", *ErrorString);
        
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
                    CHECK(Index < BufferBindings[ShaderVisibility_Vertex].Size());
                    
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
                CHECK(Index < BufferBindings[ShaderVisibility_Pixel].Size());
                
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
        
        [Descriptor release];
    }
    
    ~FMetalGraphicsPipelineState()
    {
        [PipelineState release];
    }

    virtual void SetDebugName(const FString& InName) override final {}
    virtual FString GetDebugName() const override final { return ""; }
    
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

class FMetalComputePipelineState : public FRHIComputePipelineState
{
public:
    FMetalComputePipelineState()  = default;
    ~FMetalComputePipelineState() = default;

    virtual void SetDebugName(const FString& InName) override final {}
    virtual FString GetDebugName() const override final { return ""; }
};

class FMetalRayTracingPipelineState : public FRHIRayTracingPipelineState
{
public:
    FMetalRayTracingPipelineState()  = default;
    ~FMetalRayTracingPipelineState() = default;

    virtual void SetDebugName(const FString& InName) override final {}
    virtual FString GetDebugName() const override final { return ""; }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

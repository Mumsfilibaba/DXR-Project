#pragma once
#include "MetalDeviceContext.h"
#include "MetalShader.h"
#include "MetalRefCounted.h"
#include "Core/Utilities/StringUtilities.h"
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalVertexInputLayout>       FMetalVertexInputLayoutRef;
typedef TSharedRef<class FMetalDepthStencilState>       FMetalDepthStencilStateRef;
typedef TSharedRef<class FMetalGraphicsPipelineState>   FMetalGraphicsPipelineStateRef;
typedef TSharedRef<class FMetalComputePipelineState>    FMetalComputePipelineStateRef;
typedef TSharedRef<class FMetalRayTracingPipelineState> FMetalRayTracingPipelineStateRef;


class FMetalVertexInputLayout : public FRHIVertexInputLayout, public FMetalRefCounted
{
public:
    FMetalVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer);
    virtual ~FMetalVertexInputLayout() = default;

    virtual int32 AddRef() override final { return FMetalRefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FMetalRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FMetalRefCounted::GetRefCount(); }

    MTLVertexDescriptor* GetMTLVertexDescriptor() const 
    { 
        return VertexDescriptor;
    }
    
private:
    MTLVertexDescriptor* VertexDescriptor;
};


class FMetalDepthStencilState : public FRHIDepthStencilState, public FMetalObject, public FMetalRefCounted
{
public:
    FMetalDepthStencilState(FMetalDeviceContext* DeviceContext, const FRHIDepthStencilStateInitializer& InInitializer);
    virtual ~FMetalDepthStencilState();

    bool Initialize();

    virtual int32 AddRef() override final { return FMetalRefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FMetalRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FMetalRefCounted::GetRefCount(); }

    virtual FRHIDepthStencilStateInitializer GetDesc() const override final
    {
        return Initializer;
    }
    
    id<MTLDepthStencilState> GetMTLDepthStencilState() const 
    { 
        return DepthStencilState; 
    }
    
private:
    id<MTLDepthStencilState>         DepthStencilState;
    FRHIDepthStencilStateInitializer Initializer;
};


class FMetalRasterizerState : public FRHIRasterizerState, public FMetalObject
{
public:
    FMetalRasterizerState(FMetalDeviceContext* DeviceContext, const FRHIRasterizerStateDesc& InDesc)
        : FRHIRasterizerState()
        , FMetalObject(DeviceContext)
        , Desc(InDesc)
        , FillMode(ConvertFillMode(InDesc.FillMode))
        , FrontFaceWinding(InDesc.bFrontCounterClockwise ? MTLWindingCounterClockwise : MTLWindingClockwise)
    {
    }

    ~FMetalRasterizerState() = default;

    virtual FRHIRasterizerStateDesc GetDesc() const override final
    {
        return Desc;
    }

    MTLTriangleFillMode GetMTLFillMode() const 
    {
        return FillMode;
    }
    
    MTLWinding GetMTLFrontFaceWinding() const
    {
        return FrontFaceWinding;
    }

private:
    FRHIRasterizerStateDesc Desc;
    MTLTriangleFillMode     FillMode;
    MTLWinding              FrontFaceWinding;
};


class FMetalBlendState : public FRHIBlendState, public FMetalObject
{
public:
    FMetalBlendState(FMetalDeviceContext* DeviceContext, const FRHIBlendStateDesc& InDesc)
        : FRHIBlendState()
        , FMetalObject(DeviceContext)
        , Desc(InDesc)
    {
    }

    ~FMetalBlendState() = default;

    virtual FRHIBlendStateDesc GetDesc() const
    {
        return Desc;
    }

private:
    FRHIBlendStateDesc Desc;
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


class FMetalGraphicsPipelineState : public FRHIGraphicsPipelineState, public FMetalObject
{
public:
    FMetalGraphicsPipelineState(FMetalDeviceContext* DeviceContext, const FRHIGraphicsPipelineStateDesc& Initializer)
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
        CHECK(DepthStencilState != nullptr);
        
        RasterizerState = MakeSharedRef<FMetalRasterizerState>(Initializer.RasterizerState);
        CHECK(RasterizerState != nullptr);
        
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
        
        FMetalVertexInputLayout* InputLayout = static_cast<FMetalVertexInputLayout*>(Initializer.VertexInputLayout);
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
        
        NSSafeRelease(Descriptor);
    }
    
    ~FMetalGraphicsPipelineState()
    {
        NSSafeRelease(PipelineState);
    }

    virtual void SetName(const FString& InName) override final {
}

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


class FMetalComputePipelineState : public FRHIComputePipelineState
{
public:
    FMetalComputePipelineState()  = default;
    ~FMetalComputePipelineState() = default;

    virtual void SetName(const FString& InName) override final {
}

    virtual FString GetName() const override final { return ""; }
};


class FMetalRayTracingPipelineState : public FRHIRayTracingPipelineState
{
public:
    FMetalRayTracingPipelineState()  = default;
    ~FMetalRayTracingPipelineState() = default;

    virtual void SetName(const FString& InName) override final {
}

    virtual FString GetName() const override final { return ""; }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

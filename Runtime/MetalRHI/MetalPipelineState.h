#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalShader.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalInputLayoutRHI>             FMetalVertexInputLayoutRef;
typedef TSharedRef<class FMetalDepthStencilStateRHI>       FMetalDepthStencilStateRef;
typedef TSharedRef<class FMetalGraphicsPipelineStateRHI>   FMetalGraphicsPipelineStateRef;
typedef TSharedRef<class FMetalComputePipelineStateRHI>    FMetalComputePipelineStateRef;
typedef TSharedRef<class FMetalRayTracingPipelineStateRHI> FMetalRayTracingPipelineStateRef;

class FMetalInputLayoutRHI : public FRHIInputLayout
{
public:
    FMetalInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements);
    virtual ~FMetalInputLayoutRHI();

    // FRHIInputLayout Interface
    virtual void* GetRHINativeState() const override final;

    virtual const FRHIInputElementDesc* GetInputElementDesc(uint32 Index) const override final;
    virtual uint32 GetNumInputElementDescs() const override final;

    MTLVertexDescriptor* GetMTLVertexDescriptor() const 
    { 
        return VertexDescriptor;
    }

private:
    TArray<FRHIInputElementDesc> InputElements;
    MTLVertexDescriptor*         VertexDescriptor;
};

class FMetalDepthStencilStateRHI : public FRHIDepthStencilState, public FMetalDeviceChild
{
public:
    FMetalDepthStencilStateRHI(FMetalDevice* InDevice, const FRHIDepthStencilStateDesc& InDesc);
    virtual ~FMetalDepthStencilStateRHI();

    // FRHIDepthStencilState Interface
    virtual void* GetRHINativeState() const override final;
    
    bool Initialize();

    id<MTLDepthStencilState> GetMTLDepthStencilState() const 
    { 
        return DepthStencilState; 
    }
    
private:
    id<MTLDepthStencilState>  DepthStencilState;
};

class FMetalRasterizerStateRHI : public FRHIRasterizerState
{
public:
    FMetalRasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc);
    virtual ~FMetalRasterizerStateRHI();

    // FRHIRasterizerState Interface
    virtual void* GetRHINativeState() const override final;

    MTLTriangleFillMode GetMTLFillMode() const
    {
        return FillMode;
    }

    MTLWinding GetMTLFrontFaceWinding() const
    {
        return FrontFaceWinding;
    }

private:
    MTLTriangleFillMode     FillMode;
    MTLWinding              FrontFaceWinding;
};

class FMetalBlendStateRHI : public FRHIBlendState
{
public:
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

public:
    FMetalBlendStateRHI(const FRHIBlendStateDesc& InDesc);
    virtual ~FMetalBlendStateRHI();

    // FRHIBlendState Interface
    virtual void* GetRHINativeState() const override final;

    const FBlendAttachment& GetColorAttachment(uint32 Index) const
    {
         return ColorAttachments[Index];
    }

private:
    FBlendAttachment   ColorAttachments[RHI_MAX_RENDER_TARGETS];
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

class FMetalGraphicsPipelineStateRHI : public FRHIGraphicsPipelineState, public FMetalDeviceChild
{
public:
    FMetalGraphicsPipelineStateRHI(FMetalDevice* InDevice, const FRHIGraphicsPipelineStateDesc& InDesc);
    virtual ~FMetalGraphicsPipelineStateRHI();
    
    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;
    
    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

    bool Initialize();

    FMetalBlendStateRHI*        GetMetalBlendState()        const { return BlendState.Get(); }
    FMetalDepthStencilStateRHI* GetMetalDepthStencilState() const { return DepthStencilState.Get(); }
    FMetalRasterizerStateRHI*   GetMetalRasterizerState()   const { return RasterizerState.Get(); }

    uint32 GetBufferBinding(EShaderVisibility::Type ShaderVisibility, uint32 BufferIndex) const
    {
        return BufferBindings[ShaderVisibility][BufferIndex];
    }
    
    uint32 GetNumBuffers(EShaderVisibility::Type ShaderVisibility) const
    {
        return NumBuffers[ShaderVisibility];
    }

    id<MTLRenderPipelineState> GetMTLPipelineState() const
    {
        return PipelineState;
    }

private:
    FRHIGraphicsPipelineStateDesc                 Desc;
    TSharedRef<FMetalBlendStateRHI>               BlendState;
    TSharedRef<FMetalDepthStencilStateRHI>        DepthStencilState;
    TSharedRef<FMetalRasterizerStateRHI>          RasterizerState;
    id<MTLRenderPipelineState>                    PipelineState;
    TArray<FMetalResourceBinding>                 VertexBuffers;
    TStaticArray<uint8, kMaxConstantBuffers>      BufferBindings[EShaderVisibility::Count];
    TStaticArray<uint8, EShaderVisibility::Count> NumBuffers;
    TArray<FMetalResourceBinding>                 TextureBindings[EShaderVisibility::Count];
    TArray<FMetalResourceBinding>                 SamplerBindings[EShaderVisibility::Count];
};

class FMetalComputePipelineStateRHI : public FRHIComputePipelineState
{
public:
    FMetalComputePipelineStateRHI()  = default;
    ~FMetalComputePipelineStateRHI() = default;

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;
};

class FMetalRayTracingPipelineStateRHI : public FRHIRayTracingPipelineState
{
public:
    FMetalRayTracingPipelineStateRHI()  = default;
    ~FMetalRayTracingPipelineStateRHI() = default;

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

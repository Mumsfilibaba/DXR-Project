#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RayTracing/RHIRayTracingPipelineState.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalShader.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalInputLayoutRHI>             FMetalVertexInputLayoutRef;
typedef TSharedRef<class FMetalDepthStencilStateRHI>       FMetalDepthStencilStateRef;
typedef TSharedRef<class FMetalGraphicsPipelineStateRHI>   FMetalGraphicsPipelineStateRef;
typedef TSharedRef<class FMetalComputePipelineStateRHI>    FMetalComputePipelineStateRef;
typedef TSharedRef<class FMetalMeshletPipelineStateRHI>    FMetalMeshletPipelineStateRef;
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
    id<MTLDepthStencilState> DepthStencilState;
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

    MTLCullMode GetMTLCullMode() const
    {
        return CullMode;
    }

private:
    MTLTriangleFillMode FillMode;
    MTLWinding          FrontFaceWinding;
    MTLCullMode         CullMode;
};

class FMetalBlendStateRHI : public FRHIBlendState
{
public:
    struct FBlendAttachment
    {
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

    bool IsAlphaToCoverageEnabled() const
    {
        return bAlphaToCoverageEnable;
    }

    bool IsLogicOpEnabled() const
    {
        return bLogicOpEnable;
    }

private:
    FBlendAttachment ColorAttachments[RHI_MAX_RENDER_TARGETS];
    bool             bAlphaToCoverageEnable;
    bool             bLogicOpEnable;
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

struct FMetalPipelineBindingLayout
{
    FMetalPipelineBindingLayout();
    ~FMetalPipelineBindingLayout();

    void Reset();
    void Collect(NSArray<id<MTLBinding>>* Bindings, EShaderVisibility::Type ShaderStage, bool bSkipVertexStreams);

    uint32 GetBufferBinding(EShaderVisibility::Type ShaderVisibility, uint32 BufferIndex) const
    {
        return BufferBindings[ShaderVisibility][BufferIndex];
    }

    uint32 GetNumBuffers(EShaderVisibility::Type ShaderVisibility) const
    {
        return NumBuffers[ShaderVisibility];
    }

    TArray<FMetalResourceBinding>                 VertexBuffers;
    TStaticArray<uint8, MAX_CONSTANT_BUFFERS>     BufferBindings[EShaderVisibility::Count];
    TStaticArray<uint8, EShaderVisibility::Count> NumBuffers;
    TArray<FMetalResourceBinding>                 TextureBindings[EShaderVisibility::Count];
    TArray<FMetalResourceBinding>                 SamplerBindings[EShaderVisibility::Count];
};

class FMetalGraphicsPipelineStateRHI : public FRHIGraphicsPipelineState, public FMetalDeviceChild
{
public:
    FMetalGraphicsPipelineStateRHI(FMetalDevice* InDevice, const FRHIGraphicsPipelineStateDesc& InDesc);
    virtual ~FMetalGraphicsPipelineStateRHI();

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize();

    FMetalBlendStateRHI*        GetMetalBlendState()        const { return BlendState.Get(); }
    FMetalDepthStencilStateRHI* GetMetalDepthStencilState() const { return DepthStencilState.Get(); }
    FMetalRasterizerStateRHI*   GetMetalRasterizerState()   const { return RasterizerState.Get(); }

    uint32 GetBufferBinding(EShaderVisibility::Type ShaderVisibility, uint32 BufferIndex) const
    {
        return Bindings.GetBufferBinding(ShaderVisibility, BufferIndex);
    }

    uint32 GetNumBuffers(EShaderVisibility::Type ShaderVisibility) const
    {
        return Bindings.GetNumBuffers(ShaderVisibility);
    }

    id<MTLRenderPipelineState> GetMTLPipelineState() const
    {
        return PipelineState;
    }

    MTLPrimitiveType GetMTLPrimitiveType() const
    {
        return PrimitiveType;
    }

private:
    FRHIGraphicsPipelineStateDesc          Desc;
    TSharedRef<FMetalBlendStateRHI>        BlendState;
    TSharedRef<FMetalDepthStencilStateRHI> DepthStencilState;
    TSharedRef<FMetalRasterizerStateRHI>   RasterizerState;
    id<MTLRenderPipelineState>             PipelineState;
    FMetalPipelineBindingLayout            Bindings;
    MTLPrimitiveType                       PrimitiveType;
};

class FMetalComputePipelineStateRHI : public FRHIComputePipelineState, public FMetalDeviceChild
{
public:
    FMetalComputePipelineStateRHI(FMetalDevice* InDevice);
    virtual ~FMetalComputePipelineStateRHI();

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize(const FRHIComputePipelineStateDesc& InDesc);

    uint32 GetBufferBinding(uint32 BufferIndex) const
    {
        return Bindings.GetBufferBinding(EShaderVisibility::Compute, BufferIndex);
    }

    uint32 GetNumBuffers() const
    {
        return Bindings.GetNumBuffers(EShaderVisibility::Compute);
    }

    id<MTLComputePipelineState> GetMTLPipelineState() const
    {
        return PipelineState;
    }

    uint32 GetMaxTotalThreadsPerThreadgroup() const
    {
        return MaxTotalThreadsPerThreadgroup;
    }

private:
    id<MTLComputePipelineState> PipelineState;
    FMetalPipelineBindingLayout Bindings;
    uint32                      MaxTotalThreadsPerThreadgroup;
};

class FMetalMeshletPipelineStateRHI : public FRHIMeshletPipelineState, public FMetalDeviceChild
{
public:
    FMetalMeshletPipelineStateRHI(FMetalDevice* InDevice, const FRHIMeshletPipelineStateDesc& InDesc);
    virtual ~FMetalMeshletPipelineStateRHI();

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize();

    FMetalDepthStencilStateRHI* GetMetalDepthStencilState() const { return DepthStencilState.Get(); }
    FMetalRasterizerStateRHI*   GetMetalRasterizerState()   const { return RasterizerState.Get(); }

    uint32 GetBufferBinding(EShaderVisibility::Type ShaderVisibility, uint32 BufferIndex) const
    {
        return Bindings.GetBufferBinding(ShaderVisibility, BufferIndex);
    }

    uint32 GetNumBuffers(EShaderVisibility::Type ShaderVisibility) const
    {
        return Bindings.GetNumBuffers(ShaderVisibility);
    }

    id<MTLRenderPipelineState> GetMTLPipelineState() const
    {
        return PipelineState;
    }

private:
    FRHIMeshletPipelineStateDesc           Desc;
    TSharedRef<FMetalBlendStateRHI>        BlendState;
    TSharedRef<FMetalDepthStencilStateRHI> DepthStencilState;
    TSharedRef<FMetalRasterizerStateRHI>   RasterizerState;
    id<MTLRenderPipelineState>             PipelineState;
    FMetalPipelineBindingLayout            Bindings;
};

class FMetalRayTracingPipelineStateRHI : public FRHIRayTracingPipelineState, public FMetalDeviceChild
{
public:
    FMetalRayTracingPipelineStateRHI(FMetalDevice* InDevice, const FRHIRayTracingPipelineStateDesc& InDesc);
    virtual ~FMetalRayTracingPipelineStateRHI();

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    // FRHIRayTracingPipelineState Interface
    virtual void GetExportName(ERayTracingShaderRecordKind Kind, uint32 RecordIndex, String& OutExportName) const override final;
    virtual uint32 GetNumExportNames(ERayTracingShaderRecordKind Kind) const override final;

    bool Initialize();

    id<MTLComputePipelineState> GetMTLPipelineState() const
    {
        return PipelineState;
    }

private:
    TArray<String>& GetExportNameArray(ERayTracingShaderRecordKind Kind);
    const TArray<String>& GetExportNameArray(ERayTracingShaderRecordKind Kind) const;

    FRHIRayTracingPipelineStateDesc Desc;
    id<MTLComputePipelineState>     PipelineState;
    TArray<String>                  RayGenerationExports;
    TArray<String>                  MissExports;
    TArray<String>                  CallableExports;
    TArray<String>                  HitGroupExports;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

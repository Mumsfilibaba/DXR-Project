#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RayTracing/RHIRayTracingPipelineState.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalSamplerState.h"
#include "MetalRHI/MetalShader.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalInputLayoutRHI>             FMetalVertexInputLayoutRef;
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

    uint32 GetNumVertexStreams() const
    {
        return NumVertexStreams;
    }

private:
    TArray<FRHIInputElementDesc> InputElements;
    MTLVertexDescriptor*         VertexDescriptor;
    uint32                       NumVertexStreams;
};

class FMetalDepthStencilStateRHI : public FRHIDepthStencilState
{
public:
    FMetalDepthStencilStateRHI(const FRHIDepthStencilStateDesc& InDesc);
    virtual ~FMetalDepthStencilStateRHI();

    // FRHIDepthStencilState Interface
    virtual void* GetRHINativeState() const override final;
};

class FMetalRasterizerStateRHI : public FRHIRasterizerState
{
public:
    FMetalRasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc);
    virtual ~FMetalRasterizerStateRHI();

    // FRHIRasterizerState Interface
    virtual void* GetRHINativeState() const override final;
};

class FMetalBlendStateRHI : public FRHIBlendState
{
public:
    struct FBlendAttachment
    {
        bool operator==(const FBlendAttachment&) const = default;

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

static_assert(MAX_CONSTANT_BUFFERS <= 16 && MAX_SRVS <= 16 && MAX_UAVS <= 16 && MAX_SAMPLER_STATES <= 16, "Bind plan masks are 16 bits wide");

struct FMetalStageBindPlan
{
    bool operator==(const FMetalStageBindPlan&) const = default;

    uint8  ConstantBufferSlots[MAX_CONSTANT_BUFFERS];
    uint8  ShaderResourceSlots[MAX_SRVS];
    uint8  UnorderedAccessSlots[MAX_UAVS];
    uint8  SamplerSlots[MAX_SAMPLER_STATES];
    uint16 ConstantBufferMask;
    uint16 ShaderResourceMask;
    uint16 ShaderResourceBufferMask;
    uint16 UnorderedAccessMask;
    uint16 UnorderedAccessBufferMask;
    uint16 SamplerMask;
    uint16 StaticSamplerMask;
    uint8  ShaderConstantsSlot;
    uint8  ResourceHeapSlot;
    uint8  SamplerHeapSlot;
    uint8  NumShaderConstants;
    uint8  ShaderResourceNullTypes[MAX_SRVS];
    uint8  UnorderedAccessNullTypes[MAX_UAVS];
};

struct FMetalPipelineBindingLayout
{
    static constexpr uint8 InvalidSlot = UINT8_MAX;

    FMetalPipelineBindingLayout();
    ~FMetalPipelineBindingLayout();

    bool Collect(const TArray<FMSLShaderBinding>& ShaderBindings, EShaderVisibility::Type Stage, uint16 ShaderConstantsSize);
    void PruneToReflection(EShaderVisibility::Type Stage, NSArray<id<MTLBinding>>* ReflectedBindings);
    bool ConflictsWithVertexInputs(const FMetalInputLayoutRHI* InputLayout) const;

    uint8 GetSlot(EShaderVisibility::Type Stage, EMSLBindingType BindingType, uint32 RegisterIndex) const;

    bool UsesBindlessHeaps() const
    {
        return bUsesBindlessHeaps;
    }

    FMetalStageBindPlan Stages[EShaderVisibility::Count];
    bool                bUsesBindlessHeaps;
};

struct FMetalStaticSamplerBinding
{
    EShaderVisibility::Type           Stage;
    uint8                             RegisterIndex;
    TSharedRef<FMetalSamplerStateRHI> Sampler;
};

enum class EMetalRenderPipelineType : uint8
{
    Graphics = 0,
    Meshlet  = 1,
};

struct FMetalRenderStateBlock
{
    bool operator==(const FMetalRenderStateBlock&) const = default;

    id<MTLDepthStencilState> DepthStencilState  = nil;
    MTLWinding               FrontFacingWinding = MTLWindingClockwise;
    MTLCullMode              CullMode           = MTLCullModeNone;
    MTLTriangleFillMode      FillMode           = MTLTriangleFillModeFill;
    MTLDepthClipMode         DepthClipMode      = MTLDepthClipModeClip;
};

struct FMetalCachedRenderPipeline : public FRefCounted
{
    FMetalCachedRenderPipeline();
    ~FMetalCachedRenderPipeline();

    id<MTLRenderPipelineState>   PipelineState = nil;
    MTLRenderPipelineReflection* Reflection    = nil;
    NSArray<id<MTLFunction>>*    Functions     = nil;
};

struct FMetalCachedComputePipeline : public FRefCounted
{
    FMetalCachedComputePipeline();
    ~FMetalCachedComputePipeline();

    id<MTLComputePipelineState>   PipelineState = nil;
    MTLComputePipelineReflection* Reflection    = nil;
    NSArray<id<MTLFunction>>*     Functions     = nil;
};

class FMetalRenderPipeline : public FMetalDeviceChild
{
public:
    FMetalRenderPipeline(FMetalDevice* InDevice, EMetalRenderPipelineType InType);
    virtual ~FMetalRenderPipeline();

    bool Initialize(
        const FRHIDepthStencilStateDesc&   DepthStencilDesc, 
        const FRHIRasterizerStateDesc&     RasterizerDesc, 
        const FRHIGraphicsPipelineFormats& Formats,
        const FRHIViewInstancingState&     InViewInstancing);

    bool CreateStaticSamplers(const TArrayView<const FRHIStaticSamplerInfo>& Infos);
    void SetPipelineState(const TSharedRef<FMetalCachedRenderPipeline>& InPipeline);

    void Apply(id<MTLRenderCommandEncoder> Encoder, const FMetalRenderPipeline* Previous) const;

    FMetalPipelineBindingLayout&              GetBindings()               { return Bindings; }
    const FMetalPipelineBindingLayout&        GetBindings()         const { return Bindings; }
    const TArray<FMetalStaticSamplerBinding>& GetStaticSamplers()   const { return StaticSamplers; }
    id<MTLRenderPipelineState>                GetMTLPipelineState() const { return Pipeline ? Pipeline->PipelineState : nil; }
    MTLPrimitiveType                          GetPrimitiveType()    const { return PrimitiveType; }
    EMetalRenderPipelineType                  GetType()             const { return Type; }
    const FRHIViewInstancingState&            GetViewInstancing()   const { return ViewInstancing; }
    uint32                                    GetNumVertexStreams() const { return NumVertexStreams; }

    void SetPrimitiveType(MTLPrimitiveType InPrimitiveType)
    {
        PrimitiveType = InPrimitiveType;
    }

    void SetNumVertexStreams(uint32 InNumVertexStreams)
    {
        NumVertexStreams = InNumVertexStreams;
    }

private:
    EMetalRenderPipelineType               Type;
    TSharedRef<FMetalCachedRenderPipeline> Pipeline;
    FMetalRenderStateBlock                 RenderState;
    FMetalPipelineBindingLayout            Bindings;
    TArray<FMetalStaticSamplerBinding>     StaticSamplers;
    MTLPrimitiveType                       PrimitiveType;
    FRHIViewInstancingState                ViewInstancing;
    uint32                                 NumVertexStreams;
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

    const FMetalRenderPipeline& GetRenderPipeline() const { return RenderPipeline; }

private:
    FRHIGraphicsPipelineStateDesc Desc;
    FMetalRenderPipeline          RenderPipeline;
    String                        DebugName;
};

class FMetalComputePipelineStateRHI : public FRHIComputePipelineState, public FMetalDeviceChild
{
public:
    FMetalComputePipelineStateRHI(FMetalDevice* InDevice, const FRHIComputePipelineStateDesc& InDesc);
    virtual ~FMetalComputePipelineStateRHI();

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize();

    const FMetalPipelineBindingLayout&        GetBindings() const              { return Bindings; }
    const TArray<FMetalStaticSamplerBinding>& GetStaticSamplers() const        { return StaticSamplers; }
    id<MTLComputePipelineState>               GetMTLPipelineState() const      { return Pipeline ? Pipeline->PipelineState : nil; }
    MTLSize                                   GetThreadsPerThreadgroup() const { return ThreadsPerThreadgroup; }

private:
    FRHIComputePipelineStateDesc            Desc;
    TSharedRef<FMetalCachedComputePipeline> Pipeline;
    FMetalPipelineBindingLayout             Bindings;
    TArray<FMetalStaticSamplerBinding>      StaticSamplers;
    MTLSize                                 ThreadsPerThreadgroup;
    String                                  DebugName;
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

    const FMetalRenderPipeline& GetRenderPipeline() const        { return RenderPipeline; }
    MTLSize                     GetMeshThreadgroupSize() const   { return MeshThreadgroupSize; }
    MTLSize                     GetObjectThreadgroupSize() const { return ObjectThreadgroupSize; }

private:
    FRHIMeshletPipelineStateDesc Desc;
    FMetalRenderPipeline         RenderPipeline;
    MTLSize                      MeshThreadgroupSize;
    MTLSize                      ObjectThreadgroupSize;
    String                       DebugName;
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
    String                          DebugName;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

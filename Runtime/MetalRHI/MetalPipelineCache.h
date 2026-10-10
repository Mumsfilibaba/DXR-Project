#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Platform/CriticalSection.h"
#include "MetalRHI/MetalPipelineState.h"

class FMetalDevice;

struct FMetalVertexAttributeKey
{
    bool operator==(const FMetalVertexAttributeKey&) const = default;

    uint32                AttributeIndex;
    MTLVertexFormat       Format;
    uint32                Offset;
    uint32                BufferIndex;
    uint32                Stride;
    MTLVertexStepFunction StepFunction;
    uint32                StepRate;
};

struct FMetalRenderPipelineKey
{
    static FMetalRenderPipelineKey Create(MTLRenderPipelineDescriptor* Descriptor);
    static FMetalRenderPipelineKey Create(MTLMeshRenderPipelineDescriptor* Descriptor);

    bool operator==(const FMetalRenderPipelineKey& Other) const;
    uint64 GetHash() const;

    EMetalRenderPipelineType              Type;
    id<MTLFunction>                       VertexOrMeshFunction;
    id<MTLFunction>                       ObjectFunction;
    id<MTLFunction>                       FragmentFunction;
    TArray<FMetalVertexAttributeKey>      VertexAttributes;
    MTLPixelFormat                        ColorFormats[RHI_MAX_RENDER_TARGETS];
    FMetalBlendStateRHI::FBlendAttachment Blend[RHI_MAX_RENDER_TARGETS];
    MTLPixelFormat                        DepthFormat;
    MTLPixelFormat                        StencilFormat;
    MTLPrimitiveTopologyClass             TopologyClass;
    uint32                                SampleCount;
    uint32                                MaxVertexAmplificationCount;
    bool                                  bAlphaToCoverage;
};

class FMetalPipelineCache
{
public:
    explicit FMetalPipelineCache(FMetalDevice* InDevice);
    ~FMetalPipelineCache();

    TSharedRef<FMetalCachedRenderPipeline>  GetOrCreateRenderPipeline(const FMetalRenderPipelineKey& Key, MTLRenderPipelineDescriptor* Descriptor);
    TSharedRef<FMetalCachedRenderPipeline>  GetOrCreateMeshRenderPipeline(const FMetalRenderPipelineKey& Key, MTLMeshRenderPipelineDescriptor* Descriptor);
    TSharedRef<FMetalCachedComputePipeline> GetOrCreateComputePipeline(id<MTLFunction> Function, MTLComputePipelineDescriptor* Descriptor);

    void Prune();

private:
    struct FRenderEntry
    {
        FMetalRenderPipelineKey                Key;
        TSharedRef<FMetalCachedRenderPipeline> Pipeline;
    };

    template<typename CreateFunctionType>
    TSharedRef<FMetalCachedRenderPipeline> FindOrCreateRenderPipeline(const FMetalRenderPipelineKey& Key, CreateFunctionType&& CreateFunction);
    TSharedRef<FMetalCachedRenderPipeline> FindRenderPipeline(uint64 Hash, const FMetalRenderPipelineKey& Key) const;

    FMetalDevice*                                               Device;
    TMap<uint64, TArray<FRenderEntry>>                          RenderPipelines;
    TMap<const void*, TSharedRef<FMetalCachedComputePipeline>>  ComputePipelines;
    FCriticalSection                                            RenderPipelinesCS;
    FCriticalSection                                            ComputePipelinesCS;
};

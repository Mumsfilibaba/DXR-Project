#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/Pair.h"
#include "Core/Platform/CriticalSection.h"
#include "RHI/RHIPipelineState.h"
#include "RHI/RHISamplerState.h"
#include "RHI/RHIShader.h"
#include "RHI/RayTracing/RHIRayTracingPipelineState.h"

struct EPipelineCacheKind
{
    enum Type : uint8
    {
        DepthStencilState = 0,
        RasterizerState,
        BlendState,
        InputLayout,
        Shader,
        GraphicsPipeline,
        ComputePipeline,
        MeshletPipeline,
        RayTracingPipeline,
        Count,
    };
};

class RHI_API FRHIPipelineStateCache
{
public:
    static bool Initialize();
    static void Release();

    static FORCEINLINE FRHIPipelineStateCache& Get()
    {
        return *PipelineStateCache;
    }

    static FORCEINLINE FRHIPipelineStateCache* TryGet()
    {
        return PipelineStateCache;
    }

public:
    struct FStats
    {
        uint32 NumRequests[EPipelineCacheKind::Count] = { };
        uint32 NumCreated[EPipelineCacheKind::Count]  = { };
    };

    NODISCARD FRHIDepthStencilState*       GetOrCreateDepthStencilState(const FRHIDepthStencilStateDesc& Desc);
    NODISCARD FRHIRasterizerState*         GetOrCreateRasterizerState(const FRHIRasterizerStateDesc& Desc);
    NODISCARD FRHIBlendState*              GetOrCreateBlendState(const FRHIBlendStateDesc& Desc);
    NODISCARD FRHIInputLayout*             GetOrCreateInputLayout(const TArray<FRHIInputElementDesc>& InputElements);
    NODISCARD FRHIShader*                  GetOrCreateShader(EShaderStage Stage, const TArray<uint8>& ShaderCode);
    NODISCARD FRHIGraphicsPipelineState*   GetOrCreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& Desc);
    NODISCARD FRHIComputePipelineState*    GetOrCreateComputePipelineState(const FRHIComputePipelineStateDesc& Desc);
    NODISCARD FRHIMeshletPipelineState*    GetOrCreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc& Desc);
    NODISCARD FRHIRayTracingPipelineState* GetOrCreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& Desc);

    void FlushPipelineStates();

    NODISCARD const FStats& GetStats() const
    {
        return Stats;
    }

    void LogStats() const;

private:
    FRHIPipelineStateCache();
    ~FRHIPipelineStateCache();

    struct FGraphicsKey
    {
        explicit FGraphicsKey(const FRHIGraphicsPipelineStateDesc& InDesc);

        NODISCARD bool operator==(const FGraphicsKey& Other) const;

        FRHIGraphicsPipelineStateDesc Desc;
        TArray<FRHIStaticSamplerInfo> StaticSamplers;

        FRHIVertexShaderRef        VertexShader;
        FRHIHullShaderRef          HullShader;
        FRHIDomainShaderRef        DomainShader;
        FRHIGeometryShaderRef      GeometryShader;
        FRHIPixelShaderRef         PixelShader;
        FRHIInputLayoutRef         InputLayout;
        FRHIDepthStencilStateRef   DepthStencilState;
        FRHIRasterizerStateRef     RasterizerState;
        FRHIBlendStateRef          BlendState;
    };

    struct FComputeKey
    {
        explicit FComputeKey(const FRHIComputePipelineStateDesc& InDesc);

        NODISCARD bool operator==(const FComputeKey& Other) const;

        FRHIComputePipelineStateDesc  Desc;
        TArray<FRHIStaticSamplerInfo> StaticSamplers;
        FRHIComputeShaderRef          Shader;
    };

    struct FMeshletKey
    {
        explicit FMeshletKey(const FRHIMeshletPipelineStateDesc& InDesc);

        NODISCARD bool operator==(const FMeshletKey& Other) const;

        FRHIMeshletPipelineStateDesc  Desc;
        TArray<FRHIStaticSamplerInfo> StaticSamplers;

        FRHIAmplificationShaderRef AmplificationShader;
        FRHIMeshShaderRef          MeshShader;
        FRHIPixelShaderRef         PixelShader;
        FRHIDepthStencilStateRef   DepthStencilState;
        FRHIRasterizerStateRef     RasterizerState;
        FRHIBlendStateRef          BlendState;
    };

    struct FRayTracingKey
    {
        explicit FRayTracingKey(const FRHIRayTracingPipelineStateDesc& InDesc);

        NODISCARD bool operator==(const FRayTracingKey& Other) const
        {
            return Desc == Other.Desc;
        }

        FRHIRayTracingPipelineStateDesc  Desc;
        TArray<FRHIRayTracingShaderRef>  ReferencedShaders;
        FRHIRayTracingPipelineStateRef   BasePipeline;
    };

    struct FShaderKey
    {
        EShaderStage   Stage = EShaderStage::Unknown;
        TArray<uint8>  ShaderCode;

        NODISCARD bool operator==(const FShaderKey& Other) const;
    };

    /** @brief Content hash to the entries that share it, since distinct keys can collide. */
    template<typename KeyType, typename ValueType>
    using TCacheMap = TMap<uint64, TArray<TPair<KeyType, ValueType>>>;

    template<typename ObjectType, typename KeyType, typename CreateFunctorType>
    ObjectType* FindOrCreate(TCacheMap<KeyType, TSharedRef<ObjectType>>& Map, uint64 Hash, const KeyType& Key, EPipelineCacheKind::Type Kind, CreateFunctorType&& Create);

    TCacheMap<FRHIDepthStencilStateDesc, FRHIDepthStencilStateRef> DepthStencilStates;
    TCacheMap<FRHIRasterizerStateDesc, FRHIRasterizerStateRef>     RasterizerStates;
    TCacheMap<FRHIBlendStateDesc, FRHIBlendStateRef>               BlendStates;
    TCacheMap<TArray<FRHIInputElementDesc>, FRHIInputLayoutRef>    InputLayouts;
    TCacheMap<FShaderKey, FRHIShaderRef>                           Shaders;
    TCacheMap<FGraphicsKey, FRHIGraphicsPipelineStateRef>          GraphicsPipelines;
    TCacheMap<FComputeKey, FRHIComputePipelineStateRef>            ComputePipelines;
    TCacheMap<FMeshletKey, FRHIMeshletPipelineStateRef>            MeshletPipelines;
    TCacheMap<FRayTracingKey, FRHIRayTracingPipelineStateRef>      RayTracingPipelines;

    FStats           Stats;
    FCriticalSection CacheCS;

    static FRHIPipelineStateCache* PipelineStateCache;
};

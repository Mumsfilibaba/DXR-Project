#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/Set.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Tasks/TaskHandle.h"
#include "RendererCore/Shaders/ShaderType.h"
#include "RHI/RHIShader.h"

template<EShaderStage Stage>
struct TShaderStageType;

#define IMPLEMENT_SHADER_STAGE_TYPE(InStage, InType) \
    template<>                                       \
    struct TShaderStageType<EShaderStage::InStage>   \
    {                                                \
        using FType    = InType;                     \
        using FRefType = InType##Ref;                \
    }

IMPLEMENT_SHADER_STAGE_TYPE(Vertex,          FRHIVertexShader);
IMPLEMENT_SHADER_STAGE_TYPE(Hull,            FRHIHullShader);
IMPLEMENT_SHADER_STAGE_TYPE(Domain,          FRHIDomainShader);
IMPLEMENT_SHADER_STAGE_TYPE(Geometry,        FRHIGeometryShader);
IMPLEMENT_SHADER_STAGE_TYPE(Mesh,            FRHIMeshShader);
IMPLEMENT_SHADER_STAGE_TYPE(Amplification,   FRHIAmplificationShader);
IMPLEMENT_SHADER_STAGE_TYPE(Pixel,           FRHIPixelShader);
IMPLEMENT_SHADER_STAGE_TYPE(Compute,         FRHIComputeShader);
IMPLEMENT_SHADER_STAGE_TYPE(RayGen,          FRHIRayGenShader);
IMPLEMENT_SHADER_STAGE_TYPE(RayAnyHit,       FRHIRayAnyHitShader);
IMPLEMENT_SHADER_STAGE_TYPE(RayClosestHit,   FRHIRayClosestHitShader);
IMPLEMENT_SHADER_STAGE_TYPE(RayMiss,         FRHIRayMissShader);
IMPLEMENT_SHADER_STAGE_TYPE(RayIntersection, FRHIRayIntersectionShader);
IMPLEMENT_SHADER_STAGE_TYPE(RayCallable,     FRHIRayCallableShader);

#undef IMPLEMENT_SHADER_STAGE_TYPE

struct FShaderCacheKey
{
    NODISCARD bool operator==(const FShaderCacheKey& Other) const
    {
        return Type == Other.Type && PermutationID == Other.PermutationID;
    }

    FShaderType* Type          = nullptr;
    int32        PermutationID = 0;
};

template<>
struct THash<FShaderCacheKey>
{
    NODISCARD static uint64 GetHash(const FShaderCacheKey& Value)
    {
        uint64 Result = THash<FShaderType*>::GetHash(Value.Type);
        HashCombine(Result, Value.PermutationID);
        return Result;
    }
};

class RENDERERCORE_API FShaderCache
{
public:
    static bool Initialize();
    static void Release();

    NODISCARD static FShaderPermutationDesc CreatePermutationDesc(int32 PermutationID);

    static FORCEINLINE FShaderCache& Get()
    {
        return *ShaderCache;
    }

    static FORCEINLINE FShaderCache* TryGet()
    {
        return ShaderCache;
    }

public:
    void PrewarmAsync();
    void FlushCompiledShaders();

    template<typename ShaderType>
    NODISCARD typename TShaderStageType<ShaderType::Stage>::FRefType GetShader(const typename ShaderType::FPermutation& Permutation)
    {
        using FStageType = TShaderStageType<ShaderType::Stage>;

        typename ShaderType::FPermutation LocalPermutation = Permutation;
        if constexpr (CShaderTypeHasRemapPermutation<ShaderType>)
        {
            LocalPermutation = ShaderType::RemapPermutation(LocalPermutation);
        }

        FRHIShaderRef Shader = GetOrCompile(ShaderType::GetStaticType(), LocalPermutation.GetPermutationID());
        if (!Shader)
        {
            return typename FStageType::FRefType();
        }

        return typename FStageType::FRefType(static_cast<typename FStageType::FType*>(Shader.ReleaseOwnership()));
    }

    template<typename ShaderType>
    NODISCARD typename TShaderStageType<ShaderType::Stage>::FRefType GetShader()
    {
        return GetShader<ShaderType>(typename ShaderType::FPermutation());
    }

private:
    FShaderCache();
    ~FShaderCache();

    NODISCARD FRHIShaderRef GetOrCompile(FShaderType& Type, int32 PermutationID);

    void RecordPermutation(FShaderType& Type, int32 PermutationID);
    void SaveManifest();

    TMap<FShaderCacheKey, FRHIShaderRef> Shaders;
    FCriticalSection                     ShadersCS;
    TMap<FShaderType*, TSet<int32>>      RequestedPermutations;
    FCriticalSection                     RequestedPermutationsCS;
    bool                                 bManifestDirty;
    FTaskHandle                          PrewarmTask;

    static FShaderCache* ShaderCache;
};

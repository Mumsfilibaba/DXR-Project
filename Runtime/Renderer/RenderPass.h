#pragma once
#include "Core/Templates/TypeHash.h"
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"
#include "RendererCore/Shaders/CommonShaderPermutations.h"
#include "RendererCore/VertexDeclaration.h"
#include "Engine/Resources/Material.h"

class FSceneRenderer;
struct FFrameResources;
struct FVertexStreamBinding;

constexpr uint32 PIPELINE_FLAG_DOUBLE_SIDED   = 1u << 0;
constexpr uint32 PIPELINE_FLAG_DEPTH_CLIPPING = 1u << 1;

struct FGraphicsPipelineKey
{
    FGraphicsPipelineKey() = default;

    FGraphicsPipelineKey(int32 InPermutationID, uint32 InPipelineFlags, uint8 InDeclarationID)
        : PermutationID(InPermutationID)
        , PipelineFlags(InPipelineFlags)
        , DeclarationID(InDeclarationID)
    {
    }

    bool operator==(const FGraphicsPipelineKey& Other) const
    {
        return PermutationID == Other.PermutationID
            && PipelineFlags == Other.PipelineFlags
            && DeclarationID == Other.DeclarationID;
    }

    int32  PermutationID = 0;
    uint32 PipelineFlags = 0;
    uint8  DeclarationID = 0;
};

template<>
struct THash<FGraphicsPipelineKey>
{
    static uint64 GetHash(const FGraphicsPipelineKey& Value)
    {
        uint64 Result = THash<int32>::GetHash(Value.PermutationID);
        HashCombine(Result, Value.PipelineFlags);
        HashCombine(Result, Value.DeclarationID);
        return Result;
    }
};

struct FMaterialFeatures
{
    FMaterialFeatures() = default;

    FMaterialFeatures(const FMaterial* Material, const FVertexDeclaration& Declaration)
        : Flags(Material->GetMaterialFlags() & FMaterial::GetSupportedMaterialFlags(Declaration))
    {
    }

    explicit FMaterialFeatures(EMaterialFlags InFlags)
        : Flags(InFlags)
    {
    }

    bool HasHeightMap() const
    {
        return IsEnumFlagSet(Flags, EMaterialFlags::EnableHeight);
    }

    bool HasAlphaMask() const
    {
        return IsEnumFlagSet(Flags, EMaterialFlags::EnableAlpha);
    }

    bool HasNormalMap() const
    {
        return IsEnumFlagSet(Flags, EMaterialFlags::EnableNormalMapping);
    }

    bool IsNormalMapPositiveY() const
    {
        return IsEnumFlagSet(Flags, EMaterialFlags::NormalMapPositiveY);
    }

    bool HasParallaxClipping() const
    {
        return IsEnumFlagSet(Flags, EMaterialFlags::EnableParallaxClipping) && HasHeightMap();
    }

    bool IsDoubleSided() const
    {
        return IsEnumFlagSet(Flags, EMaterialFlags::DoubleSided);
    }

    EVertexAttributeFlags GetDepthOnlyAttributes() const
    {
        return CreateDepthOnlyAttributes(HasHeightMap(), HasAlphaMask());
    }

    NODISCARD FMaterialPermutation CreatePermutation() const
    {
        FMaterialPermutation Permutation;
        Permutation.Set<FParallax>(HasHeightMap());
        Permutation.Set<FClipping>(HasParallaxClipping());
        Permutation.Set<FAlphaMask>(HasAlphaMask());
        Permutation.Set<FDoubleSided>(IsDoubleSided());
        return Permutation;
    }

    EMaterialFlags Flags = EMaterialFlags::None;
};

struct FComputePipelineStateInstance
{
    FRHIComputeShaderRef        Shader;
    FRHIComputePipelineStateRef PipelineState;
};

struct FGraphicsPipelineStateInstance
{
    FRHIVertexShaderRef          VertexShader;
    FRHIGeometryShaderRef        GeometryShader;
    FRHIPixelShaderRef           PixelShader;
    const FVertexStreamBinding*  StreamBinding = nullptr;
    FRHIGraphicsPipelineStateRef PipelineState;
};

class FRenderPass
{
public:
    FRenderPass(FSceneRenderer* InRenderer);
    virtual ~FRenderPass();

    virtual void PreparePipelineState(FMaterial*, const FVertexDeclaration&, const FFrameResources&) { }

    FSceneRenderer* GetRenderer() const
    {
        return Renderer;
    }

private:
    FSceneRenderer* Renderer;
};

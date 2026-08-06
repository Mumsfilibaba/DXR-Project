#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"
#include "RendererCore/VertexDeclaration.h"
#include "Engine/Resources/Material.h"

class FSceneRenderer;
struct FFrameResources;
struct FVertexStreamBinding;

constexpr uint64 PSO_KEY_BINDLESS_BIT      = uint64(1) << 32;
constexpr uint64 PSO_KEY_DECLARATION_SHIFT = 33;

inline uint64 MakeMaterialPSOKey(int32 MaterialFlags, bool bBindless, uint8 DeclarationID)
{
    const uint64 Flags = static_cast<uint64>(static_cast<uint32>(MaterialFlags));
    const uint64 Key   = Flags | (static_cast<uint64>(DeclarationID) << PSO_KEY_DECLARATION_SHIFT);
    return bBindless ? (Key | PSO_KEY_BINDLESS_BIT) : Key;
}

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
        EVertexAttributeFlags Attributes = EVertexAttributeFlags::Position;
        if (HasAlphaMask() || HasHeightMap())
        {
            Attributes |= EVertexAttributeFlags::TexCoord0;
        }

        if (HasHeightMap())
        {
            Attributes |= EVertexAttributeFlags::TangentBasis;
        }

        return Attributes;
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
    FRHIDepthStencilStateRef     DepthStencilState;
    FRHIBlendStateRef            BlendState;
    FRHIRasterizerStateRef       RasterizerState;
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

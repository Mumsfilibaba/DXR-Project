#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"

class FMaterial;
class FSceneRenderer;
struct FFrameResources;

constexpr uint64 PSO_KEY_BINDLESS_BIT = uint64(1) << 32;

inline uint64 MakeMaterialPSOKey(int32 MaterialFlags, bool bBindless)
{
    const uint64 Flags = static_cast<uint64>(static_cast<uint32>(MaterialFlags));
    return bBindless ? (Flags | PSO_KEY_BINDLESS_BIT) : Flags;
}

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

    FRHIInputLayoutRef           InputLayout;
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

    virtual void PreparePipelineState(FMaterial* Material, const FFrameResources& FrameResources) { }

    FSceneRenderer* GetRenderer() const
    {
        return Renderer;
    }

private:
    FSceneRenderer* Renderer;
};

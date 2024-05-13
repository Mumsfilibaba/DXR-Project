#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"

class FMaterial;
class FSceneRenderer;
struct FFrameResources;

struct FPipelineStateInstance
{
    FRHIVertexShaderRef          VertexShader;
    FRHIGeometryShaderRef        GeometryShader;
    FRHIPixelShaderRef           PixelShader;

    FRHIVertexInputLayoutRef     InputLayout;
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

    virtual void InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources) { }

    FSceneRenderer* GetRenderer() const
    {
        return Renderer;
    }

private:
    FSceneRenderer* Renderer;
};
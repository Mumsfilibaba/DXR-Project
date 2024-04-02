#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/Scene/Scene.h"

class FRayTracer : public FRenderPass
{
public:
    FRayTracer(FSceneRenderer* InRenderer)
        : FRenderPass(InRenderer)
    {
    }

    bool Initialize(FFrameResources& Resources);
    void Release();

    void PreRender(FRHICommandList& CommandList, FFrameResources& Resources, FRendererScene* Scene);

private:
    FRHIRayTracingPipelineStateRef Pipeline;
    FRHIRayGenShaderRef            RayGenShader;
    FRHIRayMissShaderRef           RayMissShader;
    FRHIRayClosestHitShaderRef     RayClosestHitShader;
};
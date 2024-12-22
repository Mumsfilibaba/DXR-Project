#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"

class FRayTracer : public FRenderPass
{
public:
    FRayTracer(FSceneRenderer* InRenderer)
        : FRenderPass(InRenderer)
    {
    }

    bool Initialize(FFrameResources& Resources);
    void Release();

    void PreRender(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene);

private:
    FRHIRayTracingPipelineStateRef Pipeline;
    FRHIRayGenShaderRef            RayGenShader;
    FRHIRayMissShaderRef           RayMissShader;
    FRHIRayClosestHitShaderRef     RayClosestHitShader;
};
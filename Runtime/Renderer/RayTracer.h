#pragma once
#include "FrameResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/Scene/Scene.h"

class RENDERER_API FRayTracer
{
public:
    FRayTracer()  = default;
    ~FRayTracer() = default;

    bool Init(FFrameResources& Resources);
    void Release();

    void PreRender(FRHICommandList& CommandList, FFrameResources& Resources, const FScene& Scene);

private:
    FRHIRayTracingPipelineStateRef Pipeline;
    FRHIRayGenShaderRef            RayGenShader;
    FRHIRayMissShaderRef           RayMissShader;
    FRHIRayClosestHitShaderRef     RayClosestHitShader;
};
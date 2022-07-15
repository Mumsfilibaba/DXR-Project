#pragma once
#include "FrameResources.h"

#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRayTracer

class RENDERER_API FRayTracer
{
public:
    FRayTracer() = default;
    ~FRayTracer() = default;

    bool Init(FFrameResources& Resources);
    void Release();

    void PreRender(FRHICommandList& CmdList, FFrameResources& Resources, const FScene& Scene);

private:
    TSharedRef<FRHIRayTracingPipelineState> Pipeline;

    TSharedRef<FRHIRayGenShader>        RayGenShader;
    TSharedRef<FRHIRayMissShader>       RayMissShader;
    TSharedRef<FRHIRayClosestHitShader> RayClosestHitShader;
};
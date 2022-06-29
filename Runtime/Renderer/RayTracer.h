#pragma once
#include "FrameResources.h"

#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRayTracer

class RENDERER_API CRayTracer
{
public:
    CRayTracer() = default;
    ~CRayTracer() = default;

    bool Init(SFrameResources& Resources);
    void Release();

    void PreRender(CRHICommandList& CmdList, SFrameResources& Resources, const CScene& Scene);

private:
    TSharedRef<FRHIRayTracingPipelineState> Pipeline;

    TSharedRef<FRHIRayGenShader>        RayGenShader;
    TSharedRef<FRHIRayMissShader>       RayMissShader;
    TSharedRef<FRHIRayClosestHitShader> RayClosestHitShader;
};
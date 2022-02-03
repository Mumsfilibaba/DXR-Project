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
    TSharedRef<CRHIRayTracingPipelineState> Pipeline;

    TSharedRef<CRHIRayGenShader>        RayGenShader;
    TSharedRef<CRHIRayMissShader>       RayMissShader;
    TSharedRef<CRHIRayClosestHitShader> RayClosestHitShader;
};
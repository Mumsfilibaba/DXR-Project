#pragma once
#include "FrameResources.h"

#include "CoreRHI/RHICommandList.h"

#include "Scene/Scene.h"

class CRayTracer
{
public:
    CRayTracer() = default;
    ~CRayTracer() = default;

    bool Init( SFrameResources& Resources );
    void Release();

    void PreRender( CRHICommandList& CmdList, SFrameResources& Resources, const CScene& Scene );

private:
    TSharedRef<CRHIRayTracingPipelineState> Pipeline;
    TSharedRef<CRHIRayGenShader>        RayGenShader;
    TSharedRef<CRHIRayMissShader>       RayMissShader;
    TSharedRef<CRHIRayClosestHitShader> RayClosestHitShader;
};
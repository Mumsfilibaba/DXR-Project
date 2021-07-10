#pragma once
#include "FrameResources.h"

#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

class RayTracer
{
public:
    RayTracer() = default;
    ~RayTracer() = default;

    bool Init( FrameResources& Resources );
    void Release();

    void PreRender( CommandList& CmdList, FrameResources& Resources, const Scene& Scene );

private:
    TRef<RayTracingPipelineState> Pipeline;
    TRef<RayGenShader>        RayGenShader;
    TRef<RayMissShader>       RayMissShader;
    TRef<RayClosestHitShader> RayClosestHitShader;
};
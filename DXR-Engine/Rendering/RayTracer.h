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

    void PreRender( CommandList& CmdList, FrameResources& Resources, const CScene& Scene );

private:
    TSharedRef<RayTracingPipelineState> Pipeline;
    TSharedRef<RayGenShader>        RayGenShader;
    TSharedRef<RayMissShader>       RayMissShader;
    TSharedRef<RayClosestHitShader> RayClosestHitShader;
};
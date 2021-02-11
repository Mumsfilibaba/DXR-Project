#pragma once
#include "FrameResources.h"

#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

class RayTracer
{
public:
    RayTracer()  = default;
    ~RayTracer() = default;

    Bool Init();

    void PreRender(CommandList& CmdList, FrameResources& Resources, const Scene& Scene);

private:
    TRef<RayTracingPipelineState> Pipeline;
};
#pragma once
#include "FrameResources.h"

#include "RenderLayer/CommandList.h"

class RayTracer
{
public:
    RayTracer()  = default;
    ~RayTracer() = default;

    Bool Init(FrameResources& Resource);

    void PreRender(CommandList& CmdList, FrameResources& Resources);

private:
    TSharedRef<RayTracingPipelineState> Pipeline;
};
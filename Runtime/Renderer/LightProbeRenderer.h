#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHICommandList.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/Graph/FrameResources.h"

class FLightProbeRenderer : public FRenderPass
{
public:
    FLightProbeRenderer(FSceneRenderer* InRenderer);
    virtual ~FLightProbeRenderer();

    bool Initialize(FFrameResources& FrameResources);
};

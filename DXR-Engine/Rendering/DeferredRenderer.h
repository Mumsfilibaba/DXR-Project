#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "Scene/Scene.h"

#include "RenderLayer/CommandList.h"

class DeferredRenderer
{
public:
    DeferredRenderer()  = default;
    ~DeferredRenderer() = default;

    Bool Init(FrameResources& FrameResources);
    void Release();

    void RenderPrePass(CommandList& CmdList, const FrameResources& FrameResources);
    void RenderBasePass(CommandList& CmdList, const FrameResources& FrameResources);
    void RenderDeferredTiledLightPass(CommandList& CmdList, const FrameResources& FrameResources, const LightSetup& LightSetup);

    Bool ResizeResources(FrameResources& FrameResources);

private:
    Bool CreateGBuffer(FrameResources& FrameResources);

    TSharedRef<GraphicsPipelineState> PipelineState;
    TSharedRef<GraphicsPipelineState> PrePassPipelineState;
    TSharedRef<ComputePipelineState>  TiledLightPassPSO;
    TSharedRef<ComputePipelineState>  TiledLightPassPSODebug;
};
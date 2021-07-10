#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "Scene/Scene.h"

#include "RenderLayer/CommandList.h"

class DeferredRenderer
{
public:
    DeferredRenderer() = default;
    ~DeferredRenderer() = default;

    bool Init( FrameResources& FrameResources );

    void Release();

    void RenderPrePass( CommandList& CmdList, FrameResources& FrameResources, const Scene& Scene );
    void RenderBasePass( CommandList& CmdList, const FrameResources& FrameResources );
    void RenderDeferredTiledLightPass( CommandList& CmdList, const FrameResources& FrameResources, const LightSetup& LightSetup );

    bool ResizeResources( FrameResources& FrameResources );

private:
    bool CreateGBuffer( FrameResources& FrameResources );

    TRef<GraphicsPipelineState> PipelineState;
    TRef<VertexShader>          BaseVertexShader;
    TRef<PixelShader>           BasePixelShader;

    TRef<GraphicsPipelineState> PrePassPipelineState;
    TRef<VertexShader>          PrePassVertexShader;

    TRef<ComputePipelineState>  TiledLightPassPSO;
    TRef<ComputeShader>         TiledLightShader;
    TRef<ComputePipelineState>  TiledLightPassPSODebug;
    TRef<ComputeShader>         TiledLightDebugShader;

    TRef<ComputePipelineState> ReduceDepthInitalPSO;
    TRef<ComputeShader>        ReduceDepthInitalShader;

    TRef<ComputePipelineState> ReduceDepthPSO;
    TRef<ComputeShader>        ReduceDepthShader;
};
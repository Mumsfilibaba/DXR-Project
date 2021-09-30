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

    void RenderPrePass( CommandList& CmdList, FrameResources& FrameResources, const CScene& Scene );
    void RenderBasePass( CommandList& CmdList, const FrameResources& FrameResources );
    void RenderDeferredTiledLightPass( CommandList& CmdList, const FrameResources& FrameResources, const LightSetup& LightSetup );

    bool ResizeResources( FrameResources& FrameResources );

private:
    bool CreateGBuffer( FrameResources& FrameResources );

    TSharedRef<GraphicsPipelineState> PipelineState;
    TSharedRef<VertexShader>          BaseVertexShader;
    TSharedRef<PixelShader>           BasePixelShader;

    TSharedRef<GraphicsPipelineState> PrePassPipelineState;
    TSharedRef<VertexShader>          PrePassVertexShader;

    TSharedRef<ComputePipelineState>  TiledLightPassPSO;
    TSharedRef<ComputeShader>         TiledLightShader;
    TSharedRef<ComputePipelineState>  TiledLightPassPSODebug;
    TSharedRef<ComputeShader>         TiledLightDebugShader;

    TSharedRef<ComputePipelineState> ReduceDepthInitalPSO;
    TSharedRef<ComputeShader>        ReduceDepthInitalShader;

    TSharedRef<ComputePipelineState> ReduceDepthPSO;
    TSharedRef<ComputeShader>        ReduceDepthShader;
};
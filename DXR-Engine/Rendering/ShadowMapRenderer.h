#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

class ShadowMapRenderer
{
public:
    ShadowMapRenderer()  = default;
    ~ShadowMapRenderer() = default;

    bool Init(LightSetup& LightSetup, FrameResources& Resources);

    void RenderPointLightShadows(CommandList& CmdList, const LightSetup& LightSetup, const Scene& Scene);
    void RenderDirectionalLightShadows(CommandList& CmdList, const LightSetup& LightSetup, const Scene& Scene);

    void Release();

private:
    bool CreateShadowMaps(LightSetup& FrameResources);

    TRef<ConstantBuffer> PerShadowMapBuffer;

    TRef<GraphicsPipelineState> DirLightPipelineState;
    TRef<VertexShader>          DirLightShader;
    TRef<GraphicsPipelineState> PointLightPipelineState;
    TRef<VertexShader>          PointLightVertexShader;
    TRef<PixelShader>           PointLightPixelShader;

    bool   UpdateDirLight   = true;
    bool   UpdatePointLight = true;
    uint64 DirLightFrame    = 0;
    uint64 PointLightFrame  = 0;
};
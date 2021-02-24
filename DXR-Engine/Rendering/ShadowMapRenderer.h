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

    Bool Init(LightSetup& LightSetup, FrameResources& Resources);

    void RenderPointLightShadows(CommandList& CmdList, const LightSetup& LightSetup, const Scene& Scene);
    void RenderDirectionalLightShadows(CommandList& CmdList, const LightSetup& LightSetup, const Scene& Scene);

    void Release();

private:
    Bool CreateShadowMaps(LightSetup& FrameResources);

    TRef<ConstantBuffer> PerShadowMapBuffer;

    TRef<GraphicsPipelineState> DirLightPipelineState;
    TRef<VertexShader>          DirLightShader;
    TRef<GraphicsPipelineState> PointLightPipelineState;
    TRef<VertexShader>          PointLightVertexShader;
    TRef<PixelShader>           PointLightPixelShader;

    Bool   UpdateDirLight   = true;
    Bool   UpdatePointLight = true;
    UInt64 DirLightFrame    = 0;
    UInt64 PointLightFrame  = 0;
};
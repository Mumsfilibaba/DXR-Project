#pragma once
#include "RenderLayer/CommandList.h"

#include "FrameResources.h"

#include "Scene/Scene.h"

struct PointLightProperties
{
    XMFLOAT3 Color         = XMFLOAT3(1.0f, 1.0f, 1.0f);
    Float    ShadowBias    = 0.005f;
    XMFLOAT3 Position      = XMFLOAT3(0.0f, 0.0f, 0.0f);
    Float    FarPlane      = 10.0f;
    Float    MaxShadowBias = 0.05f;
    Float    Radius        = 5.0f;
    Float Padding0;
    Float Padding1;
};

struct DirectionalLightProperties
{
    XMFLOAT3   Color         = XMFLOAT3(1.0f, 1.0f, 1.0f);
    Float      ShadowBias    = 0.005f;
    XMFLOAT3   Direction     = XMFLOAT3(0.0f, -1.0f, 0.0f);
    Float      MaxShadowBias = 0.05f;
    XMFLOAT4X4 LightMatrix;
};

class ShadowMapRenderer
{
public:
    ShadowMapRenderer()  = default;
    ~ShadowMapRenderer() = default;

    Bool Init(SceneLightSetup& LightSetup, FrameResources& Resources);

    void RenderPointLightShadows(CommandList& CmdList, const SceneLightSetup& LightSetup, const Scene& Scene);
    void RenderDirectionalLightShadows(CommandList& CmdList, const SceneLightSetup& LightSetup, const Scene& Scene);

    void Release();

private:
    Bool CreateShadowMaps(SceneLightSetup& FrameResources);

    TSharedRef<ConstantBuffer> PerShadowMapBuffer;

    TSharedRef<GraphicsPipelineState> DirLightPipelineState;
    TSharedRef<GraphicsPipelineState> PointLightPipelineState;

    Bool   UpdateDirLight   = true;
    Bool   UpdatePointLight = true;
    UInt64 DirLightFrame    = 0;
    UInt64 PointLightFrame  = 0;
};
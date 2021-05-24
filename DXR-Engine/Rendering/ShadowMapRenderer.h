#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

struct SCascadeGenerationInfo
{
    XMFLOAT3 LightDirection;
    float CascadeSplitLambda;
};

struct SCascadeMatrices
{
    XMFLOAT4X4 ViewProjection;
    XMFLOAT4X4 View;
};

class ShadowMapRenderer
{
public:
    bool Init(LightSetup& LightSetup, FrameResources& Resources);

    void Release();
    
    void RenderPointLightShadows(CommandList& CmdList, const LightSetup& LightSetup, const Scene& Scene);
    void RenderDirectionalLightShadows(CommandList& CmdList, const LightSetup& LightSetup, const FrameResources& FrameResources, const Scene& Scene);

private:
    bool CreateShadowMaps(LightSetup& FrameResources);

    TRef<ConstantBuffer> PerShadowMapBuffer;

    TRef<GraphicsPipelineState> DirLightPipelineState;
    TRef<VertexShader>          DirLightShader;
    
    TRef<GraphicsPipelineState> PointLightPipelineState;
    TRef<VertexShader>          PointLightVertexShader;
    TRef<PixelShader>           PointLightPixelShader;

    TRef<ConstantBuffer>   CascadeGenerationData;
    TRef<StructuredBuffer> CascadeMatrixBuffer;
    TRef<ShaderResourceView>  CascadeMatrixBufferSRV;
    TRef<UnorderedAccessView> CascadeMatrixBufferUAV;

    TRef<ComputePipelineState> CascadeGen;
    TRef<ComputeShader>        CascadeGenShader;

    bool UpdateDirLight   = true;
    bool UpdatePointLight = true;
    
    uint64 DirLightFrame    = 0;
    uint64 PointLightFrame  = 0;
};
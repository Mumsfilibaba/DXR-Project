#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RenderLayer/CommandList.h"

#include "Scene/Scene.h"

struct SCascadeGenerationInfo
{
    CVector3 LightDirection;
    float CascadeSplitLambda;
    CVector3 LightUp;
    float Padding0;
};

struct SCascadeMatrices
{
    CMatrix4 ViewProjection;
    CMatrix4 View;
};

struct SCascadeSplits
{
    CVector3 MinExtent;
    float Split;
    CVector3 MaxExtent;
    float FarPlane;
};

struct SPerShadowMap
{
    CMatrix4 Matrix;
    CVector3 Position;
    float    FarPlane;
};

struct SPerCascade
{
    int32 CascadeIndex;
    int32 Padding0;
    int32 Padding1;
    int32 Padding2;
};

class ShadowMapRenderer
{
public:
    ShadowMapRenderer() = default;
    ~ShadowMapRenderer() = default;

    bool Init( LightSetup& LightSetup, FrameResources& Resources );

    void Release();

    void RenderPointLightShadows( CommandList& CmdList, const LightSetup& LightSetup, const Scene& Scene );
    void RenderDirectionalLightShadows( CommandList& CmdList, const LightSetup& LightSetup, const FrameResources& FrameResources, const Scene& Scene );

    bool ResizeResources( uint32 Width, uint32 Height, LightSetup& LightSetup );

private:
    bool CreateShadowMask( uint32 Width, uint32 Height, LightSetup& LightSetup );

    bool CreateShadowMaps( LightSetup& LightSetup, FrameResources& FrameResources );

    TRef<ConstantBuffer> PerShadowMapBuffer;

    TRef<GraphicsPipelineState> DirectionalLightPSO;
    TRef<VertexShader>          DirectionalLightShader;

    TRef<ComputePipelineState> DirectionalShadowMaskPSO;
    TRef<ComputeShader>        DirectionalShadowMaskShader;

    TRef<GraphicsPipelineState> PointLightPipelineState;
    TRef<VertexShader>          PointLightVertexShader;
    TRef<PixelShader>           PointLightPixelShader;

    TRef<ConstantBuffer> PerCascadeBuffer;
    TRef<ConstantBuffer> CascadeGenerationData;

    TRef<ComputePipelineState> CascadeGen;
    TRef<ComputeShader>        CascadeGenShader;

    bool UpdateDirLight = true;
    bool UpdatePointLight = true;

    uint64 DirLightFrame = 0;
    uint64 PointLightFrame = 0;
};
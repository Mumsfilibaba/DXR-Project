#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"

struct SCascadeGenerationInfo
{
    CVector3 LightDirection;
    float CascadeSplitLambda;
    CVector3 LightUp;
    float CascadeResolution;
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

class RENDERER_API CShadowMapRenderer
{
public:
    CShadowMapRenderer() = default;
    ~CShadowMapRenderer() = default;

    bool Init( SLightSetup& LightSetup, SFrameResources& Resources );

    void Release();

    /* Render Point light shadows */
    void RenderPointLightShadows( CRHICommandList& CmdList, const SLightSetup& LightSetup, const CScene& Scene );

    /* Render Directional light shadows */
    void RenderDirectionalLightShadows( CRHICommandList& CmdList, const SLightSetup& LightSetup, const SFrameResources& FrameResources, const CScene& Scene );

    /* Render ShadowMasks */
    void RenderShadowMasks( CRHICommandList& CmdList, const SLightSetup& LightSetup, const SFrameResources& FrameResources );

    /* Resize the resources that are dependent on the viewport */
    bool ResizeResources( uint32 Width, uint32 Height, SLightSetup& LightSetup );

private:

    bool CreateShadowMask( uint32 Width, uint32 Height, SLightSetup& LightSetup );
    bool CreateShadowMaps( SLightSetup& LightSetup, SFrameResources& FrameResources );

    TSharedRef<CRHIConstantBuffer>        PerShadowMapBuffer;

    TSharedRef<CRHIGraphicsPipelineState> DirectionalLightPSO;
    TSharedRef<CRHIVertexShader>          DirectionalLightShader;

    TSharedRef<CRHIComputePipelineState>  DirectionalShadowMaskPSO;
    TSharedRef<CRHIComputeShader>         DirectionalShadowMaskShader;

    TSharedRef<CRHIGraphicsPipelineState> PointLightPipelineState;
    TSharedRef<CRHIVertexShader>          PointLightVertexShader;
    TSharedRef<CRHIPixelShader>           PointLightPixelShader;

    TSharedRef<CRHIConstantBuffer>        PerCascadeBuffer;
    TSharedRef<CRHIConstantBuffer>        CascadeGenerationData;

    TSharedRef<CRHIComputePipelineState>  CascadeGen;
    TSharedRef<CRHIComputeShader>         CascadeGenShader;

    bool bUpdateDirLight   = true;
    bool bUpdatePointLight = true;

    uint64 DirLightFrame   = 0;
    uint64 PointLightFrame = 0;

    //uint64 FrameIndex = 0;
};

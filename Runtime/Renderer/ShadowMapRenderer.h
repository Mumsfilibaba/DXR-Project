#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCascadeGenerationInfo

struct SCascadeGenerationInfo
{
    CVector3 LightDirection;
    float CascadeSplitLambda;
    CVector3 LightUp;
    float CascadeResolution;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCascadeMatrices

struct SCascadeMatrices
{
    CMatrix4 ViewProjection;
    CMatrix4 View;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCascadeSplits

struct SCascadeSplits
{
    CVector3 MinExtent;
    float Split;
    CVector3 MaxExtent;
    float FarPlane;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SPerShadowMap

struct SPerShadowMap
{
    CMatrix4 Matrix;
    CVector3 Position;
    float    FarPlane;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SPerCascade

struct SPerCascade
{
    int32 CascadeIndex;
    int32 Padding0;
    int32 Padding1;
    int32 Padding2;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CShadowMapRenderer

class RENDERER_API CShadowMapRenderer
{
public:
    CShadowMapRenderer() = default;
    ~CShadowMapRenderer() = default;

    bool Init(SLightSetup& LightSetup, SFrameResources& Resources);

    void Release();

     /** @brief: Render Point light shadows */
    void RenderPointLightShadows(CRHICommandList& CmdList, const SLightSetup& LightSetup, const CScene& Scene);

     /** @brief: Render Directional light shadows */
    void RenderDirectionalLightShadows(CRHICommandList& CmdList, const SLightSetup& LightSetup, const SFrameResources& FrameResources, const CScene& Scene);

     /** @brief: Render ShadowMasks */
    void RenderShadowMasks(CRHICommandList& CmdList, const SLightSetup& LightSetup, const SFrameResources& FrameResources);

     /** @brief: Resize the resources that are dependent on the viewport */
    bool ResizeResources(uint32 Width, uint32 Height, SLightSetup& LightSetup);

private:

    bool CreateShadowMask(uint32 Width, uint32 Height, SLightSetup& LightSetup);
    bool CreateShadowMaps(SLightSetup& LightSetup, SFrameResources& FrameResources);

    TSharedRef<FRHIConstantBuffer>        PerShadowMapBuffer;

    TSharedRef<FRHIGraphicsPipelineState> DirectionalLightPSO;
    TSharedRef<FRHIVertexShader>          DirectionalLightShader;

    TSharedRef<FRHIComputePipelineState>  DirectionalShadowMaskPSO;
    TSharedRef<FRHIComputeShader>         DirectionalShadowMaskShader;

    TSharedRef<FRHIGraphicsPipelineState> PointLightPipelineState;
    TSharedRef<FRHIVertexShader>          PointLightVertexShader;
    TSharedRef<FRHIPixelShader>           PointLightPixelShader;

    TSharedRef<FRHIConstantBuffer>        PerCascadeBuffer;
    TSharedRef<FRHIConstantBuffer>        CascadeGenerationData;

    TSharedRef<FRHIComputePipelineState>  CascadeGen;
    TSharedRef<FRHIComputeShader>         CascadeGenShader;

    bool bUpdateDirLight   = true;
    bool bUpdatePointLight = true;

    uint64 DirLightFrame   = 0;
    uint64 PointLightFrame = 0;

    //uint64 FrameIndex = 0;
};

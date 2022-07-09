#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCascadeGenerationInfo

struct SCascadeGenerationInfo
{
    FVector3 LightDirection;
    float CascadeSplitLambda;
    FVector3 LightUp;
    float CascadeResolution;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCascadeMatrices

struct SCascadeMatrices
{
    FMatrix4 ViewProjection;
    FMatrix4 View;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SCascadeSplits

struct SCascadeSplits
{
    FVector3 MinExtent;
    float Split;
    FVector3 MaxExtent;
    float FarPlane;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SPerShadowMap

struct SPerShadowMap
{
    FMatrix4 Matrix;
    FVector3 Position;
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
    void RenderPointLightShadows(FRHICommandList& CmdList, const SLightSetup& LightSetup, const FScene& Scene);

     /** @brief: Render Directional light shadows */
    void RenderDirectionalLightShadows(FRHICommandList& CmdList, const SLightSetup& LightSetup, const SFrameResources& FrameResources, const FScene& Scene);

     /** @brief: Render ShadowMasks */
    void RenderShadowMasks(FRHICommandList& CmdList, const SLightSetup& LightSetup, const SFrameResources& FrameResources);

     /** @brief: Resize the resources that are dependent on the viewport */
    bool ResizeResources(uint32 Width, uint32 Height, SLightSetup& LightSetup);

private:

    bool CreateShadowMask(uint32 Width, uint32 Height, SLightSetup& LightSetup);
    bool CreateShadowMaps(SLightSetup& LightSetup, SFrameResources& FrameResources);

    TSharedRef<FRHIConstantBuffer>        PerShadowMapBuffer;

    TSharedRef<FRHIGraphicsPipelineState> DirectionalLightPSO;
    TSharedRef<FRHIVertexShader>          DirectionalLightShader;

    FRHIComputePipelineStateRef  DirectionalShadowMaskPSO;
    FRHIComputeShaderRef         DirectionalShadowMaskShader;

    TSharedRef<FRHIGraphicsPipelineState> PointLightPipelineState;
    TSharedRef<FRHIVertexShader>          PointLightVertexShader;
    TSharedRef<FRHIPixelShader>           PointLightPixelShader;

    TSharedRef<FRHIConstantBuffer>        PerCascadeBuffer;
    TSharedRef<FRHIConstantBuffer>        CascadeGenerationData;

    FRHIComputePipelineStateRef  CascadeGen;
    FRHIComputeShaderRef         CascadeGenShader;

    bool bUpdateDirLight   = true;
    bool bUpdatePointLight = true;

    uint64 DirLightFrame   = 0;
    uint64 PointLightFrame = 0;

    //uint64 FrameIndex = 0;
};

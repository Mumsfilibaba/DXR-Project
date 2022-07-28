#pragma once
#include "FrameResources.h"
#include "LightSetup.h"

#include "RHI/RHICommandList.h"

#include "Engine/Scene/Scene.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCascadeGenerationInfo

struct FCascadeGenerationInfo
{
    FVector3 LightDirection;
    float CascadeSplitLambda;
    FVector3 LightUp;
    float CascadeResolution;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCascadeMatrices

struct FCascadeMatrices
{
    FMatrix4 ViewProjection;
    FMatrix4 View;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCascadeSplits

struct FCascadeSplits
{
    FVector3 MinExtent;
    float    Split;
    FVector3 MaxExtent;
    float    FarPlane;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FPerShadowMap

struct FPerShadowMap
{
    FMatrix4 Matrix;
    FVector3 Position;
    float    FarPlane;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FPerCascade

struct FPerCascade
{
    int32 CascadeIndex;
    int32 Padding0;
    int32 Padding1;
    int32 Padding2;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FShadowMapRenderer

class RENDERER_API FShadowMapRenderer
{
public:
    FShadowMapRenderer() = default;
    ~FShadowMapRenderer() = default;

    bool Init(FLightSetup& LightSetup, FFrameResources& Resources);

    void Release();

     /** @brief: Render Point light shadows */
    void RenderPointLightShadows(FRHICommandList& CmdList, const FLightSetup& LightSetup, const FScene& Scene);

     /** @brief: Render Directional light shadows */
    void RenderDirectionalLightShadows(FRHICommandList& CmdList, const FLightSetup& LightSetup, const FFrameResources& FrameResources, const FScene& Scene);

     /** @brief: Render ShadowMasks */
    void RenderShadowMasks(FRHICommandList& CmdList, const FLightSetup& LightSetup, const FFrameResources& FrameResources);

     /** @brief: Resize the resources that are dependent on the viewport */
    bool ResizeResources(uint32 Width, uint32 Height, FLightSetup& LightSetup);

private:
    bool CreateShadowMask(uint32 Width, uint32 Height, FLightSetup& LightSetup);
    bool CreateShadowMaps(FLightSetup& LightSetup, FFrameResources& FrameResources);

    FRHIConstantBufferRef        PerShadowMapBuffer;

    TSharedRef<FRHIGraphicsPipelineState> DirectionalLightPSO;
    TSharedRef<FRHIVertexShader>          DirectionalLightShader;

    FRHIComputePipelineStateRef  DirectionalShadowMaskPSO;
    FRHIComputeShaderRef         DirectionalShadowMaskShader;

    TSharedRef<FRHIGraphicsPipelineState> PointLightPipelineState;
    TSharedRef<FRHIVertexShader>          PointLightVertexShader;
    TSharedRef<FRHIPixelShader>           PointLightPixelShader;

    FRHIConstantBufferRef        PerCascadeBuffer;
    FRHIConstantBufferRef        CascadeGenerationData;

    FRHIComputePipelineStateRef  CascadeGen;
    FRHIComputeShaderRef         CascadeGenShader;

    bool bUpdateDirLight   = true;
    bool bUpdatePointLight = true;

    uint64 DirLightFrame   = 0;
    uint64 PointLightFrame = 0;

    //uint64 FrameIndex = 0;
};

#pragma once
#include "FrameResources.h"
#include "LightSetup.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/Scene/Scene.h"

struct FCascadeGenerationInfo
{
    FVector3 LightDirection;
    float    CascadeSplitLambda;
    
    FVector3 LightUp;
    float    CascadeResolution;

    FMatrix4 ShadowMatrix;
};

MARK_AS_REALLOCATABLE(FCascadeGenerationInfo);


struct FCascadeMatrices
{
    FMatrix4 ViewProjection;
    FMatrix4 View;
};

MARK_AS_REALLOCATABLE(FCascadeMatrices);


struct FCascadeSplits
{
    FVector3 MinExtent;
    float    Split;
    FVector3 MaxExtent;
    float    NearPlane;

    float    FarPlane;
    float    Padding0;
    float    Padding1;
    float    Padding2;

    FVector4 FrustumPlanes[6];

    FVector4 Offsets;
    FVector4 Scale;
};

MARK_AS_REALLOCATABLE(FCascadeSplits);


struct FPerShadowMap
{
    FMatrix4 Matrix;
    FVector3 Position;
    float    FarPlane;
};

MARK_AS_REALLOCATABLE(FPerShadowMap);


struct FPerCascade
{
    int32 CascadeIndex;
    int32 Padding0;
    int32 Padding1;
    int32 Padding2;
};

MARK_AS_REALLOCATABLE(FPerCascade);


class RENDERER_API FShadowMapRenderer
{
public:
    FShadowMapRenderer() = default;
    ~FShadowMapRenderer() = default;

    bool Init(FLightSetup& LightSetup, FFrameResources& Resources);

    void Release();

     /** @brief - Render Point light shadows */
    void RenderPointLightShadows(
        FRHICommandList& CommandList,
        const FLightSetup& LightSetup, 
        const FScene& Scene);

     /** @brief - Render Directional light shadows */
    void RenderDirectionalLightShadows(
        FRHICommandList& CommandList,
        const FLightSetup& LightSetup, 
        const FFrameResources& FrameResources,
        const FScene& Scene);

     /** @brief - Render ShadowMasks */
    void RenderShadowMasks(
        FRHICommandList& CommandList,
        const FLightSetup& LightSetup, 
        const FFrameResources& FrameResources);

     /** @brief - Resize the resources that are dependent on the viewport */
    bool ResizeResources(uint32 Width, uint32 Height, FLightSetup& LightSetup);

private:
    bool CreateShadowMask(uint32 Width, uint32 Height, FLightSetup& LightSetup);
    bool CreateShadowMaps(FLightSetup& LightSetup, FFrameResources& FrameResources);

    FRHIBufferRef                PerShadowMapBuffer;

    FRHIGraphicsPipelineStateRef DirectionalLightPSO;
    FRHIVertexShaderRef          DirectionalLightShader;

    FRHIComputePipelineStateRef  DirectionalShadowMaskPSO;
    FRHIComputeShaderRef         DirectionalShadowMaskShader;

    FRHIComputePipelineStateRef  DirectionalShadowMaskPSO_Debug;
    FRHIComputeShaderRef         DirectionalShadowMaskShader_Debug;

    FRHIGraphicsPipelineStateRef PointLightPipelineState;
    FRHIVertexShaderRef          PointLightVertexShader;
    FRHIPixelShaderRef           PointLightPixelShader;

    FRHIBufferRef                PerCascadeBuffer;
    FRHIBufferRef                CascadeGenerationData;

    FRHIComputePipelineStateRef  CascadeGen;
    FRHIComputeShaderRef         CascadeGenShader;

    bool bUpdateDirLight   = true;
    bool bUpdatePointLight = true;

    uint64 DirLightFrame   = 0;
    uint64 PointLightFrame = 0;
};

#pragma once
#include "RenderPass.h"
#include "FrameResources.h"
#include "LightSetup.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"

#define NUM_FRUSTUM_PLANES (6)

struct FCascadeMatrices
{
    FMatrix4 ViewProjection;
    FMatrix4 View;
};

MARK_AS_REALLOCATABLE(FCascadeMatrices);

struct FCascadeSplit
{
    FVector4 FrustumPlanes[NUM_FRUSTUM_PLANES];

    FVector4 Offsets;
    FVector4 Scale;

    FVector3 MinExtent;
    float    Split;
    
    FVector3 MaxExtent;
    float    NearPlane;

    float FarPlane;
    float MinDepth;
    float MaxDepth;
    float PreviousSplit;
};

MARK_AS_REALLOCATABLE(FCascadeSplit);

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

class FShadowMapRenderer : public FRenderPass
{
public:
    FShadowMapRenderer(FSceneRenderer* InRenderer)
        : FRenderPass(InRenderer)
    {
    }

    bool Initialize(FLightSetup& LightSetup, FFrameResources& Resources);
    void Release();

     /** @brief - Render Point light shadows */
    void RenderPointLightShadows(FRHICommandList& CommandList, const FLightSetup& LightSetup, FScene* Scene);

     /** @brief - Render Directional light shadows */
    void RenderDirectionalLightShadows(FRHICommandList& CommandList, const FLightSetup& LightSetup, const FFrameResources& FrameResources, FScene* Scene);

     /** @brief - Render ShadowMasks */
    void RenderShadowMasks(FRHICommandList& CommandList, const FLightSetup& LightSetup, const FFrameResources& FrameResources);

     /** @brief - Resize the resources that are dependent on the viewport */
    bool ResizeResources(FRHICommandList& CommandList, uint32 Width, uint32 Height, FLightSetup& LightSetup);

private:
    bool CreateShadowMask(uint32 Width, uint32 Height, FLightSetup& LightSetup);
    bool CreateShadowMaps(FLightSetup& LightSetup, FFrameResources& FrameResources);

    FRHIBufferRef                PerShadowMapBuffer;

    FRHIGraphicsPipelineStateRef DirectionalLightPSO;
    FRHIGraphicsPipelineStateRef DirectionalLightMaskedPSO;
    FRHIGraphicsPipelineStateRef DirectionalLightMaskedPackedPSO;
    FRHIVertexShaderRef          DirectionalLightVS;
    FRHIVertexShaderRef          DirectionalLightMaskedVS;
    FRHIPixelShaderRef           DirectionalLightMaskedPS;
    FRHIPixelShaderRef           DirectionalLightMaskedPackedPS;

    FRHIComputePipelineStateRef  DirectionalShadowMaskPSO;
    FRHIComputeShaderRef         DirectionalShadowMaskShader;

    FRHIComputePipelineStateRef  DirectionalShadowMaskPSO_Debug;
    FRHIComputeShaderRef         DirectionalShadowMaskShader_Debug;

    FRHIGraphicsPipelineStateRef PointLightPipelineState;
    FRHIVertexShaderRef          PointLightVertexShader;
    FRHIPixelShaderRef           PointLightPixelShader;

    FRHIBufferRef                PerCascadeBuffer;

    FRHIComputePipelineStateRef  CascadeGen;
    FRHIComputeShaderRef         CascadeGenShader;
};

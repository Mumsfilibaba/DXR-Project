#pragma once
#include "RHI/RHICommandList.h"
#include "RHI/RHIShader.h"
#include "Engine/World/World.h"
#include "Renderer/RenderPass.h"
#include "Renderer/FrameResources.h"

class FDebugRenderer : public FRenderPass
{
public:
    FDebugRenderer(FSceneRenderer* InRenderer);
    virtual ~FDebugRenderer();

    bool Initialize(FFrameResources& Resources);

    void PreparePipelineState(EFormat OutputFormat);

    void RenderObjectAABBs(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, FRHITexture* InRenderTarget = nullptr, FRHITexture* InDepthTarget = nullptr);
    void RenderPointLights(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, FRHITexture* InRenderTarget = nullptr, FRHITexture* InDepthTarget = nullptr);
    void RenderLightProbes(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, FRHITexture* InRenderTarget = nullptr, FRHITexture* InDepthTarget = nullptr);

private:
    // Geometry Data
    FRHIBufferRef                AABBVertexBuffer;
    FRHIBufferRef                AABBIndexBuffer_Wireframe;
    FRHIBufferRef                AABBIndexBuffer_Solid;
    uint32                       AABBIndexCount_Wireframe;
    uint32                       AABBIndexCount_Solid;

    FRHIBufferRef                SphereVertexBuffer;
    FRHIBufferRef                SphereIndexBuffer;
    uint32                       SphereIndexCount;

    // Wireframe AABBs
    FRHIGraphicsPipelineStateRef AABB_NoDepth_PSO;
    EFormat                      AABB_NoDepth_PSOFormat = EFormat::Unknown;
    FRHIGraphicsPipelineStateRef AABB_Depth_PSO;
    EFormat                      AABB_Depth_PSOFormat = EFormat::Unknown;
    FRHIVertexShaderRef          AABB_VS;
    FRHIPixelShaderRef           AABB_PS;
    FRHIInputLayoutRef           AABBInputLayout;
    FRHIDepthStencilStateRef     AABB_NoDepthStencilState;
    FRHIDepthStencilStateRef     AABB_DepthStencilState;
    FRHIRasterizerStateRef       AABBRasterizerState;
    FRHIBlendStateRef            AABBBlendState;

    // Solid AABBs
    FRHIGraphicsPipelineStateRef AABBSolid_PSO;
    EFormat                      AABBSolid_PSOFormat = EFormat::Unknown;
    FRHIVertexShaderRef          AABBSolid_VS;
    FRHIPixelShaderRef           AABBSolid_PS;
    FRHIInputLayoutRef           AABBSolidInputLayout;
    FRHIDepthStencilStateRef     AABBSolidDepthStencilState;
    FRHIRasterizerStateRef       AABBSolidRasterizerState;
    FRHIBlendStateRef            AABBSolidBlendState;

    // PointLights
    FRHIGraphicsPipelineStateRef LightDebug_PSO;
    EFormat                      LightDebug_PSOFormat = EFormat::Unknown;
    FRHIVertexShaderRef          LightDebug_VS;
    FRHIPixelShaderRef           LightDebug_PS;
    FRHIInputLayoutRef           DebugSphereInputLayout;
    FRHIDepthStencilStateRef     LightDebugDepthStencilState;
    FRHIRasterizerStateRef       LightDebugRasterizerState;
    FRHIBlendStateRef            LightDebugBlendState;

    // LightProbes
    FRHIGraphicsPipelineStateRef ProbeDebug_PSO;
    EFormat                      ProbeDebug_PSOFormat = EFormat::Unknown;
    FRHIVertexShaderRef          ProbeDebug_VS;
    FRHIPixelShaderRef           ProbeDebug_PS;
    FRHIDepthStencilStateRef     ProbeDebugDepthStencilState;
    FRHIRasterizerStateRef       ProbeDebugRasterizerState;
    FRHIBlendStateRef            ProbeDebugBlendState;
};
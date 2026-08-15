#pragma once
#include "Core/Containers/Array.h"
#include "Engine/World/Components/DirectionalLightComponent.h"
#include "Renderer/Graph/PassResources.h"
#include "RendererCore/Interfaces/IRendererModule.h"
#include "RendererCore/RenderGraph/RenderGraphResources.h"
#include "RendererCore/RenderGraph/RenderGraphViews.h"

class FScene;
class FSceneRenderer;
struct FFrameResources;
struct FPassResources;
class FRenderGraphPassResources;

struct FSceneRenderGraphContext
{
    FSceneRenderer*                       SceneRenderer                          = nullptr;
    FScene*                               Scene                                  = nullptr;
    const FSceneRenderView*               View                                   = nullptr;
    FFrameResources*                      FrameResources                         = nullptr;
    const TArray<uint32>*                 SelectedObjectIDs                      = nullptr;
    FRenderGraphTexture*                  GBufferAlbedo                          = nullptr;
    FRenderGraphTexture*                  GBufferNormal                          = nullptr;
    FRenderGraphTexture*                  GBufferMaterial                        = nullptr;
    FRenderGraphTexture*                  GBufferVelocity                        = nullptr;
    FRenderGraphTexture*                  GBufferDepth                           = nullptr;
    FRenderGraphTexture*                  SSAOBuffer                             = nullptr;
    FRenderGraphTexture*                  SceneTarget                            = nullptr;
    FRenderGraphTexture*                  TonemappedTarget                       = nullptr;
    FRenderGraphTexture*                  DirectionalShadowMask                  = nullptr;
    FRenderGraphTexture*                  CascadeIndexBuffer                     = nullptr;
    FRenderGraphTexture*                  ReducedDepthBuffer0                    = nullptr;
    FRenderGraphTexture*                  ReducedDepthBuffer1                    = nullptr;
    FRenderGraphTexture*                  BackBuffer                             = nullptr;
    FRenderGraphTexture*                  ShadowCascades                         = nullptr;
    FRenderGraphTexture*                  PointLightShadowMaps                   = nullptr;
    FRenderGraphTexture*                  RayTracingOutput                       = nullptr;
    FRenderGraphTexture*                  IntegrationLUT                         = nullptr;
    FRenderGraphTexture*                  TAAHistory[2]                          = {};
    FRenderGraphTexture*                  ReflectionTrace                        = nullptr;
    FRenderGraphTexture*                  ReflectionHistory[2]                   = {};
    FRenderGraphTexture*                  ReflectionMoments[2]                   = {};
    FRenderGraphTexture*                  ReflectionDenoised[2]                  = {};
    FRenderGraphTexture*                  ReflectionNoise                        = nullptr;
#if EDITOR_BUILD
    FRenderGraphTexture*                  EditorNoJitterDepth                    = nullptr;
    FRenderGraphTexture*                  EditorObjectID_NoJitter                = nullptr;
    FRenderGraphTexture*                  SelectionMask                          = nullptr;
    FRenderGraphTexture*                  SelectionErosionTemp                   = nullptr;
    FRenderGraphTexture*                  SelectionErodedMask                    = nullptr;
    FRenderGraphTexture*                  SelectionDilationTemp                  = nullptr;
    FRenderGraphTexture*                  SelectionDilatedMask                   = nullptr;
    FRenderGraphTexture*                  SelectionRing                          = nullptr;
    FRenderGraphBuffer*                   SelectedIDsBuffer                      = nullptr;
#endif
    FRenderGraphBuffer*                   CameraBuffer                           = nullptr;
    FRenderGraphBuffer*                   PerObjectBuffer                        = nullptr;
    FRenderGraphBuffer*                   MaterialDataBuffer                     = nullptr;
    FRenderGraphBuffer*                   PointLightsBuffer                      = nullptr;
    FRenderGraphBuffer*                   PointLightsPosRadBuffer                = nullptr;
    FRenderGraphBuffer*                   ShadowCastingPointLightsBuffer         = nullptr;
    FRenderGraphBuffer*                   ShadowCastingPointLightsPosRadBuffer   = nullptr;
    FRenderGraphBuffer*                   DirectionalLightDataBuffer             = nullptr;
    FRenderGraphBuffer*                   CascadeGenerationDataBuffer            = nullptr;
    FRenderGraphBuffer*                   CascadeMatrixBuffer                    = nullptr;
    FRenderGraphBuffer*                   CascadeSplitsBuffer                    = nullptr;
    FRenderGraphBuffer*                   LightProbeBuffer                       = nullptr;
    FRenderGraphBuffer*                   RayTracingSceneConstantsBuffer         = nullptr;
    FRenderGraphBuffer*                   RayTracingGeometryTableBuffer          = nullptr;
    FRenderGraphBuffer*                   PerCascadeBuffer                       = nullptr;
    FRenderGraphBuffer*                   ShadowSettingsBuffer                   = nullptr;
    FRenderGraphBuffer*                   PerShadowMapBuffer                     = nullptr;
    FRenderGraphBuffer*                   SinglePassShadowMapBuffer              = nullptr;
    FRenderGraphBuffer*                   SkyboxVertexBuffer                     = nullptr;
    FRenderGraphBuffer*                   SkyboxIndexBuffer                      = nullptr;
    FRenderGraphRenderTargetView*         SceneTargetRenderTargetView            = nullptr;
    FRenderGraphDepthStencilView*         GBufferDepthReadOnlyDSV                = nullptr;
    FRenderGraphDepthStencilView*         ShadowCascadeDSVs[NUM_SHADOW_CASCADES] = {};
    FRenderGraphDepthStencilView*         ShadowCascadesCombinedDSV              = nullptr;
    FRenderGraphUnorderedAccessView*      CascadeMatrixBufferUAV                 = nullptr;
    FRenderGraphShaderResourceView*       CascadeMatrixBufferSRV                 = nullptr;
    FRenderGraphUnorderedAccessView*      CascadeSplitsBufferUAV                 = nullptr;
    FRenderGraphShaderResourceView*       CascadeSplitsBufferSRV                 = nullptr;
    TArray<FRenderGraphDepthStencilView*> PointLightShadowMapDSVs;
    TArray<FRenderGraphDepthStencilView*> PointLightShadowMapFaceDSVs;

    NODISCARD FPassResources CreatePassResources(const FRenderGraphPassResources& GraphResources) const
    {
        return PassResourceSync::Create(GraphResources, *this);
    }
};

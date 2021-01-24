#pragma once
#include "Windows/WindowsWindow.h"

#include "Time/Clock.h"

#include "Application/InputCodes.h"
#include "Application/Events/EventHandler.h"

#include "Scene/Actor.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"

#include "Mesh.h"
#include "Material.h"
#include "MeshFactory.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/CommandList.h"
#include "RenderLayer/Viewport.h"

#include "RenderPasses/DeferredSceneRenderPass.h"
#include "RenderPasses/SkyboxSceneRenderPass.h"
#include "RenderPasses/DebugSceneRenderPass.h"
#include "RenderPasses/SSAOSceneRenderPass.h"
#include "RenderPasses/PrePassSceneRenderPass.h"
#include "RenderPasses/ForwardSceneRenderPass.h"
#include "RenderPasses/LightProbeSceneRenderPass.h"
#include "RenderPasses/DirectionalLightShadowSceneRenderPass.h"
#include "RenderPasses/PointLightShadowSceneRenderPass.h"
#include "RenderPasses/TiledDeferredLightSceneRenderPass.h"

#include "DebugUI.h"

#define ENABLE_VSM 0

struct LightSettings
{
    UInt16 ShadowMapWidth       = 4096;
    UInt16 ShadowMapHeight      = 4096;
    UInt16 PointLightShadowSize = 1024;
};

class Renderer
{
public:
    Renderer()  = default;
    ~Renderer() = default;

    Bool Init();
    void Tick(const Scene& CurrentScene);
    
    void SetLightSettings(const LightSettings& InLightSettings);
    
    FORCEINLINE const LightSettings& GetLightSettings()
    {
        return Resources.CurrentLightSettings;
    }
    
private:
    Bool InitRayTracing();
    Bool InitLightBuffers();
    Bool InitPrePass();
    Bool InitShadowMapPass();
    Bool InitDeferred();
    Bool InitGBuffer();
    Bool InitIntegrationLUT();
    Bool InitRayTracingTexture();
    Bool InitDebugStates();
    Bool InitAA();
    Bool InitForwardPass();
    Bool InitSSAO();
    Bool InitSSAO_RenderTarget();

    Bool CreateShadowMaps();

    void TraceRays(Texture2D* BackBuffer, CommandList& InCmdList);

private:
    CommandList CmdList;

    DeferredSceneRenderPass   DeferredRenderPass;
    SkyboxSceneRenderPass     SkyboxRenderPass;
    DebugSceneRenderPass      DebugRenderPass;
    SSAOSceneRenderPass       SSAORenderPass;
    PrePassSceneRenderPass    PrePassRenderPass;
    ForwardSceneRenderPass    ForwardRenderPass;
    LightProbeSceneRenderPass LightProbeRenderPass;
    DirectionalLightShadowSceneRenderPass DirectionalLightShadowRenderPass;
    PointLightShadowSceneRenderPass       PointLightShadowRenderPass;
    TiledDeferredLightSceneRenderPass     DeferredLightRenderPass;

    SharedRenderPassResources Resources;

    // TODO: Fix raytracing
    TSharedRef<RayTracingPipelineState>	RaytracingPSO;
    TSharedPtr<RayTracingScene>			RayTracingScene;
    TArray<RayTracingGeometryInstance>	RayTracingGeometryInstances;

    TSharedRef<GraphicsPipelineState> PostPSO;

    TSharedRef<Viewport> MainWindowViewport;

    UInt32 LastFrameNumDrawCalls		= 0;
    UInt32 LastFrameNumDispatchCalls	= 0;
    UInt32 LastFrameNumCommands			= 0;
};
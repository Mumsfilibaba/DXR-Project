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
#include "RenderPasses/ForwardSceneRenderPass.h"
#include "RenderPasses/LightProbeSceneRenderPass.h"
#include "RenderPasses/DirectionalLightShadowSceneRenderPass.h"
#include "RenderPasses/PointLightShadowSceneRenderPass.h"
#include "RenderPasses/TiledDeferredLightSceneRenderPass.h"

#include "DebugUI.h"

#define ENABLE_VSM 0

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
    Bool InitRayTracingTexture();
    Bool InitAA();

    void ResizeResources(UInt32 Width, UInt32 Height);

    void TraceRays(Texture2D* BackBuffer, CommandList& InCmdList);

private:
    CommandList CmdList;

    DeferredSceneRenderPass   DeferredRenderPass;
    SkyboxSceneRenderPass     SkyboxRenderPass;
    DebugSceneRenderPass      DebugRenderPass;
    SSAOSceneRenderPass       SSAORenderPass;
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
    TSharedRef<GraphicsPipelineState> FXAAPSO;

    UInt32 LastFrameNumDrawCalls		= 0;
    UInt32 LastFrameNumDispatchCalls	= 0;
    UInt32 LastFrameNumCommands			= 0;
};
#pragma once
#include "DeferredRenderer.h"
#include "ShadowMapRenderer.h"
#include "ScreenSpaceOcclusionRenderer.h"
#include "LightProbeRenderer.h"
#include "SkyboxRenderPass.h"
#include "ForwardRenderer.h"
#include "RayTracer.h"
#include "DebugRenderer.h"

#include "Core/Time/Timer.h"
#include "Core/Threading/TaskManagerInterface.h"

#include "Engine/Scene/Actor.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Assets/MeshFactory.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

#include "RHI/RHIModule.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIViewport.h"

#include "Debug/TextureDebugger.h"
#include "Debug/RendererInfoWindow.h"
#include "Debug/GPUProfilerWindow.h"

#include "Application/WindowMessageHandler.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRendererWindowHandler

class FRendererWindowHandler final 
    : public FWindowMessageHandler
{
public:

    DECLARE_DELEGATE(CWindowResizedDelegate, const FWindowResizeEvent& ResizeEvent);
    CWindowResizedDelegate WindowResizedDelegate;

    FRendererWindowHandler() = default;
    ~FRendererWindowHandler() = default;

    virtual bool OnWindowResized(const FWindowResizeEvent& ResizeEvent) override final
    {
        WindowResizedDelegate.Execute(ResizeEvent);
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRenderer

class RENDERER_API FRenderer
{
public:
    FRenderer();
    ~FRenderer() = default;

    bool Init();

    void Tick(const FScene& Scene);

    void Release();

    void PerformFrustumCullingAndSort(const FScene& Scene);
    void PerformFXAA(FRHICommandList& InCmdList);
    void PerformBackBufferBlit(FRHICommandList& InCmdList);

    FORCEINLINE TSharedRef<FRenderTargetDebugWindow> GetTextureDebugger() const
    {
        return TextureDebugger;
    }

    FORCEINLINE const FRHICommandStatistics& GetStatistics() const
    {
        return FrameStatistics;
    }

private:
    void OnWindowResize(const FWindowResizeEvent& Event);

    bool InitAA();
    bool InitShadingImage();

    NOINLINE void FrustumCullingAndSortingInternal(
        const FCamera* Camera,
        const TPair<uint32, uint32>& DrawCommands,
        TArray<uint32>& OutDeferredDrawCommands,
        TArray<uint32>& OutForwardDrawCommands);

    TSharedPtr<FRendererWindowHandler> WindowHandler;

    TSharedRef<FRenderTargetDebugWindow> TextureDebugger;
    TSharedRef<FRendererInfoWindow> InfoWindow;
    TSharedRef<FGPUProfilerWindow>  GPUProfilerWindow;

    FRHICommandList CommandList;

    FDeferredRenderer             DeferredRenderer;
    FShadowMapRenderer            ShadowMapRenderer;
    FScreenSpaceOcclusionRenderer SSAORenderer;
    FLightProbeRenderer           LightProbeRenderer;
    FSkyboxRenderPass             SkyboxRenderPass;
    FForwardRenderer              ForwardRenderer;
    FRayTracer                    RayTracer;
    FDebugRenderer                DebugRenderer;

    FFrameResources Resources;
    FLightSetup     LightSetup;

    FRHITexture2DRef            ShadingImage;
    FRHIComputePipelineStateRef ShadingRatePipeline;
    FRHIComputeShaderRef        ShadingRateShader;

    FRHIGraphicsPipelineStateRef PostPSO;
    FRHIPixelShaderRef           PostShader;
    FRHIGraphicsPipelineStateRef FXAAPSO;
    FRHIPixelShaderRef           FXAAShader;
    FRHIGraphicsPipelineStateRef FXAADebugPSO;
    FRHIPixelShaderRef           FXAADebugShader;

    FRHITimestampQueryRef TimestampQueries;

    FRHICommandStatistics FrameStatistics;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

extern RENDERER_API FRenderer GRenderer;

inline void AddDebugTexture(
    const FRHIShaderResourceViewRef& ImageView,
    const FRHITextureRef& Image,
    EResourceAccess BeforeState,
    EResourceAccess AfterState)
{
    GRenderer.GetTextureDebugger()->AddTextureForDebugging(ImageView, Image, BeforeState, AfterState);
}

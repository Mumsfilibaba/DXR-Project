#pragma once
#include "DeferredRenderer.h"
#include "ShadowMapRenderer.h"
#include "ScreenSpaceOcclusionRenderer.h"
#include "LightProbeRenderer.h"
#include "SkyboxRenderPass.h"
#include "ForwardRenderer.h"
#include "RayTracer.h"

#include "Core/Time/Timer.h"
#include "Core/Threading/AsyncTaskManager.h"

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

#include "Canvas/WindowMessageHandler.h"
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

    void PerformAABBDebugPass(FRHICommandList& InCmdList);

    FORCEINLINE TSharedRef<FTextureDebugWindow> GetTextureDebugger() const
    {
        return TextureDebugger;
    }

    FORCEINLINE const FRHICommandStatistics& GetStatistics() const
    {
        return FrameStatistics;
    }

private:
    void OnWindowResize(const FWindowResizeEvent& Event);

    bool InitBoundingBoxDebugPass();

    bool InitAA();
    
    bool InitShadingImage();

    NOINLINE void FrustumCullingAndSortingInternal(
        const FCamera* Camera,
        const TPair<uint32, uint32>& DrawCommands,
        TArray<uint32>& OutDeferredDrawCommands,
        TArray<uint32>& OutForwardDrawCommands);

    TSharedPtr<FRendererWindowHandler> WindowHandler;

    TSharedRef<FTextureDebugWindow> TextureDebugger;
    TSharedRef<FRendererInfoWindow> InfoWindow;
    TSharedRef<FGPUProfilerWindow>  GPUProfilerWindow;

    FRHICommandList PreShadowsCmdList;
    FRHICommandList PointShadowCmdList;
    FRHICommandList DirShadowCmdList;
    FRHICommandList PrepareGBufferCmdList;
    FRHICommandList PrePassCmdList;
    FRHICommandList ShadingRateCmdList;
    FRHICommandList RayTracingCmdList;
    FRHICommandList BasePassCmdList;
    FRHICommandList MainCmdList;

    FAsyncTask PointShadowTask;
    FAsyncTask DirShadowTask;
    FAsyncTask PrePassTask;
    FAsyncTask BasePassTask;
    FAsyncTask RayTracingTask;

    FDeferredRenderer             DeferredRenderer;
    FShadowMapRenderer            ShadowMapRenderer;
    FScreenSpaceOcclusionRenderer SSAORenderer;
    FLightProbeRenderer           LightProbeRenderer;
    FSkyboxRenderPass             SkyboxRenderPass;
    FForwardRenderer              ForwardRenderer;
    FRayTracer                    RayTracer;

    FFrameResources Resources;
    FLightSetup     LightSetup;

    FRHITexture2DRef            ShadingImage;
    FRHIComputePipelineStateRef ShadingRatePipeline;
    FRHIComputeShaderRef        ShadingRateShader;

    TSharedRef<FRHIVertexBuffer>          AABBVertexBuffer;
    TSharedRef<FRHIIndexBuffer>           AABBIndexBuffer;
    FRHIGraphicsPipelineStateRef AABBDebugPipelineState;
    FRHIVertexShaderRef          AABBVertexShader;
    FRHIPixelShaderRef           AABBPixelShader;

    FRHIGraphicsPipelineStateRef PostPSO;
    FRHIPixelShaderRef           PostShader;
    FRHIGraphicsPipelineStateRef FXAAPSO;
    FRHIPixelShaderRef           FXAAShader;
    FRHIGraphicsPipelineStateRef FXAADebugPSO;
    FRHIPixelShaderRef           FXAADebugShader;

    TSharedRef<FRHITimestampQuery> TimestampQueries;

    FRHICommandStatistics FrameStatistics;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

extern RENDERER_API FRenderer GRenderer;

inline void AddDebugTexture(
    const TSharedRef<FRHIShaderResourceView>& ImageView,
    const TSharedRef<FRHITexture>& Image,
    EResourceAccess BeforeState,
    EResourceAccess AfterState)
{
    GRenderer.GetTextureDebugger()->AddTextureForDebugging(ImageView, Image, BeforeState, AfterState);
}

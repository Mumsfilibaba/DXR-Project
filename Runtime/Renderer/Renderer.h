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
// CRendererWindowHandler

class CRendererWindowHandler final : public CWindowMessageHandler
{
public:

    DECLARE_DELEGATE(CWindowResizedDelegate, const SWindowResizeEvent& ResizeEvent);
    CWindowResizedDelegate WindowResizedDelegate;

    CRendererWindowHandler() = default;
    ~CRendererWindowHandler() = default;

    virtual bool OnWindowResized(const SWindowResizeEvent& ResizeEvent) override final
    {
        WindowResizedDelegate.Execute(ResizeEvent);
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRendererStatistics

struct SRendererStatistics
{
    uint32 NumDrawCalls = 0;
    uint32 NumDispatchCalls = 0;
    uint32 NumRenderCommands = 0;

    void Reset()
    {
        NumDrawCalls      = 0;
        NumDispatchCalls  = 0;
        NumRenderCommands = 0;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRenderer

class RENDERER_API CRenderer
{
public:

    CRenderer();
    ~CRenderer() = default;

    bool Init();

    void Tick(const CScene& Scene);

    void Release();

    void PerformFrustumCullingAndSort(const CScene& Scene);

    void PerformFXAA(FRHICommandList& InCmdList);
    
    void PerformBackBufferBlit(FRHICommandList& InCmdList);

    void PerformAABBDebugPass(FRHICommandList& InCmdList);

    FORCEINLINE TSharedRef<CTextureDebugWindow> GetTextureDebugger() const
    {
        return TextureDebugger;
    }

    FORCEINLINE const SRendererStatistics& GetStatistics() const
    {
        return FrameStatistics;
    }

private:

    void OnWindowResize(const SWindowResizeEvent& Event);

    bool InitBoundingBoxDebugPass();

    bool InitAA();
    
    bool InitShadingImage();

    NOINLINE void FrustumCullingAndSortingInternal( const CCamera* Camera
                                                  , const TPair<uint32, uint32>& DrawCommands
                                                  , TArray<uint32>& OutDeferredDrawCommands
                                                  , TArray<uint32>& OutForwardDrawCommands);

    TSharedPtr<CRendererWindowHandler> WindowHandler;

    TSharedRef<CTextureDebugWindow> TextureDebugger;
    TSharedRef<CRendererInfoWindow> InfoWindow;
    TSharedRef<CGPUProfilerWindow>  GPUProfilerWindow;

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

    CDeferredRenderer             DeferredRenderer;
    CShadowMapRenderer            ShadowMapRenderer;
    CScreenSpaceOcclusionRenderer SSAORenderer;
    CLightProbeRenderer           LightProbeRenderer;
    CSkyboxRenderPass             SkyboxRenderPass;
    CForwardRenderer              ForwardRenderer;
    CRayTracer                    RayTracer;

    SFrameResources Resources;
    SLightSetup     LightSetup;

    TSharedRef<FRHITexture2D>            ShadingImage;
    TSharedRef<FRHIComputePipelineState> ShadingRatePipeline;
    TSharedRef<FRHIComputeShader>        ShadingRateShader;

    TSharedRef<FRHIVertexBuffer>          AABBVertexBuffer;
    TSharedRef<FRHIIndexBuffer>           AABBIndexBuffer;
    TSharedRef<FRHIGraphicsPipelineState> AABBDebugPipelineState;
    TSharedRef<FRHIVertexShader>          AABBVertexShader;
    TSharedRef<FRHIPixelShader>           AABBPixelShader;

    TSharedRef<FRHIGraphicsPipelineState> PostPSO;
    TSharedRef<FRHIPixelShader>           PostShader;
    TSharedRef<FRHIGraphicsPipelineState> FXAAPSO;
    TSharedRef<FRHIPixelShader>           FXAAShader;
    TSharedRef<FRHIGraphicsPipelineState> FXAADebugPSO;
    TSharedRef<FRHIPixelShader>           FXAADebugShader;

    TSharedRef<FRHITimestampQuery> TimestampQueries;

    SRendererStatistics FrameStatistics;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

extern RENDERER_API CRenderer GRenderer;

inline void AddDebugTexture( const TSharedRef<FRHIShaderResourceView>& ImageView
                           , const TSharedRef<FRHITexture>& Image
                           , EResourceAccess BeforeState
                           , EResourceAccess AfterState)
{
    GRenderer.GetTextureDebugger()->AddTextureForDebugging(ImageView, Image, BeforeState, AfterState);
}

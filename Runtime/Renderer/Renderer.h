#pragma once
#include "DeferredRenderer.h"
#include "ShadowMapRenderer.h"
#include "ScreenSpaceOcclusionRenderer.h"
#include "LightProbeRenderer.h"
#include "SkyboxRenderPass.h"
#include "ForwardRenderer.h"
#include "RayTracer.h"

#include "Core/Time/Timer.h"
#include "Core/Threading/DispatchQueue.h"

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
        NumDrawCalls = 0;
        NumDispatchCalls = 0;
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

    void PerformFrustumCulling(const CScene& Scene);
    void PerformFXAA(CRHICommandList& InCmdList);
    void PerformBackBufferBlit(CRHICommandList& InCmdList);

    void PerformAABBDebugPass(CRHICommandList& InCmdList);

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

    TSharedPtr<CRendererWindowHandler> WindowHandler;

    TSharedRef<CTextureDebugWindow> TextureDebugger;
    TSharedRef<CRendererInfoWindow> InfoWindow;
    TSharedRef<CGPUProfilerWindow>  GPUProfilerWindow;

    CRHICommandList PreShadowsCmdList;
    CRHICommandList PointShadowCmdList;
    CRHICommandList DirShadowCmdList;
    CRHICommandList PrepareGBufferCmdList;
    CRHICommandList PrePassCmdList;
    CRHICommandList ShadingRateCmdList;
    CRHICommandList RayTracingCmdList;
    CRHICommandList BasePassCmdList;
    CRHICommandList MainCmdList;

    SDispatch PointShadowTask;
    SDispatch DirShadowTask;
    SDispatch PrePassTask;
    SDispatch BasePassTask;
    SDispatch RayTracingTask;

    CDeferredRenderer             DeferredRenderer;
    CShadowMapRenderer            ShadowMapRenderer;
    CScreenSpaceOcclusionRenderer SSAORenderer;
    CLightProbeRenderer           LightProbeRenderer;
    CSkyboxRenderPass             SkyboxRenderPass;
    CForwardRenderer              ForwardRenderer;
    CRayTracer                    RayTracer;

    SFrameResources Resources;
    SLightSetup     LightSetup;

    TSharedRef<CRHITexture2D>            ShadingImage;
    TSharedRef<CRHIComputePipelineState> ShadingRatePipeline;
    TSharedRef<CRHIComputeShader>        ShadingRateShader;

    TSharedRef<CRHIBuffer>                AABBVertexBuffer;
    TSharedRef<CRHIBuffer>                AABBIndexBuffer;
    TSharedRef<CRHIGraphicsPipelineState> AABBDebugPipelineState;
    TSharedRef<CRHIVertexShader>          AABBVertexShader;
    TSharedRef<CRHIPixelShader>           AABBPixelShader;

    TSharedRef<CRHIGraphicsPipelineState> PostPSO;
    TSharedRef<CRHIPixelShader>           PostShader;
    TSharedRef<CRHIGraphicsPipelineState> FXAAPSO;
    TSharedRef<CRHIPixelShader>           FXAAShader;
    TSharedRef<CRHIGraphicsPipelineState> FXAADebugPSO;
    TSharedRef<CRHIPixelShader>           FXAADebugShader;

    TSharedRef<CRHITimestampQuery> TimestampQueries;

    SRendererStatistics FrameStatistics;

};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

extern RENDERER_API CRenderer GRenderer;

inline void AddDebugTexture(const TSharedRef<CRHIShaderResourceView>& ImageView, const TSharedRef<CRHITexture>& Image, ERHIResourceState BeforeState, ERHIResourceState AfterState)
{
    GRenderer.GetTextureDebugger()->AddTextureForDebugging(ImageView, Image, BeforeState, AfterState);
}

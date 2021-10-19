#pragma once

#include "Scene/Actor.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"

#include "Assets/MeshFactory.h"

#include "Resources/Mesh.h"
#include "Resources/Material.h"

#include "DeferredRenderer.h"
#include "ShadowMapRenderer.h"
#include "ScreenSpaceOcclusionRenderer.h"
#include "LightProbeRenderer.h"
#include "SkyboxRenderPass.h"
#include "ForwardRenderer.h"
#include "RayTracer.h"

#include "CoreRHI/RHIModule.h"
#include "CoreRHI/RHICommandList.h"
#include "CoreRHI/RHIViewport.h"

#include "UIRenderer.h"

#include "Core/Time/Timer.h"
#include "Core/Threading/DispatchQueue.h"
#include "Core/Application/WindowMessageHandler.h"

class CRendererWindowHandler final : public CWindowMessageHandler
{
public:

    DECLARE_DELEGATE( CWindowResizedDelegate, const SWindowResizeEvent& ResizeEvent );
    CWindowResizedDelegate WindowResizedDelegate;

    CRendererWindowHandler() = default;
    ~CRendererWindowHandler() = default;

    virtual bool OnWindowResized( const SWindowResizeEvent& ResizeEvent ) override final
    {
        WindowResizedDelegate.Execute( ResizeEvent );
        return true;
    }
};

class CORE_API CRenderer
{
public:

    CRenderer() = default;
    ~CRenderer() = default;

    bool Init();

    void Tick( const CScene& Scene );

    void Release();

    void PerformFrustumCulling( const CScene& Scene );
    void PerformFXAA( CRHICommandList& InCmdList );
    void PerformBackBufferBlit( CRHICommandList& InCmdList );

    void PerformAABBDebugPass( CRHICommandList& InCmdList );

    void RenderDebugInterface();

private:

    void OnWindowResize( const SWindowResizeEvent& Event );

    bool InitBoundingBoxDebugPass();
    bool InitAA();
    bool InitShadingImage();

    void ResizeResources( uint32 Width, uint32 Height );

    CRendererWindowHandler WindowHandler;

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

    TSharedRef<CRHIVertexBuffer> AABBVertexBuffer;
    TSharedRef<CRHIIndexBuffer>  AABBIndexBuffer;
    TSharedRef<CRHIGraphicsPipelineState> AABBDebugPipelineState;
    TSharedRef<CRHIVertexShader>          AABBVertexShader;
    TSharedRef<CRHIPixelShader>           AABBPixelShader;

    TSharedRef<CRHIGraphicsPipelineState> PostPSO;
    TSharedRef<CRHIPixelShader>           PostShader;
    TSharedRef<CRHIGraphicsPipelineState> FXAAPSO;
    TSharedRef<CRHIPixelShader>           FXAAShader;
    TSharedRef<CRHIGraphicsPipelineState> FXAADebugPSO;
    TSharedRef<CRHIPixelShader>           FXAADebugShader;

    TSharedRef<CGPUProfiler> GPUProfiler;

    uint32 LastFrameNumDrawCalls = 0;
    uint32 LastFrameNumDispatchCalls = 0;
    uint32 LastFrameNumCommands = 0;
};

extern CORE_API CRenderer GRenderer;

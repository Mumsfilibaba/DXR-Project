#include "Renderer.h"

#include "Interface/InterfaceApplication.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

#include "RHI/RHICore.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/TextureFactory.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Engine.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Lights/DirectionalLight.h"

#include "Core/Math/Frustum.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"

#include "Renderer/Debug/GPUProfiler.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

TConsoleVariable<bool> GEnableSSAO( true );

TConsoleVariable<bool> GEnableFXAA( true );
TConsoleVariable<bool> GFXAADebug( false );

TConsoleVariable<bool> GEnableVariableRateShading( false );

TConsoleVariable<bool> GPrePassEnabled( true );
TConsoleVariable<bool> GDrawAABBs( false );
TConsoleVariable<bool> GVSyncEnabled( false );
TConsoleVariable<bool> GFrustumCullEnabled( true );
TConsoleVariable<bool> GRayTracingEnabled( true );

static const uint32 ShadowMapSampleCount = 2;

///////////////////////////////////////////////////////////////////////////////////////////////////

struct SCameraBufferDesc
{
    CMatrix4 ViewProjection;
    CMatrix4 View;
    CMatrix4 ViewInv;
    CMatrix4 Projection;
    CMatrix4 ProjectionInv;
    CMatrix4 ViewProjectionInv;

    CVector3 Position;
    float    NearPlane;

    CVector3 Forward;
    float    FarPlane;

    CVector3 Right;
    float    AspectRatio;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

RENDERER_API CRenderer GRenderer;

///////////////////////////////////////////////////////////////////////////////////////////////////

CRenderer::CRenderer()
    : WindowHandler( MakeShared<CRendererWindowHandler>() )
{
}

bool CRenderer::Init()
{
    INIT_CONSOLE_VARIABLE( "r.EnableSSAO", &GEnableSSAO );
    INIT_CONSOLE_VARIABLE( "r.EnableFXAA", &GEnableFXAA );
    INIT_CONSOLE_VARIABLE( "r.EnableVariableRateShading", &GEnableVariableRateShading );
    INIT_CONSOLE_VARIABLE( "r.EnablePrePass", &GPrePassEnabled );
    INIT_CONSOLE_VARIABLE( "r.EnableDrawAABBs", &GDrawAABBs );
    INIT_CONSOLE_VARIABLE( "r.EnableVerticalSync", &GVSyncEnabled );
    INIT_CONSOLE_VARIABLE( "r.EnableFrustumCulling", &GFrustumCullEnabled );
    INIT_CONSOLE_VARIABLE( "r.EnableRayTracing", &GRayTracingEnabled );
    INIT_CONSOLE_VARIABLE( "r.FXAADebug", &GFXAADebug );

    Resources.MainWindowViewport = RHICreateViewport( GEngine->MainWindow.Get(), 0, 0, EFormat::R8G8B8A8_Unorm, EFormat::Unknown );
    if ( !Resources.MainWindowViewport )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        Resources.MainWindowViewport->SetName( "Main Window Viewport" );
    }

    Resources.CameraBuffer = RHICreateConstantBuffer<SCameraBufferDesc>( BufferFlag_Default, EResourceState::Common, nullptr );
    if ( !Resources.CameraBuffer )
    {
        LOG_ERROR( "[Renderer]: Failed to create CameraBuffer" );
        return false;
    }
    else
    {
        Resources.CameraBuffer->SetName( "CameraBuffer" );
    }

    // Init standard input layout
    SInputLayoutStateCreateInfo InputLayout =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, 0, 0,  EInputClassification::Vertex, 0 },
        { "NORMAL",   0, EFormat::R32G32B32_Float, 0, 12, EInputClassification::Vertex, 0 },
        { "TANGENT",  0, EFormat::R32G32B32_Float, 0, 24, EInputClassification::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,    0, 36, EInputClassification::Vertex, 0 },
    };

    Resources.StdInputLayout = RHICreateInputLayout( InputLayout );
    if ( !Resources.StdInputLayout )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        Resources.StdInputLayout->SetName( "Standard InputLayoutState" );
    }

    {
        SSamplerStateCreateInfo CreateInfo;
        CreateInfo.AddressU = ESamplerMode::Border;
        CreateInfo.AddressV = ESamplerMode::Border;
        CreateInfo.AddressW = ESamplerMode::Border;
        CreateInfo.Filter = ESamplerFilter::MinMagMipPoint;
        CreateInfo.BorderColor = SColorF( 1.0f, 1.0f, 1.0f, 1.0f );

        Resources.DirectionalLightShadowSampler = RHICreateSamplerState( CreateInfo );
        if ( !Resources.DirectionalLightShadowSampler )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            Resources.DirectionalLightShadowSampler->SetName( "ShadowMap Sampler" );
        }
    }

    {
        SSamplerStateCreateInfo CreateInfo;
        CreateInfo.AddressU = ESamplerMode::Wrap;
        CreateInfo.AddressV = ESamplerMode::Wrap;
        CreateInfo.AddressW = ESamplerMode::Wrap;
        CreateInfo.Filter = ESamplerFilter::Comparison_MinMagMipLinear;
        CreateInfo.ComparisonFunc = EComparisonFunc::LessEqual;

        Resources.PointLightShadowSampler = RHICreateSamplerState( CreateInfo );
        if ( !Resources.PointLightShadowSampler )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            Resources.PointLightShadowSampler->SetName( "ShadowMap Comparison Sampler" );
        }
    }

    if ( !InitAA() )
    {
        return false;
    }

    if ( !InitBoundingBoxDebugPass() )
    {
        return false;
    }

    if ( !InitShadingImage() )
    {
        return false;
    }

    if ( !LightSetup.Init() )
    {
        return false;
    }

    if ( !DeferredRenderer.Init( Resources ) )
    {
        return false;
    }

    if ( !ShadowMapRenderer.Init( LightSetup, Resources ) )
    {
        return false;
    }

    if ( !SSAORenderer.Init( Resources ) )
    {
        return false;
    }

    if ( !LightProbeRenderer.Init( LightSetup, Resources ) )
    {
        return false;
    }

    if ( !SkyboxRenderPass.Init( Resources ) )
    {
        return false;
    }

    if ( !ForwardRenderer.Init( Resources ) )
    {
        return false;
    }

    if ( RHISupportsRayTracing() )
    {
        if ( !RayTracer.Init( Resources ) )
        {
            return false;
        }
    }

    LightProbeRenderer.RenderSkyLightProbe( MainCmdList, LightSetup, Resources );

    CRHICommandQueue::Get().ExecuteCommandList( MainCmdList );

    CInterfaceApplication& Application = CInterfaceApplication::Get();

    // Register EventFunc
    WindowHandler->WindowResizedDelegate.BindRaw( this, &CRenderer::OnWindowResize );
    Application.AddWindowMessageHandler( WindowHandler, uint32(-1) );

    // Register Windows
    TextureDebugger = CTextureDebugWindow::Make();
    Application.AddWindow( TextureDebugger );

    InfoWindow = CRendererInfoWindow::Make();
    Application.AddWindow( InfoWindow );

    GPUProfilerWindow = CGPUProfilerWindow::Make();
    Application.AddWindow( GPUProfilerWindow );

    return true;
}

void CRenderer::PerformFrustumCulling( const CScene& Scene )
{
    TRACE_SCOPE( "Frustum Culling" );

    CCamera* Camera = Scene.GetCamera();
    CFrustum CameraFrustum = CFrustum( Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix() );
    for ( const SMeshDrawCommand& Command : Scene.GetMeshDrawCommands() )
    {
        CMatrix4 TransformMatrix = Command.CurrentActor->GetTransform().GetMatrix();
        TransformMatrix = TransformMatrix.Transpose();

        CVector3 Top = CVector3( &Command.Mesh->BoundingBox.Top.x );
        Top = TransformMatrix.TransformPosition( Top );
        CVector3 Bottom = CVector3( &Command.Mesh->BoundingBox.Bottom.x );
        Bottom = TransformMatrix.TransformPosition( Bottom );

        SAABB Box;
        Box.Top = Top;
        Box.Bottom = Bottom;
        if ( CameraFrustum.CheckAABB( Box ) )
        {
            if ( Command.Material->ShouldRenderInForwardPass() )
            {
                Resources.ForwardVisibleCommands.Emplace( Command );
            }
            else
            {
                Resources.DeferredVisibleCommands.Emplace( Command );
            }
        }
    }
}

void CRenderer::PerformFXAA( CRHICommandList& InCmdList )
{
    INSERT_DEBUG_CMDLIST_MARKER( InCmdList, "Begin FXAA" );

    TRACE_SCOPE( "FXAA" );

    GPU_TRACE_SCOPE( InCmdList, "FXAA" );

    struct FXAASettings
    {
        float Width;
        float Height;
    } Settings;

    Settings.Width = static_cast<float>(Resources.BackBuffer->GetWidth());
    Settings.Height = static_cast<float>(Resources.BackBuffer->GetHeight());

    CRHIRenderTargetView* BackBufferRTV = Resources.BackBuffer->GetRenderTargetView();
    InCmdList.SetRenderTargets( &BackBufferRTV, 1, nullptr );

    CRHIShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetShaderResourceView();
    if ( GFXAADebug.GetBool() )
    {
        InCmdList.SetShaderResourceView( FXAADebugShader.Get(), FinalTargetSRV, 0 );
        InCmdList.SetSamplerState( FXAADebugShader.Get(), Resources.FXAASampler.Get(), 0 );
        InCmdList.Set32BitShaderConstants( FXAADebugShader.Get(), &Settings, 2 );
        InCmdList.SetGraphicsPipelineState( FXAADebugPSO.Get() );
    }
    else
    {
        InCmdList.SetShaderResourceView( FXAAShader.Get(), FinalTargetSRV, 0 );
        InCmdList.SetSamplerState( FXAAShader.Get(), Resources.FXAASampler.Get(), 0 );
        InCmdList.Set32BitShaderConstants( FXAAShader.Get(), &Settings, 2 );
        InCmdList.SetGraphicsPipelineState( FXAAPSO.Get() );
    }

    InCmdList.DrawInstanced( 3, 1, 0, 0 );

    INSERT_DEBUG_CMDLIST_MARKER( InCmdList, "End FXAA" );
}

void CRenderer::PerformBackBufferBlit( CRHICommandList& InCmdList )
{
    INSERT_DEBUG_CMDLIST_MARKER( InCmdList, "Begin Draw BackBuffer" );

    TRACE_SCOPE( "Draw to BackBuffer" );

    CRHIRenderTargetView* BackBufferRTV = Resources.BackBuffer->GetRenderTargetView();
    InCmdList.SetRenderTargets( &BackBufferRTV, 1, nullptr );

    CRHIShaderResourceView* FinalTargetSRV = Resources.FinalTarget->GetShaderResourceView();
    InCmdList.SetShaderResourceView( PostShader.Get(), FinalTargetSRV, 0 );
    InCmdList.SetSamplerState( PostShader.Get(), Resources.GBufferSampler.Get(), 0 );

    InCmdList.SetGraphicsPipelineState( PostPSO.Get() );
    InCmdList.DrawInstanced( 3, 1, 0, 0 );

    INSERT_DEBUG_CMDLIST_MARKER( InCmdList, "End Draw BackBuffer" );
}

void CRenderer::PerformAABBDebugPass( CRHICommandList& InCmdList )
{
    INSERT_DEBUG_CMDLIST_MARKER( InCmdList, "Begin DebugPass" );

    TRACE_SCOPE( "DebugPass" );

    InCmdList.SetGraphicsPipelineState( AABBDebugPipelineState.Get() );

    InCmdList.SetPrimitiveTopology( EPrimitiveTopology::LineList );

    InCmdList.SetConstantBuffer( AABBVertexShader.Get(), Resources.CameraBuffer.Get(), 0 );

    InCmdList.SetVertexBuffers( &AABBVertexBuffer, 1, 0 );
    InCmdList.SetIndexBuffer( AABBIndexBuffer.Get() );

    for ( const SMeshDrawCommand& Command : Resources.DeferredVisibleCommands )
    {
        SAABB& Box = Command.Mesh->BoundingBox;
        CVector3 Scale = CVector3( Box.GetWidth(), Box.GetHeight(), Box.GetDepth() );
        CVector3 Position = Box.GetCenter();

        CMatrix4 TranslationMatrix = CMatrix4::Translation( Position.x, Position.y, Position.z );
        CMatrix4 ScaleMatrix = CMatrix4::Scale( Scale.x, Scale.y, Scale.z );
        CMatrix4 TransformMatrix = Command.CurrentActor->GetTransform().GetMatrix();
        TransformMatrix = TransformMatrix.Transpose();
        TransformMatrix = (ScaleMatrix * TranslationMatrix) * TransformMatrix;
        TransformMatrix.Transpose();

        InCmdList.Set32BitShaderConstants( AABBVertexShader.Get(), &TranslationMatrix, 16 );

        InCmdList.DrawIndexedInstanced( 24, 1, 0, 0, 0 );
    }

    INSERT_DEBUG_CMDLIST_MARKER( InCmdList, "End DebugPass" );
}

void CRenderer::Tick( const CScene& Scene )
{
    Resources.BackBuffer = Resources.MainWindowViewport->GetBackBuffer();

    // Prepare Lights
#if 1
    PreShadowsCmdList.BeginExternalCapture();
#endif

    CGPUProfiler::Get().BeginGPUFrame( PreShadowsCmdList );

    INSERT_DEBUG_CMDLIST_MARKER( PreShadowsCmdList, "--BEGIN FRAME--" );

    LightSetup.BeginFrame( PreShadowsCmdList, Scene );

    // Init point light task
    const auto RenderPointShadows = [&]()
    {
        CRenderer::ShadowMapRenderer.RenderPointLightShadows( PointShadowCmdList, LightSetup, Scene );
    };

    if ( !PointShadowTask.Delegate.IsBound() )
    {
        PointShadowTask.Delegate.BindLambda( RenderPointShadows );
    }

    CDispatchQueue::Get().Dispatch( PointShadowTask );

    // Init directional light task
    const auto RenderDirShadows = [&]()
    {
        CRenderer::ShadowMapRenderer.RenderDirectionalLightShadows( DirShadowCmdList, LightSetup, Resources, Scene );
    };

    DirShadowTask.Delegate.BindLambda( RenderDirShadows );
    CDispatchQueue::Get().Dispatch( DirShadowTask );

    // Perform frustum culling
    Resources.DeferredVisibleCommands.Clear();
    Resources.ForwardVisibleCommands.Clear();

    // Clear the images that were debuggable last frame 
    // TODO: Make this persistent, we do not need to do this every frame, right know it is because the resource-state system needs overhaul
    TextureDebugger->ClearImages();

    if ( !GFrustumCullEnabled.GetBool() )
    {
        for ( const SMeshDrawCommand& Command : Scene.GetMeshDrawCommands() )
        {
            if ( Command.Material->HasAlphaMask() )
            {
                Resources.ForwardVisibleCommands.Emplace( Command );
            }
            else
            {
                Resources.DeferredVisibleCommands.Emplace( Command );
            }
        }
    }
    else
    {
        PerformFrustumCulling( Scene );
    }

    // Update camera-buffer
    SCameraBufferDesc CamBuff;
    CamBuff.ViewProjection = Scene.GetCamera()->GetViewProjectionMatrix();
    CamBuff.View = Scene.GetCamera()->GetViewMatrix();
    CamBuff.ViewInv = Scene.GetCamera()->GetViewInverseMatrix();
    CamBuff.Projection = Scene.GetCamera()->GetProjectionMatrix();
    CamBuff.ProjectionInv = Scene.GetCamera()->GetProjectionInverseMatrix();
    CamBuff.ViewProjectionInv = Scene.GetCamera()->GetViewProjectionInverseMatrix();
    CamBuff.Position = Scene.GetCamera()->GetPosition();
    CamBuff.Forward = Scene.GetCamera()->GetForward();
    CamBuff.Right = Scene.GetCamera()->GetRight();
    CamBuff.NearPlane = Scene.GetCamera()->GetNearPlane();
    CamBuff.FarPlane = Scene.GetCamera()->GetFarPlane();
    CamBuff.AspectRatio = Scene.GetCamera()->GetAspectRatio();

    PrepareGBufferCmdList.TransitionBuffer( Resources.CameraBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest );

    PrepareGBufferCmdList.UpdateBuffer( Resources.CameraBuffer.Get(), 0, sizeof( SCameraBufferDesc ), &CamBuff );

    PrepareGBufferCmdList.TransitionBuffer( Resources.CameraBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer );

    PrepareGBufferCmdList.TransitionTexture( Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::RenderTarget );
    PrepareGBufferCmdList.TransitionTexture( Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::RenderTarget );
    PrepareGBufferCmdList.TransitionTexture( Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::RenderTarget );
    PrepareGBufferCmdList.TransitionTexture( Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::RenderTarget );
    PrepareGBufferCmdList.TransitionTexture( Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceState::PixelShaderResource, EResourceState::DepthWrite );

    SColorF BlackClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    PrepareGBufferCmdList.ClearRenderTargetView( Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetRenderTargetView(), BlackClearColor );
    PrepareGBufferCmdList.ClearRenderTargetView( Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetRenderTargetView(), BlackClearColor );
    PrepareGBufferCmdList.ClearRenderTargetView( Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetRenderTargetView(), BlackClearColor );
    PrepareGBufferCmdList.ClearRenderTargetView( Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetRenderTargetView(), BlackClearColor );
    PrepareGBufferCmdList.ClearDepthStencilView( Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView(), SDepthStencil( 1.0f, 0 ) );

    if ( GPrePassEnabled.GetBool() )
    {
        const auto RenderPrePass = [&]()
        {
            CRenderer::DeferredRenderer.RenderPrePass( PrePassCmdList, Resources, Scene );
        };

        PrePassTask.Delegate.BindLambda( RenderPrePass );
        CDispatchQueue::Get().Dispatch( PrePassTask );
    }

    if ( ShadingImage && GEnableVariableRateShading.GetBool() )
    {
        INSERT_DEBUG_CMDLIST_MARKER( ShadingRateCmdList, "Begin VRS Image" );
        ShadingRateCmdList.SetShadingRate( EShadingRate::VRS_1x1 );

        ShadingRateCmdList.TransitionTexture( ShadingImage.Get(), EResourceState::ShadingRateSource, EResourceState::UnorderedAccess );

        ShadingRateCmdList.SetComputePipelineState( ShadingRatePipeline.Get() );

        CRHIUnorderedAccessView* ShadingImageUAV = ShadingImage->GetUnorderedAccessView();
        ShadingRateCmdList.SetUnorderedAccessView( ShadingRateShader.Get(), ShadingImageUAV, 0 );

        ShadingRateCmdList.Dispatch( ShadingImage->GetWidth(), ShadingImage->GetHeight(), 1 );

        ShadingRateCmdList.TransitionTexture( ShadingImage.Get(), EResourceState::UnorderedAccess, EResourceState::ShadingRateSource );

        ShadingRateCmdList.SetShadingRateImage( ShadingImage.Get() );

        INSERT_DEBUG_CMDLIST_MARKER( ShadingRateCmdList, "End VRS Image" );
    }
    else if ( RHISupportsVariableRateShading() )
    {
        ShadingRateCmdList.SetShadingRate( EShadingRate::VRS_1x1 );
    }

    if ( /* DISABLES CODE */ (false) /*IsRayTracingSupported())*/ )
    {
        const auto RenderRayTracing = [&]()
        {
            GPU_TRACE_SCOPE( RayTracingCmdList, "Ray Tracing" );
            CRenderer::RayTracer.PreRender( RayTracingCmdList, Resources, Scene );
        };

        RayTracingTask.Delegate.BindLambda( RenderRayTracing );
        CDispatchQueue::Get().Dispatch( RayTracingTask );
    }

    {
        const auto RenderBasePass = [&]()
        {
            CRenderer::DeferredRenderer.RenderBasePass( BasePassCmdList, Resources );
        };

        BasePassTask.Delegate.BindLambda( RenderBasePass );
        CDispatchQueue::Get().Dispatch( BasePassTask );
    }

    MainCmdList.TransitionTexture( Resources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(), EResourceState::RenderTarget, EResourceState::NonPixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( Resources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetShaderResourceView() ),
        Resources.GBuffer[GBUFFER_ALBEDO_INDEX],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource );

    MainCmdList.TransitionTexture( Resources.GBuffer[GBUFFER_NORMAL_INDEX].Get(), EResourceState::RenderTarget, EResourceState::NonPixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( Resources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView() ),
        Resources.GBuffer[GBUFFER_NORMAL_INDEX],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource );

    MainCmdList.TransitionTexture( Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(), EResourceState::RenderTarget, EResourceState::NonPixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetShaderResourceView() ),
        Resources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource );

    MainCmdList.TransitionTexture( Resources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(), EResourceState::RenderTarget, EResourceState::NonPixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( Resources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView() ),
        Resources.GBuffer[GBUFFER_MATERIAL_INDEX],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource );

    MainCmdList.TransitionTexture( Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceState::DepthWrite, EResourceState::NonPixelShaderResource );
    MainCmdList.TransitionTexture( Resources.SSAOBuffer.Get(), EResourceState::NonPixelShaderResource, EResourceState::UnorderedAccess );

    const SColorF WhiteColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    MainCmdList.ClearUnorderedAccessView( Resources.SSAOBuffer->GetUnorderedAccessView(), WhiteColor );

    if ( GEnableSSAO.GetBool() )
    {
        GPU_TRACE_SCOPE( MainCmdList, "SSAO" );
        SSAORenderer.Render( MainCmdList, Resources );
    }

    MainCmdList.TransitionTexture( Resources.SSAOBuffer.Get(), EResourceState::UnorderedAccess, EResourceState::NonPixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( Resources.SSAOBuffer->GetShaderResourceView() ),
        Resources.SSAOBuffer,
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource );

    {
        MainCmdList.TransitionTexture( Resources.FinalTarget.Get(), EResourceState::PixelShaderResource, EResourceState::UnorderedAccess );
        MainCmdList.TransitionTexture( Resources.BackBuffer, EResourceState::Present, EResourceState::RenderTarget );
        MainCmdList.TransitionTexture( LightSetup.IrradianceMap.Get(), EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource );
        MainCmdList.TransitionTexture( LightSetup.SpecularIrradianceMap.Get(), EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource );
        MainCmdList.TransitionTexture( Resources.IntegrationLUT.Get(), EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource );

        ShadowMapRenderer.RenderShadowMasks( MainCmdList, LightSetup, Resources );

        DeferredRenderer.RenderDeferredTiledLightPass( MainCmdList, Resources, LightSetup );
    }

    MainCmdList.TransitionTexture( Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::DepthWrite );
    MainCmdList.TransitionTexture( Resources.FinalTarget.Get(), EResourceState::UnorderedAccess, EResourceState::RenderTarget );

    SkyboxRenderPass.Render( MainCmdList, Resources, Scene );

    MainCmdList.TransitionTexture( LightSetup.PointLightShadowMaps.Get(), EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( LightSetup.DirectionalShadowMask->GetShaderResourceView() ),
        LightSetup.DirectionalShadowMask,
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( LightSetup.ShadowMapCascades[0]->GetShaderResourceView() ),
        LightSetup.ShadowMapCascades[0],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( LightSetup.ShadowMapCascades[1]->GetShaderResourceView() ),
        LightSetup.ShadowMapCascades[1],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( LightSetup.ShadowMapCascades[2]->GetShaderResourceView() ),
        LightSetup.ShadowMapCascades[2],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( LightSetup.ShadowMapCascades[3]->GetShaderResourceView() ),
        LightSetup.ShadowMapCascades[3],
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource );

    MainCmdList.TransitionTexture( LightSetup.IrradianceMap.Get(), EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource );
    MainCmdList.TransitionTexture( LightSetup.SpecularIrradianceMap.Get(), EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource );
    MainCmdList.TransitionTexture( Resources.IntegrationLUT.Get(), EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( Resources.IntegrationLUT->GetShaderResourceView() ),
        Resources.IntegrationLUT,
        EResourceState::PixelShaderResource,
        EResourceState::PixelShaderResource );

    if ( !Resources.ForwardVisibleCommands.IsEmpty() )
    {
        GPU_TRACE_SCOPE( MainCmdList, "Forward Pass" );
        ForwardRenderer.Render( MainCmdList, Resources, LightSetup );
    }

    MainCmdList.TransitionTexture( Resources.FinalTarget.Get(), EResourceState::RenderTarget, EResourceState::PixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( Resources.FinalTarget->GetShaderResourceView() ),
        Resources.FinalTarget,
        EResourceState::PixelShaderResource,
        EResourceState::PixelShaderResource );

    MainCmdList.TransitionTexture( Resources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceState::DepthWrite, EResourceState::PixelShaderResource );

    TextureDebugger->AddTextureForDebugging(
        MakeSharedRef<CRHIShaderResourceView>( Resources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView() ),
        Resources.GBuffer[GBUFFER_DEPTH_INDEX],
        EResourceState::PixelShaderResource,
        EResourceState::PixelShaderResource );

    if ( GEnableFXAA.GetBool() )
    {
        PerformFXAA( MainCmdList );
    }
    else
    {
        PerformBackBufferBlit( MainCmdList );
    }

    if ( GDrawAABBs.GetBool() )
    {
        PerformAABBDebugPass( MainCmdList );
    }

    INSERT_DEBUG_CMDLIST_MARKER( MainCmdList, "Begin UI Render" );

    {
        TRACE_SCOPE( "Render UI" );

        if ( RHISupportsVariableRateShading() )
        {
            MainCmdList.SetShadingRate( EShadingRate::VRS_1x1 );
            MainCmdList.SetShadingRateImage( nullptr );
        }

        CInterfaceApplication::Get().DrawWindows( MainCmdList );
    }

    INSERT_DEBUG_CMDLIST_MARKER( MainCmdList, "End UI Render" );

    MainCmdList.TransitionTexture( Resources.BackBuffer, EResourceState::RenderTarget, EResourceState::Present );

    INSERT_DEBUG_CMDLIST_MARKER( MainCmdList, "--END FRAME--" );

    CGPUProfiler::Get().EndGPUFrame( MainCmdList );

#if 1
    MainCmdList.EndExternalCapture();
#endif

    CDispatchQueue::Get().WaitForAll();

    {
        TRACE_SCOPE( "ExecuteCommandList" );

        CRHICommandList* CmdLists[9] =
        {
            &PreShadowsCmdList,
            &PointShadowCmdList,
            &DirShadowCmdList,
            &PrepareGBufferCmdList,
            &PrePassCmdList,
            &ShadingRateCmdList,
            &RayTracingCmdList,
            &BasePassCmdList,
            &MainCmdList
        };

        CRHICommandQueue::Get().ExecuteCommandLists( CmdLists, ArrayCount( CmdLists ) );

        FrameStatistics.NumDrawCalls = CRHICommandQueue::Get().GetNumDrawCalls();
        FrameStatistics.NumDispatchCalls = CRHICommandQueue::Get().GetNumDispatchCalls();
        FrameStatistics.NumRenderCommands = CRHICommandQueue::Get().GetNumCommands();
    }

    {
        TRACE_SCOPE( "Present" );
        Resources.MainWindowViewport->Present( GVSyncEnabled.GetBool() );
    }
}

void CRenderer::Release()
{
    CRHICommandQueue::Get().WaitForGPU();

    PreShadowsCmdList.Reset();
    PointShadowCmdList.Reset();
    DirShadowCmdList.Reset();
    PrepareGBufferCmdList.Reset();
    PrePassCmdList.Reset();
    ShadingRateCmdList.Reset();
    RayTracingCmdList.Reset();
    BasePassCmdList.Reset();
    MainCmdList.Reset();

    DeferredRenderer.Release();
    ShadowMapRenderer.Release();
    SSAORenderer.Release();
    LightProbeRenderer.Release();
    SkyboxRenderPass.Release();
    ForwardRenderer.Release();
    RayTracer.Release();

    Resources.Release();
    LightSetup.Release();

    AABBVertexBuffer.Reset();
    AABBIndexBuffer.Reset();
    AABBDebugPipelineState.Reset();
    AABBVertexShader.Reset();
    AABBPixelShader.Reset();

    PostPSO.Reset();
    PostShader.Reset();
    FXAAPSO.Reset();
    FXAAShader.Reset();
    FXAADebugPSO.Reset();
    FXAADebugShader.Reset();

    ShadingImage.Reset();
    ShadingRatePipeline.Reset();
    ShadingRateShader.Reset();

    TimestampQueries.Reset();

    FrameStatistics.Reset();

	if ( CInterfaceApplication::IsInitialized() )
	{
		CInterfaceApplication& Application = CInterfaceApplication::Get();
		Application.RemoveWindow( TextureDebugger );
		TextureDebugger.Reset();

		Application.RemoveWindow( InfoWindow );
		InfoWindow.Reset();

		Application.RemoveWindow( GPUProfilerWindow );
		GPUProfilerWindow.Reset();
	}
}

void CRenderer::OnWindowResize( const SWindowResizeEvent& Event )
{
    const uint32 Width = Event.Width;
    const uint32 Height = Event.Height;

    if ( !Resources.MainWindowViewport->Resize( Width, Height ) )
    {
        CDebug::DebugBreak();
        return;
    }

    if ( !DeferredRenderer.ResizeResources( Resources ) )
    {
        CDebug::DebugBreak();
        return;
    }

    if ( !SSAORenderer.ResizeResources( Resources ) )
    {
        CDebug::DebugBreak();
        return;
    }

    if ( !ShadowMapRenderer.ResizeResources( Width, Height, LightSetup ) )
    {
        CDebug::DebugBreak();
        return;
    }
}

bool CRenderer::InitBoundingBoxDebugPass()
{
    TArray<uint8> ShaderCode;
    if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/Debug.hlsl", "VSMain", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    AABBVertexShader = RHICreateVertexShader( ShaderCode );
    if ( !AABBVertexShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        AABBVertexShader->SetName( "Debug VertexShader" );
    }

    if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/Debug.hlsl", "PSMain", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    AABBPixelShader = RHICreatePixelShader( ShaderCode );
    if ( !AABBPixelShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        AABBPixelShader->SetName( "Debug PixelShader" );
    }

    SInputLayoutStateCreateInfo InputLayout =
    {
        { "POSITION", 0, EFormat::R32G32B32_Float, 0, 0, EInputClassification::Vertex, 0 },
    };

    TSharedRef<CRHIInputLayoutState> InputLayoutState = RHICreateInputLayout( InputLayout );
    if ( !InputLayoutState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        InputLayoutState->SetName( "Debug InputLayoutState" );
    }

    SDepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc = EComparisonFunc::LessEqual;
    DepthStencilStateInfo.DepthEnable = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState( DepthStencilStateInfo );
    if ( !DepthStencilState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName( "Debug DepthStencilState" );
    }

    SRasterizerStateCreateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState( RasterizerStateInfo );
    if ( !RasterizerState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName( "Debug RasterizerState" );
    }

    SBlendStateCreateInfo BlendStateInfo;

    TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState( BlendStateInfo );
    if ( !BlendState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName( "Debug BlendState" );
    }

    SGraphicsPipelineStateCreateInfo PSOProperties;
    PSOProperties.BlendState = BlendState.Get();
    PSOProperties.DepthStencilState = DepthStencilState.Get();
    PSOProperties.InputLayoutState = InputLayoutState.Get();
    PSOProperties.RasterizerState = RasterizerState.Get();
    PSOProperties.ShaderState.VertexShader = AABBVertexShader.Get();
    PSOProperties.ShaderState.PixelShader = AABBPixelShader.Get();
    PSOProperties.PrimitiveTopologyType = EPrimitiveTopologyType::Line;
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = Resources.RenderTargetFormat;
    PSOProperties.PipelineFormats.NumRenderTargets = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat = Resources.DepthBufferFormat;

    AABBDebugPipelineState = RHICreateGraphicsPipelineState( PSOProperties );
    if ( !AABBDebugPipelineState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        AABBDebugPipelineState->SetName( "Debug PipelineState" );
    }

    TStaticArray<CVector3, 8> Vertices =
    {
        CVector3( -0.5f, -0.5f,  0.5f ),
        CVector3( 0.5f, -0.5f,  0.5f ),
        CVector3( -0.5f,  0.5f,  0.5f ),
        CVector3( 0.5f,  0.5f,  0.5f ),

        CVector3( 0.5f, -0.5f, -0.5f ),
        CVector3( -0.5f, -0.5f, -0.5f ),
        CVector3( 0.5f,  0.5f, -0.5f ),
        CVector3( -0.5f,  0.5f, -0.5f )
    };

    SResourceData VertexData( Vertices.Data(), Vertices.SizeInBytes() );

    AABBVertexBuffer = RHICreateVertexBuffer<CVector3>( Vertices.Size(), BufferFlag_Default, EResourceState::Common, &VertexData );
    if ( !AABBVertexBuffer )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        AABBVertexBuffer->SetName( "AABB VertexBuffer" );
    }

    // Create IndexBuffer
    TStaticArray<uint16, 24> Indices =
    {
        0, 1,
        1, 3,
        3, 2,
        2, 0,
        1, 4,
        3, 6,
        6, 4,
        4, 5,
        5, 7,
        7, 6,
        0, 5,
        2, 7,
    };

    SResourceData IndexData( Indices.Data(), Indices.SizeInBytes() );

    AABBIndexBuffer = RHICreateIndexBuffer( EIndexFormat::uint16, Indices.Size(), BufferFlag_Default, EResourceState::Common, &IndexData );
    if ( !AABBIndexBuffer )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        AABBIndexBuffer->SetName( "AABB IndexBuffer" );
    }

    return true;
}

bool CRenderer::InitAA()
{
    TArray<uint8> ShaderCode;
    if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/FullscreenVS.hlsl", "Main", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    TSharedRef<CRHIVertexShader> VShader = RHICreateVertexShader( ShaderCode );
    if ( !VShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        VShader->SetName( "Fullscreen VertexShader" );
    }

    if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/PostProcessPS.hlsl", "Main", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    PostShader = RHICreatePixelShader( ShaderCode );
    if ( !PostShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        PostShader->SetName( "PostProcess PixelShader" );
    }

    SDepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc = EComparisonFunc::Always;
    DepthStencilStateInfo.DepthEnable = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState( DepthStencilStateInfo );
    if ( !DepthStencilState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName( "PostProcess DepthStencilState" );
    }

    SRasterizerStateCreateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState( RasterizerStateInfo );
    if ( !RasterizerState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName( "PostProcess RasterizerState" );
    }

    SBlendStateCreateInfo BlendStateInfo;
    BlendStateInfo.IndependentBlendEnable = false;
    BlendStateInfo.RenderTarget[0].BlendEnable = false;

    TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState( BlendStateInfo );
    if ( !BlendState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName( "PostProcess BlendState" );
    }

    SGraphicsPipelineStateCreateInfo PSOProperties;
    PSOProperties.InputLayoutState = nullptr;
    PSOProperties.BlendState = BlendState.Get();
    PSOProperties.DepthStencilState = DepthStencilState.Get();
    PSOProperties.RasterizerState = RasterizerState.Get();
    PSOProperties.ShaderState.VertexShader = VShader.Get();
    PSOProperties.ShaderState.PixelShader = PostShader.Get();
    PSOProperties.PrimitiveTopologyType = EPrimitiveTopologyType::Triangle;
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
    PSOProperties.PipelineFormats.NumRenderTargets = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat = EFormat::Unknown;

    PostPSO = RHICreateGraphicsPipelineState( PSOProperties );
    if ( !PostPSO )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        PostPSO->SetName( "PostProcess PipelineState" );
    }

    // FXAA
    SSamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Clamp;
    CreateInfo.AddressV = ESamplerMode::Clamp;
    CreateInfo.AddressW = ESamplerMode::Clamp;
    CreateInfo.Filter = ESamplerFilter::MinMagMipLinear;

    Resources.FXAASampler = RHICreateSamplerState( CreateInfo );
    if ( !Resources.FXAASampler )
    {
        return false;
    }

    if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/FXAA_PS.hlsl", "Main", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    FXAAShader = RHICreatePixelShader( ShaderCode );
    if ( !FXAAShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        FXAAShader->SetName( "FXAA PixelShader" );
    }

    PSOProperties.ShaderState.PixelShader = FXAAShader.Get();

    FXAAPSO = RHICreateGraphicsPipelineState( PSOProperties );
    if ( !FXAAPSO )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        FXAAPSO->SetName( "FXAA PipelineState" );
    }

    TArray<SShaderDefine> Defines =
    {
        SShaderDefine( "ENABLE_DEBUG", "1" )
    };

    if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/FXAA_PS.hlsl", "Main", &Defines, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    FXAADebugShader = RHICreatePixelShader( ShaderCode );
    if ( !FXAADebugShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        FXAADebugShader->SetName( "FXAA PixelShader" );
    }

    PSOProperties.ShaderState.PixelShader = FXAADebugShader.Get();

    FXAADebugPSO = RHICreateGraphicsPipelineState( PSOProperties );
    if ( !FXAADebugPSO )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        FXAADebugPSO->SetName( "FXAA Debug PipelineState" );
    }

    return true;
}

bool CRenderer::InitShadingImage()
{
    SShadingRateSupport Support;
    RHICheckShadingRateSupport( Support );

    if ( Support.Tier != EShadingRateTier::Tier2 || Support.ShadingRateImageTileSize == 0 )
    {
        return true;
    }

    uint32 Width = Resources.MainWindowViewport->GetWidth() / Support.ShadingRateImageTileSize;
    uint32 Height = Resources.MainWindowViewport->GetHeight() / Support.ShadingRateImageTileSize;
    ShadingImage = RHICreateTexture2D( EFormat::R8_Uint, Width, Height, 1, 1, TextureFlags_RWTexture, EResourceState::ShadingRateSource, nullptr );
    if ( !ShadingImage )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        ShadingImage->SetName( "Shading Rate Image" );
    }

    TArray<uint8> ShaderCode;
    if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/ShadingImage.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    ShadingRateShader = RHICreateComputeShader( ShaderCode );
    if ( !ShadingRateShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        ShadingRateShader->SetName( "ShadingRate Image Shader" );
    }

    SComputePipelineStateCreateInfo CreateInfo( ShadingRateShader.Get() );
    ShadingRatePipeline = RHICreateComputePipelineState( CreateInfo );
    if ( !ShadingRatePipeline )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        ShadingRatePipeline->SetName( "ShadingRate Image Pipeline" );
    }

    return true;
}
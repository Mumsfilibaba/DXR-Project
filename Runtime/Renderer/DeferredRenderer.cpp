#include "DeferredRenderer.h"
#include "MeshDrawCommand.h"

#include "RHI/RHIInterface.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/Mesh.h"
#include "Engine//Resources/Material.h"

#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"

#include "Renderer/Debug/GPUProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

TConsoleVariable<bool> GDrawTileDebug( false );

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

bool CDeferredRenderer::Init( SFrameResources& FrameResources )
{
    INIT_CONSOLE_VARIABLE( "r.DrawTileDebug", &GDrawTileDebug );

    if ( !CreateGBuffer( FrameResources ) )
    {
        return false;
    }

    {
        SSamplerStateCreateInfo CreateInfo;
        CreateInfo.AddressU = ESamplerMode::Clamp;
        CreateInfo.AddressV = ESamplerMode::Clamp;
        CreateInfo.AddressW = ESamplerMode::Clamp;
        CreateInfo.Filter = ESamplerFilter::MinMagMipPoint;

        FrameResources.GBufferSampler = RHICreateSamplerState( CreateInfo );
        if ( !FrameResources.GBufferSampler )
        {
            return false;
        }
    }

    TArray<uint8> ShaderCode;
    {
        TArray<SShaderDefine> Defines =
        {
            { "ENABLE_PARALLAX_MAPPING", "1" },
            { "ENABLE_NORMAL_MAPPING",   "1" },
        };

        if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/GeometryPass.hlsl", "VSMain", &Defines, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode ) )
        {
            CDebug::DebugBreak();
            return false;
        }

        BaseVertexShader = RHICreateVertexShader( ShaderCode );
        if ( !BaseVertexShader )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            BaseVertexShader->SetName( "GeometryPass VertexShader" );
        }

        if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/GeometryPass.hlsl", "PSMain", &Defines, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode ) )
        {
            CDebug::DebugBreak();
            return false;
        }

        BasePixelShader = RHICreatePixelShader( ShaderCode );
        if ( !BasePixelShader )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            BasePixelShader->SetName( "GeometryPass PixelShader" );
        }

        SDepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc = EComparisonFunc::LessEqual;
        DepthStencilStateInfo.bDepthEnable = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

        TSharedRef<CRHIDepthStencilState> GeometryDepthStencilState = RHICreateDepthStencilState( DepthStencilStateInfo );
        if ( !GeometryDepthStencilState )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            GeometryDepthStencilState->SetName( "GeometryPass DepthStencilState" );
        }

        SRasterizerStateCreateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::Back;

        TSharedRef<CRHIRasterizerState> GeometryRasterizerState = RHICreateRasterizerState( RasterizerStateInfo );
        if ( !GeometryRasterizerState )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            GeometryRasterizerState->SetName( "GeometryPass RasterizerState" );
        }

        SBlendStateCreateInfo BlendStateInfo;
        BlendStateInfo.bIndependentBlendEnable = false;
        BlendStateInfo.RenderTarget[0].bBlendEnable = false;

        TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState( BlendStateInfo );
        if ( !BlendState )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            BlendState->SetName( "GeometryPass BlendState" );
        }

        SGraphicsPipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.InputLayoutState = FrameResources.StdInputLayout.Get();
        PipelineStateInfo.BlendState = BlendState.Get();
        PipelineStateInfo.DepthStencilState = GeometryDepthStencilState.Get();
        PipelineStateInfo.RasterizerState = GeometryRasterizerState.Get();
        PipelineStateInfo.ShaderState.VertexShader = BaseVertexShader.Get();
        PipelineStateInfo.ShaderState.PixelShader = BasePixelShader.Get();
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = FrameResources.DepthBufferFormat;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[1] = FrameResources.NormalFormat;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[2] = EFormat::R8G8B8A8_Unorm;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[3] = FrameResources.ViewNormalFormat;
        PipelineStateInfo.PipelineFormats.NumRenderTargets = 4;

        PipelineState = RHICreateGraphicsPipelineState( PipelineStateInfo );
        if ( !PipelineState )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            PipelineState->SetName( "GeometryPass PipelineState" );
        }
    }

    // PrePass
    {
        if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/PrePass.hlsl", "Main", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode ) )
        {
            CDebug::DebugBreak();
            return false;
        }

        PrePassVertexShader = RHICreateVertexShader( ShaderCode );
        if ( !PrePassVertexShader )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            PrePassVertexShader->SetName( "PrePass VertexShader" );
        }

        SDepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc = EComparisonFunc::Less;
        DepthStencilStateInfo.bDepthEnable = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

        TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState( DepthStencilStateInfo );
        if ( !DepthStencilState )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            DepthStencilState->SetName( "Prepass DepthStencilState" );
        }

        SRasterizerStateCreateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::Back;

        TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState( RasterizerStateInfo );
        if ( !RasterizerState )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            RasterizerState->SetName( "Prepass RasterizerState" );
        }

        SBlendStateCreateInfo BlendStateInfo;
        BlendStateInfo.bIndependentBlendEnable = false;
        BlendStateInfo.RenderTarget[0].bBlendEnable = false;

        TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState( BlendStateInfo );
        if ( !BlendState )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            BlendState->SetName( "Prepass BlendState" );
        }

        SGraphicsPipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.InputLayoutState = FrameResources.StdInputLayout.Get();
        PipelineStateInfo.BlendState = BlendState.Get();
        PipelineStateInfo.DepthStencilState = DepthStencilState.Get();
        PipelineStateInfo.RasterizerState = RasterizerState.Get();
        PipelineStateInfo.ShaderState.VertexShader = PrePassVertexShader.Get();
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = FrameResources.DepthBufferFormat;

        PrePassPipelineState = RHICreateGraphicsPipelineState( PipelineStateInfo );
        if ( !PrePassPipelineState )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            PrePassPipelineState->SetName( "PrePass PipelineState" );
        }
    }

    constexpr uint32  LUTSize = 512;
    constexpr EFormat LUTFormat = EFormat::R16G16_Float;
    if ( !RHIUAVSupportsFormat( LUTFormat ) )
    {
        LOG_ERROR( "[CRenderer]: R16G16_Float is not supported for UAVs" );

        CDebug::DebugBreak();
        return false;
    }

    TSharedRef<CRHITexture2D> StagingTexture = RHICreateTexture2D( LUTFormat, LUTSize, LUTSize, 1, 1, TextureFlag_UAV, EResourceState::Common, nullptr );
    if ( !StagingTexture )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        StagingTexture->SetName( "Staging IntegrationLUT" );
    }

    FrameResources.IntegrationLUT = RHICreateTexture2D( LUTFormat, LUTSize, LUTSize, 1, 1, TextureFlag_SRV, EResourceState::Common, nullptr );
    if ( !FrameResources.IntegrationLUT )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.IntegrationLUT->SetName( "IntegrationLUT" );
    }

    SSamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Clamp;
    CreateInfo.AddressV = ESamplerMode::Clamp;
    CreateInfo.AddressW = ESamplerMode::Clamp;
    CreateInfo.Filter = ESamplerFilter::MinMagMipPoint;

    FrameResources.IntegrationLUTSampler = RHICreateSamplerState( CreateInfo );
    if ( !FrameResources.IntegrationLUTSampler )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.IntegrationLUTSampler->SetName( "IntegrationLUT Sampler" );
    }

    if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/BRDFIntegationGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    TSharedRef<CRHIComputeShader> CShader = RHICreateComputeShader( ShaderCode );
    if ( !CShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        CShader->SetName( "BRDFIntegationGen ComputeShader" );
    }

    {
        SComputePipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.Shader = CShader.Get();

        TSharedRef<CRHIComputePipelineState> BRDF_PipelineState = RHICreateComputePipelineState( PipelineStateInfo );
        if ( !BRDF_PipelineState )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            BRDF_PipelineState->SetName( "BRDFIntegationGen PipelineState" );
        }

        CRHICommandList CmdList;

        CmdList.TransitionTexture( StagingTexture.Get(), EResourceState::Common, EResourceState::UnorderedAccess );

        CmdList.SetComputePipelineState( BRDF_PipelineState.Get() );

        CRHIUnorderedAccessView* StagingUAV = StagingTexture->GetUnorderedAccessView();
        CmdList.SetUnorderedAccessView( CShader.Get(), StagingUAV, 0 );

        constexpr uint32 ThreadCount = 16;
        const uint32 DispatchWidth = NMath::DivideByMultiple( LUTSize, ThreadCount );
        const uint32 DispatchHeight = NMath::DivideByMultiple( LUTSize, ThreadCount );
        CmdList.Dispatch( DispatchWidth, DispatchHeight, 1 );

        CmdList.UnorderedAccessTextureBarrier( StagingTexture.Get() );

        CmdList.TransitionTexture( StagingTexture.Get(), EResourceState::UnorderedAccess, EResourceState::CopySource );
        CmdList.TransitionTexture( FrameResources.IntegrationLUT.Get(), EResourceState::Common, EResourceState::CopyDest );

        CmdList.CopyTexture( FrameResources.IntegrationLUT.Get(), StagingTexture.Get() );

        CmdList.TransitionTexture( FrameResources.IntegrationLUT.Get(), EResourceState::CopyDest, EResourceState::PixelShaderResource );

        CRHICommandQueue::Get().ExecuteCommandList( CmdList );
    }

    {
        if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/DeferredLightPass.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode ) )
        {
            CDebug::DebugBreak();
            return false;
        }

        TiledLightShader = RHICreateComputeShader( ShaderCode );
        if ( !TiledLightShader )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            TiledLightShader->SetName( "DeferredLightPass Shader" );
        }

        SComputePipelineStateCreateInfo DeferredLightPassCreateInfo;
        DeferredLightPassCreateInfo.Shader = TiledLightShader.Get();

        TiledLightPassPSO = RHICreateComputePipelineState( DeferredLightPassCreateInfo );
        if ( !TiledLightPassPSO )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            TiledLightPassPSO->SetName( "DeferredLightPass PipelineState" );
        }
    }

    {
        TArray<SShaderDefine> Defines =
        {
            SShaderDefine( "DRAW_TILE_DEBUG", "1" )
        };

        if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/DeferredLightPass.hlsl", "Main", &Defines, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode ) )
        {
            CDebug::DebugBreak();
            return false;
        }

        TiledLightDebugShader = RHICreateComputeShader( ShaderCode );
        if ( !TiledLightDebugShader )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            TiledLightDebugShader->SetName( "DeferredLightPass Debug Shader" );
        }

        SComputePipelineStateCreateInfo DeferredLightPassCreateInfo;
        DeferredLightPassCreateInfo.Shader = TiledLightDebugShader.Get();

        TiledLightPassPSODebug = RHICreateComputePipelineState( DeferredLightPassCreateInfo );
        if ( !TiledLightPassPSODebug )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            TiledLightPassPSODebug->SetName( "DeferredLightPass PipelineState Debug" );
        }
    }

    {
        if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/DepthReduction.hlsl", "ReductionMainInital", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode ) )
        {
            CDebug::DebugBreak();
            return false;
        }

        ReduceDepthInitalShader = RHICreateComputeShader( ShaderCode );
        if ( !ReduceDepthInitalShader )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            ReduceDepthInitalShader->SetName( "DepthReduction Inital ComputeShader" );
        }

        SComputePipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.Shader = ReduceDepthInitalShader.Get();

        ReduceDepthInitalPSO = RHICreateComputePipelineState( PipelineStateInfo );
        if ( !ReduceDepthInitalPSO )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            ReduceDepthInitalPSO->SetName( "DepthReduction Initial PipelineState" );
        }
    }

    {
        if ( !CRHIShaderCompiler::CompileFromFile( "../Runtime/Shaders/DepthReduction.hlsl", "ReductionMain", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode ) )
        {
            CDebug::DebugBreak();
            return false;
        }

        ReduceDepthShader = RHICreateComputeShader( ShaderCode );
        if ( !ReduceDepthShader )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            ReduceDepthShader->SetName( "DepthReduction ComputeShader" );
        }

        SComputePipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.Shader = ReduceDepthShader.Get();

        ReduceDepthPSO = RHICreateComputePipelineState( PipelineStateInfo );
        if ( !ReduceDepthPSO )
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            ReduceDepthPSO->SetName( "DepthReduction PipelineState" );
        }
    }

    return true;
}

void CDeferredRenderer::Release()
{
    PrePassPipelineState.Reset();
    PrePassVertexShader.Reset();

    PipelineState.Reset();
    BaseVertexShader.Reset();
    BasePixelShader.Reset();

    TiledLightPassPSO.Reset();
    TiledLightPassPSODebug.Reset();
    TiledLightShader.Reset();
    TiledLightDebugShader.Reset();

    ReduceDepthInitalPSO.Reset();
    ReduceDepthInitalShader.Reset();

    ReduceDepthPSO.Reset();
    ReduceDepthShader.Reset();
}

void CDeferredRenderer::RenderPrePass( CRHICommandList& CmdList, SFrameResources& FrameResources, const CScene& Scene )
{
    const float RenderWidth = float( FrameResources.MainWindowViewport->GetWidth() );
    const float RenderHeight = float( FrameResources.MainWindowViewport->GetHeight() );

    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "Begin PrePass" );

    {
        TRACE_SCOPE( "PrePass" );

        GPU_TRACE_SCOPE( CmdList, "Pre Pass" );

        CmdList.SetPrimitiveTopology( EPrimitiveTopology::TriangleList );

        CmdList.SetViewport( RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f );
        CmdList.SetScissorRect( RenderWidth, RenderHeight, 0, 0 );

        struct SPerObject
        {
            CMatrix4 Matrix;
        } PerObjectBuffer;

        CmdList.SetRenderTargets( nullptr, 0, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView() );

        CmdList.SetGraphicsPipelineState( PrePassPipelineState.Get() );

        CmdList.SetConstantBuffer( PrePassVertexShader.Get(), FrameResources.CameraBuffer.Get(), 0 );

        for ( const SMeshDrawCommand& Command : FrameResources.DeferredVisibleCommands )
        {
            if ( Command.Material->ShouldRenderInPrePass() )
            {
                CmdList.SetVertexBuffers( &Command.VertexBuffer, 1, 0 );
                CmdList.SetIndexBuffer( Command.IndexBuffer );

                PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

                CmdList.Set32BitShaderConstants( PrePassVertexShader.Get(), &PerObjectBuffer, 16 );

                CmdList.DrawIndexedInstanced( Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0 );
            }
        }
    }

    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "End PrePass" );

    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "Begin Depth Reduction" );

    {
        TRACE_SCOPE( "Depth Reduction" );

        GPU_TRACE_SCOPE( CmdList, "Depth Reduction" );

        struct ReductionConstants
        {
            CMatrix4 CamProjection;
            float NearPlane;
            float FarPlane;
        } ReductionConstants;

        ReductionConstants.CamProjection = Scene.GetCamera()->GetProjectionMatrix();
        ReductionConstants.NearPlane = Scene.GetCamera()->GetNearPlane();
        ReductionConstants.FarPlane = Scene.GetCamera()->GetFarPlane();

        // Perform the first reduction
        CmdList.TransitionTexture( FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceState::DepthWrite, EResourceState::NonPixelShaderResource );
        CmdList.TransitionTexture( FrameResources.ReducedDepthBuffer[0].Get(), EResourceState::NonPixelShaderResource, EResourceState::UnorderedAccess );
        CmdList.TransitionTexture( FrameResources.ReducedDepthBuffer[1].Get(), EResourceState::NonPixelShaderResource, EResourceState::UnorderedAccess );

        CmdList.SetComputePipelineState( ReduceDepthInitalPSO.Get() );

        CmdList.SetShaderResourceView( ReduceDepthInitalShader.Get(), FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(), 0 );
        CmdList.SetUnorderedAccessView( ReduceDepthInitalShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0 );

        CmdList.Set32BitShaderConstants( ReduceDepthInitalShader.Get(), &ReductionConstants, NMath::BytesToNum32BitConstants( sizeof( ReductionConstants ) ) );

        uint32 ThreadsX = FrameResources.ReducedDepthBuffer[0]->GetWidth();
        uint32 ThreadsY = FrameResources.ReducedDepthBuffer[0]->GetHeight();
        CmdList.Dispatch( ThreadsX, ThreadsY, 1 );

        CmdList.TransitionTexture( FrameResources.ReducedDepthBuffer[0].Get(), EResourceState::UnorderedAccess, EResourceState::NonPixelShaderResource );
        CmdList.TransitionTexture( FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceState::NonPixelShaderResource, EResourceState::DepthWrite );

        // Perform the other reductions
        CmdList.SetComputePipelineState( ReduceDepthPSO.Get() );

        CmdList.SetShaderResourceView( ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0 );
        CmdList.SetUnorderedAccessView( ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetUnorderedAccessView(), 0 );

        ThreadsX = NMath::DivideByMultiple( ThreadsX, 16 );
        ThreadsY = NMath::DivideByMultiple( ThreadsY, 16 );
        CmdList.Dispatch( ThreadsX, ThreadsY, 1 );

        CmdList.TransitionTexture( FrameResources.ReducedDepthBuffer[0].Get(), EResourceState::NonPixelShaderResource, EResourceState::UnorderedAccess );
        CmdList.TransitionTexture( FrameResources.ReducedDepthBuffer[1].Get(), EResourceState::UnorderedAccess, EResourceState::NonPixelShaderResource );

        CmdList.SetShaderResourceView( ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetShaderResourceView(), 0 );
        CmdList.SetUnorderedAccessView( ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0 );

        ThreadsX = NMath::DivideByMultiple( ThreadsX, 16 );
        ThreadsY = NMath::DivideByMultiple( ThreadsY, 16 );
        CmdList.Dispatch( ThreadsX, ThreadsY, 1 );

        CmdList.TransitionTexture( FrameResources.ReducedDepthBuffer[0].Get(), EResourceState::UnorderedAccess, EResourceState::NonPixelShaderResource );
    }

    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "End Depth Reduction" );
}

void CDeferredRenderer::RenderBasePass( CRHICommandList& CmdList, const SFrameResources& FrameResources )
{
    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "Begin GeometryPass" );

    TRACE_SCOPE( "GeometryPass" );

    GPU_TRACE_SCOPE( CmdList, "Base Pass" );

    const float RenderWidth = float( FrameResources.MainWindowViewport->GetWidth() );
    const float RenderHeight = float( FrameResources.MainWindowViewport->GetHeight() );

    CmdList.SetPrimitiveTopology( EPrimitiveTopology::TriangleList );

    CmdList.SetViewport( RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f );
    CmdList.SetScissorRect( RenderWidth, RenderHeight, 0, 0 );

    CRHIRenderTargetView* RenderTargets[] =
    {
        FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetRenderTargetView(),
        FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->GetRenderTargetView(),
        FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetRenderTargetView(),
        FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetRenderTargetView(),
    };
    CmdList.SetRenderTargets( RenderTargets, 4, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView() );

    // Setup Pipeline
    CmdList.SetGraphicsPipelineState( PipelineState.Get() );

    struct STransformBuffer
    {
        CMatrix4 Transform;
        CMatrix4 TransformInv;
    } TransformPerObject;

    for ( const SMeshDrawCommand& Command : FrameResources.DeferredVisibleCommands )
    {
        CmdList.SetVertexBuffers( &Command.VertexBuffer, 1, 0 );
        CmdList.SetIndexBuffer( Command.IndexBuffer );

        if ( Command.Material->IsBufferDirty() )
        {
            Command.Material->BuildBuffer( CmdList );
        }

        CmdList.SetConstantBuffer( BaseVertexShader.Get(), FrameResources.CameraBuffer.Get(), 0 );

        CRHIConstantBuffer* MaterialBuffer = Command.Material->GetMaterialBuffer();
        CmdList.SetConstantBuffer( BasePixelShader.Get(), MaterialBuffer, 0 );

        TransformPerObject.Transform = Command.CurrentActor->GetTransform().GetMatrix();
        TransformPerObject.TransformInv = Command.CurrentActor->GetTransform().GetMatrixInverse();

        CRHIShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
        CmdList.SetShaderResourceView( BasePixelShader.Get(), ShaderResourceViews[0], 0 );
        CmdList.SetShaderResourceView( BasePixelShader.Get(), ShaderResourceViews[1], 1 );
        CmdList.SetShaderResourceView( BasePixelShader.Get(), ShaderResourceViews[2], 2 );
        CmdList.SetShaderResourceView( BasePixelShader.Get(), ShaderResourceViews[3], 3 );
        CmdList.SetShaderResourceView( BasePixelShader.Get(), ShaderResourceViews[4], 4 );
        CmdList.SetShaderResourceView( BasePixelShader.Get(), ShaderResourceViews[5], 5 );
        CmdList.SetShaderResourceView( BasePixelShader.Get(), ShaderResourceViews[6], 6 );

        CRHISamplerState* Sampler = Command.Material->GetMaterialSampler();
        CmdList.SetSamplerState( BasePixelShader.Get(), Sampler, 0 );

        CmdList.Set32BitShaderConstants( BaseVertexShader.Get(), &TransformPerObject, 32 );

        CmdList.DrawIndexedInstanced( Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0 );
    }

    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "End GeometryPass" );
}

void CDeferredRenderer::RenderDeferredTiledLightPass( CRHICommandList& CmdList, const SFrameResources& FrameResources, const SLightSetup& LightSetup )
{
    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "Begin LightPass" );

    TRACE_SCOPE( "LightPass" );

    GPU_TRACE_SCOPE( CmdList, "Light Pass" );

    CRHIComputeShader* LightPassShader = nullptr;
    if ( GDrawTileDebug.GetBool() )
    {
        LightPassShader = TiledLightDebugShader.Get();
        CmdList.SetComputePipelineState( TiledLightPassPSODebug.Get() );
    }
    else
    {
        LightPassShader = TiledLightShader.Get();
        CmdList.SetComputePipelineState( TiledLightPassPSO.Get() );
    }

    CmdList.SetShaderResourceView( LightPassShader, FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetShaderResourceView(), 0 );
    CmdList.SetShaderResourceView( LightPassShader, FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView(), 1 );
    CmdList.SetShaderResourceView( LightPassShader, FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView(), 2 );
    CmdList.SetShaderResourceView( LightPassShader, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(), 3 );
    //CmdList.SetShaderResourceView(LightPassShader, nullptr, 4); // Reflection
    CmdList.SetShaderResourceView( LightPassShader, LightSetup.IrradianceMap->GetShaderResourceView(), 4 );
    CmdList.SetShaderResourceView( LightPassShader, LightSetup.SpecularIrradianceMap->GetShaderResourceView(), 5 );
    CmdList.SetShaderResourceView( LightPassShader, FrameResources.IntegrationLUT->GetShaderResourceView(), 6 );
    CmdList.SetShaderResourceView( LightPassShader, LightSetup.DirectionalShadowMask->GetShaderResourceView(), 7 );
    CmdList.SetShaderResourceView( LightPassShader, LightSetup.PointLightShadowMaps->GetShaderResourceView(), 8 );
    CmdList.SetShaderResourceView( LightPassShader, FrameResources.SSAOBuffer->GetShaderResourceView(), 9 );

    //CmdList.SetShaderResourceView(LightPassShader, LightSetup.CascadeMatrixBufferSRV.Get(), 13);
    //CmdList.SetShaderResourceView(LightPassShader, LightSetup.CascadeSplitsBufferSRV.Get(), 14);

    CmdList.SetConstantBuffer( LightPassShader, FrameResources.CameraBuffer.Get(), 0 );
    CmdList.SetConstantBuffer( LightPassShader, LightSetup.PointLightsBuffer.Get(), 1 );
    CmdList.SetConstantBuffer( LightPassShader, LightSetup.PointLightsPosRadBuffer.Get(), 2 );
    CmdList.SetConstantBuffer( LightPassShader, LightSetup.ShadowCastingPointLightsBuffer.Get(), 3 );
    CmdList.SetConstantBuffer( LightPassShader, LightSetup.ShadowCastingPointLightsPosRadBuffer.Get(), 4 );
    CmdList.SetConstantBuffer( LightPassShader, LightSetup.DirectionalLightsBuffer.Get(), 5 );

    CmdList.SetSamplerState( LightPassShader, FrameResources.IntegrationLUTSampler.Get(), 0 );
    CmdList.SetSamplerState( LightPassShader, FrameResources.IrradianceSampler.Get(), 1 );
    CmdList.SetSamplerState( LightPassShader, FrameResources.PointLightShadowSampler.Get(), 2 );
    //CmdList.SetSamplerState(LightPassShader, FrameResources.DirectionalLightShadowSampler.Get(), 3);

    CRHIUnorderedAccessView* FinalTargetUAV = FrameResources.FinalTarget->GetUnorderedAccessView();
    CmdList.SetUnorderedAccessView( LightPassShader, FinalTargetUAV, 0 );

    struct LightPassSettings
    {
        int32 NumPointLights;
        int32 NumShadowCastingPointLights;
        int32 NumSkyLightMips;
        int32 ScreenWidth;
        int32 ScreenHeight;
    } Settings;

    Settings.NumShadowCastingPointLights = LightSetup.ShadowCastingPointLightsData.Size();
    Settings.NumPointLights = LightSetup.PointLightsData.Size();
    Settings.NumSkyLightMips = LightSetup.SpecularIrradianceMap->GetNumMips();
    Settings.ScreenWidth = FrameResources.FinalTarget->GetWidth();
    Settings.ScreenHeight = FrameResources.FinalTarget->GetHeight();

    CmdList.Set32BitShaderConstants( LightPassShader, &Settings, 5 );

    const CIntVector3 ThreadsXYZ = LightPassShader->GetThreadGroupXYZ();
    const uint32 WorkGroupWidth = NMath::DivideByMultiple<uint32>( Settings.ScreenWidth, ThreadsXYZ.x );
    const uint32 WorkGroupHeight = NMath::DivideByMultiple<uint32>( Settings.ScreenHeight, ThreadsXYZ.y );
    CmdList.Dispatch( WorkGroupWidth, WorkGroupHeight, 1 );

    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "End LightPass" );
}

bool CDeferredRenderer::ResizeResources( SFrameResources& FrameResources )
{
    return CreateGBuffer( FrameResources );
}

bool CDeferredRenderer::CreateGBuffer( SFrameResources& FrameResources )
{
    const uint32 Width = FrameResources.MainWindowViewport->GetWidth();
    const uint32 Height = FrameResources.MainWindowViewport->GetHeight();
    const uint32 Usage = TextureFlags_RenderTarget;

    // Albedo
    FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX] = RHICreateTexture2D( FrameResources.AlbedoFormat, Width, Height, 1, 1, Usage, EResourceState::Common, nullptr );
    if ( FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX] )
    {
        FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->SetName( "GBuffer Albedo" );
    }
    else
    {
        return false;
    }

    // Normal
    FrameResources.GBuffer[GBUFFER_NORMAL_INDEX] = RHICreateTexture2D( FrameResources.NormalFormat, Width, Height, 1, 1, Usage, EResourceState::Common, nullptr );
    if ( FrameResources.GBuffer[GBUFFER_NORMAL_INDEX] )
    {
        FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->SetName( "GBuffer Normal" );
    }
    else
    {
        return false;
    }

    // Material Properties
    FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX] = RHICreateTexture2D( FrameResources.MaterialFormat, Width, Height, 1, 1, Usage, EResourceState::Common, nullptr );
    if ( FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX] )
    {
        FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->SetName( "GBuffer Material" );
    }
    else
    {
        return false;
    }

    // DepthStencil
    const SClearValue DepthClearValue( FrameResources.DepthBufferFormat, 1.0f, 0 );

    FrameResources.GBuffer[GBUFFER_DEPTH_INDEX] = RHICreateTexture2D( FrameResources.DepthBufferFormat, Width, Height, 1, 1, TextureFlags_ShadowMap, EResourceState::Common, nullptr, DepthClearValue );
    if ( FrameResources.GBuffer[GBUFFER_DEPTH_INDEX] )
    {
        FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->SetName( "GBuffer DepthStencil" );
    }
    else
    {
        return false;
    }

    constexpr uint32 Alignment = 16;
    const uint32 ReducedWidth = NMath::DivideByMultiple( Width, Alignment );
    const uint32 ReducedHeight = NMath::DivideByMultiple( Height, Alignment );

    for ( uint32 i = 0; i < 2; i++ )
    {
        FrameResources.ReducedDepthBuffer[i] = RHICreateTexture2D( EFormat::R32G32_Float, ReducedWidth, ReducedHeight, 1, 1, TextureFlags_RWTexture, EResourceState::NonPixelShaderResource, nullptr );
        if ( FrameResources.ReducedDepthBuffer[i] )
        {
            FrameResources.ReducedDepthBuffer[i]->SetName( "Reduced DepthStencil[" + ToString( i ) + "]" );
        }
        else
        {
            return false;
        }
    }

    // View Normal
    FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX] = RHICreateTexture2D( FrameResources.ViewNormalFormat, Width, Height, 1, 1, Usage, EResourceState::Common, nullptr );
    if ( FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX] )
    {
        FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->SetName( "GBuffer ViewNormal" );
    }
    else
    {
        return false;
    }

    // Final Image
    FrameResources.FinalTarget = RHICreateTexture2D( FrameResources.FinalTargetFormat, Width, Height, 1, 1, Usage | TextureFlag_UAV, EResourceState::Common, nullptr );
    if ( FrameResources.FinalTarget )
    {
        FrameResources.FinalTarget->SetName( "Final Target" );
    }
    else
    {
        return false;
    }

    return true;
}

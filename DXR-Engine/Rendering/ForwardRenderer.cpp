#include "ForwardRenderer.h"

#include "CoreRHI/RHIModule.h"
#include "CoreRHI/RHIShaderCompiler.h"

#include "Rendering/MeshDrawCommand.h"
#include "Rendering/Resources/Mesh.h"
#include "Rendering/Resources/Material.h"

#include "Scene/Actor.h"

#include "Core/Debug/Profiler.h"

bool CForwardRenderer::Init( SFrameResources& FrameResources )
{
    TArray<SShaderDefine> Defines =
    {
        { "ENABLE_PARALLAX_MAPPING", "1" },
        { "ENABLE_NORMAL_MAPPING",   "1" },
    };

    TArray<uint8> ShaderCode;
    if ( !CRHIShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/ForwardPass.hlsl", "VSMain", &Defines, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    VShader = RHICreateVertexShader( ShaderCode );
    if ( !VShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        VShader->SetName( "ForwardPass VertexShader" );
    }

    if ( !CRHIShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/ForwardPass.hlsl", "PSMain", &Defines, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    PShader = RHICreatePixelShader( ShaderCode );
    if ( !PShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        PShader->SetName( "ForwardPass PixelShader" );
    }

    SDepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc = EComparisonFunc::LessEqual;
    DepthStencilStateInfo.DepthEnable = true;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

    TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState( DepthStencilStateInfo );
    if ( !DepthStencilState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName( "ForwardPass DepthStencilState" );
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
        RasterizerState->SetName( "ForwardPass RasterizerState" );
    }

    SBlendStateCreateInfo BlendStateInfo;
    BlendStateInfo.IndependentBlendEnable = false;
    BlendStateInfo.RenderTarget[0].BlendEnable = true;

    TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState( BlendStateInfo );
    if ( !BlendState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName( "ForwardPass BlendState" );
    }

    SGraphicsPipelineStateCreateInfo PSOProperties;
    PSOProperties.ShaderState.VertexShader = VShader.Get();
    PSOProperties.ShaderState.PixelShader = PShader.Get();
    PSOProperties.InputLayoutState = FrameResources.StdInputLayout.Get();
    PSOProperties.DepthStencilState = DepthStencilState.Get();
    PSOProperties.BlendState = BlendState.Get();
    PSOProperties.RasterizerState = RasterizerState.Get();
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = FrameResources.FinalTargetFormat;
    PSOProperties.PipelineFormats.NumRenderTargets = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat = FrameResources.DepthBufferFormat;
    PSOProperties.PrimitiveTopologyType = EPrimitiveTopologyType::Triangle;

    PipelineState = RHICreateGraphicsPipelineState( PSOProperties );
    if ( !PipelineState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName( "Forward PipelineState" );
    }

    return true;
}

void CForwardRenderer::Release()
{
    PipelineState.Reset();
    VShader.Reset();
    PShader.Reset();
}

void CForwardRenderer::Render( CRHICommandList& CmdList, const SFrameResources& FrameResources, const SLightSetup& LightSetup )
{
    // Forward Pass
    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "Begin ForwardPass" );

    TRACE_SCOPE( "ForwardPass" );

    CmdList.TransitionTexture( LightSetup.ShadowMapCascades[0].Get(), EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource );

    const float RenderWidth = float( FrameResources.FinalTarget->GetWidth() );
    const float RenderHeight = float( FrameResources.FinalTarget->GetHeight() );

    CmdList.SetPrimitiveTopology( EPrimitiveTopology::TriangleList );

    CmdList.SetViewport( RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f );
    CmdList.SetScissorRect( RenderWidth, RenderHeight, 0, 0 );

    CRHIRenderTargetView* FinalTargetRTV = FrameResources.FinalTarget->GetRenderTargetView();
    CmdList.SetRenderTargets( &FinalTargetRTV, 1, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView() );

    CmdList.SetConstantBuffer( PShader.Get(), FrameResources.CameraBuffer.Get(), 0 );
    // TODO: Fix pointlight count in shader
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsBuffer.Get(), 1);
    //CmdList.SetConstantBuffer(PShader.Get(), LightSetup.PointLightsPosRadBuffer.Get(), 2);
    CmdList.SetConstantBuffer( PShader.Get(), LightSetup.ShadowCastingPointLightsBuffer.Get(), 1 );
    CmdList.SetConstantBuffer( PShader.Get(), LightSetup.ShadowCastingPointLightsPosRadBuffer.Get(), 2 );
    CmdList.SetConstantBuffer( PShader.Get(), LightSetup.DirectionalLightsBuffer.Get(), 3 );

    CmdList.SetShaderResourceView( PShader.Get(), LightSetup.IrradianceMap->GetShaderResourceView(), 0 );
    CmdList.SetShaderResourceView( PShader.Get(), LightSetup.SpecularIrradianceMap->GetShaderResourceView(), 1 );
    CmdList.SetShaderResourceView( PShader.Get(), FrameResources.IntegrationLUT->GetShaderResourceView(), 2 );
    //TODO: Fix Dirlight shadows
    //CmdList.SetShaderResourceView(PShader.Get(), LightSetup.ShadowMapCascades[0]->GetShaderResourceView(), 3);
    CmdList.SetShaderResourceView( PShader.Get(), LightSetup.PointLightShadowMaps->GetShaderResourceView(), 3 );

    CmdList.SetSamplerState( PShader.Get(), FrameResources.IntegrationLUTSampler.Get(), 1 );
    CmdList.SetSamplerState( PShader.Get(), FrameResources.IrradianceSampler.Get(), 2 );
    CmdList.SetSamplerState( PShader.Get(), FrameResources.PointLightShadowSampler.Get(), 3 );
    //CmdList.SetSamplerState(PShader.Get(), FrameResources.DirectionalLightShadowSampler.Get(), 4);

    struct STransformBuffer
    {
        CMatrix4 Transform;
        CMatrix4 TransformInv;
    } TransformPerObject;

    CmdList.SetGraphicsPipelineState( PipelineState.Get() );
    for ( const SMeshDrawCommand& Command : FrameResources.ForwardVisibleCommands )
    {
        CmdList.SetVertexBuffers( &Command.VertexBuffer, 1, 0 );
        CmdList.SetIndexBuffer( Command.IndexBuffer );

        if ( Command.Material->IsBufferDirty() )
        {
            Command.Material->BuildBuffer( CmdList );
        }

        CRHIConstantBuffer* ConstantBuffer = Command.Material->GetMaterialBuffer();
        CmdList.SetConstantBuffer( PShader.Get(), ConstantBuffer, 4 );

        CRHIShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
        CmdList.SetShaderResourceView( PShader.Get(), ShaderResourceViews[0], 4 );
        CmdList.SetShaderResourceView( PShader.Get(), ShaderResourceViews[1], 5 );
        CmdList.SetShaderResourceView( PShader.Get(), ShaderResourceViews[2], 6 );
        CmdList.SetShaderResourceView( PShader.Get(), ShaderResourceViews[3], 7 );
        CmdList.SetShaderResourceView( PShader.Get(), ShaderResourceViews[4], 8 );
        CmdList.SetShaderResourceView( PShader.Get(), ShaderResourceViews[5], 9 );
        CmdList.SetShaderResourceView( PShader.Get(), ShaderResourceViews[6], 10 );

        CRHISamplerState* SamplerState = Command.Material->GetMaterialSampler();
        CmdList.SetSamplerState( PShader.Get(), SamplerState, 0 );

        TransformPerObject.Transform = Command.CurrentActor->GetTransform().GetMatrix();
        TransformPerObject.TransformInv = Command.CurrentActor->GetTransform().GetMatrixInverse();

        CmdList.Set32BitShaderConstants( VShader.Get(), &TransformPerObject, 32 );

        CmdList.DrawIndexedInstanced( Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0 );
    }

    CmdList.TransitionTexture( LightSetup.ShadowMapCascades[0].Get(), EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource );

    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "End ForwardPass" );
}

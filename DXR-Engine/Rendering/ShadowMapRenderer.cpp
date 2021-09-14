#include "ShadowMapRenderer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Rendering/Resources/Mesh.h"
#include "Rendering/MeshDrawCommand.h"

#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"

#include "Core/Math/Frustum.h"
#include "Core/Debug/Profiler.h"
#include "Core/Debug/Console/Console.h"

bool ShadowMapRenderer::Init( LightSetup& LightSetup, FrameResources& FrameResources )
{
    if ( !CreateShadowMaps( LightSetup, FrameResources ) )
    {
        return false;
    }
	
	UNREFERENCED_VARIABLE(UpdateDirLight);
	UNREFERENCED_VARIABLE(DirLightFrame);
	UNREFERENCED_VARIABLE(PointLightFrame);
	
    TArray<uint8> ShaderCode;

    // Point Shadow Maps
    {
        PerShadowMapBuffer = CreateConstantBuffer<SPerShadowMap>( BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr );
        if ( !PerShadowMapBuffer )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PerShadowMapBuffer->SetName( "Per ShadowMap Buffer" );
        }

        if ( !ShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/ShadowMap.hlsl", "Point_VSMain", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode ) )
        {
            Debug::DebugBreak();
            return false;
        }

        PointLightVertexShader = CreateVertexShader( ShaderCode );
        if ( !PointLightVertexShader )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PointLightVertexShader->SetName( "Point ShadowMap VertexShader" );
        }

        if ( !ShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/ShadowMap.hlsl", "Point_PSMain", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode ) )
        {
            Debug::DebugBreak();
            return false;
        }

        PointLightPixelShader = CreatePixelShader( ShaderCode );
        if ( !PointLightPixelShader )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PointLightPixelShader->SetName( "Point ShadowMap PixelShader" );
        }

        DepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc = EComparisonFunc::LessEqual;
        DepthStencilStateInfo.DepthEnable = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

        TSharedRef<DepthStencilState> DepthStencilState = CreateDepthStencilState( DepthStencilStateInfo );
        if ( !DepthStencilState )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            DepthStencilState->SetName( "Shadow DepthStencilState" );
        }

        RasterizerStateCreateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::Back;

        TSharedRef<RasterizerState> RasterizerState = CreateRasterizerState( RasterizerStateInfo );
        if ( !RasterizerState )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            RasterizerState->SetName( "Shadow RasterizerState" );
        }

        BlendStateCreateInfo BlendStateInfo;

        TSharedRef<BlendState> BlendState = CreateBlendState( BlendStateInfo );
        if ( !BlendState )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            BlendState->SetName( "Shadow BlendState" );
        }

        GraphicsPipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.BlendState = BlendState.Get();
        PipelineStateInfo.DepthStencilState = DepthStencilState.Get();
        PipelineStateInfo.IBStripCutValue = EIndexBufferStripCutValue::Disabled;
        PipelineStateInfo.InputLayoutState = FrameResources.StdInputLayout.Get();
        PipelineStateInfo.PrimitiveTopologyType = EPrimitiveTopologyType::Triangle;
        PipelineStateInfo.RasterizerState = RasterizerState.Get();
        PipelineStateInfo.SampleCount = 1;
        PipelineStateInfo.SampleQuality = 0;
        PipelineStateInfo.SampleMask = 0xffffffff;
        PipelineStateInfo.ShaderState.VertexShader = PointLightVertexShader.Get();
        PipelineStateInfo.ShaderState.PixelShader = PointLightPixelShader.Get();
        PipelineStateInfo.PipelineFormats.NumRenderTargets = 0;
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = LightSetup.ShadowMapFormat;

        PointLightPipelineState = CreateGraphicsPipelineState( PipelineStateInfo );
        if ( !PointLightPipelineState )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PointLightPipelineState->SetName( "Point ShadowMap PipelineState" );
        }
    }

    // Cascaded shadowmap
    {
        PerCascadeBuffer = CreateConstantBuffer<SPerCascade>( EBufferFlags::BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr );
        if ( !PerCascadeBuffer )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PerCascadeBuffer->SetName( "Per Cascade Buffer" );
        }

        if ( !ShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/ShadowMap.hlsl", "Cascade_VSMain", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode ) )
        {
            Debug::DebugBreak();
            return false;
        }

        DirectionalLightShader = CreateVertexShader( ShaderCode );
        if ( !DirectionalLightShader )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            DirectionalLightShader->SetName( "ShadowMap VertexShader" );
        }

        DepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc = EComparisonFunc::LessEqual;
        DepthStencilStateInfo.DepthEnable = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

        TSharedRef<DepthStencilState> DepthStencilState = CreateDepthStencilState( DepthStencilStateInfo );
        if ( !DepthStencilState )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            DepthStencilState->SetName( "Shadow DepthStencilState" );
        }

        RasterizerStateCreateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::Back;

        TSharedRef<RasterizerState> RasterizerState = CreateRasterizerState( RasterizerStateInfo );
        if ( !RasterizerState )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            RasterizerState->SetName( "Shadow RasterizerState" );
        }

        BlendStateCreateInfo BlendStateInfo;
        TSharedRef<BlendState> BlendState = CreateBlendState( BlendStateInfo );
        if ( !BlendState )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            BlendState->SetName( "Shadow BlendState" );
        }

        GraphicsPipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.BlendState = BlendState.Get();
        PipelineStateInfo.DepthStencilState = DepthStencilState.Get();
        PipelineStateInfo.IBStripCutValue = EIndexBufferStripCutValue::Disabled;
        PipelineStateInfo.InputLayoutState = FrameResources.StdInputLayout.Get();
        PipelineStateInfo.PrimitiveTopologyType = EPrimitiveTopologyType::Triangle;
        PipelineStateInfo.RasterizerState = RasterizerState.Get();
        PipelineStateInfo.SampleCount = 1;
        PipelineStateInfo.SampleQuality = 0;
        PipelineStateInfo.SampleMask = 0xffffffff;
        PipelineStateInfo.ShaderState.VertexShader = DirectionalLightShader.Get();
        PipelineStateInfo.ShaderState.PixelShader = nullptr;
        PipelineStateInfo.PipelineFormats.NumRenderTargets = 0;
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = LightSetup.ShadowMapFormat;

        DirectionalLightPSO = CreateGraphicsPipelineState( PipelineStateInfo );
        if ( !DirectionalLightPSO )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            DirectionalLightPSO->SetName( "ShadowMap PipelineState" );
        }
    }

    // Cascade Matrix Generation
    {
        if ( !ShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/CascadeMatrixGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode ) )
        {
            Debug::DebugBreak();
            return false;
        }

        CascadeGenShader = CreateComputeShader( ShaderCode );
        if ( !CascadeGenShader )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            CascadeGenShader->SetName( "CascadeGen ComputeShader" );
        }

        ComputePipelineStateCreateInfo CascadePSO;
        CascadePSO.Shader = CascadeGenShader.Get();

        CascadeGen = CreateComputePipelineState( CascadePSO );
        if ( !CascadeGen )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            CascadeGen->SetName( "CascadeGen PSO" );
        }
    }

    // Create buffers for cascade matrix generation
    {
        CascadeGenerationData = CreateConstantBuffer<SCascadeGenerationInfo>( BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr );
        if ( !CascadeGenerationData )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            CascadeGenerationData->SetName( "Cascade GenerationData" );
        }

        LightSetup.CascadeMatrixBuffer = CreateStructuredBuffer<SCascadeMatrices>( NUM_SHADOW_CASCADES, BufferFlags_RWBuffer, EResourceState::UnorderedAccess, nullptr );
        if ( !LightSetup.CascadeMatrixBuffer )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeMatrixBuffer->SetName( "Cascade MatrixBuffer" );
        }

        LightSetup.CascadeMatrixBufferSRV = CreateShaderResourceView( LightSetup.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES );
        if ( !LightSetup.CascadeMatrixBufferSRV )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeMatrixBufferSRV->SetName( "Cascade MatrixBuffer SRV" );
        }

        LightSetup.CascadeMatrixBufferUAV = CreateUnorderedAccessView( LightSetup.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES );
        if ( !LightSetup.CascadeMatrixBufferUAV )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeMatrixBufferUAV->SetName( "Cascade MatrixBuffer UAV" );
        }

        LightSetup.CascadeSplitsBuffer = CreateStructuredBuffer<SCascadeSplits>( NUM_SHADOW_CASCADES, BufferFlags_RWBuffer, EResourceState::UnorderedAccess, nullptr );
        if ( !LightSetup.CascadeSplitsBuffer )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeSplitsBuffer->SetName( "Cascade SplitBuffer" );
        }

        LightSetup.CascadeSplitsBufferSRV = CreateShaderResourceView( LightSetup.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES );
        if ( !LightSetup.CascadeSplitsBufferSRV )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeSplitsBufferSRV->SetName( "Cascade SplitBuffer SRV" );
        }

        LightSetup.CascadeSplitsBufferUAV = CreateUnorderedAccessView( LightSetup.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES );
        if ( !LightSetup.CascadeSplitsBufferUAV )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeSplitsBufferUAV->SetName( "Cascade SplitBuffer UAV" );
        }
    }

    // Directional Light ShadowMask
    {
        if ( !ShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/DirectionalShadowMaskGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode ) )
        {
            Debug::DebugBreak();
            return false;
        }

        DirectionalShadowMaskShader = CreateComputeShader( ShaderCode );
        if ( !DirectionalShadowMaskShader )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            DirectionalShadowMaskShader->SetName( "Directional ShadowMask ComputeShader" );
        }

        ComputePipelineStateCreateInfo MaskPSO;
        MaskPSO.Shader = DirectionalShadowMaskShader.Get();

        DirectionalShadowMaskPSO = CreateComputePipelineState( MaskPSO );
        if ( !DirectionalShadowMaskPSO )
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            DirectionalShadowMaskPSO->SetName( "Directional ShadowMask PSO" );
        }
    }

    return true;
}

void ShadowMapRenderer::RenderPointLightShadows( CommandList& CmdList, const LightSetup& LightSetup, const Scene& Scene )
{
    //PointLightFrame++;
    //if (PointLightFrame > 6)
    //{
    //    UpdatePointLight = true;
    //    PointLightFrame = 0;
    //}

    CmdList.SetPrimitiveTopology( EPrimitiveTopology::TriangleList );

    CmdList.TransitionTexture( LightSetup.PointLightShadowMaps.Get(), EResourceState::PixelShaderResource, EResourceState::DepthWrite );

    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "Begin Render PointLight ShadowMaps" );

    //if (UpdatePointLight)
    {
        GPU_TRACE_SCOPE( CmdList, "PointLight ShadowMaps" );

        TRACE_SCOPE( "Render PointLight ShadowMaps" );

        const uint32 PointLightShadowSize = LightSetup.PointLightShadowSize;
        CmdList.SetViewport( static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 1.0f, 0.0f, 0.0f );
        CmdList.SetScissorRect( static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0 );

        CmdList.SetGraphicsPipelineState( PointLightPipelineState.Get() );

        // PerObject Structs
        struct ShadowPerObject
        {
            CMatrix4 Matrix;
        } ShadowPerObjectBuffer;

        SPerShadowMap PerShadowMapData;
        for ( int32 i = 0; i < LightSetup.PointLightShadowMapsGenerationData.Size(); i++ )
        {
            for ( uint32 Face = 0; Face < 6; Face++ )
            {
                auto& Cube = LightSetup.PointLightShadowMapDSVs[i];
                CmdList.ClearDepthStencilView( Cube[Face].Get(), DepthStencilF( 1.0f, 0 ) );

                CmdList.SetRenderTargets( nullptr, 0, Cube[Face].Get() );

                auto& Data = LightSetup.PointLightShadowMapsGenerationData[i];
                PerShadowMapData.Matrix = Data.Matrix[Face];
                PerShadowMapData.Position = Data.Position;
                PerShadowMapData.FarPlane = Data.FarPlane;

                CmdList.TransitionBuffer( PerShadowMapBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest );

                CmdList.UpdateBuffer( PerShadowMapBuffer.Get(), 0, sizeof( SPerShadowMap ), &PerShadowMapData );

                CmdList.TransitionBuffer( PerShadowMapBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer );

                CmdList.SetConstantBuffer( PointLightVertexShader.Get(), PerShadowMapBuffer.Get(), 0 );
                CmdList.SetConstantBuffer( PointLightPixelShader.Get(), PerShadowMapBuffer.Get(), 0 );

                // Draw all objects to depthbuffer
                ConsoleVariable* GlobalFrustumCullEnabled = GConsole.FindVariable( "r.EnableFrustumCulling" );
                if ( GlobalFrustumCullEnabled->GetBool() )
                {
                    Frustum CameraFrustum = Frustum( Data.FarPlane, Data.ViewMatrix[Face], Data.ProjMatrix[Face] );
                    for ( const MeshDrawCommand& Command : Scene.GetMeshDrawCommands() )
                    {
                        CMatrix4 TransformMatrix = Command.CurrentActor->GetTransform().GetMatrix();
                        TransformMatrix = TransformMatrix.Transpose();

                        CVector3 Top = CVector3( &Command.Mesh->BoundingBox.Top.x );
                        Top = TransformMatrix.TransformPosition( Top );
                        CVector3 Bottom = CVector3( &Command.Mesh->BoundingBox.Bottom.x );
                        Bottom = TransformMatrix.TransformPosition( Bottom );

                        AABB Box;
                        Box.Top = Top;
                        Box.Bottom = Bottom;
                        if ( CameraFrustum.CheckAABB( Box ) )
                        {
                            CmdList.SetVertexBuffers( &Command.VertexBuffer, 1, 0 );
                            CmdList.SetIndexBuffer( Command.IndexBuffer );

                            ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

                            CmdList.Set32BitShaderConstants( PointLightVertexShader.Get(), &ShadowPerObjectBuffer, 16 );

                            CmdList.DrawIndexedInstanced( Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0 );
                        }
                    }
                }
                else
                {
                    for ( const MeshDrawCommand& Command : Scene.GetMeshDrawCommands() )
                    {
                        CmdList.SetVertexBuffers( &Command.VertexBuffer, 1, 0 );
                        CmdList.SetIndexBuffer( Command.IndexBuffer );

                        ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

                        CmdList.Set32BitShaderConstants( PointLightVertexShader.Get(), &ShadowPerObjectBuffer, 16 );

                        CmdList.DrawIndexedInstanced( Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0 );
                    }
                }
            }
        }

        UpdatePointLight = false;
    }

    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "End Render PointLight ShadowMaps" );

    CmdList.TransitionTexture( LightSetup.PointLightShadowMaps.Get(), EResourceState::DepthWrite, EResourceState::NonPixelShaderResource );
}

void ShadowMapRenderer::RenderDirectionalLightShadows( CommandList& CmdList, const LightSetup& LightSetup, const FrameResources& FrameResources, const Scene& Scene )
{
    // Generate matrices for directional light
    {
        GPU_TRACE_SCOPE( CmdList, "Generate Cascade Matrices" );

        CmdList.TransitionBuffer( LightSetup.CascadeMatrixBuffer.Get(), EResourceState::NonPixelShaderResource, EResourceState::UnorderedAccess );
        CmdList.TransitionBuffer( LightSetup.CascadeSplitsBuffer.Get(), EResourceState::NonPixelShaderResource, EResourceState::UnorderedAccess );

        SCascadeGenerationInfo GenerationInfo;
        GenerationInfo.CascadeSplitLambda = LightSetup.CascadeSplitLambda;
        GenerationInfo.LightUp = LightSetup.DirectionalLightData.Up;
        GenerationInfo.LightDirection = LightSetup.DirectionalLightData.Direction;

        CmdList.TransitionBuffer( CascadeGenerationData.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest );
        CmdList.UpdateBuffer( CascadeGenerationData.Get(), 0, sizeof( SCascadeGenerationInfo ), &GenerationInfo );
        CmdList.TransitionBuffer( CascadeGenerationData.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer );

        CmdList.SetComputePipelineState( CascadeGen.Get() );

        CmdList.SetConstantBuffer( CascadeGenShader.Get(), FrameResources.CameraBuffer.Get(), 0 );
        CmdList.SetConstantBuffer( CascadeGenShader.Get(), CascadeGenerationData.Get(), 1 );

        CmdList.SetUnorderedAccessView( CascadeGenShader.Get(), LightSetup.CascadeMatrixBufferUAV.Get(), 0 );
        CmdList.SetUnorderedAccessView( CascadeGenShader.Get(), LightSetup.CascadeSplitsBufferUAV.Get(), 1 );

        CmdList.SetShaderResourceView( CascadeGenShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0 );

        CmdList.Dispatch( NUM_SHADOW_CASCADES, 1, 1 );

        CmdList.TransitionBuffer( LightSetup.CascadeMatrixBuffer.Get(), EResourceState::UnorderedAccess, EResourceState::PixelShaderResource );
        CmdList.TransitionBuffer( LightSetup.CascadeSplitsBuffer.Get(), EResourceState::UnorderedAccess, EResourceState::NonPixelShaderResource );
    }

    // Render directional shadows
    {
        INSERT_DEBUG_CMDLIST_MARKER( CmdList, "Begin Render DirectionalLight ShadowMaps" );

        TRACE_SCOPE( "Render DirectionalLight ShadowMaps" );

        GPU_TRACE_SCOPE( CmdList, "DirectionalLight ShadowMaps" );

        CmdList.TransitionTexture( LightSetup.ShadowMapCascades[0].Get(), EResourceState::NonPixelShaderResource, EResourceState::DepthWrite );
        CmdList.TransitionTexture( LightSetup.ShadowMapCascades[1].Get(), EResourceState::NonPixelShaderResource, EResourceState::DepthWrite );
        CmdList.TransitionTexture( LightSetup.ShadowMapCascades[2].Get(), EResourceState::NonPixelShaderResource, EResourceState::DepthWrite );
        CmdList.TransitionTexture( LightSetup.ShadowMapCascades[3].Get(), EResourceState::NonPixelShaderResource, EResourceState::DepthWrite );

        CmdList.SetPrimitiveTopology( EPrimitiveTopology::TriangleList );
        CmdList.SetGraphicsPipelineState( DirectionalLightPSO.Get() );

        // PerObject Structs
        struct SShadowPerObject
        {
            CMatrix4 Matrix;
        } ShadowPerObjectBuffer;

        SPerCascade PerCascadeData;
        for ( uint32 i = 0; i < NUM_SHADOW_CASCADES; i++ )
        {
            DepthStencilView* CascadeDSV = LightSetup.ShadowMapCascades[i]->GetDepthStencilView();
            CmdList.ClearDepthStencilView( CascadeDSV, DepthStencilF( 1.0f, 0 ) );

            CmdList.SetRenderTargets( nullptr, 0, CascadeDSV );

            const uint16 CascadeSize = LightSetup.CascadeSize;
            CmdList.SetViewport( static_cast<float>(CascadeSize), static_cast<float>(CascadeSize), 0.0f, 1.0f, 0.0f, 0.0f );
            CmdList.SetScissorRect( CascadeSize, CascadeSize, 0, 0 );

            PerCascadeData.CascadeIndex = i;

            CmdList.TransitionBuffer( PerCascadeBuffer.Get(), EResourceState::VertexAndConstantBuffer, EResourceState::CopyDest );

            CmdList.UpdateBuffer( PerCascadeBuffer.Get(), 0, sizeof( SPerCascade ), &PerCascadeData );

            CmdList.TransitionBuffer( PerCascadeBuffer.Get(), EResourceState::CopyDest, EResourceState::VertexAndConstantBuffer );

            CmdList.SetConstantBuffer( DirectionalLightShader.Get(), PerCascadeBuffer.Get(), 0 );

            CmdList.SetShaderResourceView( DirectionalLightShader.Get(), LightSetup.CascadeMatrixBufferSRV.Get(), 0 );

            // Draw all objects to depthbuffer
            for ( const MeshDrawCommand& Command : Scene.GetMeshDrawCommands() )
            {
                CmdList.SetVertexBuffers( &Command.VertexBuffer, 1, 0 );
                CmdList.SetIndexBuffer( Command.IndexBuffer );

                ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

                CmdList.Set32BitShaderConstants( DirectionalLightShader.Get(), &ShadowPerObjectBuffer, 16 );

                CmdList.DrawIndexedInstanced( Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0 );
            }
        }

        CmdList.TransitionTexture( LightSetup.ShadowMapCascades[0].Get(), EResourceState::DepthWrite, EResourceState::NonPixelShaderResource );
        CmdList.TransitionTexture( LightSetup.ShadowMapCascades[1].Get(), EResourceState::DepthWrite, EResourceState::NonPixelShaderResource );
        CmdList.TransitionTexture( LightSetup.ShadowMapCascades[2].Get(), EResourceState::DepthWrite, EResourceState::NonPixelShaderResource );
        CmdList.TransitionTexture( LightSetup.ShadowMapCascades[3].Get(), EResourceState::DepthWrite, EResourceState::NonPixelShaderResource );

        CmdList.TransitionBuffer( LightSetup.CascadeMatrixBuffer.Get(), EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource );

        INSERT_DEBUG_CMDLIST_MARKER( CmdList, "End Render DirectionalLight ShadowMaps" );
    }

    // Generate Directional Shadow Mask
    {
        GPU_TRACE_SCOPE( CmdList, "DirectionalLight Shadow Mask" );

        CmdList.TransitionTexture( LightSetup.DirectionalShadowMask.Get(), EResourceState::NonPixelShaderResource, EResourceState::UnorderedAccess );

        CmdList.SetComputePipelineState( DirectionalShadowMaskPSO.Get() );

        CmdList.SetConstantBuffer( DirectionalShadowMaskShader.Get(), FrameResources.CameraBuffer.Get(), 0 );
        CmdList.SetConstantBuffer( DirectionalShadowMaskShader.Get(), LightSetup.DirectionalLightsBuffer.Get(), 1 );

        CmdList.SetShaderResourceView( DirectionalShadowMaskShader.Get(), LightSetup.CascadeMatrixBufferSRV.Get(), 0 );
        CmdList.SetShaderResourceView( DirectionalShadowMaskShader.Get(), LightSetup.CascadeSplitsBufferSRV.Get(), 1 );

        CmdList.SetShaderResourceView( DirectionalShadowMaskShader.Get(), FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView(), 2 );
        CmdList.SetShaderResourceView( DirectionalShadowMaskShader.Get(), FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(), 3 );

        CmdList.SetShaderResourceView( DirectionalShadowMaskShader.Get(), LightSetup.ShadowMapCascades[0]->GetShaderResourceView(), 4 );
        CmdList.SetShaderResourceView( DirectionalShadowMaskShader.Get(), LightSetup.ShadowMapCascades[1]->GetShaderResourceView(), 5 );
        CmdList.SetShaderResourceView( DirectionalShadowMaskShader.Get(), LightSetup.ShadowMapCascades[2]->GetShaderResourceView(), 6 );
        CmdList.SetShaderResourceView( DirectionalShadowMaskShader.Get(), LightSetup.ShadowMapCascades[3]->GetShaderResourceView(), 7 );

        CmdList.SetUnorderedAccessView( DirectionalShadowMaskShader.Get(), LightSetup.DirectionalShadowMask->GetUnorderedAccessView(), 0 );

        CmdList.SetSamplerState( DirectionalShadowMaskShader.Get(), FrameResources.DirectionalLightShadowSampler.Get(), 0 );

        const CIntPoint3 ThreadGroupXYZ = DirectionalShadowMaskShader->GetThreadGroupXYZ();
        const uint32 ThreadsX = NMath::DivideByMultiple( LightSetup.DirectionalShadowMask->GetWidth(), ThreadGroupXYZ.x );
        const uint32 ThreadsY = NMath::DivideByMultiple( LightSetup.DirectionalShadowMask->GetHeight(), ThreadGroupXYZ.y );
        CmdList.Dispatch( ThreadsX, ThreadsY, 1 );

        CmdList.TransitionTexture( LightSetup.DirectionalShadowMask.Get(), EResourceState::UnorderedAccess, EResourceState::NonPixelShaderResource );
    }
}

bool ShadowMapRenderer::ResizeResources( uint32 Width, uint32 Height, LightSetup& LightSetup )
{
    return CreateShadowMask( Width, Height, LightSetup );
}

void ShadowMapRenderer::Release()
{
    PerShadowMapBuffer.Reset();

    DirectionalShadowMaskPSO.Reset();
    DirectionalShadowMaskShader.Reset();

    DirectionalLightPSO.Reset();
    DirectionalLightShader.Reset();

    PointLightPipelineState.Reset();
    PointLightVertexShader.Reset();
    PointLightPixelShader.Reset();

    PerCascadeBuffer.Reset();

    CascadeGen.Reset();
    CascadeGenShader.Reset();

    CascadeGenerationData.Reset();
}

bool ShadowMapRenderer::CreateShadowMask( uint32 Width, uint32 Height, LightSetup& LightSetup )
{
    LightSetup.DirectionalShadowMask = CreateTexture2D(
        EFormat::R16_Float,
        Width, Height, 1, 1, ETextureFlags::TextureFlags_RWTexture,
        EResourceState::Common,
        nullptr );
    if ( LightSetup.DirectionalShadowMask )
    {
        LightSetup.DirectionalShadowMask->SetName( "Directional Shadow Mask" );
    }
    else
    {
        return false;
    }

    return true;
}

bool ShadowMapRenderer::CreateShadowMaps( LightSetup& LightSetup, FrameResources& FrameResources )
{
    const uint32 Width = FrameResources.MainWindowViewport->GetWidth();
    const uint32 Height = FrameResources.MainWindowViewport->GetHeight();

    if ( !CreateShadowMask( Width, Height, LightSetup ) )
    {
        return false;
    }

    LightSetup.PointLightShadowMaps = CreateTextureCubeArray(
        LightSetup.ShadowMapFormat,
        LightSetup.PointLightShadowSize,
        1, LightSetup.MaxPointLightShadows,
        TextureFlags_ShadowMap,
        EResourceState::PixelShaderResource,
        nullptr );
    if ( LightSetup.PointLightShadowMaps )
    {
        LightSetup.PointLightShadowMaps->SetName( "PointLight ShadowMaps" );

        LightSetup.PointLightShadowMapDSVs.Resize( LightSetup.MaxPointLightShadows );
        for ( uint32 i = 0; i < LightSetup.MaxPointLightShadows; i++ )
        {
            for ( uint32 Face = 0; Face < 6; Face++ )
            {
                TStaticArray<TSharedRef<DepthStencilView>, 6>& DepthCube = LightSetup.PointLightShadowMapDSVs[i];
                DepthCube[Face] = CreateDepthStencilView(
                    LightSetup.PointLightShadowMaps.Get(),
                    LightSetup.ShadowMapFormat,
                    GetCubeFaceFromIndex( Face ), 0, i );
                if ( !DepthCube[Face] )
                {
                    Debug::DebugBreak();
                    return false;
                }
            }
        }
    }
    else
    {
        return false;
    }

    for ( uint32 i = 0; i < NUM_SHADOW_CASCADES; i++ )
    {
        const uint16 CascadeSize = LightSetup.CascadeSize;

        LightSetup.ShadowMapCascades[i] = CreateTexture2D(
            LightSetup.ShadowMapFormat,
            CascadeSize, CascadeSize,
            1, 1, TextureFlags_ShadowMap,
            EResourceState::NonPixelShaderResource,
            nullptr );
        if ( LightSetup.ShadowMapCascades[i] )
        {
            LightSetup.ShadowMapCascades[i]->SetName( "Shadow Map Cascade[" + std::to_string( i ) + "]" );
        }
        else
        {
            return false;
        }
    }

    return true;
}

#include "ShadowMapRenderer.h"
#include "MeshDrawCommand.h"

#include "RHI/RHIInstance.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/Mesh.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Lights/DirectionalLight.h"

#include "Core/Math/Frustum.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"

#include "Renderer/Debug/GPUProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CShadowMapRenderer

bool CShadowMapRenderer::Init(SLightSetup& LightSetup, SFrameResources& FrameResources)
{
    if (!CreateShadowMaps(LightSetup, FrameResources))
    {
        return false;
    }

    UNREFERENCED_VARIABLE(bUpdateDirLight);
    UNREFERENCED_VARIABLE(DirLightFrame);
    UNREFERENCED_VARIABLE(PointLightFrame);

    TArray<uint8> ShaderCode;

    // Point Shadow Maps
    {
        PerShadowMapBuffer = RHICreateConstantBuffer<SPerShadowMap>(BufferFlag_Default, EResourceAccess::VertexAndConstantBuffer, nullptr);
        if (!PerShadowMapBuffer)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            PerShadowMapBuffer->SetName("Per ShadowMap Buffer");
        }

        if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/ShadowMap.hlsl", "Point_VSMain", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }

        PointLightVertexShader = RHICreateVertexShader(ShaderCode);
        if (!PointLightVertexShader)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            PointLightVertexShader->SetName("Point ShadowMap VertexShader");
        }

        if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/ShadowMap.hlsl", "Point_PSMain", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }

        PointLightPixelShader = RHICreatePixelShader(ShaderCode);
        if (!PointLightPixelShader)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            PointLightPixelShader->SetName("Point ShadowMap PixelShader");
        }

        CRHIDepthStencilStateInitializer DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc = ERHIComparisonFunc::LessEqual;
        DepthStencilStateInfo.bDepthEnable = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

        TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInfo);
        if (!DepthStencilState)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            DepthStencilState->SetName("Shadow DepthStencilState");
        }

        SRHIRasterizerStateDesc RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::Back;

        TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
        if (!RasterizerState)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            RasterizerState->SetName("Shadow RasterizerState");
        }

        SRHIBlendStateDesc BlendStateInfo;

        TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState(BlendStateInfo);
        if (!BlendState)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            BlendState->SetName("Shadow BlendState");
        }

        SRHIGraphicsPipelineStateDesc PipelineStateInfo;
        PipelineStateInfo.BlendState = BlendState.Get();
        PipelineStateInfo.DepthStencilState = DepthStencilState.Get();
        PipelineStateInfo.IBStripCutValue = EIndexBufferStripCutValue::Disabled;
        PipelineStateInfo.InputLayoutState = FrameResources.StdInputLayout.Get();
        PipelineStateInfo.PrimitiveTopologyType = ERHIPrimitiveTopologyType::Triangle;
        PipelineStateInfo.RasterizerState = RasterizerState.Get();
        PipelineStateInfo.SampleCount = 1;
        PipelineStateInfo.SampleQuality = 0;
        PipelineStateInfo.SampleMask = 0xffffffff;
        PipelineStateInfo.ShaderState.VertexShader = PointLightVertexShader.Get();
        PipelineStateInfo.ShaderState.PixelShader = PointLightPixelShader.Get();
        PipelineStateInfo.PipelineFormats.NumRenderTargets = 0;
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = LightSetup.ShadowMapFormat;

        PointLightPipelineState = RHICreateGraphicsPipelineState(PipelineStateInfo);
        if (!PointLightPipelineState)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            PointLightPipelineState->SetName("Point ShadowMap PipelineState");
        }
    }

    // Cascaded shadowmap
    {
        PerCascadeBuffer = RHICreateConstantBuffer<SPerCascade>(ERHIBufferFlags::BufferFlag_Default, EResourceAccess::VertexAndConstantBuffer, nullptr);
        if (!PerCascadeBuffer)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            PerCascadeBuffer->SetName("Per Cascade Buffer");
        }

        if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/ShadowMap.hlsl", "Cascade_VSMain", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }

        DirectionalLightShader = RHICreateVertexShader(ShaderCode);
        if (!DirectionalLightShader)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            DirectionalLightShader->SetName("ShadowMap VertexShader");
        }

        CRHIDepthStencilStateInitializer DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc = ERHIComparisonFunc::LessEqual;
        DepthStencilStateInfo.bDepthEnable = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

        TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInfo);
        if (!DepthStencilState)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            DepthStencilState->SetName("Shadow DepthStencilState");
        }

        SRHIRasterizerStateDesc RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::Back;

        TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
        if (!RasterizerState)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            RasterizerState->SetName("Shadow RasterizerState");
        }

        SRHIBlendStateDesc BlendStateInfo;
        TSharedRef<CRHIBlendState> BlendState = RHICreateBlendState(BlendStateInfo);
        if (!BlendState)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            BlendState->SetName("Shadow BlendState");
        }

        SRHIGraphicsPipelineStateDesc PipelineStateInfo;
        PipelineStateInfo.BlendState = BlendState.Get();
        PipelineStateInfo.DepthStencilState = DepthStencilState.Get();
        PipelineStateInfo.IBStripCutValue = EIndexBufferStripCutValue::Disabled;
        PipelineStateInfo.InputLayoutState = FrameResources.StdInputLayout.Get();
        PipelineStateInfo.PrimitiveTopologyType = ERHIPrimitiveTopologyType::Triangle;
        PipelineStateInfo.RasterizerState = RasterizerState.Get();
        PipelineStateInfo.SampleCount = 1;
        PipelineStateInfo.SampleQuality = 0;
        PipelineStateInfo.SampleMask = 0xffffffff;
        PipelineStateInfo.ShaderState.VertexShader = DirectionalLightShader.Get();
        PipelineStateInfo.ShaderState.PixelShader = nullptr;
        PipelineStateInfo.PipelineFormats.NumRenderTargets = 0;
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = LightSetup.ShadowMapFormat;

        DirectionalLightPSO = RHICreateGraphicsPipelineState(PipelineStateInfo);
        if (!DirectionalLightPSO)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            DirectionalLightPSO->SetName("ShadowMap PipelineState");
        }
    }

    // Cascade Matrix Generation
    {
        if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/CascadeMatrixGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }

        CascadeGenShader = RHICreateComputeShader(ShaderCode);
        if (!CascadeGenShader)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            CascadeGenShader->SetName("CascadeGen ComputeShader");
        }

        SRHIComputePipelineStateDesc CascadePSO;
        CascadePSO.Shader = CascadeGenShader.Get();

        CascadeGen = RHICreateComputePipelineState(CascadePSO);
        if (!CascadeGen)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            CascadeGen->SetName("CascadeGen PSO");
        }
    }

    // Create buffers for cascade matrix generation
    {
        CascadeGenerationData = RHICreateConstantBuffer<SCascadeGenerationInfo>(BufferFlag_Default, EResourceAccess::VertexAndConstantBuffer, nullptr);
        if (!CascadeGenerationData)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            CascadeGenerationData->SetName("Cascade GenerationData");
        }

        LightSetup.CascadeMatrixBuffer = RHICreateStructuredBuffer<SCascadeMatrices>(NUM_SHADOW_CASCADES, BufferFlags_RWBuffer, EResourceAccess::UnorderedAccess, nullptr);
        if (!LightSetup.CascadeMatrixBuffer)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeMatrixBuffer->SetName("Cascade MatrixBuffer");
        }

        LightSetup.CascadeMatrixBufferSRV = RHICreateShaderResourceView(LightSetup.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES);
        if (!LightSetup.CascadeMatrixBufferSRV)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeMatrixBufferSRV->SetName("Cascade MatrixBuffer SRV");
        }

        LightSetup.CascadeMatrixBufferUAV = RHICreateUnorderedAccessView(LightSetup.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES);
        if (!LightSetup.CascadeMatrixBufferUAV)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeMatrixBufferUAV->SetName("Cascade MatrixBuffer UAV");
        }

        LightSetup.CascadeSplitsBuffer = RHICreateStructuredBuffer<SCascadeSplits>(NUM_SHADOW_CASCADES, BufferFlags_RWBuffer, EResourceAccess::UnorderedAccess, nullptr);
        if (!LightSetup.CascadeSplitsBuffer)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeSplitsBuffer->SetName("Cascade SplitBuffer");
        }

        LightSetup.CascadeSplitsBufferSRV = RHICreateShaderResourceView(LightSetup.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES);
        if (!LightSetup.CascadeSplitsBufferSRV)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeSplitsBufferSRV->SetName("Cascade SplitBuffer SRV");
        }

        LightSetup.CascadeSplitsBufferUAV = RHICreateUnorderedAccessView(LightSetup.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES);
        if (!LightSetup.CascadeSplitsBufferUAV)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeSplitsBufferUAV->SetName("Cascade SplitBuffer UAV");
        }
    }

    // Directional Light ShadowMask
    {
        if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/DirectionalShadowMaskGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }

        DirectionalShadowMaskShader = RHICreateComputeShader(ShaderCode);
        if (!DirectionalShadowMaskShader)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            DirectionalShadowMaskShader->SetName("Directional ShadowMask ComputeShader");
        }

        SRHIComputePipelineStateDesc MaskPSO;
        MaskPSO.Shader = DirectionalShadowMaskShader.Get();

        DirectionalShadowMaskPSO = RHICreateComputePipelineState(MaskPSO);
        if (!DirectionalShadowMaskPSO)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            DirectionalShadowMaskPSO->SetName("Directional ShadowMask PSO");
        }
    }

    return true;
}

void CShadowMapRenderer::RenderPointLightShadows(CRHICommandList& CmdList, const SLightSetup& LightSetup, const CScene& Scene)
{
    //PointLightFrame++;
    //if (PointLightFrame > 6)
    //{
    //    bUpdatePointLight = true;
    //    PointLightFrame = 0;
    //}

    CmdList.SetPrimitiveTopology(ERHIPrimitiveTopology::TriangleList);

    CmdList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite);

    INSERT_COMMAND_LIST_MARKER(CmdList, "Begin Render PointLight ShadowMaps");

    //if (bUpdatePointLight)
    {
        GPU_TRACE_SCOPE(CmdList, "PointLight ShadowMaps");

        TRACE_SCOPE("Render PointLight ShadowMaps");

        const uint32 PointLightShadowSize = LightSetup.PointLightShadowSize;
        CmdList.SetViewport(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 1.0f, 0.0f, 0.0f);
        CmdList.SetScissorRect(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0);

        CmdList.SetGraphicsPipelineState(PointLightPipelineState.Get());

        // PerObject Structs
        struct ShadowPerObject
        {
            CMatrix4 Matrix;
        } ShadowPerObjectBuffer;

        SPerShadowMap PerShadowMapData;
        for (int32 i = 0; i < LightSetup.PointLightShadowMapsGenerationData.Size(); i++)
        {
            for (uint32 Face = 0; Face < 6; Face++)
            {
                auto& Cube = LightSetup.PointLightShadowMapDSVs[i];
                CmdList.ClearDepthStencilView(Cube[Face].Get(), SDepthStencil(1.0f, 0));

                CmdList.SetRenderTargets(nullptr, 0, Cube[Face].Get());

                auto& Data = LightSetup.PointLightShadowMapsGenerationData[i];
                PerShadowMapData.Matrix = Data.Matrix[Face];
                PerShadowMapData.Position = Data.Position;
                PerShadowMapData.FarPlane = Data.FarPlane;

                CmdList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);

                CmdList.UpdateBuffer(PerShadowMapBuffer.Get(), 0, sizeof(SPerShadowMap), &PerShadowMapData);

                CmdList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

                CmdList.SetConstantBuffer(PointLightVertexShader.Get(), PerShadowMapBuffer.Get(), 0);
                CmdList.SetConstantBuffer(PointLightPixelShader.Get(), PerShadowMapBuffer.Get(), 0);

                // Draw all objects to depth buffer
                IConsoleVariable* GlobalFrustumCullEnabled = CConsoleManager::Get().FindVariable("renderer.EnableFrustumCulling");
                if (GlobalFrustumCullEnabled && GlobalFrustumCullEnabled->GetBool())
                {
                    CFrustum CameraFrustum = CFrustum(Data.FarPlane, Data.ViewMatrix[Face], Data.ProjMatrix[Face]);
                    for (const SMeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                    {
                        CMatrix4 TransformMatrix = Command.CurrentActor->GetTransform().GetMatrix();
                        TransformMatrix = TransformMatrix.Transpose();

                        CVector3 Top = CVector3(&Command.Mesh->BoundingBox.Top.x);
                        Top = TransformMatrix.TransformPosition(Top);
                        CVector3 Bottom = CVector3(&Command.Mesh->BoundingBox.Bottom.x);
                        Bottom = TransformMatrix.TransformPosition(Bottom);

                        SAABB Box;
                        Box.Top    = Top;
                        Box.Bottom = Bottom;
                        if (CameraFrustum.CheckAABB(Box))
                        {
                            CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
                            CmdList.SetIndexBuffer(Command.IndexBuffer, ERHIIndexFormat::uint32);

                            ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

                            CmdList.Set32BitShaderConstants(PointLightVertexShader.Get(), &ShadowPerObjectBuffer, 16);

                            CmdList.DrawIndexedInstanced(Command.NumIndices, 1, 0, 0, 0);
                        }
                    }
                }
                else
                {
                    for (const SMeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                    {
                        CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
                        CmdList.SetIndexBuffer(Command.IndexBuffer, ERHIIndexFormat::uint32);

                        ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

                        CmdList.Set32BitShaderConstants(PointLightVertexShader.Get(), &ShadowPerObjectBuffer, 16);

                        CmdList.DrawIndexedInstanced(Command.NumIndices, 1, 0, 0, 0);
                    }
                }
            }
        }

        bUpdatePointLight = false;
    }

    INSERT_COMMAND_LIST_MARKER(CmdList, "End Render PointLight ShadowMaps");

    CmdList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
}

void CShadowMapRenderer::RenderDirectionalLightShadows(CRHICommandList& CmdList, const SLightSetup& LightSetup, const SFrameResources& FrameResources, const CScene& Scene)
{
    // Generate matrices for directional light
    {
        GPU_TRACE_SCOPE(CmdList, "Generate Cascade Matrices");

        CmdList.TransitionBuffer(LightSetup.CascadeMatrixBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
        CmdList.TransitionBuffer(LightSetup.CascadeSplitsBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

        SCascadeGenerationInfo GenerationInfo;
        GenerationInfo.CascadeSplitLambda = LightSetup.CascadeSplitLambda;
        GenerationInfo.LightUp = LightSetup.DirectionalLightData.Up;
        GenerationInfo.LightDirection = LightSetup.DirectionalLightData.Direction;
        GenerationInfo.CascadeResolution = (float)LightSetup.CascadeSize;

        CmdList.TransitionBuffer(CascadeGenerationData.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);
        CmdList.UpdateBuffer(CascadeGenerationData.Get(), 0, sizeof(SCascadeGenerationInfo), &GenerationInfo);
        CmdList.TransitionBuffer(CascadeGenerationData.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

        CmdList.SetComputePipelineState(CascadeGen.Get());

        CmdList.SetConstantBuffer(CascadeGenShader.Get(), FrameResources.CameraBuffer.Get(), 0);
        CmdList.SetConstantBuffer(CascadeGenShader.Get(), CascadeGenerationData.Get(), 1);

        CmdList.SetUnorderedAccessView(CascadeGenShader.Get(), LightSetup.CascadeMatrixBufferUAV.Get(), 0);
        CmdList.SetUnorderedAccessView(CascadeGenShader.Get(), LightSetup.CascadeSplitsBufferUAV.Get(), 1);

        CmdList.SetShaderResourceView(CascadeGenShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);

        CmdList.Dispatch(NUM_SHADOW_CASCADES, 1, 1);

        CmdList.TransitionBuffer(LightSetup.CascadeMatrixBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource);
        CmdList.TransitionBuffer(LightSetup.CascadeSplitsBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    }

    // Render directional shadows
    {
        INSERT_COMMAND_LIST_MARKER(CmdList, "Begin Render DirectionalLight ShadowMaps");

        TRACE_SCOPE("Render DirectionalLight ShadowMaps");

        GPU_TRACE_SCOPE(CmdList, "DirectionalLight ShadowMaps");

        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[0].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[1].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[2].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[3].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);

        CmdList.SetPrimitiveTopology(ERHIPrimitiveTopology::TriangleList);
        CmdList.SetGraphicsPipelineState(DirectionalLightPSO.Get());

        // PerObject Structs
        struct SShadowPerObject
        {
            CMatrix4 Matrix;
        } ShadowPerObjectBuffer;

        SPerCascade PerCascadeData;
        for (uint32 i = 0; i < NUM_SHADOW_CASCADES; i++)
        {
            CRHIDepthStencilView* CascadeDSV = LightSetup.ShadowMapCascades[i]->GetDepthStencilView();
            CmdList.ClearDepthStencilView(CascadeDSV, SDepthStencil(1.0f, 0));

            CmdList.SetRenderTargets(nullptr, 0, CascadeDSV);

            const uint16 CascadeSize = LightSetup.CascadeSize;
            CmdList.SetViewport(static_cast<float>(CascadeSize), static_cast<float>(CascadeSize), 0.0f, 1.0f, 0.0f, 0.0f);
            CmdList.SetScissorRect(CascadeSize, CascadeSize, 0, 0);

            PerCascadeData.CascadeIndex = i;

            CmdList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);

            CmdList.UpdateBuffer(PerCascadeBuffer.Get(), 0, sizeof(SPerCascade), &PerCascadeData);

            CmdList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

            CmdList.SetConstantBuffer(DirectionalLightShader.Get(), PerCascadeBuffer.Get(), 0);

            CmdList.SetShaderResourceView(DirectionalLightShader.Get(), LightSetup.CascadeMatrixBufferSRV.Get(), 0);

            // Draw all objects to shadow-map
            for (const SMeshDrawCommand& Command : Scene.GetMeshDrawCommands())
            {
                CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
                CmdList.SetIndexBuffer(Command.IndexBuffer, ERHIIndexFormat::uint32);

                ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

                CmdList.Set32BitShaderConstants(DirectionalLightShader.Get(), &ShadowPerObjectBuffer, 16);

                CmdList.DrawIndexedInstanced(Command.NumIndices, 1, 0, 0, 0);
            }
        }

        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[0].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[1].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[2].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[3].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);

        CmdList.TransitionBuffer(LightSetup.CascadeMatrixBuffer.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);

        INSERT_COMMAND_LIST_MARKER(CmdList, "End Render DirectionalLight ShadowMaps");
    }
}

void CShadowMapRenderer::RenderShadowMasks(CRHICommandList& CmdList, const SLightSetup& LightSetup, const SFrameResources& FrameResources)
{
    // Generate Directional Shadow Mask
    {
        GPU_TRACE_SCOPE(CmdList, "DirectionalLight Shadow Mask");

        CmdList.TransitionTexture(LightSetup.DirectionalShadowMask.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

        CmdList.SetComputePipelineState(DirectionalShadowMaskPSO.Get());

        CmdList.SetConstantBuffer(DirectionalShadowMaskShader.Get(), FrameResources.CameraBuffer.Get(), 0);
        CmdList.SetConstantBuffer(DirectionalShadowMaskShader.Get(), LightSetup.DirectionalLightsBuffer.Get(), 1);

        CmdList.SetShaderResourceView(DirectionalShadowMaskShader.Get(), LightSetup.CascadeMatrixBufferSRV.Get(), 0);
        CmdList.SetShaderResourceView(DirectionalShadowMaskShader.Get(), LightSetup.CascadeSplitsBufferSRV.Get(), 1);

        CmdList.SetShaderResourceView(DirectionalShadowMaskShader.Get(), FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView(), 2);
        CmdList.SetShaderResourceView(DirectionalShadowMaskShader.Get(), FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(), 3);

        CmdList.SetShaderResourceView(DirectionalShadowMaskShader.Get(), LightSetup.ShadowMapCascades[0]->GetShaderResourceView(), 4);
        CmdList.SetShaderResourceView(DirectionalShadowMaskShader.Get(), LightSetup.ShadowMapCascades[1]->GetShaderResourceView(), 5);
        CmdList.SetShaderResourceView(DirectionalShadowMaskShader.Get(), LightSetup.ShadowMapCascades[2]->GetShaderResourceView(), 6);
        CmdList.SetShaderResourceView(DirectionalShadowMaskShader.Get(), LightSetup.ShadowMapCascades[3]->GetShaderResourceView(), 7);

        CmdList.SetUnorderedAccessView(DirectionalShadowMaskShader.Get(), LightSetup.DirectionalShadowMask->GetUnorderedAccessView(), 0);

        CmdList.SetSamplerState(DirectionalShadowMaskShader.Get(), FrameResources.DirectionalLightShadowSampler.Get(), 0);

        const CIntVector3 ThreadGroupXYZ = DirectionalShadowMaskShader->GetThreadGroupXYZ();
        const uint32 ThreadsX = NMath::DivideByMultiple(LightSetup.DirectionalShadowMask->GetWidth(), ThreadGroupXYZ.x);
        const uint32 ThreadsY = NMath::DivideByMultiple(LightSetup.DirectionalShadowMask->GetHeight(), ThreadGroupXYZ.y);
        CmdList.Dispatch(ThreadsX, ThreadsY, 1);

        CmdList.TransitionTexture(LightSetup.DirectionalShadowMask.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    }
}

bool CShadowMapRenderer::ResizeResources(uint32 Width, uint32 Height, SLightSetup& LightSetup)
{
    return CreateShadowMask(Width, Height, LightSetup);
}

void CShadowMapRenderer::Release()
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

bool CShadowMapRenderer::CreateShadowMask(uint32 Width, uint32 Height, SLightSetup& LightSetup)
{
    LightSetup.DirectionalShadowMask = RHICreateTexture2D(LightSetup.ShadowMaskFormat, Width, Height, 1, 1, ERHITextureFlags::TextureFlags_RWTexture, EResourceAccess::NonPixelShaderResource, nullptr);
    if (LightSetup.DirectionalShadowMask)
    {
        LightSetup.DirectionalShadowMask->SetName("Directional Shadow Mask 0");
    }
    else
    {
        return false;
    }

    return true;
}

bool CShadowMapRenderer::CreateShadowMaps(SLightSetup& LightSetup, SFrameResources& FrameResources)
{
    const uint32 Width = FrameResources.MainWindowViewport->GetWidth();
    const uint32 Height = FrameResources.MainWindowViewport->GetHeight();

    if (!CreateShadowMask(Width, Height, LightSetup))
    {
        return false;
    }

    const SClearValue DepthClearValue(LightSetup.ShadowMapFormat, 1.0f, 0);

    LightSetup.PointLightShadowMaps = RHICreateTextureCubeArray(LightSetup.ShadowMapFormat, LightSetup.PointLightShadowSize, 1, LightSetup.MaxPointLightShadows, TextureFlags_ShadowMap, EResourceAccess::PixelShaderResource, nullptr, DepthClearValue);
    if (LightSetup.PointLightShadowMaps)
    {
        LightSetup.PointLightShadowMaps->SetName("PointLight ShadowMaps");

        LightSetup.PointLightShadowMapDSVs.Resize(LightSetup.MaxPointLightShadows);
        for (uint32 i = 0; i < LightSetup.MaxPointLightShadows; i++)
        {
            for (uint32 Face = 0; Face < 6; Face++)
            {
                TStaticArray<TSharedRef<CRHIDepthStencilView>, 6>& DepthCube = LightSetup.PointLightShadowMapDSVs[i];
                DepthCube[Face] = RHICreateDepthStencilView(LightSetup.PointLightShadowMaps.Get(), LightSetup.ShadowMapFormat, GetCubeFaceFromIndex(Face), 0, i);
                if (!DepthCube[Face])
                {
                    CDebug::DebugBreak();
                    return false;
                }
            }
        }
    }
    else
    {
        return false;
    }

    for (uint32 i = 0; i < NUM_SHADOW_CASCADES; i++)
    {
        const uint16 CascadeSize = LightSetup.CascadeSize;

        LightSetup.ShadowMapCascades[i] = RHICreateTexture2D(LightSetup.ShadowMapFormat, CascadeSize, CascadeSize, 1, 1, TextureFlags_ShadowMap, EResourceAccess::NonPixelShaderResource, nullptr, DepthClearValue);
        if (LightSetup.ShadowMapCascades[i])
        {
            LightSetup.ShadowMapCascades[i]->SetName("Shadow Map Cascade[" + ToString(i) + "]");
        }
        else
        {
            return false;
        }
    }

    return true;
}

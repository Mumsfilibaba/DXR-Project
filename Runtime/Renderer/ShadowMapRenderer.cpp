#include "ShadowMapRenderer.h"
#include "MeshDrawCommand.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/Mesh.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Lights/DirectionalLight.h"

#include "Core/Math/Frustum.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"

#include "Renderer/Debug/GPUProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FShadowMapRenderer

bool FShadowMapRenderer::Init(FLightSetup& LightSetup, FFrameResources& FrameResources)
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
        FRHIConstantBufferInitializer CBInitializer(EBufferUsageFlags::Default, sizeof(FPerShadowMap));

        PerShadowMapBuffer = RHICreateConstantBuffer(CBInitializer);
        if (!PerShadowMapBuffer)
        {
            PlatformDebugBreak();
            return false;
        }

        {
            FShaderCompileInfo CompileInfo("Point_VSMain", EShaderModel::SM_6_0, EShaderStage::Vertex);
            if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                PlatformDebugBreak();
                return false;
            }
        }

        PointLightVertexShader = RHICreateVertexShader(ShaderCode);
        if (!PointLightVertexShader)
        {
            PlatformDebugBreak();
            return false;
        }

        {
            FShaderCompileInfo CompileInfo("Point_PSMain", EShaderModel::SM_6_0, EShaderStage::Pixel);
            if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                PlatformDebugBreak();
                return false;
            }
        }

        PointLightPixelShader = RHICreatePixelShader(ShaderCode);
        if (!PointLightPixelShader)
        {
            PlatformDebugBreak();
            return false;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc      = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable   = true;
        DepthStencilStateInitializer.DepthWriteMask = EDepthWriteMask::All;

        TSharedRef<FRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!DepthStencilState)
        {
            PlatformDebugBreak();
            return false;
        }

        FRHIRasterizerStateInitializer RasterizerInitializer;
        RasterizerInitializer.CullMode = ECullMode::Back;

        FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerInitializer);
        if (!RasterizerState)
        {
            PlatformDebugBreak();
            return false;
        }

        FRHIBlendStateInitializer BlendStateInitializer;

        FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!BlendState)
        {
            PlatformDebugBreak();
            return false;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.BlendState                         = BlendState.Get();
        PSOInitializer.DepthStencilState                  = DepthStencilState.Get();
        PSOInitializer.IBStripCutValue                    = IndexBufferStripCutValue_Disabled;
        PSOInitializer.VertexInputLayout                  = FrameResources.StdInputLayout.Get();
        PSOInitializer.PrimitiveTopologyType              = EPrimitiveTopologyType::Triangle;
        PSOInitializer.RasterizerState                    = RasterizerState.Get();
        PSOInitializer.SampleCount                        = 1;
        PSOInitializer.SampleQuality                      = 0;
        PSOInitializer.SampleMask                         = 0xffffffff;
        PSOInitializer.ShaderState.VertexShader           = PointLightVertexShader.Get();
        PSOInitializer.ShaderState.PixelShader            = PointLightPixelShader.Get();
        PSOInitializer.PipelineFormats.NumRenderTargets   = 0;
        PSOInitializer.PipelineFormats.DepthStencilFormat = LightSetup.ShadowMapFormat;

        PointLightPipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!PointLightPipelineState)
        {
            PlatformDebugBreak();
            return false;
        }
        else
        {
            PointLightPipelineState->SetName("Point ShadowMap PipelineState");
        }
    }

    // Cascaded shadow map
    {
        FRHIConstantBufferInitializer Initializer(EBufferUsageFlags::Default, sizeof(FPerCascade));

        PerCascadeBuffer = RHICreateConstantBuffer(Initializer);
        if (!PerCascadeBuffer)
        {
            PlatformDebugBreak();
            return false;
        }
        else
        {
            PerCascadeBuffer->SetName("Per Cascade Buffer");
        }

        {
            FShaderCompileInfo CompileInfo("Cascade_VSMain", EShaderModel::SM_6_0, EShaderStage::Vertex);
            if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                PlatformDebugBreak();
                return false;
            }
        }

        DirectionalLightShader = RHICreateVertexShader(ShaderCode);
        if (!DirectionalLightShader)
        {
            PlatformDebugBreak();
            return false;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc      = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable   = true;
        DepthStencilStateInitializer.DepthWriteMask = EDepthWriteMask::All;

        TSharedRef<FRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!DepthStencilState)
        {
            PlatformDebugBreak();
            return false;
        }

        FRHIRasterizerStateInitializer RasterizerInitializer;
        RasterizerInitializer.CullMode = ECullMode::Back;

        FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerInitializer);
        if (!RasterizerState)
        {
            PlatformDebugBreak();
            return false;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!BlendState)
        {
            PlatformDebugBreak();
            return false;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.BlendState                         = BlendState.Get();
        PSOInitializer.DepthStencilState                  = DepthStencilState.Get();
        PSOInitializer.IBStripCutValue                    = IndexBufferStripCutValue_Disabled;
        PSOInitializer.VertexInputLayout                  = FrameResources.StdInputLayout.Get();
        PSOInitializer.PrimitiveTopologyType              = EPrimitiveTopologyType::Triangle;
        PSOInitializer.RasterizerState                    = RasterizerState.Get();
        PSOInitializer.SampleCount                        = 1;
        PSOInitializer.SampleQuality                      = 0;
        PSOInitializer.SampleMask                         = 0xffffffff;
        PSOInitializer.ShaderState.VertexShader           = DirectionalLightShader.Get();
        PSOInitializer.ShaderState.PixelShader            = nullptr;
        PSOInitializer.PipelineFormats.NumRenderTargets   = 0;
        PSOInitializer.PipelineFormats.DepthStencilFormat = LightSetup.ShadowMapFormat;

        DirectionalLightPSO = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!DirectionalLightPSO)
        {
            PlatformDebugBreak();
            return false;
        }
        else
        {
            DirectionalLightPSO->SetName("ShadowMap PipelineState");
        }
    }

    // Cascade Matrix Generation
    {
        {
            FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
            if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/CascadeMatrixGen.hlsl", CompileInfo, ShaderCode))
            {
                PlatformDebugBreak();
                return false;
            }
        }

        CascadeGenShader = RHICreateComputeShader(ShaderCode);
        if (!CascadeGenShader)
        {
            PlatformDebugBreak();
            return false;
        }

        FRHIComputePipelineStateInitializer CascadePSO;
        CascadePSO.Shader = CascadeGenShader.Get();

        CascadeGen = RHICreateComputePipelineState(CascadePSO);
        if (!CascadeGen)
        {
            PlatformDebugBreak();
            return false;
        }
        else
        {
            CascadeGen->SetName("CascadeGen PSO");
        }
    }

    // Create buffers for cascade matrix generation
    {
        FRHIConstantBufferInitializer CBInitializer(EBufferUsageFlags::Default, sizeof(FCascadeGenerationInfo));

        CascadeGenerationData = RHICreateConstantBuffer(CBInitializer);
        if (!CascadeGenerationData)
        {
            PlatformDebugBreak();
            return false;
        }
        else
        {
            CascadeGenerationData->SetName("Cascade GenerationData");
        }

        FRHIGenericBufferInitializer GenericInitializer(EBufferUsageFlags::RWBuffer, NUM_SHADOW_CASCADES, sizeof(FCascadeMatrices), EResourceAccess::UnorderedAccess);

        LightSetup.CascadeMatrixBuffer = RHICreateGenericBuffer(GenericInitializer);
        if (!LightSetup.CascadeMatrixBuffer)
        {
            PlatformDebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeMatrixBuffer->SetName("Cascade MatrixBuffer");
        }

        FRHIBufferSRVInitializer SRVInitializer(LightSetup.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES);
        LightSetup.CascadeMatrixBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!LightSetup.CascadeMatrixBufferSRV)
        {
            PlatformDebugBreak();
            return false;
        }

        FRHIBufferUAVInitializer UAVInitializer(LightSetup.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES);
        LightSetup.CascadeMatrixBufferUAV = RHICreateUnorderedAccessView(UAVInitializer);
        if (!LightSetup.CascadeMatrixBufferUAV)
        {
            PlatformDebugBreak();
            return false;
        }

        GenericInitializer = FRHIGenericBufferInitializer(EBufferUsageFlags::RWBuffer, NUM_SHADOW_CASCADES, sizeof(FCascadeSplits), EResourceAccess::UnorderedAccess);

        LightSetup.CascadeSplitsBuffer = RHICreateGenericBuffer(GenericInitializer);
        if (!LightSetup.CascadeSplitsBuffer)
        {
            PlatformDebugBreak();
            return false;
        }
        else
        {
            LightSetup.CascadeSplitsBuffer->SetName("Cascade SplitBuffer");
        }

        SRVInitializer = FRHIBufferSRVInitializer(LightSetup.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES);
        LightSetup.CascadeSplitsBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!LightSetup.CascadeSplitsBufferSRV)
        {
            PlatformDebugBreak();
            return false;
        }

        UAVInitializer = FRHIBufferUAVInitializer(LightSetup.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES);
        LightSetup.CascadeSplitsBufferUAV = RHICreateUnorderedAccessView(UAVInitializer);
        if (!LightSetup.CascadeSplitsBufferUAV)
        {
            PlatformDebugBreak();
            return false;
        }
    }

    // Directional Light ShadowMask
    {
        {
            FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
            if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/DirectionalShadowMaskGen.hlsl", CompileInfo, ShaderCode))
            {
                PlatformDebugBreak();
                return false;
            }
        }

        DirectionalShadowMaskShader = RHICreateComputeShader(ShaderCode);
        if (!DirectionalShadowMaskShader)
        {
            PlatformDebugBreak();
            return false;
        }

        FRHIComputePipelineStateInitializer MaskPSO;
        MaskPSO.Shader = DirectionalShadowMaskShader.Get();

        DirectionalShadowMaskPSO = RHICreateComputePipelineState(MaskPSO);
        if (!DirectionalShadowMaskPSO)
        {
            PlatformDebugBreak();
            return false;
        }
        else
        {
            DirectionalShadowMaskPSO->SetName("Directional ShadowMask PSO");
        }
    }

    return true;
}

void FShadowMapRenderer::RenderPointLightShadows(FRHICommandList& CmdList, const FLightSetup& LightSetup, const FScene& Scene)
{
    //PointLightFrame++;
    //if (PointLightFrame > 6)
    //{
    //    bUpdatePointLight = true;
    //    PointLightFrame = 0;
    //}

    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);

    CmdList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Render PointLight ShadowMaps");

    //if (bUpdatePointLight)
    {
        GPU_TRACE_SCOPE(CmdList, "PointLight ShadowMaps");

        TRACE_SCOPE("Render PointLight ShadowMaps");

        CmdList.SetGraphicsPipelineState(PointLightPipelineState.Get());

        // PerObject Structs
        struct SShadowPerObject
        {
            FMatrix4 Matrix;
        } ShadowPerObjectBuffer;

        FPerShadowMap PerShadowMapData;
        for (int32 Cube = 0; Cube < LightSetup.PointLightShadowMapsGenerationData.Size(); ++Cube)
        {
            for (uint32 Face = 0; Face < 6; ++Face)
            {
                const uint32 ArrayIndex = (Cube * kRHINumCubeFaces) + Face;

                FRHIRenderPassInitializer RenderPass;
                RenderPass.DepthStencilView = FRHIDepthStencilView(LightSetup.PointLightShadowMaps.Get(), uint16(ArrayIndex), 0);

                CmdList.BeginRenderPass(RenderPass);

                // NOTE: For now, MetalRHI require a renderpass to be started for these two to be valid
                const uint32 PointLightShadowSize = LightSetup.PointLightShadowSize;
                CmdList.SetViewport(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 1.0f, 0.0f, 0.0f);
                CmdList.SetScissorRect(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0);

                auto& Data = LightSetup.PointLightShadowMapsGenerationData[Cube];
                PerShadowMapData.Matrix   = Data.Matrix[Face];
                PerShadowMapData.Position = Data.Position;
                PerShadowMapData.FarPlane = Data.FarPlane;

                CmdList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);

                CmdList.UpdateBuffer(PerShadowMapBuffer.Get(), 0, sizeof(FPerShadowMap), &PerShadowMapData);

                CmdList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

                CmdList.SetConstantBuffer(PointLightVertexShader.Get(), PerShadowMapBuffer.Get(), 0);
                CmdList.SetConstantBuffer(PointLightPixelShader.Get(), PerShadowMapBuffer.Get(), 0);

                // Draw all objects to depth buffer
                IConsoleVariable* GlobalFrustumCullEnabled = FConsoleManager::Get().FindVariable("Renderer.EnableFrustumCulling");
                if (GlobalFrustumCullEnabled && GlobalFrustumCullEnabled->GetBool())
                {
                    FFrustum CameraFrustum = FFrustum(Data.FarPlane, Data.ViewMatrix[Face], Data.ProjMatrix[Face]);
                    for (const FMeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                    {
                        FMatrix4 TransformMatrix = Command.CurrentActor->GetTransform().GetMatrix();
                        TransformMatrix = TransformMatrix.Transpose();

                        FVector3 Top = FVector3(&Command.Mesh->BoundingBox.Top.x);
                        Top = TransformMatrix.TransformPosition(Top);

                        FVector3 Bottom = FVector3(&Command.Mesh->BoundingBox.Bottom.x);
                        Bottom = TransformMatrix.TransformPosition(Bottom);

                        FAABB Box(Top, Bottom);
                        if (CameraFrustum.CheckAABB(Box))
                        {
                            CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
                            CmdList.SetIndexBuffer(Command.IndexBuffer);

                            ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

                            CmdList.Set32BitShaderConstants(PointLightVertexShader.Get(), &ShadowPerObjectBuffer, 16);

                            CmdList.DrawIndexedInstanced(Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0);
                        }
                    }
                }
                else
                {
                    for (const FMeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                    {
                        CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
                        CmdList.SetIndexBuffer(Command.IndexBuffer);

                        ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

                        CmdList.Set32BitShaderConstants(PointLightVertexShader.Get(), &ShadowPerObjectBuffer, 16);

                        CmdList.DrawIndexedInstanced(Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0);
                    }
                }

                CmdList.EndRenderPass();
            }
        }

        bUpdatePointLight = false;
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Render PointLight ShadowMaps");

    CmdList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
}

void FShadowMapRenderer::RenderDirectionalLightShadows(FRHICommandList& CmdList, const FLightSetup& LightSetup, const FFrameResources& FrameResources, const FScene& Scene)
{
    // Generate matrices for directional light
    {
        GPU_TRACE_SCOPE(CmdList, "Generate Cascade Matrices");

        CmdList.TransitionBuffer(LightSetup.CascadeMatrixBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
        CmdList.TransitionBuffer(LightSetup.CascadeSplitsBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

        FCascadeGenerationInfo GenerationInfo;
        GenerationInfo.CascadeSplitLambda = LightSetup.CascadeSplitLambda;
        GenerationInfo.LightUp            = LightSetup.DirectionalLightData.Up;
        GenerationInfo.LightDirection     = LightSetup.DirectionalLightData.Direction;
        GenerationInfo.CascadeResolution  = (float)LightSetup.CascadeSize;

        CmdList.TransitionBuffer(CascadeGenerationData.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);
        CmdList.UpdateBuffer(CascadeGenerationData.Get(), 0, sizeof(FCascadeGenerationInfo), &GenerationInfo);
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
        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Render DirectionalLight ShadowMaps");

        TRACE_SCOPE("Render DirectionalLight ShadowMaps");

        GPU_TRACE_SCOPE(CmdList, "DirectionalLight ShadowMaps");

        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[0].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[1].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[2].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[3].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);

        CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);
        CmdList.SetGraphicsPipelineState(DirectionalLightPSO.Get());

        // PerObject Structs
        struct SShadowPerObject
        {
            FMatrix4 Matrix;
        } ShadowPerObjectBuffer;

        FPerCascade PerCascadeData;
        for (uint32 i = 0; i < NUM_SHADOW_CASCADES; i++)
        {
            FRHIRenderPassInitializer RenderPass;
            RenderPass.DepthStencilView = FRHIDepthStencilView(LightSetup.ShadowMapCascades[i].Get());

            CmdList.BeginRenderPass(RenderPass);

            const uint16 CascadeSize = LightSetup.CascadeSize;
            CmdList.SetViewport(static_cast<float>(CascadeSize), static_cast<float>(CascadeSize), 0.0f, 1.0f, 0.0f, 0.0f);
            CmdList.SetScissorRect(CascadeSize, CascadeSize, 0, 0);

            PerCascadeData.CascadeIndex = i;

            CmdList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);

            CmdList.UpdateBuffer(PerCascadeBuffer.Get(), 0, sizeof(FPerCascade), &PerCascadeData);

            CmdList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

            CmdList.SetConstantBuffer(DirectionalLightShader.Get(), PerCascadeBuffer.Get(), 0);

            CmdList.SetShaderResourceView(DirectionalLightShader.Get(), LightSetup.CascadeMatrixBufferSRV.Get(), 0);

            // Draw all objects to shadow-map
            for (const FMeshDrawCommand& Command : Scene.GetMeshDrawCommands())
            {
                CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
                CmdList.SetIndexBuffer(Command.IndexBuffer);

                ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

                CmdList.Set32BitShaderConstants(DirectionalLightShader.Get(), &ShadowPerObjectBuffer, 16);

                CmdList.DrawIndexedInstanced(Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0);
            }

            CmdList.EndRenderPass();
        }

        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[0].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[1].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[2].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
        CmdList.TransitionTexture(LightSetup.ShadowMapCascades[3].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);

        CmdList.TransitionBuffer(LightSetup.CascadeMatrixBuffer.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);

        INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Render DirectionalLight ShadowMaps");
    }
}

void FShadowMapRenderer::RenderShadowMasks(FRHICommandList& CmdList, const FLightSetup& LightSetup, const FFrameResources& FrameResources)
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

        const FIntVector3 ThreadGroupXYZ = DirectionalShadowMaskShader->GetThreadGroupXYZ();
        const uint32 ThreadsX = NMath::DivideByMultiple(LightSetup.DirectionalShadowMask->GetWidth(), ThreadGroupXYZ.x);
        const uint32 ThreadsY = NMath::DivideByMultiple(LightSetup.DirectionalShadowMask->GetHeight(), ThreadGroupXYZ.y);
        CmdList.Dispatch(ThreadsX, ThreadsY, 1);

        CmdList.TransitionTexture(LightSetup.DirectionalShadowMask.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    }
}

bool FShadowMapRenderer::ResizeResources(uint32 Width, uint32 Height, FLightSetup& LightSetup)
{
    return CreateShadowMask(Width, Height, LightSetup);
}

void FShadowMapRenderer::Release()
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

bool FShadowMapRenderer::CreateShadowMask(uint32 Width, uint32 Height, FLightSetup& LightSetup)
{
    FRHITexture2DInitializer CascadeInitializer(LightSetup.ShadowMaskFormat, Width, Height, 1, 1, ETextureUsageFlags::RWTexture, EResourceAccess::NonPixelShaderResource);
    LightSetup.DirectionalShadowMask = RHICreateTexture2D(CascadeInitializer);
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

bool FShadowMapRenderer::CreateShadowMaps(FLightSetup& LightSetup, FFrameResources& FrameResources)
{
    const uint32 Width  = FrameResources.MainWindowViewport->GetWidth();
    const uint32 Height = FrameResources.MainWindowViewport->GetHeight();

    if (!CreateShadowMask(Width, Height, LightSetup))
    {
        return false;
    }

    const FTextureClearValue DepthClearValue(LightSetup.ShadowMapFormat, 1.0f, 0);

    FRHITextureCubeArrayInitializer PointLightInitializer( LightSetup.ShadowMapFormat
                                                         , LightSetup.PointLightShadowSize
                                                         , LightSetup.MaxPointLightShadows
                                                         , 1
                                                         , 1
                                                         , ETextureUsageFlags::ShadowMap
                                                         , EResourceAccess::PixelShaderResource
                                                         , nullptr
                                                         , DepthClearValue);

    LightSetup.PointLightShadowMaps = RHICreateTextureCubeArray(PointLightInitializer);
    if (LightSetup.PointLightShadowMaps)
    {
        LightSetup.PointLightShadowMaps->SetName("PointLight ShadowMaps");
    }
    else
    {
        return false;
    }

    const uint16 CascadeSize = LightSetup.CascadeSize;
    
    FRHITexture2DInitializer CascadeInitializer( LightSetup.ShadowMapFormat
                                               , CascadeSize
                                               , CascadeSize
                                               , 1
                                               , 1
                                               , ETextureUsageFlags::ShadowMap
                                               , EResourceAccess::NonPixelShaderResource
                                               , nullptr
                                               , DepthClearValue);

    for (uint32 Cascade = 0; Cascade < NUM_SHADOW_CASCADES; ++Cascade)
    {
        LightSetup.ShadowMapCascades[Cascade] = RHICreateTexture2D(CascadeInitializer);
        if (LightSetup.ShadowMapCascades[Cascade])
        {
            LightSetup.ShadowMapCascades[Cascade]->SetName("Shadow Map Cascade[" + ToString(Cascade) + "]");
        }
        else
        {
            return false;
        }
    }

    return true;
}

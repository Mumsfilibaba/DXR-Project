#include "ShadowMapRenderer.h"
#include "MeshDrawCommand.h"
#include "Core/Math/Frustum.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "RHI/RHIShaderCompiler.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Lights/DirectionalLight.h"
#include "Renderer/Debug/GPUProfiler.h"

TAutoConsoleVariable<bool> GCascadeDebug(
    "Renderer.Debug.DrawCascades",
    "Draws an overlay that shows which pixel uses what shadow cascade",
    false);

bool FShadowMapRenderer::Initialize(FLightSetup& LightSetup, FFrameResources& FrameResources)
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
        FRHIBufferDesc PerShadowMapBufferDesc(sizeof(FPerShadowMap), sizeof(FPerShadowMap), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
        PerShadowMapBuffer = RHICreateBuffer(PerShadowMapBufferDesc, EResourceAccess::VertexAndConstantBuffer, nullptr);
        if (!PerShadowMapBuffer)
        {
            DEBUG_BREAK();
            return false;
        }

        {
            FRHIShaderCompileInfo CompileInfo("Point_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex);
            if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return false;
            }
        }

        PointLightVertexShader = RHICreateVertexShader(ShaderCode);
        if (!PointLightVertexShader)
        {
            DEBUG_BREAK();
            return false;
        }

        {
            FRHIShaderCompileInfo CompileInfo("Point_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel);
            if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return false;
            }
        }

        PointLightPixelShader = RHICreatePixelShader(ShaderCode);
        if (!PointLightPixelShader)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = true;

        FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateInitializer RasterizerInitializer;
        RasterizerInitializer.CullMode = ECullMode::Back;

        FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerInitializer);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.BlendState                         = BlendState.Get();
        PSOInitializer.DepthStencilState                  = DepthStencilState.Get();
        PSOInitializer.bPrimitiveRestartEnable            = false;
        PSOInitializer.VertexInputLayout                  = FrameResources.MeshInputLayout.Get();
        PSOInitializer.PrimitiveTopology                  = EPrimitiveTopology::TriangleList;
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
            DEBUG_BREAK();
            return false;
        }
        else
        {
            PointLightPipelineState->SetName("Point ShadowMap PipelineState");
        }
    }

    // Cascaded shadow map
    {
        FRHIBufferDesc PerCascadeBufferDesc(sizeof(FPerCascade), sizeof(FPerCascade), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
        PerCascadeBuffer = RHICreateBuffer(PerCascadeBufferDesc, EResourceAccess::VertexAndConstantBuffer, nullptr);
        if (!PerCascadeBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            PerCascadeBuffer->SetName("Per Cascade Buffer");
        }

        FRHIShaderCompileInfo CompileInfo("Cascade_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        DirectionalLightVS = RHICreateVertexShader(ShaderCode);
        if (!DirectionalLightVS)
        {
            DEBUG_BREAK();
            return false;
        }

        TArray<FShaderDefine> Defines =
        {
            { "ENABLE_ALPHA_MASK", "(1)" }
        };

        CompileInfo = FRHIShaderCompileInfo("Cascade_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, Defines);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        DirectionalLightMaskedVS = RHICreateVertexShader(ShaderCode);
        if (!DirectionalLightMaskedVS)
        {
            DEBUG_BREAK();
            return false;
        }

        CompileInfo = FRHIShaderCompileInfo("Cascade_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, Defines);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        DirectionalLightMaskedPS = RHICreatePixelShader(ShaderCode);
        if (!DirectionalLightMaskedPS)
        {
            DEBUG_BREAK();
            return false;
        }

        // Initialize standard input layout
        FRHIVertexInputLayoutInitializer InputLayoutInitializer =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, EVertexInputClass::Vertex, 0 }
        };

        FRHIVertexInputLayoutRef InputLayout = RHICreateVertexInputLayout(InputLayoutInitializer);
        if (!InputLayout)
        {
            DEBUG_BREAK();
            return false;
        }

        // Initialize standard input layout
        InputLayoutInitializer =
        {
            { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexMasked), 0, 0,  EVertexInputClass::Vertex, 0 },
            { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexMasked), 0, 12, EVertexInputClass::Vertex, 0 }
        };

        FRHIVertexInputLayoutRef MaskedInputLayout = RHICreateVertexInputLayout(InputLayoutInitializer);
        if (!MaskedInputLayout)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = true;

        FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateInitializer RasterizerInitializer;
        RasterizerInitializer.CullMode = ECullMode::Back;

        FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerInitializer);
        if (!RasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.BlendState                         = BlendState.Get();
        PSOInitializer.DepthStencilState                  = DepthStencilState.Get();
        PSOInitializer.bPrimitiveRestartEnable            = false;
        PSOInitializer.VertexInputLayout                  = InputLayout.Get();
        PSOInitializer.PrimitiveTopology                  = EPrimitiveTopology::TriangleList;
        PSOInitializer.RasterizerState                    = RasterizerState.Get();
        PSOInitializer.SampleCount                        = 1;
        PSOInitializer.SampleQuality                      = 0;
        PSOInitializer.SampleMask                         = 0xffffffff;
        PSOInitializer.ShaderState.VertexShader           = DirectionalLightVS.Get();
        PSOInitializer.ShaderState.PixelShader            = nullptr;
        PSOInitializer.PipelineFormats.NumRenderTargets   = 0;
        PSOInitializer.PipelineFormats.DepthStencilFormat = LightSetup.ShadowMapFormat;

        DirectionalLightPSO = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!DirectionalLightPSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            DirectionalLightPSO->SetName("CSM PipelineState");
        }

        PSOInitializer.VertexInputLayout        = MaskedInputLayout.Get();
        PSOInitializer.ShaderState.VertexShader = DirectionalLightMaskedVS.Get();
        PSOInitializer.ShaderState.PixelShader  = DirectionalLightMaskedPS.Get();

        DirectionalLightMaskedPSO = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!DirectionalLightMaskedPSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            DirectionalLightMaskedPSO->SetName("Masked CSM PipelineState");
        }
    }

    // Cascade Matrix Generation
    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/CascadeMatrixGen.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        CascadeGenShader = RHICreateComputeShader(ShaderCode);
        if (!CascadeGenShader)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIComputePipelineStateInitializer CascadePSO;
        CascadePSO.Shader = CascadeGenShader.Get();

        CascadeGen = RHICreateComputePipelineState(CascadePSO);
        if (!CascadeGen)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            CascadeGen->SetName("CascadeGen PSO");
        }
    }

    // Create buffers for cascade matrix generation
    {
        FRHIBufferDesc CascadeGenerationDataDesc(sizeof(FCascadeGenerationInfo), sizeof(FCascadeGenerationInfo), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
        CascadeGenerationData = RHICreateBuffer(CascadeGenerationDataDesc, EResourceAccess::VertexAndConstantBuffer, nullptr);
        if (!CascadeGenerationData)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            CascadeGenerationData->SetName("Cascade GenerationData");
        }

        FRHIBufferDesc CascadeMatrixBufferDesc(sizeof(FCascadeMatrices) * NUM_SHADOW_CASCADES, sizeof(FCascadeMatrices), EBufferUsageFlags::Default | EBufferUsageFlags::RWBuffer);
        LightSetup.CascadeMatrixBuffer = RHICreateBuffer(CascadeMatrixBufferDesc, EResourceAccess::UnorderedAccess, nullptr);
        if (!LightSetup.CascadeMatrixBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            LightSetup.CascadeMatrixBuffer->SetName("Cascade MatrixBuffer");
        }

        FRHIBufferSRVDesc SRVInitializer(LightSetup.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES);
        LightSetup.CascadeMatrixBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!LightSetup.CascadeMatrixBufferSRV)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBufferUAVDesc UAVInitializer(LightSetup.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES);
        LightSetup.CascadeMatrixBufferUAV = RHICreateUnorderedAccessView(UAVInitializer);
        if (!LightSetup.CascadeMatrixBufferUAV)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBufferDesc CascadeSplitsBufferDesc(sizeof(FCascadeSplits) * NUM_SHADOW_CASCADES, sizeof(FCascadeSplits), EBufferUsageFlags::Default | EBufferUsageFlags::RWBuffer);
        LightSetup.CascadeSplitsBuffer = RHICreateBuffer(CascadeSplitsBufferDesc, EResourceAccess::UnorderedAccess, nullptr);
        if (!LightSetup.CascadeSplitsBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            LightSetup.CascadeSplitsBuffer->SetName("Cascade SplitBuffer");
        }

        SRVInitializer = FRHIBufferSRVDesc(LightSetup.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES);
        LightSetup.CascadeSplitsBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!LightSetup.CascadeSplitsBufferSRV)
        {
            DEBUG_BREAK();
            return false;
        }

        UAVInitializer = FRHIBufferUAVDesc(LightSetup.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES);
        LightSetup.CascadeSplitsBufferUAV = RHICreateUnorderedAccessView(UAVInitializer);
        if (!LightSetup.CascadeSplitsBufferUAV)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    // Directional Light ShadowMask
    {
        {
            FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
            if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/DirectionalShadowMaskGen.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return false;
            }
        }

        DirectionalShadowMaskShader = RHICreateComputeShader(ShaderCode);
        if (!DirectionalShadowMaskShader)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIComputePipelineStateInitializer MaskPSOInitializer(DirectionalShadowMaskShader.Get());
        DirectionalShadowMaskPSO = RHICreateComputePipelineState(MaskPSOInitializer);
        if (!DirectionalShadowMaskPSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            DirectionalShadowMaskPSO->SetName("Directional ShadowMask PSO");
        }
    }

    // Directional Light ShadowMask Debugging
    {
        {
            TArray<FShaderDefine> Defines =
            {
                { "ENABLE_DEBUG", "(1)" },
            };

            FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, Defines);
            if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/DirectionalShadowMaskGen.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return false;
            }
        }

        DirectionalShadowMaskShader_Debug = RHICreateComputeShader(ShaderCode);
        if (!DirectionalShadowMaskShader_Debug)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIComputePipelineStateInitializer MaskPSOInitializer(DirectionalShadowMaskShader_Debug.Get());
        DirectionalShadowMaskPSO_Debug = RHICreateComputePipelineState(MaskPSOInitializer);
        if (!DirectionalShadowMaskPSO_Debug)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            DirectionalShadowMaskPSO_Debug->SetName("Directional ShadowMask PSO Debug");
        }
    }

    return true;
}

void FShadowMapRenderer::RenderPointLightShadows(FRHICommandList& CommandList, const FLightSetup& LightSetup, const FScene& Scene)
{
    //PointLightFrame++;
    //if (PointLightFrame > 6)
    //{
    //    bUpdatePointLight = true;
    //    PointLightFrame = 0;
    //}

    CommandList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render PointLight ShadowMaps");

    //if (bUpdatePointLight)
    {
        GPU_TRACE_SCOPE(CommandList, "PointLight ShadowMaps");

        TRACE_SCOPE("Render PointLight ShadowMaps");

        CommandList.SetGraphicsPipelineState(PointLightPipelineState.Get());

        // PerObject Structs
        struct FShadowPerObject
        {
            FMatrix4 Matrix;
        } ShadowPerObjectBuffer;

        FPerShadowMap PerShadowMapData;
        for (int32 Cube = 0; Cube < LightSetup.PointLightShadowMapsGenerationData.Size(); ++Cube)
        {
            for (uint32 Face = 0; Face < 6; ++Face)
            {
                const uint32 ArrayIndex = (Cube * kRHINumCubeFaces) + Face;

                auto& Data = LightSetup.PointLightShadowMapsGenerationData[Cube];
                PerShadowMapData.Matrix   = Data.Matrix[Face];
                PerShadowMapData.Position = Data.Position;
                PerShadowMapData.FarPlane = Data.FarPlane;

                CommandList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);
                CommandList.UpdateBuffer(PerShadowMapBuffer.Get(), FBufferRegion(0, sizeof(FPerShadowMap)), &PerShadowMapData);
                CommandList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

                FRHIRenderPassDesc RenderPass;
                RenderPass.DepthStencilView = FRHIDepthStencilView(LightSetup.PointLightShadowMaps.Get(), uint16(ArrayIndex), 0);

                CommandList.BeginRenderPass(RenderPass);

                const uint32 PointLightShadowSize = LightSetup.PointLightShadowSize;
                FRHIViewportRegion ViewportRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 0.0f, 0.0f, 1.0f);
                CommandList.SetViewport(ViewportRegion);

                FRHIScissorRegion ScissorRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0);
                CommandList.SetScissorRect(ScissorRegion);

                CommandList.SetConstantBuffer(PointLightVertexShader.Get(), PerShadowMapBuffer.Get(), 0);
                CommandList.SetConstantBuffer(PointLightPixelShader.Get(), PerShadowMapBuffer.Get(), 0);

                // Draw all objects to depth buffer
                static IConsoleVariable* GFrustumCullEnabled = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.FrustumCulling");
                if (GFrustumCullEnabled && GFrustumCullEnabled->GetBool())
                {
                    const FFrustum CameraFrustum = FFrustum(Data.FarPlane, Data.ViewMatrix[Face], Data.ProjMatrix[Face]);
                    for (const FMeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                    {
                        FMatrix4 TransformMatrix = Command.CurrentActor->GetTransform().GetMatrix();
                        TransformMatrix = TransformMatrix.Transpose();

                        const FVector3 Top    = TransformMatrix.Transform(Command.Mesh->BoundingBox.Top);
                        const FVector3 Bottom = TransformMatrix.Transform(Command.Mesh->BoundingBox.Bottom);

                        FAABB Box(Top, Bottom);
                        if (CameraFrustum.CheckAABB(Box))
                        {
                            CommandList.SetVertexBuffers(MakeArrayView(&Command.VertexBuffer, 1), 0);
                            CommandList.SetIndexBuffer(Command.IndexBuffer, Command.IndexFormat);

                            ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();
                            CommandList.Set32BitShaderConstants(PointLightVertexShader.Get(), &ShadowPerObjectBuffer, 16);

                            CommandList.DrawIndexedInstanced(Command.NumIndices, 1, 0, 0, 0);
                        }
                    }
                }
                else
                {
                    for (const FMeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                    {
                        CommandList.SetVertexBuffers(MakeArrayView(&Command.VertexBuffer, 1), 0);
                        CommandList.SetIndexBuffer(Command.IndexBuffer, Command.IndexFormat);

                        ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();
                        CommandList.Set32BitShaderConstants(PointLightVertexShader.Get(), &ShadowPerObjectBuffer, 16);

                        CommandList.DrawIndexedInstanced(Command.NumIndices, 1, 0, 0, 0);
                    }
                }

                CommandList.EndRenderPass();
            }
        }

        bUpdatePointLight = false;
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render PointLight ShadowMaps");

    CommandList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
}

void FShadowMapRenderer::RenderDirectionalLightShadows(FRHICommandList& CommandList, const FLightSetup& LightSetup, const FFrameResources& FrameResources, const FScene& Scene)
{
    // Generate matrices for directional light
    {
        GPU_TRACE_SCOPE(CommandList, "Generate Cascade Matrices");

        CommandList.TransitionBuffer(LightSetup.CascadeMatrixBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
        CommandList.TransitionBuffer(LightSetup.CascadeSplitsBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

        FCascadeGenerationInfo GenerationInfo;
        GenerationInfo.CascadeSplitLambda = LightSetup.CascadeSplitLambda;
        GenerationInfo.LightUp            = LightSetup.DirectionalLightData.UpVector;
        GenerationInfo.LightDirection     = LightSetup.DirectionalLightData.Direction;
        GenerationInfo.CascadeResolution  = (float)LightSetup.CascadeSize;
        GenerationInfo.ShadowMatrix       = LightSetup.DirectionalLightData.ShadowMatrix;

        CommandList.TransitionBuffer(CascadeGenerationData.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);
        CommandList.UpdateBuffer(CascadeGenerationData.Get(), FBufferRegion(0, sizeof(FCascadeGenerationInfo)), &GenerationInfo);
        CommandList.TransitionBuffer(CascadeGenerationData.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

        CommandList.SetComputePipelineState(CascadeGen.Get());

        CommandList.SetConstantBuffer(CascadeGenShader.Get(), FrameResources.CameraBuffer.Get(), 0);
        CommandList.SetConstantBuffer(CascadeGenShader.Get(), CascadeGenerationData.Get(), 1);

        CommandList.SetUnorderedAccessView(CascadeGenShader.Get(), LightSetup.CascadeMatrixBufferUAV.Get(), 0);
        CommandList.SetUnorderedAccessView(CascadeGenShader.Get(), LightSetup.CascadeSplitsBufferUAV.Get(), 1);

        CommandList.SetShaderResourceView(CascadeGenShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);

        CommandList.Dispatch(NUM_SHADOW_CASCADES, 1, 1);

        CommandList.TransitionBuffer(LightSetup.CascadeMatrixBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionBuffer(LightSetup.CascadeSplitsBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    }

    // Render directional shadows
    {
        INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render DirectionalLight ShadowMaps");

        TRACE_SCOPE("Render DirectionalLight ShadowMaps");

        GPU_TRACE_SCOPE(CommandList, "DirectionalLight ShadowMaps");

        CommandList.TransitionTexture(LightSetup.ShadowMapCascades[0].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
        CommandList.TransitionTexture(LightSetup.ShadowMapCascades[1].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
        CommandList.TransitionTexture(LightSetup.ShadowMapCascades[2].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
        CommandList.TransitionTexture(LightSetup.ShadowMapCascades[3].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);

        // PerObject Structs
        struct FShadowPerObject
        {
            FMatrix4 Matrix;
        } ShadowPerObjectBuffer;

        FPerCascade PerCascadeData;
        for (uint32 Index = 0; Index < NUM_SHADOW_CASCADES; ++Index)
        {
            PerCascadeData.CascadeIndex = Index;

            CommandList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::VertexAndConstantBuffer, EResourceAccess::CopyDest);
            CommandList.UpdateBuffer(PerCascadeBuffer.Get(), FBufferRegion(0, sizeof(FPerCascade)), &PerCascadeData);
            CommandList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::VertexAndConstantBuffer);

            FRHIRenderPassDesc RenderPass;
            RenderPass.DepthStencilView = FRHIDepthStencilView(LightSetup.ShadowMapCascades[Index].Get());

            CommandList.BeginRenderPass(RenderPass);

            const uint16 CascadeSize = LightSetup.CascadeSize;

            FRHIViewportRegion ViewportRegion(static_cast<float>(CascadeSize), static_cast<float>(CascadeSize), 0.0f, 0.0f, 0.0f, 1.0f);
            CommandList.SetViewport(ViewportRegion);

            FRHIScissorRegion ScissorRegion(CascadeSize, CascadeSize, 0, 0);
            CommandList.SetScissorRect(ScissorRegion);

            CommandList.SetConstantBuffer(DirectionalLightVS.Get(), PerCascadeBuffer.Get(), 0);
            CommandList.SetShaderResourceView(DirectionalLightVS.Get(), LightSetup.CascadeMatrixBufferSRV.Get(), 0);

            // Draw all objects to shadow-map
            static IConsoleVariable* GFrustumCullEnabled = FConsoleManager::Get().FindConsoleVariable("Renderer.Feature.FrustumCulling");
            if (GFrustumCullEnabled && GFrustumCullEnabled->GetBool())
            {
                const FFrustum CameraFrustum = FFrustum(LightSetup.DirectionalLightFarPlane, LightSetup.DirectionalLightViewMatrix, LightSetup.DirectionalLightProjMatrix);
                for (const FMeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                {
                    FMatrix4 TransformMatrix = Command.CurrentActor->GetTransform().GetMatrix();
                    TransformMatrix = TransformMatrix.Transpose();

                    const FVector3 Top    = TransformMatrix.Transform(Command.Mesh->BoundingBox.Top);
                    const FVector3 Bottom = TransformMatrix.Transform(Command.Mesh->BoundingBox.Bottom);

                    const FAABB Box(Top, Bottom);
                    if (CameraFrustum.CheckAABB(Box))
                    {
                        if (Command.Material->HasAlphaMask() || Command.Material->IsDoubleSided())
                        {
                            CommandList.SetGraphicsPipelineState(DirectionalLightMaskedPSO.Get());
                            CommandList.SetVertexBuffers(MakeArrayView(&Command.Mesh->MaskedVertexBuffer, 1), 0);

                            CommandList.SetSamplerState(DirectionalLightMaskedPS.Get(), Command.Material->GetMaterialSampler(), 0);
                            CommandList.SetConstantBuffer(DirectionalLightMaskedPS.Get(), Command.Material->GetMaterialBuffer(), 1);
                            CommandList.SetShaderResourceView(DirectionalLightMaskedPS.Get(), Command.Material->GetAlphaMaskSRV(), 1);
                        }
                        else
                        {
                            CommandList.SetVertexBuffers(MakeArrayView(&Command.Mesh->PosOnlyVertexBuffer, 1), 0);
                            CommandList.SetGraphicsPipelineState(DirectionalLightPSO.Get());
                        }

                        CommandList.SetIndexBuffer(Command.IndexBuffer, Command.IndexFormat);

                        ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();
                        CommandList.Set32BitShaderConstants(DirectionalLightVS.Get(), &ShadowPerObjectBuffer, 16);

                        CommandList.DrawIndexedInstanced(Command.NumIndices, 1, 0, 0, 0);
                    }
                }
            }
            else
            {
                for (const FMeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                {
                    if (Command.Material->HasAlphaMask() || Command.Material->IsDoubleSided())
                    {
                        CommandList.SetGraphicsPipelineState(DirectionalLightMaskedPSO.Get());
                        CommandList.SetVertexBuffers(MakeArrayView(&Command.Mesh->MaskedVertexBuffer, 1), 0);

                        CommandList.SetSamplerState(DirectionalLightMaskedPS.Get(), Command.Material->GetMaterialSampler(), 0);
                        CommandList.SetConstantBuffer(DirectionalLightMaskedPS.Get(), Command.Material->GetMaterialBuffer(), 1);
                        CommandList.SetShaderResourceView(DirectionalLightMaskedPS.Get(), Command.Material->GetAlphaMaskSRV(), 1);
                    }
                    else
                    {
                        CommandList.SetVertexBuffers(MakeArrayView(&Command.Mesh->PosOnlyVertexBuffer, 1), 0);
                        CommandList.SetGraphicsPipelineState(DirectionalLightPSO.Get());
                    }

                    CommandList.SetIndexBuffer(Command.IndexBuffer, Command.IndexFormat);

                    ShadowPerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();
                    CommandList.Set32BitShaderConstants(DirectionalLightVS.Get(), &ShadowPerObjectBuffer, 16);

                    CommandList.DrawIndexedInstanced(Command.NumIndices, 1, 0, 0, 0);
                }
            }

            CommandList.EndRenderPass();
        }

        CommandList.TransitionTexture(LightSetup.ShadowMapCascades[0].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(LightSetup.ShadowMapCascades[1].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(LightSetup.ShadowMapCascades[2].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(LightSetup.ShadowMapCascades[3].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);

        INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render DirectionalLight ShadowMaps");
    }
}

void FShadowMapRenderer::RenderShadowMasks(FRHICommandList& CommandList, const FLightSetup& LightSetup, const FFrameResources& FrameResources)
{
    // Generate Directional Shadow Mask
    {
        INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render ShadowMasks");

        TRACE_SCOPE("Render ShadowMasks");

        GPU_TRACE_SCOPE(CommandList, "DirectionalLight Shadow Mask");

        CommandList.TransitionTexture(LightSetup.DirectionalShadowMask.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

        FRHIComputeShaderRef CurrentShadowMaskShader;
        if (GCascadeDebug.GetValue())
        {
            CommandList.TransitionTexture(LightSetup.CascadeIndexBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

            CurrentShadowMaskShader = DirectionalShadowMaskShader_Debug;
            CommandList.SetComputePipelineState(DirectionalShadowMaskPSO_Debug.Get());
        }
        else
        {
            CurrentShadowMaskShader = DirectionalShadowMaskShader;
            CommandList.SetComputePipelineState(DirectionalShadowMaskPSO.Get());
        }

        CommandList.SetConstantBuffer(CurrentShadowMaskShader.Get(), FrameResources.CameraBuffer.Get(), 0);
        CommandList.SetConstantBuffer(CurrentShadowMaskShader.Get(), LightSetup.DirectionalLightsBuffer.Get(), 1);

        CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), LightSetup.CascadeMatrixBufferSRV.Get(), 0);
        CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), LightSetup.CascadeSplitsBufferSRV.Get(), 1);

        CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 2);
        CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), FrameResources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView(), 3);

        CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), LightSetup.ShadowMapCascades[0]->GetShaderResourceView(), 4);
        CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), LightSetup.ShadowMapCascades[1]->GetShaderResourceView(), 5);
        CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), LightSetup.ShadowMapCascades[2]->GetShaderResourceView(), 6);
        CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), LightSetup.ShadowMapCascades[3]->GetShaderResourceView(), 7);

        CommandList.SetUnorderedAccessView(CurrentShadowMaskShader.Get(), LightSetup.DirectionalShadowMask->GetUnorderedAccessView(), 0);
        if (GCascadeDebug.GetValue())
        {
            CommandList.SetUnorderedAccessView(CurrentShadowMaskShader.Get(), LightSetup.CascadeIndexBuffer->GetUnorderedAccessView(), 1);
        }

        CommandList.SetSamplerState(CurrentShadowMaskShader.Get(), FrameResources.DirectionalLightShadowSampler.Get(), 0);

        constexpr uint32 NumThreads = 16;
        const uint32 ThreadsX = FMath::DivideByMultiple(LightSetup.DirectionalShadowMask->GetWidth(), NumThreads);
        const uint32 ThreadsY = FMath::DivideByMultiple(LightSetup.DirectionalShadowMask->GetHeight(), NumThreads);
        CommandList.Dispatch(ThreadsX, ThreadsY, 1);

        CommandList.TransitionTexture(LightSetup.DirectionalShadowMask.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);

        if (GCascadeDebug.GetValue())
        {
            CommandList.TransitionTexture(LightSetup.CascadeIndexBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
        }

        INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render ShadowMasks");
    }
}

bool FShadowMapRenderer::ResizeResources(FRHICommandList& CommandList, uint32 Width, uint32 Height, FLightSetup& LightSetup)
{
    // Destroy the old resources 
    CommandList.DestroyResource(LightSetup.DirectionalShadowMask.Get());
    CommandList.DestroyResource(LightSetup.CascadeIndexBuffer.Get());

    // Create the new resources
    return CreateShadowMask(Width, Height, LightSetup);
}

void FShadowMapRenderer::Release()
{
    PerShadowMapBuffer.Reset();

    DirectionalShadowMaskPSO.Reset();
    DirectionalShadowMaskShader.Reset();
    
    DirectionalShadowMaskPSO_Debug.Reset();
    DirectionalShadowMaskShader_Debug.Reset();
    
    DirectionalLightPSO.Reset();
    DirectionalLightVS.Reset();

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
    FRHITextureDesc ShadowMaskDesc = FRHITextureDesc::CreateTexture2D(LightSetup.ShadowMaskFormat, Width, Height, 1, 1, ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource);
    LightSetup.DirectionalShadowMask = RHICreateTexture(ShadowMaskDesc, EResourceAccess::NonPixelShaderResource);
    if (LightSetup.DirectionalShadowMask)
    {
        LightSetup.DirectionalShadowMask->SetName("Directional Shadow Mask 0");
    }
    else
    {
        return false;
    }

    FRHITextureDesc CascadeIndexBufferDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource);
    LightSetup.CascadeIndexBuffer = RHICreateTexture(CascadeIndexBufferDesc, EResourceAccess::NonPixelShaderResource);
    if (LightSetup.CascadeIndexBuffer)
    {
        LightSetup.CascadeIndexBuffer->SetName("Cascade Index Debug Buffer");
    }
    else
    {
        return false;
    }

    return true;
}

bool FShadowMapRenderer::CreateShadowMaps(FLightSetup& LightSetup, FFrameResources& FrameResources)
{
    const uint32 Width  = FrameResources.MainViewport->GetWidth();
    const uint32 Height = FrameResources.MainViewport->GetHeight();

    if (!CreateShadowMask(Width, Height, LightSetup))
    {
        return false;
    }

    const FClearValue DepthClearValue(LightSetup.ShadowMapFormat, 1.0f, 0);

    FRHITextureDesc PointLightDesc= FRHITextureDesc::CreateTextureCubeArray(
        LightSetup.ShadowMapFormat,
        LightSetup.PointLightShadowSize,
        LightSetup.MaxPointLightShadows,
        1,
        1,
        ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResource,
        DepthClearValue);

    LightSetup.PointLightShadowMaps = RHICreateTexture(PointLightDesc, EResourceAccess::PixelShaderResource);
    if (LightSetup.PointLightShadowMaps)
    {
        LightSetup.PointLightShadowMaps->SetName("PointLight ShadowMaps");
    }
    else
    {
        return false;
    }

    const uint16 CascadeSize = LightSetup.CascadeSize;
    
    FRHITextureDesc CascadeInitializer = FRHITextureDesc::CreateTexture2D(
        LightSetup.ShadowMapFormat,
        CascadeSize,
        CascadeSize,
        1,
        1,
        ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResource,
        DepthClearValue);

    for (uint32 Cascade = 0; Cascade < NUM_SHADOW_CASCADES; ++Cascade)
    {
        LightSetup.ShadowMapCascades[Cascade] = RHICreateTexture(CascadeInitializer, EResourceAccess::NonPixelShaderResource);
        if (LightSetup.ShadowMapCascades[Cascade])
        {
            LightSetup.ShadowMapCascades[Cascade]->SetName("Shadow Map Cascade[" + TTypeToString<int32>::ToString(Cascade) + "]");
        }
        else
        {
            return false;
        }
    }

    return true;
}

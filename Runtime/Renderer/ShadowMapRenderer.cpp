#include "ShadowMapRenderer.h"
#include "Scene.h"
#include "Core/Math/Frustum.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Engine/World/Components/ProxySceneComponent.h"
#include "Renderer/Debug/GPUProfiler.h"

static TAutoConsoleVariable<bool> CVarCascadeDebug(
    "Renderer.Debug.DrawCascades",
    "Draws an overlay that shows which pixel uses what shadow cascade",
    false);

static TAutoConsoleVariable<bool> CVarGPUGeneratedCascades(
    "Renderer.Feature.GPUGeneratedCascades",
    "Draws an overlay that shows which pixel uses what shadow cascade",
    true);

void FShadowMapRenderer::InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources) 
{
    // Point Shadow Maps
    const EMaterialFlags MaterialFlags = Material->GetMaterialFlags();

    FPipelineStateInstance* CachedPointLightPSO = PointLightPSOs.Find(MaterialFlags);
    if (!CachedPointLightPSO)
    {
        TArray<uint8>         ShaderCode;
        TArray<FShaderDefine> ShaderDefines;

        if (MaterialFlags & MaterialFlag_EnableHeight)
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(0)");

        if (MaterialFlags & MaterialFlag_PackedDiffuseAlpha)
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(0)");

        if (MaterialFlags & MaterialFlag_EnableAlpha)
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(0)");

        {
            FShaderCompileInfo CompileInfo("Point_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, ShaderDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return;
            }
        }

        FPipelineStateInstance NewPipelineStateInstance;
        NewPipelineStateInstance.VertexShader = RHICreateVertexShader(ShaderCode);
        if (!NewPipelineStateInstance.VertexShader)
        {
            DEBUG_BREAK();
            return;
        }

        {
            FShaderCompileInfo CompileInfo("Point_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, ShaderDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return;
            }
        }

        NewPipelineStateInstance.PixelShader = RHICreatePixelShader(ShaderCode);
        if (!NewPipelineStateInstance.PixelShader)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = true;

        NewPipelineStateInstance.DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!NewPipelineStateInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIRasterizerStateInitializer RasterizerInitializer;
        RasterizerInitializer.CullMode = ECullMode::Back;

        NewPipelineStateInstance.RasterizerState = RHICreateRasterizerState(RasterizerInitializer);
        if (!NewPipelineStateInstance.RasterizerState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        NewPipelineStateInstance.BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!NewPipelineStateInstance.BlendState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.BlendState                         = NewPipelineStateInstance.BlendState.Get();
        PSOInitializer.DepthStencilState                  = NewPipelineStateInstance.DepthStencilState.Get();
        PSOInitializer.bPrimitiveRestartEnable            = false;
        PSOInitializer.VertexInputLayout                  = FrameResources.MeshInputLayout.Get();
        PSOInitializer.PrimitiveTopology                  = EPrimitiveTopology::TriangleList;
        PSOInitializer.RasterizerState                    = NewPipelineStateInstance.RasterizerState.Get();
        PSOInitializer.SampleCount                        = 1;
        PSOInitializer.SampleQuality                      = 0;
        PSOInitializer.SampleMask                         = 0xffffffff;
        PSOInitializer.ShaderState.VertexShader           = NewPipelineStateInstance.VertexShader.Get();
        PSOInitializer.ShaderState.PixelShader            = NewPipelineStateInstance.PixelShader.Get();
        PSOInitializer.PipelineFormats.NumRenderTargets   = 0;
        PSOInitializer.PipelineFormats.DepthStencilFormat = FrameResources.ShadowMapFormat;

        NewPipelineStateInstance.PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!NewPipelineStateInstance.PipelineState)
        {
            DEBUG_BREAK();
            return;
        }
        else
        {
            const FString DebugName = FString::CreateFormatted("Point ShadowMap PipelineState %d", MaterialFlags);
            NewPipelineStateInstance.PipelineState->SetDebugName(DebugName);
        }

        PointLightPSOs.Add(MaterialFlags, Move(NewPipelineStateInstance));
    }

    // Cascaded shadow map
    FPipelineStateInstance* CachedDirectionalLightPSO = DirectionalLightPSOs.Find(MaterialFlags);
    if (!CachedDirectionalLightPSO)
    {
        TArray<uint8>         ShaderCode;
        TArray<FShaderDefine> ShaderDefines;

        if (MaterialFlags & MaterialFlag_EnableHeight)
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_PARALLAX_MAPPING", "(0)");

        if (MaterialFlags & MaterialFlag_PackedDiffuseAlpha)
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_PACKED_MATERIAL_TEXTURE", "(0)");

        if (MaterialFlags & MaterialFlag_EnableAlpha)
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(1)");
        else
            ShaderDefines.Emplace("ENABLE_ALPHA_MASK", "(0)");

        FShaderCompileInfo CompileInfo("Cascade_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return;
        }

        FPipelineStateInstance NewPipelineStateInstance;
        NewPipelineStateInstance.VertexShader = RHICreateVertexShader(ShaderCode);
        if (!NewPipelineStateInstance.VertexShader)
        {
            DEBUG_BREAK();
            return;
        }

        constexpr EMaterialFlags PSFlags = MaterialFlag_EnableHeight | MaterialFlag_PackedDiffuseAlpha | MaterialFlag_EnableAlpha;
        const bool bWantPixelShader = (MaterialFlags & PSFlags) != MaterialFlag_None;
        if (bWantPixelShader)
        {
            CompileInfo = FShaderCompileInfo("Cascade_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, ShaderDefines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return;
            }

            NewPipelineStateInstance.PixelShader = RHICreatePixelShader(ShaderCode);
            if (!NewPipelineStateInstance.PixelShader)
            {
                DEBUG_BREAK();
                return;
            }
        }

        // Initialize standard input layout
        FRHIVertexInputLayoutInitializer InputLayoutInitializer;
        if (MaterialFlags & (MaterialFlag_EnableHeight | MaterialFlag_EnableAlpha))
        {
            InputLayoutInitializer =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVertexMasked), 0, 0,  EVertexInputClass::Vertex, 0 },
                { "TEXCOORD", 0, EFormat::R32G32_Float,    sizeof(FVertexMasked), 0, 12, EVertexInputClass::Vertex, 0 }
            };
        }
        else
        {
            InputLayoutInitializer =
            {
                { "POSITION", 0, EFormat::R32G32B32_Float, sizeof(FVector3), 0, 0, EVertexInputClass::Vertex, 0 }
            };
        }

        NewPipelineStateInstance.InputLayout = RHICreateVertexInputLayout(InputLayoutInitializer);
        if (!NewPipelineStateInstance.InputLayout)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = true;

        NewPipelineStateInstance.DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!NewPipelineStateInstance.DepthStencilState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIRasterizerStateInitializer RasterizerInitializer;
        RasterizerInitializer.CullMode = ECullMode::Back;

        NewPipelineStateInstance.RasterizerState = RHICreateRasterizerState(RasterizerInitializer);
        if (!NewPipelineStateInstance.RasterizerState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        NewPipelineStateInstance.BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!NewPipelineStateInstance.BlendState)
        {
            DEBUG_BREAK();
            return;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.BlendState                         = NewPipelineStateInstance.BlendState.Get();
        PSOInitializer.DepthStencilState                  = NewPipelineStateInstance.DepthStencilState.Get();
        PSOInitializer.bPrimitiveRestartEnable            = false;
        PSOInitializer.VertexInputLayout                  = NewPipelineStateInstance.InputLayout.Get();
        PSOInitializer.PrimitiveTopology                  = EPrimitiveTopology::TriangleList;
        PSOInitializer.RasterizerState                    = NewPipelineStateInstance.RasterizerState.Get();
        PSOInitializer.SampleCount                        = 1;
        PSOInitializer.SampleQuality                      = 0;
        PSOInitializer.SampleMask                         = 0xffffffff;
        PSOInitializer.ShaderState.VertexShader           = NewPipelineStateInstance.VertexShader.Get();
        PSOInitializer.ShaderState.PixelShader            = NewPipelineStateInstance.PixelShader.Get();
        PSOInitializer.PipelineFormats.NumRenderTargets   = 0;
        PSOInitializer.PipelineFormats.DepthStencilFormat = FrameResources.ShadowMapFormat;

        NewPipelineStateInstance.PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!NewPipelineStateInstance.PipelineState)
        {
            DEBUG_BREAK();
            return;
        }
        else
        {
            const FString DebugName = FString::CreateFormatted("CSM PipelineState %d", MaterialFlags);
            NewPipelineStateInstance.PipelineState->SetDebugName(DebugName);
        }

        DirectionalLightPSOs.Add(MaterialFlags, Move(NewPipelineStateInstance));
    }
}

bool FShadowMapRenderer::Initialize(FLightSetup& LightSetup, FFrameResources& FrameResources)
{
    if (!CreateShadowMaps(LightSetup, FrameResources))
    {
        return false;
    }

    TArray<uint8> ShaderCode;

    // Point Shadow Maps
    {
        FRHIBufferDesc PerShadowMapBufferDesc(sizeof(FPerShadowMap), sizeof(FPerShadowMap), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
        PerShadowMapBuffer = RHICreateBuffer(PerShadowMapBufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!PerShadowMapBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            PerShadowMapBuffer->SetDebugName("Per ShadowMap Buffer");
        }
    }

    // Cascaded shadow map
    {
        FRHIBufferDesc PerCascadeBufferDesc(sizeof(FPerCascade), sizeof(FPerCascade), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
        PerCascadeBuffer = RHICreateBuffer(PerCascadeBufferDesc, EResourceAccess::ConstantBuffer, nullptr);
        if (!PerCascadeBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            PerCascadeBuffer->SetDebugName("Per Cascade Buffer");
        }
    }

    // Cascade Matrix Generation
    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/CascadeMatrixGen.hlsl", CompileInfo, ShaderCode))
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
            CascadeGen->SetDebugName("CascadeGen PSO");
        }
    }

    // Create buffers for cascade matrix generation
    {
        FRHIBufferDesc CascadeMatrixBufferDesc(sizeof(FCascadeMatrices) * NUM_SHADOW_CASCADES, sizeof(FCascadeMatrices), EBufferUsageFlags::Default | EBufferUsageFlags::RWBuffer);
        LightSetup.CascadeMatrixBuffer = RHICreateBuffer(CascadeMatrixBufferDesc, EResourceAccess::UnorderedAccess, nullptr);
        if (!LightSetup.CascadeMatrixBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            LightSetup.CascadeMatrixBuffer->SetDebugName("Cascade Matrices Buffer");
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

        FRHIBufferDesc CascadeSplitsBufferDesc(sizeof(FCascadeSplit) * NUM_SHADOW_CASCADES, sizeof(FCascadeSplit), EBufferUsageFlags::Default | EBufferUsageFlags::RWBuffer);
        LightSetup.CascadeSplitsBuffer = RHICreateBuffer(CascadeSplitsBufferDesc, EResourceAccess::UnorderedAccess, nullptr);
        if (!LightSetup.CascadeSplitsBuffer)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            LightSetup.CascadeSplitsBuffer->SetDebugName("Cascade SplitBuffer");
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
            FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/DirectionalShadowMaskGen.hlsl", CompileInfo, ShaderCode))
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
            DirectionalShadowMaskPSO->SetDebugName("Directional ShadowMask PSO");
        }
    }

    // Directional Light ShadowMask Debugging
    {
        {
            TArray<FShaderDefine> Defines =
            {
                { "ENABLE_DEBUG", "(1)" },
            };

            FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, Defines);
            if (!FShaderCompiler::Get().CompileFromFile("Shaders/DirectionalShadowMaskGen.hlsl", CompileInfo, ShaderCode))
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
            DirectionalShadowMaskPSO_Debug->SetDebugName("Directional ShadowMask PSO Debug");
        }
    }

    return true;
}

void FShadowMapRenderer::RenderPointLightShadows(FRHICommandList& CommandList, const FLightSetup& LightSetup, FScene* Scene)
{
    CommandList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render PointLight ShadowMaps");

    GPU_TRACE_SCOPE(CommandList, "PointLight ShadowMaps");

    TRACE_SCOPE("Render PointLight ShadowMaps");

    // PerObject Structs
    struct FShadowPerObject
    {
        FMatrix4 Matrix;
    } ShadowPerObjectBuffer;

    FPerShadowMap PerShadowMapData;
    for (int32 CubeIndex = 0; CubeIndex < Scene->PointLightViews.Size(); ++CubeIndex)
    {
        FLightView* LightView = Scene->PointLightViews[CubeIndex];
        for (uint32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; ++FaceIndex)
        {
            FLightView::FShadowData& Data = LightView->ShadowData[FaceIndex];
            PerShadowMapData.Matrix   = Data.Matrix;
            PerShadowMapData.Position = Data.Position;
            PerShadowMapData.FarPlane = Data.FarPlane;

            CommandList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
            CommandList.UpdateBuffer(PerShadowMapBuffer.Get(), FBufferRegion(0, sizeof(FPerShadowMap)), &PerShadowMapData);
            CommandList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

            const uint32 ArrayIndex = (CubeIndex * RHI_NUM_CUBE_FACES) + FaceIndex;
            FRHIRenderPassDesc RenderPass;
            RenderPass.DepthStencilView = FRHIDepthStencilView(LightSetup.PointLightShadowMaps.Get(), uint16(ArrayIndex), 0);

            CommandList.BeginRenderPass(RenderPass);

            const uint32 PointLightShadowSize = LightSetup.PointLightShadowSize;
            FViewportRegion ViewportRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 0.0f, 0.0f, 1.0f);
            CommandList.SetViewport(ViewportRegion);

            FScissorRegion ScissorRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0);
            CommandList.SetScissorRect(ScissorRegion);

            for (const FMeshBatch& Batch : LightView->MeshBatches[FaceIndex])
            {
                FPipelineStateInstance* Instance = PointLightPSOs.Find(Batch.Material->GetMaterialFlags());
                if (!Instance)
                {
                    DEBUG_BREAK();
                }

                FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                CHECK(PipelineState  != nullptr);
                CommandList.SetGraphicsPipelineState(PipelineState);

                CommandList.SetConstantBuffer(Instance->VertexShader.Get(), PerShadowMapBuffer.Get(), 0);
                CommandList.SetConstantBuffer(Instance->PixelShader.Get(), PerShadowMapBuffer.Get(), 0);

                for (FProxySceneComponent* Component : Batch.Primitives)
                {
                    CommandList.SetVertexBuffers(MakeArrayView(&Component->VertexBuffer, 1), 0);
                    CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

                    ShadowPerObjectBuffer.Matrix = Component->CurrentActor->GetTransform().GetMatrix();
                    CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &ShadowPerObjectBuffer, 16);

                    CommandList.DrawIndexedInstanced(Component->NumIndices, 1, 0, 0, 0);
                }
            }

            CommandList.EndRenderPass();
        }
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render PointLight ShadowMaps");

    CommandList.TransitionTexture(LightSetup.PointLightShadowMaps.Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
}

void FShadowMapRenderer::RenderDirectionalLightShadows(FRHICommandList& CommandList, const FLightSetup& LightSetup, const FFrameResources& FrameResources, FScene* Scene)
{
    // Generate matrices for directional light
    if (CVarGPUGeneratedCascades.GetValue())
    {
        GPU_TRACE_SCOPE(CommandList, "Generate Cascade Matrices");

        CommandList.TransitionBuffer(LightSetup.CascadeMatrixBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
        CommandList.TransitionBuffer(LightSetup.CascadeSplitsBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

        CommandList.SetComputePipelineState(CascadeGen.Get());

        CommandList.SetConstantBuffer(CascadeGenShader.Get(), FrameResources.CameraBuffer.Get(), 0);
        CommandList.SetConstantBuffer(CascadeGenShader.Get(), LightSetup.CascadeGenerationDataBuffer.Get(), 1);

        CommandList.SetUnorderedAccessView(CascadeGenShader.Get(), LightSetup.CascadeMatrixBufferUAV.Get(), 0);
        CommandList.SetUnorderedAccessView(CascadeGenShader.Get(), LightSetup.CascadeSplitsBufferUAV.Get(), 1);

        CommandList.SetShaderResourceView(CascadeGenShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);

        CommandList.Dispatch(1, 1, 1);

        CommandList.TransitionBuffer(LightSetup.CascadeMatrixBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionBuffer(LightSetup.CascadeSplitsBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    }

    // Render directional shadows
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

    for (uint32 Index = 0; Index < NUM_SHADOW_CASCADES; ++Index)
    {
        FPerCascade PerCascadeData;
        PerCascadeData.CascadeIndex = Index;

        CommandList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
        CommandList.UpdateBuffer(PerCascadeBuffer.Get(), FBufferRegion(0, sizeof(FPerCascade)), &PerCascadeData);
        CommandList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

        FRHIRenderPassDesc RenderPass;
        RenderPass.DepthStencilView = FRHIDepthStencilView(LightSetup.ShadowMapCascades[Index].Get());

        CommandList.BeginRenderPass(RenderPass);

        const float CascadeSize = static_cast<float>(LightSetup.CascadeSize);
        FViewportRegion ViewportRegion(CascadeSize, CascadeSize, 0.0f, 0.0f, 0.0f, 1.0f);
        CommandList.SetViewport(ViewportRegion);

        FScissorRegion ScissorRegion(CascadeSize, CascadeSize, 0, 0);
        CommandList.SetScissorRect(ScissorRegion);
             
        // Draw all objects to shadow-map
        CHECK(Scene->DirectionalLightIndex >= 0);

        FLightView& DirectionalLightView = Scene->LightViews[Scene->DirectionalLightIndex];
        for (int32 SubViewIndex = 0; SubViewIndex < DirectionalLightView.NumSubViews; SubViewIndex++)
        {
            for (const FMeshBatch& Batch : DirectionalLightView.MeshBatches[SubViewIndex])
            {
                FMaterial* Material = Batch.Material;

                FPipelineStateInstance* Instance = DirectionalLightPSOs.Find(Material->GetMaterialFlags());
                if (!Instance)
                {
                    DEBUG_BREAK();
                }

                FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                CHECK(PipelineState  != nullptr);
                CommandList.SetGraphicsPipelineState(PipelineState);

                if (Material->HasAlphaMask())
                {
                    CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
                    CommandList.SetConstantBuffer(Instance->PixelShader.Get(), Material->GetMaterialBuffer(), 1);
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 1);
                }

                CommandList.SetConstantBuffer(Instance->VertexShader.Get(), PerCascadeBuffer.Get(), 0);
                CommandList.SetShaderResourceView(Instance->VertexShader.Get(), LightSetup.CascadeMatrixBufferSRV.Get(), 0);

                for (const FProxySceneComponent* Component : Batch.Primitives)
                {
                    if (Material->HasHeightMap() || Material->HasAlphaMask())
                    {
                        CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->MaskedVertexBuffer, 1), 0);
                    }
                    else
                    {
                        CommandList.SetVertexBuffers(MakeArrayView(&Component->Mesh->PosOnlyVertexBuffer, 1), 0);
                    }

                    CommandList.SetIndexBuffer(Component->IndexBuffer, Component->IndexFormat);

                    ShadowPerObjectBuffer.Matrix = Component->CurrentActor->GetTransform().GetMatrix();
                    CommandList.Set32BitShaderConstants(Instance->VertexShader.Get(), &ShadowPerObjectBuffer, 16);

                    CommandList.DrawIndexedInstanced(Component->NumIndices, 1, 0, 0, 0);
                }
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

void FShadowMapRenderer::RenderShadowMasks(FRHICommandList& CommandList, const FLightSetup& LightSetup, const FFrameResources& FrameResources)
{
    // Generate Directional Shadow Mask
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render ShadowMasks");

    TRACE_SCOPE("Render ShadowMasks");

    GPU_TRACE_SCOPE(CommandList, "DirectionalLight Shadow Mask");

    CommandList.TransitionTexture(LightSetup.DirectionalShadowMask.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

    FRHIComputeShaderRef CurrentShadowMaskShader;
    if (CVarCascadeDebug.GetValue())
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
    CommandList.SetConstantBuffer(CurrentShadowMaskShader.Get(), LightSetup.DirectionalLightDataBuffer.Get(), 1);

    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), LightSetup.CascadeMatrixBufferSRV.Get(), 0);
    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), LightSetup.CascadeSplitsBufferSRV.Get(), 1);

    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), FrameResources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView(), 3);

    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), LightSetup.ShadowMapCascades[0]->GetShaderResourceView(), 4);
    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), LightSetup.ShadowMapCascades[1]->GetShaderResourceView(), 5);
    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), LightSetup.ShadowMapCascades[2]->GetShaderResourceView(), 6);
    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), LightSetup.ShadowMapCascades[3]->GetShaderResourceView(), 7);

    CommandList.SetUnorderedAccessView(CurrentShadowMaskShader.Get(), LightSetup.DirectionalShadowMask->GetUnorderedAccessView(), 0);
    if (CVarCascadeDebug.GetValue())
    {
        CommandList.SetUnorderedAccessView(CurrentShadowMaskShader.Get(), LightSetup.CascadeIndexBuffer->GetUnorderedAccessView(), 1);
    }

    CommandList.SetSamplerState(CurrentShadowMaskShader.Get(), FrameResources.DirectionalLightShadowSampler.Get(), 0);

    constexpr uint32 NumThreads = 16;
    const uint32 ThreadsX = FMath::DivideByMultiple(LightSetup.DirectionalShadowMask->GetWidth(), NumThreads);
    const uint32 ThreadsY = FMath::DivideByMultiple(LightSetup.DirectionalShadowMask->GetHeight(), NumThreads);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);
        
    CommandList.TransitionTexture(LightSetup.DirectionalShadowMask.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);

    if (CVarCascadeDebug.GetValue())
    {
        CommandList.TransitionTexture(LightSetup.CascadeIndexBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render ShadowMasks");
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

    PerCascadeBuffer.Reset();

    CascadeGen.Reset();
    CascadeGenShader.Reset();
}

bool FShadowMapRenderer::CreateShadowMask(uint32 Width, uint32 Height, FLightSetup& LightSetup)
{
    FRHITextureDesc ShadowMaskDesc = FRHITextureDesc::CreateTexture2D(LightSetup.ShadowMaskFormat, Width, Height, 1, 1, ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource);
    LightSetup.DirectionalShadowMask = RHICreateTexture(ShadowMaskDesc, EResourceAccess::NonPixelShaderResource);
    if (LightSetup.DirectionalShadowMask)
    {
        LightSetup.DirectionalShadowMask->SetDebugName("Directional Shadow Mask 0");
    }
    else
    {
        return false;
    }

    FRHITextureDesc CascadeIndexBufferDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource);
    LightSetup.CascadeIndexBuffer = RHICreateTexture(CascadeIndexBufferDesc, EResourceAccess::NonPixelShaderResource);
    if (LightSetup.CascadeIndexBuffer)
    {
        LightSetup.CascadeIndexBuffer->SetDebugName("Cascade Index Debug Buffer");
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
        LightSetup.PointLightShadowMaps->SetDebugName("PointLight ShadowMaps");
    }
    else
    {
        return false;
    }

    const uint32 CascadeSize = static_cast<uint32>(LightSetup.CascadeSize);
    
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
            LightSetup.ShadowMapCascades[Cascade]->SetDebugName("Shadow Map Cascade[" + TTypeToString<int32>::ToString(Cascade) + "]");
        }
        else
        {
            return false;
        }
    }

    return true;
}

#include "ShadowRendering.h"
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


FPointLightRenderPass::FPointLightRenderPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
    , PerShadowMapBuffer(nullptr)
{
}

FPointLightRenderPass::~FPointLightRenderPass()
{
    Release();
}

void FPointLightRenderPass::InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources)
{
    const EMaterialFlags MaterialFlags = Material->GetMaterialFlags();

    FPipelineStateInstance* CachedPointLightPSO = MaterialPSOs.Find(MaterialFlags);
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

        FShaderCompileInfo CompileInfo("Point_VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex, ShaderDefines);
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

        CompileInfo = FShaderCompileInfo("Point_PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel, ShaderDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/ShadowMap.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return;
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

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        if (MaterialFlags & MaterialFlag_DoubleSided)
        {
            RasterizerStateInitializer.CullMode = ECullMode::None;
        }
        else
        {
            RasterizerStateInitializer.CullMode = ECullMode::Back;
        }

        NewPipelineStateInstance.RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
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
            const FString DebugName = FString::CreateFormatted("Point ShadowMap PipelineState %d", MaterialFlags);
            NewPipelineStateInstance.PipelineState->SetDebugName(DebugName);
        }

        MaterialPSOs.Add(MaterialFlags, Move(NewPipelineStateInstance));
    }
}

bool FPointLightRenderPass::Initialize(FFrameResources& Resources)
{
    FRHIBufferDesc PerShadowMapBufferDesc(sizeof(FPerShadowMapHLSL), sizeof(FPerShadowMapHLSL), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
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

    return CreateResources(Resources);
}

void FPointLightRenderPass::Release()
{
    MaterialPSOs.Clear();
    PerShadowMapBuffer.Reset();
}

bool FPointLightRenderPass::CreateResources(FFrameResources& Resources)
{
    const FClearValue DepthClearValue(Resources.ShadowMapFormat, 1.0f, 0);

    const ETextureUsageFlags Flags = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResource;
    FRHITextureInfo PointLightInfo = FRHITextureInfo::CreateTextureCubeArray(Resources.ShadowMapFormat, Resources.PointLightShadowSize, Resources.MaxPointLightShadows, 1, 1, Flags, DepthClearValue);
    Resources.PointLightShadowMaps = RHICreateTexture(PointLightInfo, EResourceAccess::PixelShaderResource);
    if (Resources.PointLightShadowMaps)
    {
        Resources.PointLightShadowMaps->SetDebugName("PointLight ShadowMaps");
    }
    else
    {
        return false;
    }

    return true;
}

void FPointLightRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render PointLight ShadowMaps");

    GPU_TRACE_SCOPE(CommandList, "PointLight ShadowMaps");

    TRACE_SCOPE("Render PointLight ShadowMaps");

    CommandList.TransitionTexture(Resources.PointLightShadowMaps.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::DepthWrite);

    // PerObject Structs
    struct FShadowPerObject
    {
        FMatrix4 Matrix;
    } ShadowPerObjectBuffer;

    FPerShadowMapHLSL PerShadowMapData;
    for (int32 LightIndex = 0; LightIndex < Scene->PointLights.Size(); ++LightIndex)
    {
        FScenePointLight* ScenePointLight = Scene->PointLights[LightIndex];
        for (uint32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; ++FaceIndex)
        {
            FScenePointLight::FShadowData& Data = ScenePointLight->ShadowData[FaceIndex];
            PerShadowMapData.Matrix   = Data.Matrix;
            PerShadowMapData.Position = Data.Position;
            PerShadowMapData.FarPlane = Data.FarPlane;

            CommandList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
            CommandList.UpdateBuffer(PerShadowMapBuffer.Get(), FBufferRegion(0, sizeof(FPerShadowMapHLSL)), &PerShadowMapData);
            CommandList.TransitionBuffer(PerShadowMapBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

            const uint32 ArrayIndex = (LightIndex * RHI_NUM_CUBE_FACES) + FaceIndex;
            FRHIRenderPassDesc RenderPass;
            RenderPass.DepthStencilView            = FRHIDepthStencilView(Resources.PointLightShadowMaps.Get(), uint16(ArrayIndex), 0);
            RenderPass.DepthStencilView.LoadAction = EAttachmentLoadAction::Clear;
            RenderPass.DepthStencilView.ClearValue = FDepthStencilValue(1.0f, 0);

            CommandList.BeginRenderPass(RenderPass);

            const uint32 PointLightShadowSize = Resources.PointLightShadowSize;
            FViewportRegion ViewportRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0.0f, 0.0f, 0.0f, 1.0f);
            CommandList.SetViewport(ViewportRegion);

            FScissorRegion ScissorRegion(static_cast<float>(PointLightShadowSize), static_cast<float>(PointLightShadowSize), 0, 0);
            CommandList.SetScissorRect(ScissorRegion);

            for (const FMeshBatch& Batch : ScenePointLight->MeshBatches[FaceIndex])
            {
                FMaterial* Material = Batch.Material;

                FPipelineStateInstance* Instance = MaterialPSOs.Find(Material->GetMaterialFlags());
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
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 0);
                }
                if (Material->HasHeightMap())
                {
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                }

                CommandList.SetConstantBuffer(Instance->VertexShader.Get(), PerShadowMapBuffer.Get(), 0);
                CommandList.SetConstantBuffer(Instance->PixelShader.Get(), PerShadowMapBuffer.Get(), 0);

                for (FProxySceneComponent* Component : Batch.Primitives)
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

            CommandList.EndRenderPass();
        }
    }

    CommandList.TransitionTexture(Resources.PointLightShadowMaps.Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render PointLight ShadowMaps");
}


FCascadeGenerationPass::FCascadeGenerationPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , CascadeGen()
    , CascadeGenShader()
{
}

FCascadeGenerationPass::~FCascadeGenerationPass()
{
    Release();
}

bool FCascadeGenerationPass::Initialize(FFrameResources& Resources)
{
    TArray<uint8> ShaderCode;

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


    FRHIBufferDesc CascadeMatrixBufferDesc(sizeof(FCascadeMatricesHLSL) * NUM_SHADOW_CASCADES, sizeof(FCascadeMatricesHLSL), EBufferUsageFlags::Default | EBufferUsageFlags::RWBuffer);
    Resources.CascadeMatrixBuffer = RHICreateBuffer(CascadeMatrixBufferDesc, EResourceAccess::UnorderedAccess, nullptr);
    if (!Resources.CascadeMatrixBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.CascadeMatrixBuffer->SetDebugName("Cascade Matrices Buffer");
    }

    FRHIBufferSRVDesc SRVInitializer(Resources.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES);
    Resources.CascadeMatrixBufferSRV = RHICreateShaderResourceView(SRVInitializer);
    if (!Resources.CascadeMatrixBufferSRV)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBufferUAVDesc UAVInitializer(Resources.CascadeMatrixBuffer.Get(), 0, NUM_SHADOW_CASCADES);
    Resources.CascadeMatrixBufferUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!Resources.CascadeMatrixBufferUAV)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBufferDesc CascadeSplitsBufferDesc(sizeof(FCascadeSplitHLSL) * NUM_SHADOW_CASCADES, sizeof(FCascadeSplitHLSL), EBufferUsageFlags::Default | EBufferUsageFlags::RWBuffer);
    Resources.CascadeSplitsBuffer = RHICreateBuffer(CascadeSplitsBufferDesc, EResourceAccess::UnorderedAccess, nullptr);
    if (!Resources.CascadeSplitsBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.CascadeSplitsBuffer->SetDebugName("Cascade SplitBuffer");
    }

    SRVInitializer = FRHIBufferSRVDesc(Resources.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES);
    Resources.CascadeSplitsBufferSRV = RHICreateShaderResourceView(SRVInitializer);
    if (!Resources.CascadeSplitsBufferSRV)
    {
        DEBUG_BREAK();
        return false;
    }

    UAVInitializer = FRHIBufferUAVDesc(Resources.CascadeSplitsBuffer.Get(), 0, NUM_SHADOW_CASCADES);
    Resources.CascadeSplitsBufferUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!Resources.CascadeSplitsBufferUAV)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FCascadeGenerationPass::Release()
{
    CascadeGen.Reset();
    CascadeGenShader.Reset();
}

void FCascadeGenerationPass::Execute(FRHICommandList& CommandList, FFrameResources& Resources)
{
    GPU_TRACE_SCOPE(CommandList, "Generate Cascade Matrices");

    CommandList.TransitionBuffer(Resources.CascadeMatrixBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
    CommandList.TransitionBuffer(Resources.CascadeSplitsBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

    CommandList.SetComputePipelineState(CascadeGen.Get());

    CommandList.SetConstantBuffer(CascadeGenShader.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(CascadeGenShader.Get(), Resources.CascadeGenerationDataBuffer.Get(), 1);

    CommandList.SetUnorderedAccessView(CascadeGenShader.Get(), Resources.CascadeMatrixBufferUAV.Get(), 0);
    CommandList.SetUnorderedAccessView(CascadeGenShader.Get(), Resources.CascadeSplitsBufferUAV.Get(), 1);

    CommandList.SetShaderResourceView(CascadeGenShader.Get(), Resources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);

    CommandList.Dispatch(1, 1, 1);

    CommandList.TransitionBuffer(Resources.CascadeMatrixBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    CommandList.TransitionBuffer(Resources.CascadeSplitsBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
}


FCascadedShadowsRenderPass::FCascadedShadowsRenderPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , MaterialPSOs()
    , PerCascadeBuffer(nullptr)
{
}

FCascadedShadowsRenderPass::~FCascadedShadowsRenderPass()
{
    Release();
}

void FCascadedShadowsRenderPass::InitializePipelineState(FMaterial* Material, const FFrameResources& FrameResources)
{
    // Cascaded shadow map
    const EMaterialFlags MaterialFlags = Material->GetMaterialFlags();

    FPipelineStateInstance* CachedDirectionalLightPSO = MaterialPSOs.Find(MaterialFlags);
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

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        if (MaterialFlags & MaterialFlag_DoubleSided)
        {
            RasterizerStateInitializer.CullMode = ECullMode::None;
        }
        else
        {
            RasterizerStateInitializer.CullMode = ECullMode::Back;
        }

        NewPipelineStateInstance.RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
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

        MaterialPSOs.Add(MaterialFlags, Move(NewPipelineStateInstance));
    }
}

bool FCascadedShadowsRenderPass::Initialize(FFrameResources& Resources)
{
    FRHIBufferDesc PerCascadeBufferDesc(sizeof(FPerCascadeHLSL), sizeof(FPerCascadeHLSL), EBufferUsageFlags::Default | EBufferUsageFlags::ConstantBuffer);
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

    return CreateResources(Resources);
}

void FCascadedShadowsRenderPass::Release()
{
    MaterialPSOs.Clear();
    PerCascadeBuffer.Reset();
}

bool FCascadedShadowsRenderPass::CreateResources(FFrameResources& Resources)
{
    const ETextureUsageFlags Flags = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResource;

    const FClearValue DepthClearValue(Resources.ShadowMapFormat, 1.0f, 0);
    FRHITextureInfo CascadeInfo = FRHITextureInfo::CreateTexture2D(Resources.ShadowMapFormat, Resources.CascadeSize, Resources.CascadeSize, 1, 1, Flags, DepthClearValue);
    for (uint32 CascadeIndex = 0; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
    {
        Resources.ShadowMapCascades[CascadeIndex] = RHICreateTexture(CascadeInfo, EResourceAccess::NonPixelShaderResource);
        if (Resources.ShadowMapCascades[CascadeIndex])
        {
            const FString DebugName = FString::CreateFormatted("Shadow Map Cascade[%d]", CascadeIndex);
            Resources.ShadowMapCascades[CascadeIndex]->SetDebugName(DebugName);
        }
        else
        {
            return false;
        }
    }

    return true;
}

void FCascadedShadowsRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render DirectionalLight ShadowMaps");

    TRACE_SCOPE("Render DirectionalLight ShadowMaps");

    GPU_TRACE_SCOPE(CommandList, "DirectionalLight ShadowMaps");

    CommandList.TransitionTexture(Resources.ShadowMapCascades[0].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
    CommandList.TransitionTexture(Resources.ShadowMapCascades[1].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
    CommandList.TransitionTexture(Resources.ShadowMapCascades[2].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);
    CommandList.TransitionTexture(Resources.ShadowMapCascades[3].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);

    // PerObject Structs
    struct FShadowPerObject
    {
        FMatrix4 Matrix;
    } ShadowPerObjectBuffer;

    if (Scene->DirectionalLight)
    {
        for (uint32 Index = 0; Index < NUM_SHADOW_CASCADES; ++Index)
        {
            FPerCascadeHLSL PerCascadeData;
            PerCascadeData.CascadeIndex = Index;

            CommandList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::ConstantBuffer, EResourceAccess::CopyDest);
            CommandList.UpdateBuffer(PerCascadeBuffer.Get(), FBufferRegion(0, sizeof(FPerCascadeHLSL)), &PerCascadeData);
            CommandList.TransitionBuffer(PerCascadeBuffer.Get(), EResourceAccess::CopyDest, EResourceAccess::ConstantBuffer);

            FRHIRenderPassDesc RenderPass;
            RenderPass.DepthStencilView = FRHIDepthStencilView(Resources.ShadowMapCascades[Index].Get());

            CommandList.BeginRenderPass(RenderPass);

            const float CascadeSize = static_cast<float>(Resources.CascadeSize);
            FViewportRegion ViewportRegion(CascadeSize, CascadeSize, 0.0f, 0.0f, 0.0f, 1.0f);
            CommandList.SetViewport(ViewportRegion);

            FScissorRegion ScissorRegion(CascadeSize, CascadeSize, 0, 0);
            CommandList.SetScissorRect(ScissorRegion);

            FSceneDirectionalLight* SceneDirectionalLight = Scene->DirectionalLight;
            for (const FMeshBatch& Batch : SceneDirectionalLight->MeshBatches)
            {
                FMaterial* Material = Batch.Material;

                FPipelineStateInstance* Instance = MaterialPSOs.Find(Material->GetMaterialFlags());
                if (!Instance)
                {
                    DEBUG_BREAK();
                }

                FRHIGraphicsPipelineState* PipelineState = Instance->PipelineState.Get();
                CHECK(PipelineState != nullptr);
                CommandList.SetGraphicsPipelineState(PipelineState);

                if (Material->HasAlphaMask())
                {
                    CommandList.SetSamplerState(Instance->PixelShader.Get(), Material->GetMaterialSampler(), 0);
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->GetAlphaMaskSRV(), 0);
                }
                if (Material->HasHeightMap())
                {
                    CommandList.SetShaderResourceView(Instance->PixelShader.Get(), Material->HeightMap->GetShaderResourceView(), 1);
                }

                CommandList.SetConstantBuffer(Instance->VertexShader.Get(), PerCascadeBuffer.Get(), 0);
                CommandList.SetShaderResourceView(Instance->VertexShader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);

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

            CommandList.EndRenderPass();
        }
    }

    CommandList.TransitionTexture(Resources.ShadowMapCascades[0].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
    CommandList.TransitionTexture(Resources.ShadowMapCascades[1].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
    CommandList.TransitionTexture(Resources.ShadowMapCascades[2].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
    CommandList.TransitionTexture(Resources.ShadowMapCascades[3].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render DirectionalLight ShadowMaps");
}


FShadowMaskRenderPass::FShadowMaskRenderPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , DirectionalShadowMaskPSO(nullptr)
    , DirectionalShadowMaskShader(nullptr)
    , DirectionalShadowMaskPSO_Debug(nullptr)
    , DirectionalShadowMaskShader_Debug(nullptr)
{
}

FShadowMaskRenderPass::~FShadowMaskRenderPass()
{
    Release();
}

bool FShadowMaskRenderPass::Initialize(FFrameResources& Resources)
{
    if (!CreateResources(Resources, Resources.CurrentWidth, Resources.CurrentHeight))
    {
        return false;
    }

    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/DirectionalShadowMaskGen.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
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

    TArray<FShaderDefine> Defines =
    {
        { "ENABLE_DEBUG", "(1)" },
    };

    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, Defines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/DirectionalShadowMaskGen.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    DirectionalShadowMaskShader_Debug = RHICreateComputeShader(ShaderCode);
    if (!DirectionalShadowMaskShader_Debug)
    {
        DEBUG_BREAK();
        return false;
    }

    MaskPSOInitializer = FRHIComputePipelineStateInitializer(DirectionalShadowMaskShader_Debug.Get());
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

    return true;
}

void FShadowMaskRenderPass::Release()
{
    DirectionalShadowMaskPSO.Reset();
    DirectionalShadowMaskShader.Reset();
    DirectionalShadowMaskPSO_Debug.Reset();
    DirectionalShadowMaskShader_Debug.Reset();
}

bool FShadowMaskRenderPass::CreateResources(FFrameResources& Resources, uint32 Width, uint32 Height)
{
    const ETextureUsageFlags Flags = ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource;

    FRHITextureInfo ShadowMaskInfo = FRHITextureInfo::CreateTexture2D(Resources.ShadowMaskFormat, Width, Height, 1, 1, Flags);
    Resources.DirectionalShadowMask = RHICreateTexture(ShadowMaskInfo, EResourceAccess::NonPixelShaderResource);
    if (Resources.DirectionalShadowMask)
    {
        Resources.DirectionalShadowMask->SetDebugName("Directional Shadow Mask 0");
    }
    else
    {
        return false;
    }

    FRHITextureInfo CascadeIndexBufferInfo = FRHITextureInfo::CreateTexture2D(EFormat::R8_Uint, Width, Height, 1, 1, Flags);
    Resources.CascadeIndexBuffer = RHICreateTexture(CascadeIndexBufferInfo, EResourceAccess::NonPixelShaderResource);
    if (Resources.CascadeIndexBuffer)
    {
        Resources.CascadeIndexBuffer->SetDebugName("Cascade Index Debug Buffer");
    }
    else
    {
        return false;
    }

    return true;
}

bool FShadowMaskRenderPass::ResizeResources(FRHICommandList& CommandList, FFrameResources& Resources, uint32 Width, uint32 Height)
{
    if (Resources.DirectionalShadowMask)
        CommandList.DestroyResource(Resources.DirectionalShadowMask.Get());
    if (Resources.CascadeIndexBuffer)
        CommandList.DestroyResource(Resources.CascadeIndexBuffer.Get());

    return CreateResources(Resources, Width, Height);
}

void FShadowMaskRenderPass::Execute(FRHICommandList& CommandList, const FFrameResources& Resources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Render ShadowMasks");

    TRACE_SCOPE("Render ShadowMasks");

    GPU_TRACE_SCOPE(CommandList, "DirectionalLight Shadow Mask");

    CommandList.TransitionTexture(Resources.DirectionalShadowMask.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

    FRHIComputeShaderRef CurrentShadowMaskShader;
    if (CVarCascadeDebug.GetValue())
    {
        CommandList.TransitionTexture(Resources.CascadeIndexBuffer.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

        CurrentShadowMaskShader = DirectionalShadowMaskShader_Debug;
        CommandList.SetComputePipelineState(DirectionalShadowMaskPSO_Debug.Get());
    }
    else
    {
        CurrentShadowMaskShader = DirectionalShadowMaskShader;
        CommandList.SetComputePipelineState(DirectionalShadowMaskPSO.Get());
    }

    CommandList.SetConstantBuffer(CurrentShadowMaskShader.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(CurrentShadowMaskShader.Get(), Resources.DirectionalLightDataBuffer.Get(), 1);

    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), Resources.CascadeMatrixBufferSRV.Get(), 0);
    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), Resources.CascadeSplitsBufferSRV.Get(), 1);

    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), Resources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), Resources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView(), 3);

    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), Resources.ShadowMapCascades[0]->GetShaderResourceView(), 4);
    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), Resources.ShadowMapCascades[1]->GetShaderResourceView(), 5);
    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), Resources.ShadowMapCascades[2]->GetShaderResourceView(), 6);
    CommandList.SetShaderResourceView(CurrentShadowMaskShader.Get(), Resources.ShadowMapCascades[3]->GetShaderResourceView(), 7);

    CommandList.SetUnorderedAccessView(CurrentShadowMaskShader.Get(), Resources.DirectionalShadowMask->GetUnorderedAccessView(), 0);
    if (CVarCascadeDebug.GetValue())
    {
        CommandList.SetUnorderedAccessView(CurrentShadowMaskShader.Get(), Resources.CascadeIndexBuffer->GetUnorderedAccessView(), 1);
    }

    CommandList.SetSamplerState(CurrentShadowMaskShader.Get(), Resources.DirectionalLightShadowSampler.Get(), 0);

    constexpr uint32 NumThreads = 16;
    const uint32 ThreadsX = FMath::DivideByMultiple(Resources.DirectionalShadowMask->GetWidth(), NumThreads);
    const uint32 ThreadsY = FMath::DivideByMultiple(Resources.DirectionalShadowMask->GetHeight(), NumThreads);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);

    CommandList.TransitionTexture(Resources.DirectionalShadowMask.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);

    if (CVarCascadeDebug.GetValue())
    {
        CommandList.TransitionTexture(Resources.CascadeIndexBuffer.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Render ShadowMasks");
}
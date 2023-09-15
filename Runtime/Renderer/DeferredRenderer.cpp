#include "DeferredRenderer.h"
#include "MeshDrawCommand.h"
#include "RHI/RHI.h"
#include "RHI/RHIShaderCompiler.h"
#include "Engine/Resources/Mesh.h"
#include "Engine//Resources/Material.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Renderer/Debug/GPUProfiler.h"

TAutoConsoleVariable<bool> GDrawTileDebug(
    "Renderer.Debug.DrawTiledLightning", 
    "Draws the tiled lightning overlay, that displays how many lights are used in a certain tile", 
    false);

bool FDeferredRenderer::Init(FFrameResources& FrameResources)
{
    if (!CreateGBuffer(FrameResources))
    {
        return false;
    }

    {
        FRHISamplerStateDesc SamplerInitializer;
        SamplerInitializer.AddressU = ESamplerMode::Clamp;
        SamplerInitializer.AddressV = ESamplerMode::Clamp;
        SamplerInitializer.AddressW = ESamplerMode::Clamp;
        SamplerInitializer.Filter   = ESamplerFilter::MinMagMipPoint;

        FrameResources.GBufferSampler = RHICreateSamplerState(SamplerInitializer);
        if (!FrameResources.GBufferSampler)
        {
            return false;
        }
    }

    TArray<uint8> ShaderCode;
    
    // BasePass
    {
        TArray<FShaderDefine> Defines =
        {
            { "ENABLE_PARALLAX_MAPPING", "(1)" },
            { "ENABLE_NORMAL_MAPPING",   "(1)" },
        };

        FRHIShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_0, EShaderStage::Vertex, Defines);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/GeometryPass.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        BaseVertexShader = RHICreateVertexShader(ShaderCode);
        if (!BaseVertexShader)
        {
            DEBUG_BREAK();
            return false;
        }

        CompileInfo = FRHIShaderCompileInfo("PSMain", EShaderModel::SM_6_0, EShaderStage::Pixel, Defines);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/GeometryPass.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        BasePixelShader = RHICreatePixelShader(ShaderCode);
        if (!BasePixelShader)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateInitializer DepthStencilInitializer;
        DepthStencilInitializer.DepthFunc         = EComparisonFunc::LessEqual;
        DepthStencilInitializer.bDepthEnable      = true;
        // TODO: We should use false here, but alpha-masks are not rendered in pre-pass right now
        DepthStencilInitializer.bDepthWriteEnable = true;

        FRHIDepthStencilStateRef GeometryDepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
        if (!GeometryDepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        RasterizerStateInitializer.CullMode = ECullMode::Back;

        FRHIRasterizerStateRef GeometryRasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
        if (!GeometryRasterizerState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIBlendStateInitializer BlendStateInitializer;
        BlendStateInitializer.NumRenderTargets = 5;

        FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!BlendState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.VertexInputLayout                      = FrameResources.MeshInputLayout.Get();
        PSOInitializer.BlendState                             = BlendState.Get();
        PSOInitializer.DepthStencilState                      = GeometryDepthStencilState.Get();
        PSOInitializer.RasterizerState                        = GeometryRasterizerState.Get();
        PSOInitializer.ShaderState.VertexShader               = BaseVertexShader.Get();
        PSOInitializer.ShaderState.PixelShader                = BasePixelShader.Get();
        PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FrameResources.AlbedoFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[1] = FrameResources.NormalFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[2] = FrameResources.MaterialFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[3] = FrameResources.ViewNormalFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[4] = FrameResources.VelocityFormat;
        PSOInitializer.PipelineFormats.NumRenderTargets       = 5;
        PSOInitializer.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;

        PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!PipelineState)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            PipelineState->SetName("GeometryPass PipelineState");
        }
    }

    // PrePass
    {
        FRHIShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_0, EShaderStage::Vertex);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/PrePass.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        PrePassVertexShader = RHICreateVertexShader(ShaderCode);
        if (!PrePassVertexShader)
        {
            DEBUG_BREAK();
            return false;
        }

        CompileInfo = FRHIShaderCompileInfo("PSMain", EShaderModel::SM_6_0, EShaderStage::Pixel);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/PrePass.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        PrePassPixelShader = RHICreatePixelShader(ShaderCode);
        if (!PrePassPixelShader)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIDepthStencilStateInitializer DepthStencilStateInitializer;
        DepthStencilStateInitializer.DepthFunc         = EComparisonFunc::Less;
        DepthStencilStateInitializer.bDepthEnable      = true;
        DepthStencilStateInitializer.bDepthWriteEnable = true;

        FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilStateInitializer);
        if (!DepthStencilState)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIRasterizerStateInitializer RasterizerStateInitializer;
        RasterizerStateInitializer.CullMode = ECullMode::Back;

        FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerStateInitializer);
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
        PSOInitializer.VertexInputLayout                  = FrameResources.MeshInputLayout.Get();
        PSOInitializer.BlendState                         = BlendState.Get();
        PSOInitializer.DepthStencilState                  = DepthStencilState.Get();
        PSOInitializer.RasterizerState                    = RasterizerState.Get();
        PSOInitializer.ShaderState.VertexShader           = PrePassVertexShader.Get();
        PSOInitializer.ShaderState.PixelShader            = PrePassPixelShader.Get();
        PSOInitializer.PipelineFormats.DepthStencilFormat = FrameResources.DepthBufferFormat;

        PrePassPipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!PrePassPipelineState)
        {
            DEBUG_BREAK();
            return false;
        } 
    }

    // BRDF LUT Generation
    {
        constexpr uint32  LUTSize   = 512;
        constexpr EFormat LUTFormat = EFormat::R16G16_Float;
        if (!RHIQueryUAVFormatSupport(LUTFormat))
        {
            LOG_ERROR("[FRenderer]: R16G16_Float is not supported for UAVs");
            return false;
        }

        FRHITextureDesc LUTDesc = FRHITextureDesc::CreateTexture2D(LUTFormat, LUTSize, LUTSize, 1, 1, ETextureUsageFlags::UnorderedAccess);

        FRHITextureRef StagingTexture = RHICreateTexture(LUTDesc, EResourceAccess::Common);
        if (!StagingTexture)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            StagingTexture->SetName("Staging IntegrationLUT");
        }

        LUTDesc.UsageFlags = ETextureUsageFlags::ShaderResource;

        FrameResources.IntegrationLUT = RHICreateTexture(LUTDesc, EResourceAccess::Common);
        if (!FrameResources.IntegrationLUT)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            FrameResources.IntegrationLUT->SetName("IntegrationLUT");
        }

        FRHISamplerStateDesc SamplerInitializer;
        SamplerInitializer.AddressU = ESamplerMode::Clamp;
        SamplerInitializer.AddressV = ESamplerMode::Clamp;
        SamplerInitializer.AddressW = ESamplerMode::Clamp;
        SamplerInitializer.Filter   = ESamplerFilter::MinMagMipPoint;

        FrameResources.IntegrationLUTSampler = RHICreateSamplerState(SamplerInitializer);
        if (!FrameResources.IntegrationLUTSampler)
        {
            DEBUG_BREAK();
            return false;
        }

        {
            FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
            if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/BRDFIntegationGen.hlsl", CompileInfo, ShaderCode))
            {
                DEBUG_BREAK();
                return false;
            }
        }

        FRHIComputeShaderRef CShader = RHICreateComputeShader(ShaderCode);
        if (!CShader)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIComputePipelineStateInitializer PSOInitializer(CShader.Get());

        FRHIComputePipelineStateRef BRDF_PipelineState = RHICreateComputePipelineState(PSOInitializer);
        if (!BRDF_PipelineState)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            BRDF_PipelineState->SetName("BRDFIntegationGen PipelineState");
        }

        FRHICommandList CommandList;
        CommandList.TransitionTexture(StagingTexture.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

        CommandList.SetComputePipelineState(BRDF_PipelineState.Get());

        FRHIUnorderedAccessView* StagingUAV = StagingTexture->GetUnorderedAccessView();
        CommandList.SetUnorderedAccessView(CShader.Get(), StagingUAV, 0);

        constexpr uint32 ThreadCount = 16;
        const uint32 DispatchWidth  = FMath::DivideByMultiple(LUTSize, ThreadCount);
        const uint32 DispatchHeight = FMath::DivideByMultiple(LUTSize, ThreadCount);
        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);

        CommandList.UnorderedAccessTextureBarrier(StagingTexture.Get());

        CommandList.TransitionTexture(StagingTexture.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::CopySource);
        CommandList.TransitionTexture(FrameResources.IntegrationLUT.Get(), EResourceAccess::Common, EResourceAccess::CopyDest);

        CommandList.CopyTexture(FrameResources.IntegrationLUT.Get(), StagingTexture.Get());

        CommandList.TransitionTexture(FrameResources.IntegrationLUT.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

        CommandList.DestroyResource(CShader.Get());
        CommandList.DestroyResource(BRDF_PipelineState.Get());
        CommandList.DestroyResource(StagingTexture.Get());

        GRHICommandExecutor.ExecuteCommandList(CommandList);
    }

    // Tiled lightning
    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/DeferredLightPass.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        TiledLightShader = RHICreateComputeShader(ShaderCode);
        if (!TiledLightShader)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIComputePipelineStateInitializer DeferredLightPassInitializer(TiledLightShader.Get());
        TiledLightPassPSO = RHICreateComputePipelineState(DeferredLightPassInitializer);
        if (!TiledLightPassPSO)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    // Tiled lightning Tile debugging
    {
        TArray<FShaderDefine> Defines =
        {
            { "DRAW_TILE_DEBUG", "(1)" }
        };

        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute, Defines);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/DeferredLightPass.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        TiledLightShader_TileDebug = RHICreateComputeShader(ShaderCode);
        if (!TiledLightShader_TileDebug)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIComputePipelineStateInitializer DeferredLightPassInitializer(TiledLightShader_TileDebug.Get());
        TiledLightPassPSO_TileDebug = RHICreateComputePipelineState(DeferredLightPassInitializer);
        if (!TiledLightPassPSO_TileDebug)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            TiledLightPassPSO_TileDebug->SetName("DeferredLightPass PipelineState Tile-Debug");
        }
    }

    // Tiled lightning Cascade debugging
    {
        TArray<FShaderDefine> Defines =
        {
            { "DRAW_CASCADE_DEBUG", "(1)" }
        };

        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute, Defines);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/DeferredLightPass.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        TiledLightShader_CascadeDebug = RHICreateComputeShader(ShaderCode);
        if (!TiledLightShader_CascadeDebug)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIComputePipelineStateInitializer DeferredLightPassInitializer(TiledLightShader_CascadeDebug.Get());
        TiledLightPassPSO_CascadeDebug = RHICreateComputePipelineState(DeferredLightPassInitializer);
        if (!TiledLightPassPSO_CascadeDebug)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            TiledLightPassPSO_CascadeDebug->SetName("DeferredLightPass PipelineState Cascade-Debug");
        }
    }

    // Depth-Reduction
    {
        FRHIShaderCompileInfo CompileInfo("ReductionMainInital", EShaderModel::SM_6_0, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/DepthReduction.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        ReduceDepthInitalShader = RHICreateComputeShader(ShaderCode);
        if (!ReduceDepthInitalShader)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIComputePipelineStateInitializer PipelineStateInfo(ReduceDepthInitalShader.Get());
        ReduceDepthInitalPSO = RHICreateComputePipelineState(PipelineStateInfo);
        if (!ReduceDepthInitalPSO)
        {
            DEBUG_BREAK();
            return false;
        }
    }

    // Depth-Reduction
    {
        FRHIShaderCompileInfo CompileInfo("ReductionMain", EShaderModel::SM_6_0, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/DepthReduction.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }

        ReduceDepthShader = RHICreateComputeShader(ShaderCode);
        if (!ReduceDepthShader)
        {
            DEBUG_BREAK();
            return false;
        }

        FRHIComputePipelineStateInitializer PSOInitializer(ReduceDepthShader.Get());
        ReduceDepthPSO = RHICreateComputePipelineState(PSOInitializer);
        if (!ReduceDepthPSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            ReduceDepthPSO->SetName("DepthReduction PipelineState");
        }
    }

    return true;
}

void FDeferredRenderer::Release()
{
    PrePassPipelineState.Reset();
    PrePassVertexShader.Reset();
    PrePassPixelShader.Reset();

    PipelineState.Reset();
    BaseVertexShader.Reset();
    BasePixelShader.Reset();

    TiledLightPassPSO.Reset();
    TiledLightShader.Reset();

    TiledLightPassPSO_TileDebug.Reset();
    TiledLightShader_TileDebug.Reset();

    TiledLightPassPSO_CascadeDebug.Reset();
    TiledLightShader_CascadeDebug.Reset();

    ReduceDepthInitalPSO.Reset();
    ReduceDepthInitalShader.Reset();

    ReduceDepthPSO.Reset();
    ReduceDepthShader.Reset();
}

void FDeferredRenderer::RenderPrePass(FRHICommandList& CommandList, FFrameResources& FrameResources, const FScene& Scene)
{
    const float RenderWidth  = float(FrameResources.MainViewport->GetWidth());
    const float RenderHeight = float(FrameResources.MainViewport->GetHeight());

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin PrePass");

    {
        TRACE_SCOPE("PrePass");

        GPU_TRACE_SCOPE(CommandList, "Pre Pass");

        struct FPerObject
        {
            FMatrix4 Matrix;
        } PerObjectBuffer;

        FRHIRenderPassDesc RenderPass;
        RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBufferIndex_Depth].Get());

        CommandList.BeginRenderPass(RenderPass);

        FRHIViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
        CommandList.SetViewport(ViewportRegion);

        FRHIScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
        CommandList.SetScissorRect(ScissorRegion);

        CommandList.SetGraphicsPipelineState(PrePassPipelineState.Get());

        CommandList.SetConstantBuffer(PrePassVertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

        for (const auto CommandIndex : FrameResources.DeferredVisibleCommands)
        {
            const FMeshDrawCommand& Command = FrameResources.GlobalMeshDrawCommands[CommandIndex];
            if (Command.Material->ShouldRenderInPrePass())
            {
                CommandList.SetVertexBuffers(MakeArrayView(&Command.VertexBuffer, 1), 0);
                CommandList.SetIndexBuffer(Command.IndexBuffer, Command.IndexFormat);

                PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();
                CommandList.Set32BitShaderConstants(PrePassVertexShader.Get(), &PerObjectBuffer, 16);

                CommandList.SetConstantBuffer(BasePixelShader.Get(), Command.Material->GetMaterialBuffer(), 1);

                CommandList.SetShaderResourceView(BasePixelShader.Get(), Command.Material->GetAlphaMaskSRV(), 0);

                FRHISamplerState* Sampler = Command.Material->GetMaterialSampler();
                CommandList.SetSamplerState(BasePixelShader.Get(), Sampler, 0);

                CommandList.DrawIndexedInstanced(Command.NumIndices, 1, 0, 0, 0);
            }
        }

        CommandList.EndRenderPass();
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End PrePass");

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Depth Reduction");

    {
        TRACE_SCOPE("Depth Reduction");

        GPU_TRACE_SCOPE(CommandList, "Depth Reduction");

        struct FReductionConstants
        {
            FMatrix4 CamProjection;
            float NearPlane;
            float FarPlane;
        } ReductionConstants;

        ReductionConstants.CamProjection = Scene.GetCamera()->GetProjectionMatrix();
        ReductionConstants.NearPlane     = Scene.GetCamera()->GetNearPlane();
        ReductionConstants.FarPlane      = Scene.GetCamera()->GetFarPlane();

        // Perform the first reduction
        CommandList.TransitionTexture(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
        CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[1].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

        CommandList.SetComputePipelineState(ReduceDepthInitalPSO.Get());

        CommandList.SetShaderResourceView(ReduceDepthInitalShader.Get(), FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 0);
        CommandList.SetUnorderedAccessView(ReduceDepthInitalShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0);

        CommandList.Set32BitShaderConstants(ReduceDepthInitalShader.Get(), &ReductionConstants, FMath::BytesToNum32BitConstants(sizeof(ReductionConstants)));

        uint32 ThreadsX = FrameResources.ReducedDepthBuffer[0]->GetWidth();
        uint32 ThreadsY = FrameResources.ReducedDepthBuffer[0]->GetHeight();
        CommandList.Dispatch(ThreadsX, ThreadsY, 1);

        CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);

        // Perform the other reductions
        CommandList.SetComputePipelineState(ReduceDepthPSO.Get());

        CommandList.SetShaderResourceView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);
        CommandList.SetUnorderedAccessView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetUnorderedAccessView(), 0);

        ThreadsX = FMath::DivideByMultiple(ThreadsX, 16);
        ThreadsY = FMath::DivideByMultiple(ThreadsY, 16);
        CommandList.Dispatch(ThreadsX, ThreadsY, 1);

        CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
        CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[1].Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);

        CommandList.SetShaderResourceView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetShaderResourceView(), 0);
        CommandList.SetUnorderedAccessView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0);

        ThreadsX = FMath::DivideByMultiple(ThreadsX, 16);
        ThreadsY = FMath::DivideByMultiple(ThreadsY, 16);
        CommandList.Dispatch(ThreadsX, ThreadsY, 1);

        CommandList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Depth Reduction");
}

void FDeferredRenderer::RenderBasePass(FRHICommandList& CommandList, const FFrameResources& FrameResources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin GeometryPass");

    TRACE_SCOPE("GeometryPass");

    GPU_TRACE_SCOPE(CommandList, "Base Pass");

    const float RenderWidth  = float(FrameResources.MainViewport->GetWidth());
    const float RenderHeight = float(FrameResources.MainViewport->GetHeight());

    FRHIRenderPassDesc RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Albedo].Get());
    RenderPass.RenderTargets[1] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Normal].Get());
    RenderPass.RenderTargets[2] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Material].Get());
    RenderPass.RenderTargets[3] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_ViewNormal].Get());
    RenderPass.RenderTargets[4] = FRHIRenderTargetView(FrameResources.GBuffer[GBufferIndex_Velocity].Get());
    RenderPass.NumRenderTargets = 5;
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBufferIndex_Depth].Get(), EAttachmentLoadAction::Load);

    CommandList.BeginRenderPass(RenderPass);

    FRHIViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FRHIScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    // Setup Pipeline
    CommandList.SetGraphicsPipelineState(PipelineState.Get());

    struct FTransformBuffer
    {
        FMatrix4 Transform;
        FMatrix4 TransformInv;
    } TransformPerObject;

    for (const auto CommandIndex : FrameResources.DeferredVisibleCommands)
    {
        const FMeshDrawCommand& Command = FrameResources.GlobalMeshDrawCommands[CommandIndex];

        CommandList.SetVertexBuffers(MakeArrayView(&Command.VertexBuffer, 1), 0);
        CommandList.SetIndexBuffer(Command.IndexBuffer, Command.IndexFormat);

        CommandList.SetConstantBuffer(BaseVertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

        TransformPerObject.Transform    = Command.CurrentActor->GetTransform().GetMatrix();
        TransformPerObject.TransformInv = Command.CurrentActor->GetTransform().GetMatrixInverse();
        CommandList.Set32BitShaderConstants(BaseVertexShader.Get(), &TransformPerObject, 32);

        FRHIBuffer* PSConstantBuffers[] =
        {
            FrameResources.CameraBuffer.Get(),
            Command.Material->GetMaterialBuffer(),
        };

        CommandList.SetConstantBuffers(BasePixelShader.Get(), MakeArrayView(PSConstantBuffers), 0);

        const auto PSShaderResourceViews = MakeArrayView(Command.Material->GetShaderResourceViews(), Command.Material->GetNumShaderResourceViews());
        CommandList.SetShaderResourceViews(BasePixelShader.Get(), PSShaderResourceViews, 0);

        FRHISamplerState* Sampler = Command.Material->GetMaterialSampler();
        CommandList.SetSamplerState(BasePixelShader.Get(), Sampler, 0);

        CommandList.DrawIndexedInstanced(Command.NumIndices, 1, 0, 0, 0);
    }

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End GeometryPass");
}

void FDeferredRenderer::RenderDeferredTiledLightPass(FRHICommandList& CommandList, const FFrameResources& FrameResources, const FLightSetup& LightSetup)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin LightPass");

    TRACE_SCOPE("LightPass");

    GPU_TRACE_SCOPE(CommandList, "Light Pass");

    bool bDrawCascades = false;
    if (IConsoleVariable* CVarDrawCascades = FConsoleManager::Get().FindConsoleVariable("Renderer.Debug.DrawCascades"))
    {
        bDrawCascades = CVarDrawCascades->GetBool();
    }

    FRHIComputeShader* LightPassShader = nullptr;
    if (GDrawTileDebug.GetValue())
    {
        LightPassShader = TiledLightShader_TileDebug.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO_TileDebug.Get());
    }
    else if (bDrawCascades)
    {
        LightPassShader = TiledLightShader_CascadeDebug.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO_CascadeDebug.Get());
    }
    else
    {
        LightPassShader = TiledLightShader.Get();
        CommandList.SetComputePipelineState(TiledLightPassPSO.Get());
    }

    const FProxyLightProbe& Skylight = LightSetup.Skylight;
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBufferIndex_Albedo]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView(), 1);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBufferIndex_Material]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 3);
    CommandList.SetShaderResourceView(LightPassShader, nullptr, 4); // DXR-Reflection
    CommandList.SetShaderResourceView(LightPassShader, Skylight.IrradianceMap->GetShaderResourceView(), 5);
    CommandList.SetShaderResourceView(LightPassShader, Skylight.SpecularIrradianceMap->GetShaderResourceView(), 6);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.IntegrationLUT->GetShaderResourceView(), 7);
    CommandList.SetShaderResourceView(LightPassShader, LightSetup.DirectionalShadowMask->GetShaderResourceView(), 8);
    CommandList.SetShaderResourceView(LightPassShader, LightSetup.PointLightShadowMaps->GetShaderResourceView(), 9);
    CommandList.SetShaderResourceView(LightPassShader, FrameResources.SSAOBuffer->GetShaderResourceView(), 10);

    if (bDrawCascades)
    {
        CommandList.SetShaderResourceView(LightPassShader, LightSetup.CascadeIndexBuffer->GetShaderResourceView(), 11);
    }

    CommandList.SetConstantBuffer(LightPassShader, FrameResources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(LightPassShader, LightSetup.PointLightsBuffer.Get(), 1);
    CommandList.SetConstantBuffer(LightPassShader, LightSetup.PointLightsPosRadBuffer.Get(), 2);
    CommandList.SetConstantBuffer(LightPassShader, LightSetup.ShadowCastingPointLightsBuffer.Get(), 3);
    CommandList.SetConstantBuffer(LightPassShader, LightSetup.ShadowCastingPointLightsPosRadBuffer.Get(), 4);
    CommandList.SetConstantBuffer(LightPassShader, LightSetup.DirectionalLightsBuffer.Get(), 5);

    CommandList.SetSamplerState(LightPassShader, FrameResources.IntegrationLUTSampler.Get(), 0);
    CommandList.SetSamplerState(LightPassShader, FrameResources.IrradianceSampler.Get(), 1);
    CommandList.SetSamplerState(LightPassShader, FrameResources.GBufferSampler.Get(), 2);
    CommandList.SetSamplerState(LightPassShader, FrameResources.PointLightShadowSampler.Get(), 3);

    FRHIUnorderedAccessView* FinalTargetUAV = FrameResources.FinalTarget->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(LightPassShader, FinalTargetUAV, 0);

    struct SLightPassSettings
    {
        int32 NumPointLights;
        int32 NumShadowCastingPointLights;
        int32 NumSkyLightMips;
        int32 ScreenWidth;
        int32 ScreenHeight;
    } Settings;

    Settings.NumShadowCastingPointLights = LightSetup.ShadowCastingPointLightsData.Size();
    Settings.NumPointLights  = LightSetup.PointLightsData.Size();
    Settings.NumSkyLightMips = Skylight.SpecularIrradianceMap->GetNumMipLevels();
    Settings.ScreenWidth     = FrameResources.FinalTarget->GetWidth();
    Settings.ScreenHeight    = FrameResources.FinalTarget->GetHeight();

    CommandList.Set32BitShaderConstants(LightPassShader, &Settings, 5);

    constexpr uint32 NumThreads = 16;
    const uint32 WorkGroupWidth  = FMath::DivideByMultiple<uint32>(Settings.ScreenWidth, NumThreads);
    const uint32 WorkGroupHeight = FMath::DivideByMultiple<uint32>(Settings.ScreenHeight, NumThreads);
    CommandList.Dispatch(WorkGroupWidth, WorkGroupHeight, 1);

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End LightPass");
}

bool FDeferredRenderer::ResizeResources(FFrameResources& FrameResources)
{
    return CreateGBuffer(FrameResources);
}

bool FDeferredRenderer::CreateGBuffer(FFrameResources& FrameResources)
{
    const ETextureUsageFlags Usage = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResource;

    const uint32 Width  = FrameResources.MainViewport->GetWidth();
    const uint32 Height = FrameResources.MainViewport->GetHeight();

    if (!(Width > 0 && Height > 0))
    {
        return true;
    }

    // Albedo
    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(FrameResources.AlbedoFormat, Width, Height, 1, 1, Usage);

    FrameResources.GBuffer[GBufferIndex_Albedo] = RHICreateTexture(TextureDesc, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Albedo])
    {
        FrameResources.GBuffer[GBufferIndex_Albedo]->SetName("GBuffer Albedo");
    }
    else
    {
        return false;
    }

    // Normal
    TextureDesc.Format = FrameResources.NormalFormat;

    FrameResources.GBuffer[GBufferIndex_Normal] = RHICreateTexture(TextureDesc, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Normal])
    {
        FrameResources.GBuffer[GBufferIndex_Normal]->SetName("GBuffer Normal");
    }
    else
    {
        return false;
    }

    // Material Properties
    TextureDesc.Format = FrameResources.MaterialFormat;

    FrameResources.GBuffer[GBufferIndex_Material] = RHICreateTexture(TextureDesc, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Material])
    {
        FrameResources.GBuffer[GBufferIndex_Material]->SetName("GBuffer Material");
    }
    else
    {
        return false;
    }

    // View Normal
    TextureDesc.Format = FrameResources.ViewNormalFormat;

    FrameResources.GBuffer[GBufferIndex_ViewNormal] = RHICreateTexture(TextureDesc, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_ViewNormal])
    {
        FrameResources.GBuffer[GBufferIndex_ViewNormal]->SetName("GBuffer ViewNormal");
    }
    else
    {
        return false;
    }

    // Velocity
    TextureDesc.Format = FrameResources.VelocityFormat;

    FrameResources.GBuffer[GBufferIndex_Velocity] = RHICreateTexture(TextureDesc, EResourceAccess::NonPixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Velocity])
    {
        FrameResources.GBuffer[GBufferIndex_Velocity]->SetName("GBuffer Velocity");
    }
    else
    {
        return false;
    }

    // Final Image
    TextureDesc.Format     = FrameResources.FinalTargetFormat;
    TextureDesc.UsageFlags = Usage | ETextureUsageFlags::UnorderedAccess;

    FrameResources.FinalTarget = RHICreateTexture(TextureDesc, EResourceAccess::PixelShaderResource);
    if (FrameResources.FinalTarget)
    {
        FrameResources.FinalTarget->SetName("Final Target");
    }
    else
    {
        return false;
    }

    // DepthStencil
    const FClearValue DepthClearValue(FrameResources.DepthBufferFormat, 1.0f, 0);
    TextureDesc.Format     = FrameResources.DepthBufferFormat;
    TextureDesc.UsageFlags = ETextureUsageFlags::DepthStencil | ETextureUsageFlags::ShaderResource;
    TextureDesc.ClearValue = DepthClearValue;

    FrameResources.GBuffer[GBufferIndex_Depth] = RHICreateTexture(TextureDesc, EResourceAccess::PixelShaderResource);
    if (FrameResources.GBuffer[GBufferIndex_Depth])
    {
        FrameResources.GBuffer[GBufferIndex_Depth]->SetName("GBuffer DepthStencil");
    }
    else
    {
        return false;
    }

    constexpr uint32 Alignment = 16;

    const uint32 ReducedWidth  = FMath::DivideByMultiple(Width, Alignment);
    const uint32 ReducedHeight = FMath::DivideByMultiple(Height, Alignment);

    TextureDesc.Format     = EFormat::R32G32_Float;
    TextureDesc.Extent.x   = uint16(ReducedWidth);
    TextureDesc.Extent.y   = uint16(ReducedHeight);
    TextureDesc.UsageFlags = ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource;

    for (uint32 i = 0; i < 2; i++)
    {
        FrameResources.ReducedDepthBuffer[i] = RHICreateTexture(TextureDesc, EResourceAccess::NonPixelShaderResource);
        if (FrameResources.ReducedDepthBuffer[i])
        {
            FrameResources.ReducedDepthBuffer[i]->SetName("Reduced DepthStencil[" + TTypeToString<int32>::ToString(i) + "]");
        }
        else
        {
            return false;
        }
    }

    return true;
}

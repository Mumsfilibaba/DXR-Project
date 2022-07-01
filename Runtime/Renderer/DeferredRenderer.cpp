#include "DeferredRenderer.h"
#include "MeshDrawCommand.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHIShaderCompiler.h"

#include "Engine/Resources/Mesh.h"
#include "Engine//Resources/Material.h"

#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"

#include "Renderer/Debug/GPUProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console-variable

TAutoConsoleVariable<bool> GDrawTileDebug("Renderer.DrawTileDebug", false);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CDeferredRenderer

bool CDeferredRenderer::Init(SFrameResources& FrameResources)
{
    if (!CreateGBuffer(FrameResources))
    {
        return false;
    }

    {
        FRHISamplerStateInitializer SamplerInitializer;
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
    {
        TArray<FShaderDefine> Defines =
        {
            { "ENABLE_PARALLAX_MAPPING", "1" },
            { "ENABLE_NORMAL_MAPPING",   "1" },
        };

        FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_0, EShaderStage::Vertex, Defines.CreateView());
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/GeometryPass.hlsl", CompileInfo, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }

        BaseVertexShader = RHICreateVertexShader(ShaderCode);
        if (!BaseVertexShader)
        {
            CDebug::DebugBreak();
            return false;
        }

        CompileInfo = FShaderCompileInfo("PSMain", EShaderModel::SM_6_0, EShaderStage::Pixel, Defines.CreateView());
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/GeometryPass.hlsl", CompileInfo, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }

        BasePixelShader = RHICreatePixelShader(ShaderCode);
        if (!BasePixelShader)
        {
            CDebug::DebugBreak();
            return false;
        }

        FRHIDepthStencilStateInitializer DepthStencilInitializer;
        DepthStencilInitializer.DepthFunc      = EComparisonFunc::LessEqual;
        DepthStencilInitializer.bDepthEnable   = true;
        DepthStencilInitializer.DepthWriteMask = EDepthWriteMask::All;

        TSharedRef<FRHIDepthStencilState> GeometryDepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
        if (!GeometryDepthStencilState)
        {
            CDebug::DebugBreak();
            return false;
        }

        FRHIRasterizerStateInitializer RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::Back;

        TSharedRef<FRHIRasterizerState> GeometryRasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
        if (!GeometryRasterizerState)
        {
            CDebug::DebugBreak();
            return false;
        }

        FRHIBlendStateInitializer BlendStateInitializer;

        TSharedRef<FRHIBlendState> BlendState = RHICreateBlendState(BlendStateInitializer);
        if (!BlendState)
        {
            CDebug::DebugBreak();
            return false;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.VertexInputLayout                      = FrameResources.StdInputLayout.Get();
        PSOInitializer.BlendState                             = BlendState.Get();
        PSOInitializer.DepthStencilState                      = GeometryDepthStencilState.Get();
        PSOInitializer.RasterizerState                        = GeometryRasterizerState.Get();
        PSOInitializer.ShaderState.VertexShader               = BaseVertexShader.Get();
        PSOInitializer.ShaderState.PixelShader                = BasePixelShader.Get();
        PSOInitializer.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
        PSOInitializer.PipelineFormats.RenderTargetFormats[1] = FrameResources.NormalFormat;
        PSOInitializer.PipelineFormats.RenderTargetFormats[2] = EFormat::R8G8B8A8_Unorm;
        PSOInitializer.PipelineFormats.RenderTargetFormats[3] = FrameResources.ViewNormalFormat;
        PSOInitializer.PipelineFormats.NumRenderTargets       = 4;

        PipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!PipelineState)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            PipelineState->SetName("GeometryPass PipelineState");
        }
    }

    // PrePass
    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Vertex);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/PrePass.hlsl", CompileInfo, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }

        PrePassVertexShader = RHICreateVertexShader(ShaderCode);
        if (!PrePassVertexShader)
        {
            CDebug::DebugBreak();
            return false;
        }

        FRHIDepthStencilStateInitializer DepthStencil;
        DepthStencil.DepthFunc      = EComparisonFunc::Less;
        DepthStencil.bDepthEnable   = true;
        DepthStencil.DepthWriteMask = EDepthWriteMask::All;

        TSharedRef<FRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState(DepthStencil);
        if (!DepthStencilState)
        {
            CDebug::DebugBreak();
            return false;
        }

        FRHIRasterizerStateInitializer RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::Back;

        TSharedRef<FRHIRasterizerState> RasterizerState = RHICreateRasterizerState(RasterizerStateInfo);
        if (!RasterizerState)
        {
            CDebug::DebugBreak();
            return false;
        }

        FRHIBlendStateInitializer BlendStateInfo;

        TSharedRef<FRHIBlendState> BlendState = RHICreateBlendState(BlendStateInfo);
        if (!BlendState)
        {
            CDebug::DebugBreak();
            return false;
        }

        FRHIGraphicsPipelineStateInitializer PSOInitializer;
        PSOInitializer.VertexInputLayout                  = FrameResources.StdInputLayout.Get();
        PSOInitializer.BlendState                         = BlendState.Get();
        PSOInitializer.DepthStencilState                  = DepthStencilState.Get();
        PSOInitializer.RasterizerState                    = RasterizerState.Get();
        PSOInitializer.ShaderState.VertexShader           = PrePassVertexShader.Get();
        PSOInitializer.PipelineFormats.DepthStencilFormat = FrameResources.DepthBufferFormat;

        PrePassPipelineState = RHICreateGraphicsPipelineState(PSOInitializer);
        if (!PrePassPipelineState)
        {
            CDebug::DebugBreak();
            return false;
        } 
    }

    constexpr uint32  LUTSize   = 512;
    constexpr EFormat LUTFormat = EFormat::R16G16_Float;
    if (!RHIQueryUAVFormatSupport(LUTFormat))
    {
        LOG_ERROR("[CRenderer]: R16G16_Float is not supported for UAVs");
        return false;
    }

    FRHITexture2DInitializer LUTInitializer(LUTFormat, LUTSize, LUTSize, 1, 1, ETextureUsageFlags::AllowUAV, EResourceAccess::Common);

    TSharedRef<FRHITexture2D> StagingTexture = RHICreateTexture2D(LUTInitializer);
    if (!StagingTexture)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        StagingTexture->SetName("Staging IntegrationLUT");
    }

    LUTInitializer.UsageFlags = ETextureUsageFlags::AllowSRV;

    FrameResources.IntegrationLUT = RHICreateTexture2D(LUTInitializer);
    if (!FrameResources.IntegrationLUT)
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.IntegrationLUT->SetName("IntegrationLUT");
    }

    FRHISamplerStateInitializer SamplerInitializer;
    SamplerInitializer.AddressU = ESamplerMode::Clamp;
    SamplerInitializer.AddressV = ESamplerMode::Clamp;
    SamplerInitializer.AddressW = ESamplerMode::Clamp;
    SamplerInitializer.Filter   = ESamplerFilter::MinMagMipPoint;

    FrameResources.IntegrationLUTSampler = RHICreateSamplerState(SamplerInitializer);
    if (!FrameResources.IntegrationLUTSampler)
    {
        CDebug::DebugBreak();
        return false;
    }

    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/BRDFIntegationGen.hlsl", CompileInfo, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }
    }

    TSharedRef<FRHIComputeShader> CShader = RHICreateComputeShader(ShaderCode);
    if (!CShader)
    {
        CDebug::DebugBreak();
        return false;
    }

    {
        FRHIComputePipelineStateInitializer PSOInitializer;
        PSOInitializer.Shader = CShader.Get();

        TSharedRef<FRHIComputePipelineState> BRDF_PipelineState = RHICreateComputePipelineState(PSOInitializer);
        if (!BRDF_PipelineState)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            BRDF_PipelineState->SetName("BRDFIntegationGen PipelineState");
        }

        FRHICommandList CmdList;

        CmdList.TransitionTexture(StagingTexture.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

        CmdList.SetComputePipelineState(BRDF_PipelineState.Get());

        FRHIUnorderedAccessView* StagingUAV = StagingTexture->GetUnorderedAccessView();
        CmdList.SetUnorderedAccessView(CShader.Get(), StagingUAV, 0);

        constexpr uint32 ThreadCount = 16;
        const uint32 DispatchWidth  = NMath::DivideByMultiple(LUTSize, ThreadCount);
        const uint32 DispatchHeight = NMath::DivideByMultiple(LUTSize, ThreadCount);
        CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

        CmdList.UnorderedAccessTextureBarrier(StagingTexture.Get());

        CmdList.TransitionTexture(StagingTexture.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::CopySource);
        CmdList.TransitionTexture(FrameResources.IntegrationLUT.Get(), EResourceAccess::Common, EResourceAccess::CopyDest);

        CmdList.CopyTexture(FrameResources.IntegrationLUT.Get(), StagingTexture.Get());

        CmdList.TransitionTexture(FrameResources.IntegrationLUT.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

        FRHICommandQueue::Get().ExecuteCommandList(CmdList);
    }

    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/DeferredLightPass.hlsl", CompileInfo, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }

        TiledLightShader = RHICreateComputeShader(ShaderCode);
        if (!TiledLightShader)
        {
            CDebug::DebugBreak();
            return false;
        }

        FRHIComputePipelineStateInitializer DeferredLightPassInitializer;
        DeferredLightPassInitializer.Shader = TiledLightShader.Get();

        TiledLightPassPSO = RHICreateComputePipelineState(DeferredLightPassInitializer);
        if (!TiledLightPassPSO)
        {
            CDebug::DebugBreak();
            return false;
        }
    }

    {
        TArray<FShaderDefine> Defines =
        {
            FShaderDefine("DRAW_TILE_DEBUG", "1")
        };

        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute, Defines.CreateView());
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/DeferredLightPass.hlsl", CompileInfo, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }

        TiledLightDebugShader = RHICreateComputeShader(ShaderCode);
        if (!TiledLightDebugShader)
        {
            CDebug::DebugBreak();
            return false;
        }

        FRHIComputePipelineStateInitializer DeferredLightPassInitializer;
        DeferredLightPassInitializer.Shader = TiledLightDebugShader.Get();

        TiledLightPassPSODebug = RHICreateComputePipelineState(DeferredLightPassInitializer);
        if (!TiledLightPassPSODebug)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            TiledLightPassPSODebug->SetName("DeferredLightPass PipelineState Debug");
        }
    }

    {
        FShaderCompileInfo CompileInfo("ReductionMainInital", EShaderModel::SM_6_0, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/DepthReduction.hlsl", CompileInfo, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }

        ReduceDepthInitalShader = RHICreateComputeShader(ShaderCode);
        if (!ReduceDepthInitalShader)
        {
            CDebug::DebugBreak();
            return false;
        }

        FRHIComputePipelineStateInitializer PipelineStateInfo;
        PipelineStateInfo.Shader = ReduceDepthInitalShader.Get();

        ReduceDepthInitalPSO = RHICreateComputePipelineState(PipelineStateInfo);
        if (!ReduceDepthInitalPSO)
        {
            CDebug::DebugBreak();
            return false;
        }
    }

    {
        FShaderCompileInfo CompileInfo("ReductionMain", EShaderModel::SM_6_0, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/DepthReduction.hlsl", CompileInfo, ShaderCode))
        {
            CDebug::DebugBreak();
            return false;
        }

        ReduceDepthShader = RHICreateComputeShader(ShaderCode);
        if (!ReduceDepthShader)
        {
            CDebug::DebugBreak();
            return false;
        }

        FRHIComputePipelineStateInitializer PSOInitializer;
        PSOInitializer.Shader = ReduceDepthShader.Get();

        ReduceDepthPSO = RHICreateComputePipelineState(PSOInitializer);
        if (!ReduceDepthPSO)
        {
            CDebug::DebugBreak();
            return false;
        }
        else
        {
            ReduceDepthPSO->SetName("DepthReduction PipelineState");
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

void CDeferredRenderer::RenderPrePass(FRHICommandList& CmdList, SFrameResources& FrameResources, const CScene& Scene)
{
    const float RenderWidth  = float(FrameResources.MainWindowViewport->GetWidth());
    const float RenderHeight = float(FrameResources.MainWindowViewport->GetHeight());

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin PrePass");

    {
        TRACE_SCOPE("PrePass");

        GPU_TRACE_SCOPE(CmdList, "Pre Pass");

        struct SPerObject
        {
            CMatrix4 Matrix;
        } PerObjectBuffer;

        FRHIRenderPassInitializer RenderPass;
        RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get());

        CmdList.BeginRenderPass(RenderPass);

        CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);
        
        // NOTE: For now, MetalRHI require a renderpass to be started for these two to be valid
        CmdList.SetViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
        CmdList.SetScissorRect(RenderWidth, RenderHeight, 0, 0);

        CmdList.SetGraphicsPipelineState(PrePassPipelineState.Get());

        CmdList.SetConstantBuffer(PrePassVertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

        for (const auto CommandIndex : FrameResources.DeferredVisibleCommands)
        {
            const SMeshDrawCommand& Command = FrameResources.GlobalMeshDrawCommands[CommandIndex];
            if (Command.Material->ShouldRenderInPrePass())
            {
                CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
                CmdList.SetIndexBuffer(Command.IndexBuffer);

                PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

                CmdList.Set32BitShaderConstants(PrePassVertexShader.Get(), &PerObjectBuffer, 16);

                CmdList.DrawIndexedInstanced(Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0);
            }
        }

        CmdList.EndRenderPass();
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End PrePass");

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Depth Reduction");

    {
        TRACE_SCOPE("Depth Reduction");

        GPU_TRACE_SCOPE(CmdList, "Depth Reduction");

        struct SReductionConstants
        {
            CMatrix4 CamProjection;
            float NearPlane;
            float FarPlane;
        } ReductionConstants;

        ReductionConstants.CamProjection = Scene.GetCamera()->GetProjectionMatrix();
        ReductionConstants.NearPlane     = Scene.GetCamera()->GetNearPlane();
        ReductionConstants.FarPlane      = Scene.GetCamera()->GetFarPlane();

        // Perform the first reduction
        CmdList.TransitionTexture(FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceAccess::DepthWrite, EResourceAccess::NonPixelShaderResource);
        CmdList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
        CmdList.TransitionTexture(FrameResources.ReducedDepthBuffer[1].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);

        CmdList.SetComputePipelineState(ReduceDepthInitalPSO.Get());

        CmdList.SetShaderResourceView(ReduceDepthInitalShader.Get(), FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(), 0);
        CmdList.SetUnorderedAccessView(ReduceDepthInitalShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0);

        CmdList.Set32BitShaderConstants(ReduceDepthInitalShader.Get(), &ReductionConstants, NMath::BytesToNum32BitConstants(sizeof(ReductionConstants)));

        uint32 ThreadsX = FrameResources.ReducedDepthBuffer[0]->GetWidth();
        uint32 ThreadsY = FrameResources.ReducedDepthBuffer[0]->GetHeight();
        CmdList.Dispatch(ThreadsX, ThreadsY, 1);

        CmdList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
        CmdList.TransitionTexture(FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::DepthWrite);

        // Perform the other reductions
        CmdList.SetComputePipelineState(ReduceDepthPSO.Get());

        CmdList.SetShaderResourceView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);
        CmdList.SetUnorderedAccessView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetUnorderedAccessView(), 0);

        ThreadsX = NMath::DivideByMultiple(ThreadsX, 16);
        ThreadsY = NMath::DivideByMultiple(ThreadsY, 16);
        CmdList.Dispatch(ThreadsX, ThreadsY, 1);

        CmdList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
        CmdList.TransitionTexture(FrameResources.ReducedDepthBuffer[1].Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);

        CmdList.SetShaderResourceView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[1]->GetShaderResourceView(), 0);
        CmdList.SetUnorderedAccessView(ReduceDepthShader.Get(), FrameResources.ReducedDepthBuffer[0]->GetUnorderedAccessView(), 0);

        ThreadsX = NMath::DivideByMultiple(ThreadsX, 16);
        ThreadsY = NMath::DivideByMultiple(ThreadsY, 16);
        CmdList.Dispatch(ThreadsX, ThreadsY, 1);

        CmdList.TransitionTexture(FrameResources.ReducedDepthBuffer[0].Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Depth Reduction");
}

void CDeferredRenderer::RenderBasePass(FRHICommandList& CmdList, const SFrameResources& FrameResources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin GeometryPass");

    TRACE_SCOPE("GeometryPass");

    GPU_TRACE_SCOPE(CmdList, "Base Pass");

    const float RenderWidth = float(FrameResources.MainWindowViewport->GetWidth());
    const float RenderHeight = float(FrameResources.MainWindowViewport->GetHeight());

    FRHIRenderPassInitializer RenderPass;
    RenderPass.RenderTargets[0] = FRHIRenderTargetView(FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX].Get());
    RenderPass.RenderTargets[1] = FRHIRenderTargetView(FrameResources.GBuffer[GBUFFER_NORMAL_INDEX].Get());
    RenderPass.RenderTargets[2] = FRHIRenderTargetView(FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX].Get());
    RenderPass.RenderTargets[3] = FRHIRenderTargetView(FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get());
    RenderPass.NumRenderTargets = 4;
    RenderPass.DepthStencilView = FRHIDepthStencilView(FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(), EAttachmentLoadAction::Load);

    CmdList.BeginRenderPass(RenderPass);

    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);
    
    // NOTE: For now, MetalRHI require a renderpass to be started for these two to be valid
    CmdList.SetViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.SetScissorRect(RenderWidth, RenderHeight, 0, 0);

    // Setup Pipeline
    CmdList.SetGraphicsPipelineState(PipelineState.Get());

    struct STransformBuffer
    {
        CMatrix4 Transform;
        CMatrix4 TransformInv;
    } TransformPerObject;

    for (const auto CommandIndex : FrameResources.DeferredVisibleCommands)
    {
        const SMeshDrawCommand& Command = FrameResources.GlobalMeshDrawCommands[CommandIndex];

        CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
        CmdList.SetIndexBuffer(Command.IndexBuffer);

        if (Command.Material->IsBufferDirty())
        {
            Command.Material->BuildBuffer(CmdList);
        }

        CmdList.SetConstantBuffer(BaseVertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

        FRHIConstantBuffer* MaterialBuffer = Command.Material->GetMaterialBuffer();
        CmdList.SetConstantBuffer(BasePixelShader.Get(), MaterialBuffer, 0);

        TransformPerObject.Transform = Command.CurrentActor->GetTransform().GetMatrix();
        TransformPerObject.TransformInv = Command.CurrentActor->GetTransform().GetMatrixInverse();

        FRHIShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[0], 0);
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[1], 1);
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[2], 2);
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[3], 3);
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[4], 4);
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[5], 5);
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[6], 6);

        FRHISamplerState* Sampler = Command.Material->GetMaterialSampler();
        CmdList.SetSamplerState(BasePixelShader.Get(), Sampler, 0);

        CmdList.Set32BitShaderConstants(BaseVertexShader.Get(), &TransformPerObject, 32);

        CmdList.DrawIndexedInstanced(Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0);
    }

    CmdList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End GeometryPass");
}

void CDeferredRenderer::RenderDeferredTiledLightPass(FRHICommandList& CmdList, const SFrameResources& FrameResources, const SLightSetup& LightSetup)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin LightPass");

    TRACE_SCOPE("LightPass");

    GPU_TRACE_SCOPE(CmdList, "Light Pass");

    FRHIComputeShader* LightPassShader = nullptr;
    if (GDrawTileDebug.GetBool())
    {
        LightPassShader = TiledLightDebugShader.Get();
        CmdList.SetComputePipelineState(TiledLightPassPSODebug.Get());
    }
    else
    {
        LightPassShader = TiledLightShader.Get();
        CmdList.SetComputePipelineState(TiledLightPassPSO.Get());
    }

    CmdList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetShaderResourceView(), 0);
    CmdList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView(), 1);
    CmdList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView(), 2);
    CmdList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(), 3);
    //CmdList.SetShaderResourceView(LightPassShader, nullptr, 4); // Reflection
    CmdList.SetShaderResourceView(LightPassShader, LightSetup.IrradianceMap->GetShaderResourceView(), 4);
    CmdList.SetShaderResourceView(LightPassShader, LightSetup.SpecularIrradianceMap->GetShaderResourceView(), 5);
    CmdList.SetShaderResourceView(LightPassShader, FrameResources.IntegrationLUT->GetShaderResourceView(), 6);
    CmdList.SetShaderResourceView(LightPassShader, LightSetup.DirectionalShadowMask->GetShaderResourceView(), 7);
    CmdList.SetShaderResourceView(LightPassShader, LightSetup.PointLightShadowMaps->GetShaderResourceView(), 8);
    CmdList.SetShaderResourceView(LightPassShader, FrameResources.SSAOBuffer->GetShaderResourceView(), 9);

    //CmdList.SetShaderResourceView(LightPassShader, LightSetup.CascadeMatrixBufferSRV.Get(), 13);
    //CmdList.SetShaderResourceView(LightPassShader, LightSetup.CascadeSplitsBufferSRV.Get(), 14);

    CmdList.SetConstantBuffer(LightPassShader, FrameResources.CameraBuffer.Get(), 0);
    CmdList.SetConstantBuffer(LightPassShader, LightSetup.PointLightsBuffer.Get(), 1);
    CmdList.SetConstantBuffer(LightPassShader, LightSetup.PointLightsPosRadBuffer.Get(), 2);
    CmdList.SetConstantBuffer(LightPassShader, LightSetup.ShadowCastingPointLightsBuffer.Get(), 3);
    CmdList.SetConstantBuffer(LightPassShader, LightSetup.ShadowCastingPointLightsPosRadBuffer.Get(), 4);
    CmdList.SetConstantBuffer(LightPassShader, LightSetup.DirectionalLightsBuffer.Get(), 5);

    CmdList.SetSamplerState(LightPassShader, FrameResources.IntegrationLUTSampler.Get(), 0);
    CmdList.SetSamplerState(LightPassShader, FrameResources.IrradianceSampler.Get(), 1);
    CmdList.SetSamplerState(LightPassShader, FrameResources.PointLightShadowSampler.Get(), 2);
    //CmdList.SetSamplerState(LightPassShader, FrameResources.DirectionalLightShadowSampler.Get(), 3);

    FRHIUnorderedAccessView* FinalTargetUAV = FrameResources.FinalTarget->GetUnorderedAccessView();
    CmdList.SetUnorderedAccessView(LightPassShader, FinalTargetUAV, 0);

    struct SLightPassSettings
    {
        int32 NumPointLights;
        int32 NumShadowCastingPointLights;
        int32 NumSkyLightMips;
        int32 ScreenWidth;
        int32 ScreenHeight;
    } Settings;

    Settings.NumShadowCastingPointLights = LightSetup.ShadowCastingPointLightsData.Size();
    Settings.NumPointLights              = LightSetup.PointLightsData.Size();
    Settings.NumSkyLightMips             = LightSetup.SpecularIrradianceMap->GetNumMips();
    Settings.ScreenWidth                 = FrameResources.FinalTarget->GetWidth();
    Settings.ScreenHeight                = FrameResources.FinalTarget->GetHeight();

    CmdList.Set32BitShaderConstants(LightPassShader, &Settings, 5);

    const FIntVector3 ThreadsXYZ = LightPassShader->GetThreadGroupXYZ();
    const uint32 WorkGroupWidth = NMath::DivideByMultiple<uint32>(Settings.ScreenWidth, ThreadsXYZ.x);
    const uint32 WorkGroupHeight = NMath::DivideByMultiple<uint32>(Settings.ScreenHeight, ThreadsXYZ.y);
    CmdList.Dispatch(WorkGroupWidth, WorkGroupHeight, 1);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End LightPass");
}

bool CDeferredRenderer::ResizeResources(SFrameResources& FrameResources)
{
    return CreateGBuffer(FrameResources);
}

bool CDeferredRenderer::CreateGBuffer(SFrameResources& FrameResources)
{
    const ETextureUsageFlags Usage = ETextureUsageFlags::RenderTarget;

    const uint32 Width  = FrameResources.MainWindowViewport->GetWidth();
    const uint32 Height = FrameResources.MainWindowViewport->GetHeight();

    // Albedo
    FRHITexture2DInitializer TextureInitializer(FrameResources.AlbedoFormat, Width, Height, 1, 1, Usage, EResourceAccess::Common, nullptr);

    FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX] = RHICreateTexture2D(TextureInitializer);
    if (FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->SetName("GBuffer Albedo");
    }
    else
    {
        return false;
    }

    // Normal
    TextureInitializer.Format = FrameResources.NormalFormat;

    FrameResources.GBuffer[GBUFFER_NORMAL_INDEX] = RHICreateTexture2D(TextureInitializer);
    if (FrameResources.GBuffer[GBUFFER_NORMAL_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->SetName("GBuffer Normal");
    }
    else
    {
        return false;
    }

    // Material Properties
    TextureInitializer.Format = FrameResources.MaterialFormat;

    FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX] = RHICreateTexture2D(TextureInitializer);
    if (FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->SetName("GBuffer Material");
    }
    else
    {
        return false;
    }

    // View Normal
    TextureInitializer.Format = FrameResources.ViewNormalFormat;

    FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX] = RHICreateTexture2D(TextureInitializer);
    if (FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->SetName("GBuffer ViewNormal");
    }
    else
    {
        return false;
    }

    // Final Image
    TextureInitializer.Format     = FrameResources.FinalTargetFormat;
    TextureInitializer.UsageFlags = Usage | ETextureUsageFlags::AllowUAV;

    FrameResources.FinalTarget = RHICreateTexture2D(TextureInitializer);
    if (FrameResources.FinalTarget)
    {
        FrameResources.FinalTarget->SetName("Final Target");
    }
    else
    {
        return false;
    }

    // DepthStencil
    const FTextureClearValue DepthClearValue(FrameResources.DepthBufferFormat, 1.0f, 0);
    TextureInitializer.Format     = FrameResources.DepthBufferFormat;
    TextureInitializer.UsageFlags = ETextureUsageFlags::ShadowMap;
    TextureInitializer.ClearValue = DepthClearValue;

    FrameResources.GBuffer[GBUFFER_DEPTH_INDEX] = RHICreateTexture2D(TextureInitializer);
    if (FrameResources.GBuffer[GBUFFER_DEPTH_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->SetName("GBuffer DepthStencil");
    }
    else
    {
        return false;
    }

    constexpr uint32 Alignment = 16;

    const uint32 ReducedWidth  = NMath::DivideByMultiple(Width, Alignment);
    const uint32 ReducedHeight = NMath::DivideByMultiple(Height, Alignment);

    TextureInitializer.Format        = EFormat::R32G32_Float;
    TextureInitializer.Width         = uint16(ReducedWidth);
    TextureInitializer.Height        = uint16(ReducedHeight);
    TextureInitializer.UsageFlags    = ETextureUsageFlags::RWTexture;
    TextureInitializer.InitialAccess = EResourceAccess::NonPixelShaderResource;

    for (uint32 i = 0; i < 2; i++)
    {
        FrameResources.ReducedDepthBuffer[i] = RHICreateTexture2D(TextureInitializer);
        if (FrameResources.ReducedDepthBuffer[i])
        {
            FrameResources.ReducedDepthBuffer[i]->SetName("Reduced DepthStencil[" + ToString(i) + "]");
        }
        else
        {
            return false;
        }
    }

    return true;
}

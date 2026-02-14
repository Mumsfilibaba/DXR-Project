#include "Renderer/DebugViewPass.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "RendererCore/RenderSettings.h"

FDebugViewPass::FDebugViewPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , DebugPSO_Linear(nullptr)
    , DebugPSO_BackBuffer(nullptr)
    , DebugVertexShader(nullptr)
    , DebugPixelShader(nullptr)
{
}

FDebugViewPass::~FDebugViewPass()
{
    DebugPSO_Linear.Reset();
    DebugPSO_BackBuffer.Reset();
    DebugVertexShader.Reset();
    DebugPixelShader.Reset();
}

bool FDebugViewPass::Initialize(const FFrameResources& /*FrameResources*/)
{
    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/FullscreenVS.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    DebugVertexShader = FRHI::Get()->CreateVertexShader(ShaderCode);
    if (!DebugVertexShader)
    {
        DEBUG_BREAK();
        return false;
    }

    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/DebugView.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    DebugPixelShader = FRHI::Get()->CreatePixelShader(ShaderCode);
    if (!DebugPixelShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateInfo DepthStencilInfo;
    DepthStencilInfo.DepthFunc         = EComparisonFunc::Always;
    DepthStencilInfo.bDepthEnable      = false;
    DepthStencilInfo.bDepthWriteEnable = false;

    FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilInfo);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInfo RasterizerInitializer;
    RasterizerInitializer.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerInitializer);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInfo BlendStateInfo;
    BlendStateInfo.NumRenderTargets = 1;

    FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateInfo PSOInfo;
    PSOInfo.InputLayout                                    = nullptr;
    PSOInfo.BlendState                                     = BlendState.Get();
    PSOInfo.DepthStencilState                              = DepthStencilState.Get();
    PSOInfo.RasterizerState                                = RasterizerState.Get();
    PSOInfo.VertexShader                                   = DebugVertexShader.Get();
    PSOInfo.PixelShader                                    = DebugPixelShader.Get();
    PSOInfo.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
    PSOInfo.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSOInfo.RasterizerOutputFormats.DepthStencilFormat     = EFormat::Unknown;

    // Linear output
    PSOInfo.RasterizerOutputFormats.RenderTargetFormats[0] = FGlobalTextureFormats::FinalTargetFormat;
    DebugPSO_Linear = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
    if (!DebugPSO_Linear)
    {
        DEBUG_BREAK();
        return false;
    }

    // BackBuffer output
    PSOInfo.RasterizerOutputFormats.RenderTargetFormats[0] = RenderSettings::GetBackBufferFormat();
    DebugPSO_BackBuffer = FRHI::Get()->CreateGraphicsPipelineState(PSOInfo);
    if (!DebugPSO_BackBuffer)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FDebugViewPass::Execute(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources, FSceneRenderView::EDebugView DebugView)
{
    if (DebugView == FSceneRenderView::EDebugView::None)
    {
        return;
    }

    FRHITexture* RenderTarget = SceneRenderView.RenderTarget;
    if (!RenderTarget)
    {
        return;
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin DebugView");

    TRACE_SCOPE("DebugView");

    GPU_TRACE_SCOPE(CommandList, "DebugView");

    const float RenderWidth  = static_cast<float>(FrameResources.CurrentRenderWidth);
    const float RenderHeight = static_cast<float>(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    const bool bNeedsTransition = !RenderTarget->GetInfo().IsPresentable();
    if (bNeedsTransition)
    {
        CommandList.RequireTextureState(RenderTarget, FRHIRequiredTextureState::Make(EResourceAccess::RenderTarget));
    }

    auto RequirePixel = [&CommandList](FRHITexture* Texture)
    {
        if (Texture)
        {
            CommandList.RequireTextureState(Texture, FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
        }
    };

    auto RequireNonPixel = [&CommandList](FRHITexture* Texture)
    {
        if (Texture)
        {
            CommandList.RequireTextureState(Texture, FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
        }
    };

    RequirePixel(FrameResources.GBuffer[GBufferIndex_Albedo].Get());
    RequirePixel(FrameResources.GBuffer[GBufferIndex_Normal].Get());
    RequirePixel(FrameResources.GBuffer[GBufferIndex_Material].Get());
    RequirePixel(FrameResources.GBuffer[GBufferIndex_Velocity].Get());
    RequirePixel(FrameResources.DirectionalShadowMask.Get());
    RequirePixel(FrameResources.SSAOBuffer.Get());
    RequirePixel(FrameResources.ShadowCascades.Get());
    RequirePixel(FrameResources.CascadeIndexBuffer.Get());

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.NumRenderTargets            = 1;
    RenderPass.RenderTargets[0]            = FRHIRenderTargetView(RenderTarget, EAttachmentLoadAction::Clear);
    RenderPass.RenderTargets[0].ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);

    CommandList.BeginRenderPass(RenderPass);

    const FRHIGraphicsPipelineStateRef& PSO =
        (RenderTarget->GetFormat() == RenderSettings::GetBackBufferFormat()) ? DebugPSO_BackBuffer : DebugPSO_Linear;
    CommandList.SetGraphicsPipelineState(PSO.Get());

    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.GBuffer[GBufferIndex_Albedo]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView(), 1);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.GBuffer[GBufferIndex_Material]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.GBuffer[GBufferIndex_Velocity]->GetShaderResourceView(), 3);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 4);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.DirectionalShadowMask->GetShaderResourceView(), 5);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.SSAOBuffer->GetShaderResourceView(), 6);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.ShadowCascades->GetShaderResourceView(), 7);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.CascadeIndexBuffer->GetShaderResourceView(), 8);

    CommandList.SetConstantBuffer(DebugPixelShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    FRHISamplerState* PointSampler  = FrameResources.GBufferSampler.Get();
    FRHISamplerState* LinearSampler = FrameResources.FXAASampler ? FrameResources.FXAASampler.Get() : FrameResources.GBufferSampler.Get();
    CommandList.SetSamplerState(DebugPixelShader.Get(), LinearSampler, 0);
    CommandList.SetSamplerState(DebugPixelShader.Get(), PointSampler, 1);

    struct FDebugViewConstants
    {
        int32 DebugMode = 0;
        int32 Padding0  = 0;
        int32 Padding1  = 0;
        int32 Padding2  = 0;
    } Constants;

    Constants.DebugMode = static_cast<int32>(DebugView);

    constexpr uint32 NumConstants = sizeof(FDebugViewConstants) / sizeof(uint32);
    CommandList.SetShaderConstants(DebugPixelShader.Get(), &Constants, NumConstants);

    CommandList.DrawInstanced(3, 1, 0, 0);

    CommandList.EndRenderPass();

    RequireNonPixel(FrameResources.GBuffer[GBufferIndex_Albedo].Get());
    RequireNonPixel(FrameResources.GBuffer[GBufferIndex_Normal].Get());
    RequireNonPixel(FrameResources.GBuffer[GBufferIndex_Material].Get());
    RequireNonPixel(FrameResources.GBuffer[GBufferIndex_Velocity].Get());
    RequireNonPixel(FrameResources.DirectionalShadowMask.Get());
    RequireNonPixel(FrameResources.SSAOBuffer.Get());
    RequireNonPixel(FrameResources.ShadowCascades.Get());
    RequireNonPixel(FrameResources.CascadeIndexBuffer.Get());

    if (bNeedsTransition)
    {
        CommandList.RequireTextureState(RenderTarget, FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End DebugView");
}

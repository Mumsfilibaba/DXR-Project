#include "Renderer/DebugViewPass.h"
#include "Core/Math/Math.h"
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

    FRHIDepthStencilStateDesc DepthStencilDesc;
    DepthStencilDesc.DepthFunc         = EComparisonFunc::Always;
    DepthStencilDesc.bDepthEnable      = false;
    DepthStencilDesc.bDepthWriteEnable = false;

    FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilDesc);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerDesc;
    RasterizerDesc.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerDesc);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.NumRenderTargets = 1;

    FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateDesc);
    if (!BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateDesc PSODesc;
    PSODesc.InputLayout                                    = nullptr;
    PSODesc.BlendState                                     = BlendState.Get();
    PSODesc.DepthStencilState                              = DepthStencilState.Get();
    PSODesc.RasterizerState                                = RasterizerState.Get();
    PSODesc.VertexShader                                   = DebugVertexShader.Get();
    PSODesc.PixelShader                                    = DebugPixelShader.Get();
    PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
    PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSODesc.RasterizerOutputFormats.DepthStencilFormat     = EFormat::Unknown;
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = GlobalTextureFormats::FinalTargetFormat;

    // Linear output
    DebugPSO_Linear = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
    if (!DebugPSO_Linear)
    {
        DEBUG_BREAK();
        return false;
    }

    // BackBuffer output
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = RenderSettings::GetBackBufferFormat();

    DebugPSO_BackBuffer = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
    if (!DebugPSO_BackBuffer)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FDebugViewPass::Execute(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources, FSceneRenderView::EDebugView DebugView)
{
    ExecuteInternal(CommandList, SceneRenderView, FrameResources, DebugView, 0, 0, 0, 0, true, true);
}

void FDebugViewPass::ExecuteOverlay(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources, FSceneRenderView::EDebugView DebugView, int32 X, int32 Y, int32 Width, int32 Height)
{
    ExecuteInternal(CommandList, SceneRenderView, FrameResources, DebugView, X, Y, Width, Height, false, true);
}

void FDebugViewPass::ExecuteInternal(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources, FSceneRenderView::EDebugView DebugView, int32 X, int32 Y, int32 Width, int32 Height, bool bClearTarget, bool bPreferTonemapped)
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

    const int32 TargetWidth  = static_cast<int32>(RenderTarget->GetWidth());
    const int32 TargetHeight = static_cast<int32>(RenderTarget->GetHeight());
    const int32 ViewX        = Math::Clamp(X, 0, TargetWidth);
    const int32 ViewY        = Math::Clamp(Y, 0, TargetHeight);

    int32 ViewWidth  = Width > 0 ? Width : TargetWidth;
    int32 ViewHeight = Height > 0 ? Height : TargetHeight;

    ViewWidth  = Math::Clamp(ViewWidth, 0, TargetWidth - ViewX);
    ViewHeight = Math::Clamp(ViewHeight, 0, TargetHeight - ViewY);

    if (ViewWidth <= 0 || ViewHeight <= 0)
    {
        return;
    }

    FViewportRegion ViewportRegion(static_cast<float>(ViewWidth), static_cast<float>(ViewHeight), static_cast<float>(ViewX), static_cast<float>(ViewY), 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(ViewWidth, ViewHeight, ViewX, ViewY);
    CommandList.SetScissorRect(ScissorRegion);

    const bool bNeedsTransition = !RenderTarget->GetDesc().IsPresentable();
    if (bNeedsTransition)
    {
        CommandList.RequireTextureState(RenderTarget, FRHIRequiredTextureState::Make(EResourceAccess::RenderTarget));
    }

    const auto RequirePixel = [&CommandList](FRHITexture* Texture)
    {
        if (Texture)
        {
            CommandList.RequireTextureState(Texture, FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
        }
    };

    const auto RequireNonPixel = [&CommandList](FRHITexture* Texture)
    {
        if (Texture)
        {
            CommandList.RequireTextureState(Texture, FRHIRequiredTextureState::Make(EResourceAccess::NonPixelShaderResource));
        }
    };

    const auto RequirePixelIfNotRT = [&](FRHITexture* Texture)
    {
        if (Texture && Texture != RenderTarget)
        {
            RequirePixel(Texture);
        }
    };

    const auto RequireNonPixelIfNotRT = [&](FRHITexture* Texture)
    {
        if (Texture && Texture != RenderTarget)
        {
            RequireNonPixel(Texture);
        }
    };

    RequirePixelIfNotRT(FrameResources.GBuffer[GBufferIndex_Albedo].Get());
    RequirePixelIfNotRT(FrameResources.GBuffer[GBufferIndex_Normal].Get());
    RequirePixelIfNotRT(FrameResources.GBuffer[GBufferIndex_Material].Get());
    RequirePixelIfNotRT(FrameResources.GBuffer[GBufferIndex_Velocity].Get());
    RequirePixelIfNotRT(FrameResources.DirectionalShadowMask.Get());
    RequirePixelIfNotRT(FrameResources.SSAOBuffer.Get());
    RequirePixelIfNotRT(FrameResources.ShadowCascades.Get());
    RequirePixelIfNotRT(FrameResources.CascadeIndexBuffer.Get());
    RequirePixelIfNotRT(FrameResources.ShadowDebugBuffer.Get());
    RequirePixelIfNotRT(FrameResources.TonemappedTarget.Get());
    RequirePixelIfNotRT(FrameResources.FinalTarget.Get());
    if (FrameResources.CascadeSplitsBuffer)
    {
        CommandList.RequireBufferState(FrameResources.CascadeSplitsBuffer.Get(), EResourceAccess::PixelShaderResource);
    }

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.NumRenderTargets = 1;
    RenderPassDesc.RenderTargets[0] = FRHIRenderTargetView(RenderTarget, bClearTarget ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load);
    RenderPassDesc.RenderTargets[0].ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);

    CommandList.BeginRenderPass(RenderPassDesc);

    const FRHIGraphicsPipelineStateRef& PSO = RenderTarget->GetFormat() == RenderSettings::GetBackBufferFormat() ? DebugPSO_BackBuffer : DebugPSO_Linear;
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
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.ShadowDebugBuffer->GetShaderResourceView(), 9);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.CascadeSplitsBufferSRV.Get(), 11);

    FRHITexture* LitSourceTexture = nullptr;
    if (bPreferTonemapped && FrameResources.TonemappedTarget)
    {
        LitSourceTexture = FrameResources.TonemappedTarget.Get();
    }
    else
    {
        LitSourceTexture = FrameResources.FinalTarget.Get();
    }

    if (LitSourceTexture)
    {
        CommandList.SetShaderResourceView(DebugPixelShader.Get(), LitSourceTexture->GetShaderResourceView(), 10);
    }

    CommandList.SetConstantBuffer(DebugPixelShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    FRHISamplerState* PointSampler  = FrameResources.GBufferSampler.Get();
    FRHISamplerState* LinearSampler = FrameResources.FXAASampler ? FrameResources.FXAASampler.Get() : FrameResources.GBufferSampler.Get();
    CommandList.SetSamplerState(DebugPixelShader.Get(), LinearSampler, 0);
    CommandList.SetSamplerState(DebugPixelShader.Get(), PointSampler, 1);

    struct FDebugViewConstants
    {
        int32 DebugMode          = 0;
        int32 ShadowMapSize      = 0;
        int32 OutputWidth        = 0;
        int32 OutputHeight       = 0;
        int32 ViewX              = 0;
        int32 ViewY              = 0;
        int32 TargetWidth        = 0;
        int32 TargetHeight       = 0;
        int32 OutputIsBackBuffer = 0;
    } Constants;

    Constants.DebugMode          = static_cast<int32>(DebugView);
    Constants.ShadowMapSize      = FrameResources.ShadowCascades ? static_cast<int32>(FrameResources.ShadowCascades->GetWidth()) : 0;
    Constants.OutputWidth        = ViewWidth;
    Constants.OutputHeight       = ViewHeight;
    Constants.ViewX              = ViewX;
    Constants.ViewY              = ViewY;
    Constants.TargetWidth        = TargetWidth;
    Constants.TargetHeight       = TargetHeight;
    Constants.OutputIsBackBuffer = (RenderTarget->GetFormat() == RenderSettings::GetBackBufferFormat()) ? 1 : 0;

    constexpr uint32 NumConstants = sizeof(FDebugViewConstants) / sizeof(uint32);
    CommandList.SetShaderConstants(DebugPixelShader.Get(), &Constants, NumConstants);

    CommandList.DrawInstanced(3, 1, 0, 0);

    CommandList.EndRenderPass();

    RequireNonPixelIfNotRT(FrameResources.GBuffer[GBufferIndex_Albedo].Get());
    RequireNonPixelIfNotRT(FrameResources.GBuffer[GBufferIndex_Normal].Get());
    RequireNonPixelIfNotRT(FrameResources.GBuffer[GBufferIndex_Material].Get());
    RequireNonPixelIfNotRT(FrameResources.GBuffer[GBufferIndex_Velocity].Get());
    RequireNonPixelIfNotRT(FrameResources.DirectionalShadowMask.Get());
    RequireNonPixelIfNotRT(FrameResources.SSAOBuffer.Get());
    RequireNonPixelIfNotRT(FrameResources.ShadowCascades.Get());
    RequireNonPixelIfNotRT(FrameResources.CascadeIndexBuffer.Get());
    RequireNonPixelIfNotRT(FrameResources.ShadowDebugBuffer.Get());
    RequireNonPixelIfNotRT(FrameResources.TonemappedTarget.Get());
    RequireNonPixelIfNotRT(FrameResources.FinalTarget.Get());
    if (FrameResources.CascadeSplitsBuffer)
    {
        CommandList.RequireBufferState(FrameResources.CascadeSplitsBuffer.Get(), EResourceAccess::NonPixelShaderResource);
    }

    if (bNeedsTransition)
    {
        CommandList.RequireTextureState(RenderTarget, FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End DebugView");
}

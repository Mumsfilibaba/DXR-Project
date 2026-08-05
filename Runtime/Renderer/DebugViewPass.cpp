#include "Renderer/DebugViewPass.h"
#include "Core/Math/Math.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Renderer/Performance/GPUProfiler.h"

FDebugViewPass::FDebugViewPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , DebugPSO(nullptr)
    , DebugVertexShader(nullptr)
    , DebugPixelShader(nullptr)
    , DebugDepthStencilState(nullptr)
    , DebugRasterizerState(nullptr)
    , DebugBlendState(nullptr)
{
}

FDebugViewPass::~FDebugViewPass()
{
    DebugPSO.Reset();
    DebugVertexShader.Reset();
    DebugPixelShader.Reset();
    DebugDepthStencilState.Reset();
    DebugRasterizerState.Reset();
    DebugBlendState.Reset();
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

    DebugVertexShader = RHI::CreateVertexShader(ShaderCode);
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

    DebugPixelShader = RHI::CreatePixelShader(ShaderCode);
    if (!DebugPixelShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilDesc;
    DepthStencilDesc.DepthFunc         = EComparisonFunc::Always;
    DepthStencilDesc.bDepthEnable      = false;
    DepthStencilDesc.bDepthWriteEnable = false;

    DebugDepthStencilState = RHI::CreateDepthStencilState(DepthStencilDesc);
    if (!DebugDepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerDesc;
    RasterizerDesc.CullMode = ECullMode::None;

    DebugRasterizerState = RHI::CreateRasterizerState(RasterizerDesc);
    if (!DebugRasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.NumRenderTargets = 1;

    DebugBlendState = RHI::CreateBlendState(BlendStateDesc);
    if (!DebugBlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FDebugViewPass::PreparePipelineStateForFormat(EFormat OutputFormat)
{
    if (DebugPSO && DebugPSOFormat == OutputFormat)
    {
        return;
    }

    FRHIGraphicsPipelineStateDesc PSODesc;
    PSODesc.InputLayout                                    = nullptr;
    PSODesc.BlendState                                     = DebugBlendState.Get();
    PSODesc.DepthStencilState                              = DebugDepthStencilState.Get();
    PSODesc.RasterizerState                                = DebugRasterizerState.Get();
    PSODesc.VertexShader                                   = DebugVertexShader.Get();
    PSODesc.PixelShader                                    = DebugPixelShader.Get();
    PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
    PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = OutputFormat;
    PSODesc.RasterizerOutputFormats.DepthStencilFormat     = EFormat::Unknown;

    FRHIGraphicsPipelineStateRef NewPSO = RHI::CreateGraphicsPipelineState(PSODesc);
    if (!NewPSO)
    {
        DEBUG_BREAK();
        return;
    }

    DebugPSO       = NewPSO;
    DebugPSOFormat = OutputFormat;
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

    TRACE_SCOPE("DebugView");
    GPU_TRACE_SCOPE(CommandList, "DebugView");

    const int32 TargetWidth  = static_cast<int32>(RenderTarget->GetDesc().Extent.X);
    const int32 TargetHeight = static_cast<int32>(RenderTarget->GetDesc().Extent.Y);
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

    FScissorRegion ScissorRegion(static_cast<float>(ViewWidth), static_cast<float>(ViewHeight), static_cast<float>(ViewX), static_cast<float>(ViewY));
    CommandList.SetScissorRect(ScissorRegion);

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(RenderTarget, ERHIResourceState::RenderTarget));

    const auto RequirePixel = [&CommandList](FRHITexture* Texture)
    {
        if (Texture)
        {
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Texture, ERHIResourceState::PixelShaderResource));
        }
    };

    const auto RequirePixelIfNotRT = [&](FRHITexture* Texture)
    {
        if (Texture && Texture != RenderTarget)
        {
            RequirePixel(Texture);
        }
    };

    RequirePixelIfNotRT(FrameResources.GBuffer[EGBufferIndex::Albedo].Get());
    RequirePixelIfNotRT(FrameResources.GBuffer[EGBufferIndex::Normal].Get());
    RequirePixelIfNotRT(FrameResources.GBuffer[EGBufferIndex::Material].Get());
    RequirePixelIfNotRT(FrameResources.GBuffer[EGBufferIndex::Velocity].Get());
    RequirePixelIfNotRT(FrameResources.DirectionalShadowMask.Get());
    RequirePixelIfNotRT(FrameResources.SSAOBuffer.Get());
    RequirePixelIfNotRT(FrameResources.ShadowCascades.Get());
    RequirePixelIfNotRT(FrameResources.CascadeIndexBuffer.Get());
    RequirePixelIfNotRT(FrameResources.TonemappedTarget.Get());
    RequirePixelIfNotRT(FrameResources.SceneTarget.Get());
    RequirePixelIfNotRT(FrameResources.RayTracingOutput.Get());
    RequirePixelIfNotRT(FrameResources.ReflectionTrace.Get());
    RequirePixelIfNotRT(FrameResources.ReflectionDenoised[0].Get());

    FRHIRenderTargetView* RenderTargetView = RenderTarget->GetRenderTargetView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.NumRenderTargets = 1;
    RenderPassDesc.RenderTargets[0] = FRHIRenderPassAttachment(
        RenderTargetView, 
        bClearTarget ? EAttachmentLoadAction::Clear : EAttachmentLoadAction::Load, 
        EAttachmentStoreAction::Store, 
        FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));

    CommandList.BeginRenderPass(RenderPassDesc);

    CommandList.SetGraphicsPipelineState(DebugPSO.Get());

    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.GBuffer[EGBufferIndex::Albedo]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.GBuffer[EGBufferIndex::Normal]->GetShaderResourceView(), 1);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.GBuffer[EGBufferIndex::Material]->GetShaderResourceView(), 2);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.GBuffer[EGBufferIndex::Velocity]->GetShaderResourceView(), 3);
    CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 4);

    if (FrameResources.DirectionalShadowMask)
    {
        CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.DirectionalShadowMask->GetShaderResourceView(), 5);
    }
    
    if (FrameResources.SSAOBuffer)
    {
        CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.SSAOBuffer->GetShaderResourceView(), 6);
    }
    
    if (FrameResources.ShadowCascades)
    {
        CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.ShadowCascades->GetShaderResourceView(), 7);
    }
    
    if (FrameResources.CascadeIndexBuffer)
    {
        CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.CascadeIndexBuffer->GetShaderResourceView(), 8);
    }

    FRHITexture* LitSourceTexture = nullptr;
    if (bPreferTonemapped && FrameResources.TonemappedTarget)
    {
        LitSourceTexture = FrameResources.TonemappedTarget.Get();
    }
    else
    {
        LitSourceTexture = FrameResources.SceneTarget.Get();
    }

    if (LitSourceTexture)
    {
        CommandList.SetShaderResourceView(DebugPixelShader.Get(), LitSourceTexture->GetShaderResourceView(), 9);
    }

    if (FrameResources.RayTracingOutput)
    {
        CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.RayTracingOutput->GetShaderResourceView(), 10);
    }

    if (FrameResources.ReflectionTrace)
    {
        CommandList.SetShaderResourceView(DebugPixelShader.Get(), FrameResources.ReflectionTrace->GetShaderResourceView(), 11);
    }

    if (FRHITexture* TemporalHistory = FrameResources.ReflectionHistory[FrameResources.ReflectionHistoryIndex].Get())
    {
        CommandList.SetShaderResourceView(DebugPixelShader.Get(), TemporalHistory->GetShaderResourceView(), 12);
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
        int32 bIsOutputSceneTarget = 0;
        int32 ChannelMask        = 0;
    } Constants;

    Constants.DebugMode            = static_cast<int32>(DebugView);
    Constants.ShadowMapSize        = FrameResources.ShadowCascades ? static_cast<int32>(FrameResources.ShadowCascades->GetDesc().Extent.X) : 0;
    Constants.OutputWidth          = ViewWidth;
    Constants.OutputHeight         = ViewHeight;
    Constants.ViewX                = ViewX;
    Constants.ViewY                = ViewY;
    Constants.TargetWidth          = TargetWidth;
    Constants.TargetHeight         = TargetHeight;
    Constants.bIsOutputSceneTarget = (RenderTarget->GetDesc().Format == RendererTextureFormats::SceneTargetFormat) ? 1 : 0;
    Constants.ChannelMask          = static_cast<int32>(SceneRenderView.DebugViewChannelMask);

    constexpr uint32 NumConstants = sizeof(FDebugViewConstants) / sizeof(uint32);
    CommandList.SetShaderConstants(DebugPixelShader.Get(), &Constants, NumConstants);

    CommandList.DrawInstanced(3, 1, 0, 0);

    CommandList.EndRenderPass();

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(RenderTarget, ERHIResourceState::PixelShaderResource));
}

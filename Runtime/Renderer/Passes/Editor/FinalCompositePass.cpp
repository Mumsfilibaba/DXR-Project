#include "Core/Misc/FrameProfiler.h"
#include "Renderer/Shaders/CommonShaders.h"
#include "Renderer/Passes/Editor/FinalCompositePass.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/Settings/EditorGridSettings.h"
#include "Renderer/Settings/SelectionOutlineSettings.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

#if EDITOR_BUILD

class FFinalCompositePS
{
    DECLARE_SHADER_TYPE(FFinalCompositePS, EShaderStage::Pixel);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FFinalCompositePS, "Shaders/FinalComposite.hlsl", "Main", EShaderModel::SM_6_2);

FFinalCompositePass::FFinalCompositePass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , CompositePSO(nullptr)
    , CompositeVertexShader(nullptr)
    , CompositeShader(nullptr)
    , CompositeDepthStencilState(nullptr)
    , CompositeRasterizerState(nullptr)
    , CompositeBlendState(nullptr)
{
}

FFinalCompositePass::~FFinalCompositePass()
{
    CompositePSO.Reset();
    CompositeVertexShader.Reset();
    CompositeShader.Reset();
    CompositeDepthStencilState.Reset();
    CompositeRasterizerState.Reset();
    CompositeBlendState.Reset();
}

bool FFinalCompositePass::Initialize(const FFrameResources& /*FrameResources*/)
{
    CompositeVertexShader = FShaderCache::Get().GetShader<FFullscreenVS>();
    if (!CompositeVertexShader)
    {
        DEBUG_BREAK();
        return false;
    }

    CompositeShader = FShaderCache::Get().GetShader<FFinalCompositePS>();
    if (!CompositeShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::Always;
    DepthStencilStateDesc.bDepthEnable      = false;
    DepthStencilStateDesc.bDepthWriteEnable = false;

    CompositeDepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
    if (!CompositeDepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    CompositeRasterizerState = RHI::CreateRasterizerState(RasterizerStateDesc);
    if (!CompositeRasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.NumRenderTargets = 1;

    CompositeBlendState = RHI::CreateBlendState(BlendStateDesc);
    if (!CompositeBlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FFinalCompositePass::PreparePipelineStateForFormat(EFormat OutputFormat)
{
    if (CompositePSO && CompositePSOFormat == OutputFormat)
    {
        return;
    }

    FRHIGraphicsPipelineStateDesc PSODesc;
    PSODesc.InputLayout                                    = nullptr;
    PSODesc.BlendState                                     = CompositeBlendState.Get();
    PSODesc.DepthStencilState                              = CompositeDepthStencilState.Get();
    PSODesc.RasterizerState                                = CompositeRasterizerState.Get();
    PSODesc.VertexShader                                   = CompositeVertexShader.Get();
    PSODesc.PixelShader                                    = CompositeShader.Get();
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

    CompositePSO       = NewPSO;
    CompositePSOFormat = OutputFormat;
}

void FFinalCompositePass::Record(FRHICommandList& CommandList, const FFrameResources& FrameResources)
{
    if (!FrameResources.TonemappedTarget)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "Final Composite");

    TRACE_SCOPE("Final Composite");

    GPU_TRACE_SCOPE(CommandList, "Final Composite");

    const float RenderWidth  = static_cast<float>(FrameResources.CurrentRenderWidth);
    const float RenderHeight = static_cast<float>(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    CommandList.SetGraphicsPipelineState(CompositePSO.Get());

    CommandList.SetShaderResourceView(CompositeShader.Get(), FrameResources.TonemappedTarget->GetShaderResourceView(), 0);
    CommandList.SetConstantBuffer(CompositeShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    const FEditorGridSettings       GridSettings    = GetEditorGridSettings();
    const FSelectionOutlineSettings OutlineSettings = GetSelectionOutlineSettings();

    FRHITexture* SelectionRingTexture  = GetRenderer()->GetSelectionRingTexture();
    const bool bCanUseGrid             = GridSettings.bEnabled && (FrameResources.EditorNoJitterDepth != nullptr);
    const bool bCanUseSelectionOutline = OutlineSettings.bEnabled && (SelectionRingTexture != nullptr);

    if (bCanUseSelectionOutline)
    {
        CommandList.SetShaderResourceView(CompositeShader.Get(), SelectionRingTexture->GetShaderResourceView(), 1);
    }
    else
    {
        CommandList.SetShaderResourceView(CompositeShader.Get(), nullptr, 1);
    }

    if (bCanUseGrid)
    {
        CommandList.SetShaderResourceView(CompositeShader.Get(), FrameResources.EditorNoJitterDepth->GetShaderResourceView(), 2);
    }
    else
    {
        CommandList.SetShaderResourceView(CompositeShader.Get(), nullptr, 2);
    }

    FRHISamplerState* PointSampler  = FrameResources.GBufferSampler.Get();
    FRHISamplerState* LinearSampler = FrameResources.FXAASampler ? FrameResources.FXAASampler.Get() : FrameResources.GBufferSampler.Get();
    CommandList.SetSamplerState(CompositeShader.Get(), PointSampler, 0);
    CommandList.SetSamplerState(CompositeShader.Get(), LinearSampler, 1);

    FFinalCompositeInfoHLSL Info;
    Info.bEnableSelectionOutline = bCanUseSelectionOutline ? 1 : 0;
    Info.bEnableGrid             = bCanUseGrid ? 1 : 0;
    Info.OutlineAlpha            = OutlineSettings.Alpha;
    Info.GridPlaneY              = GridSettings.PlaneY;
    Info.GridMinorSize           = GridSettings.MinorSize;
    Info.GridMajorSize           = GridSettings.MajorSize;
    Info.GridMinorWidth          = GridSettings.MinorWidth;
    Info.GridMajorWidth          = GridSettings.MajorWidth;
    Info.OutlineColor            = OutlineSettings.Color;
    Info.GridFadeDistance        = GridSettings.FadeDistance;
    Info.GridMaxTraceDistance    = GridSettings.MaxTraceDistance;
    Info.GridMinorColor          = GridSettings.MinorColor;
    Info.GridMinorAlpha          = GridSettings.MinorAlpha;
    Info.GridMajorColor          = GridSettings.MajorColor;
    Info.GridMajorAlpha          = GridSettings.MajorAlpha;
    Info.GridHorizonFade         = GridSettings.HorizonFade;
    Info.GridDepthBias           = GridSettings.DepthBias;
    Info.Padding2                = 0.0f;

    constexpr uint32 NumConstants = sizeof(FFinalCompositeInfoHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(CompositeShader.Get(), &Info, NumConstants);

    CommandList.DrawInstanced(3, 1, 0, 0);
}

void FFinalCompositePass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    GraphBuilder.AddPass("FinalComposite", ERenderGraphPassFlags::Raster, true,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.TonemappedTarget, ERHIResourceState::PixelShaderResource);

            if (Context.EditorNoJitterDepth)
            {
                PassBuilder.ReadTexture(Context.EditorNoJitterDepth, ERHIResourceState::PixelShaderResource);
            }

            if (Context.SelectionRing)
            {
                PassBuilder.ReadTexture(Context.SelectionRing, ERHIResourceState::PixelShaderResource);
            }

            PassBuilder.SetRenderTarget(0, Context.BackBuffer, EAttachmentLoadAction::Load);
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            Record(PassCommandList, *Context.FrameResources);
        });
}

#endif

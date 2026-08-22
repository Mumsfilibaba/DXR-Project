#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Renderer/Shaders/CommonShaders.h"
#include "Renderer/Passes/FXAAPass.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

static bool GFXAADebug = false;
static FAutoConsoleVariableRef CVarFXAADebug(
    "Renderer.Debug.FXAADebug",
    "Enables FXAA (Anti-Aliasing) Debugging mode",
    GFXAADebug);

class FFXAADebug : SHADER_PERMUTATION_BOOL("ENABLE_DEBUG");

struct FFXAASettings
{
    float Width;
    float Height;
};

class FFXAAPS
{
    DECLARE_SHADER_TYPE(FFXAAPS, EShaderStage::Pixel);

    using FPermutation = TShaderPermutation<FFXAADebug>;
};

IMPLEMENT_SHADER_TYPE(FFXAAPS, "Shaders/FXAA_PS.hlsl", "Main", EShaderModel::SM_6_2);

FFXAAPass::FFXAAPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , FXAAPSO(nullptr)
    , FXAAShader(nullptr)
    , FXAADebugPSO(nullptr)
    , FXAADebugShader(nullptr)
    , FXAAVertexShader(nullptr)
    , FXAADepthStencilState(nullptr)
    , FXAARasterizerState(nullptr)
    , FXAABlendState(nullptr)
{
}

FFXAAPass::~FFXAAPass()
{
    FXAAPSO.Reset();
    FXAAShader.Reset();
    FXAADebugPSO.Reset();
    FXAADebugShader.Reset();
    FXAAVertexShader.Reset();
    FXAADepthStencilState.Reset();
    FXAARasterizerState.Reset();
    FXAABlendState.Reset();
}

bool FFXAAPass::Initialize(FFrameResources& FrameResources)
{
    FXAAVertexShader = FShaderCache::Get().GetShader<FFullscreenVS>();
    if (!FXAAVertexShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FFXAAPS::FPermutation FXAAPermutation;
    FXAAPermutation.Set<FFXAADebug>(false);

    FXAAShader = FShaderCache::Get().GetShader<FFXAAPS>(FXAAPermutation);
    if (!FXAAShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::Always;
    DepthStencilStateDesc.bDepthEnable      = false;
    DepthStencilStateDesc.bDepthWriteEnable = false;

    FXAADepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
    if (!FXAADepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    FXAARasterizerState = RHI::CreateRasterizerState(RasterizerStateDesc);
    if (!FXAARasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.NumRenderTargets = 1;

    FXAABlendState = RHI::CreateBlendState(BlendStateDesc);
    if (!FXAABlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FXAAPermutation.Set<FFXAADebug>(true);

    FXAADebugShader = FShaderCache::Get().GetShader<FFXAAPS>(FXAAPermutation);
    if (!FXAADebugShader)
    {
        DEBUG_BREAK();
        return false;
    }

    // FXAA
    FRHISamplerStateDesc SamplerDesc;
    SamplerDesc.AddressU = ESamplerMode::Clamp;
    SamplerDesc.AddressV = ESamplerMode::Clamp;
    SamplerDesc.AddressW = ESamplerMode::Clamp;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipLinear;

    FrameResources.FXAASampler = RHI::CreateSamplerState(SamplerDesc);
    if (!FrameResources.FXAASampler)
    {
        return false;
    }

    return true;
}

void FFXAAPass::PreparePipelineStateForFormat(EFormat OutputFormat)
{
    FRHIGraphicsPipelineStateDesc PSODesc;
    PSODesc.InputLayout                                    = nullptr;
    PSODesc.BlendState                                     = FXAABlendState.Get();
    PSODesc.DepthStencilState                              = FXAADepthStencilState.Get();
    PSODesc.RasterizerState                                = FXAARasterizerState.Get();
    PSODesc.VertexShader                                   = FXAAVertexShader.Get();
    PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
    PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = OutputFormat;
    PSODesc.RasterizerOutputFormats.DepthStencilFormat     = EFormat::Unknown;

    if (!FXAAPSO || FXAAPSOFormat != OutputFormat)
    {
        PSODesc.PixelShader = FXAAShader.Get();

        FRHIGraphicsPipelineStateRef NewPSO = RHI::CreateGraphicsPipelineState(PSODesc);
        if (NewPSO)
        {
            NewPSO->SetDebugName("FXAA PipelineState");
            FXAAPSO       = NewPSO;
            FXAAPSOFormat = OutputFormat;
        }
        else
        {
            DEBUG_BREAK();
        }
    }

    if (!FXAADebugPSO || FXAADebugPSOFormat != OutputFormat)
    {
        PSODesc.PixelShader = FXAADebugShader.Get();

        FRHIGraphicsPipelineStateRef NewPSO = RHI::CreateGraphicsPipelineState(PSODesc);
        if (NewPSO)
        {
            FXAADebugPSO       = NewPSO;
            FXAADebugPSOFormat = OutputFormat;
        }
        else
        {
            DEBUG_BREAK();
        }
    }
}

void FFXAAPass::Record(FRHICommandList& CommandList, const FFrameResources& FrameResources)
{
    RHI_EVENT_SCOPE(CommandList, "FXAA");

    TRACE_SCOPE("FXAA");

    GPU_TRACE_SCOPE(CommandList, "FXAA");

    FFXAASettings Settings;

    Settings.Width  = static_cast<float>(FrameResources.CurrentRenderWidth);
    Settings.Height = static_cast<float>(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(Settings.Width, Settings.Height, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(Settings.Width, Settings.Height, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    FRHIShaderResourceView* SceneTargetSRV = FrameResources.SceneTarget->GetShaderResourceView();
    if (GFXAADebug)
    {
        CommandList.SetGraphicsPipelineState(FXAADebugPSO.Get());
        CommandList.SetShaderResourceView(FXAADebugShader.Get(), SceneTargetSRV, 0);
        CommandList.SetSamplerState(FXAADebugShader.Get(), FrameResources.FXAASampler.Get(), 0);
        CommandList.SetShaderConstants(FXAADebugShader.Get(), &Settings, 2);
    }
    else
    {
        CommandList.SetGraphicsPipelineState(FXAAPSO.Get());
        CommandList.SetShaderResourceView(FXAAShader.Get(), SceneTargetSRV, 0);
        CommandList.SetSamplerState(FXAAShader.Get(), FrameResources.FXAASampler.Get(), 0);
        CommandList.SetShaderConstants(FXAAShader.Get(), &Settings, 2);
    }

    CommandList.DrawInstanced(3, 1, 0, 0);
}

void FFXAAPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    GraphBuilder.AddPass("FXAA", ERenderGraphPassFlags::Raster, GEnableFXAA,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.SceneTarget, ERHIResourceState::PixelShaderResource);
            PassBuilder.SetRenderTarget(0, Context.BackBuffer, EAttachmentLoadAction::DontCare);
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            Record(PassCommandList, *Context.FrameResources);
        });
}

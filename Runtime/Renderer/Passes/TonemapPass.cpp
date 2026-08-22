#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Renderer/Shaders/CommonShaders.h"
#include "Renderer/Passes/TonemapPass.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

static int32 GTonemappingFunction = 1;
static FAutoConsoleVariableRef CVarTonemappingFunction(
    "Renderer.Tonemapping.Function",
    "Select function to use during tonemapping. 0: Default 1: ACES 2: Reinhard 3: Uncharted 2",
    GTonemappingFunction,
    EConsoleVariableFlags::Default);

static float GTonemappingReinhardIntensity = 1.0f;
static FAutoConsoleVariableRef CVarTonemappingReinhardIntensity(
    "Renderer.Tonemapping.ReinhardIntensity",
    "Intensity/\"Exposure\" when using Reinhard tonemapping",
    GTonemappingReinhardIntensity,
    EConsoleVariableFlags::Default);

class FTonemappingPS
{
    DECLARE_SHADER_TYPE(FTonemappingPS, EShaderStage::Pixel);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FTonemappingPS, "Shaders/Tonemapping.hlsl", "TonemappingPS", EShaderModel::SM_6_2);

FTonemapPass::FTonemapPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , TonemapPSO(nullptr)
    , TonemapVertexShader(nullptr)
    , TonemapShader(nullptr)
    , TonemapDepthStencilState(nullptr)
    , TonemapRasterizerState(nullptr)
    , TonemapBlendState(nullptr)
{
}

FTonemapPass::~FTonemapPass()
{
    TonemapPSO.Reset();
    TonemapVertexShader.Reset();
    TonemapShader.Reset();
    TonemapDepthStencilState.Reset();
    TonemapRasterizerState.Reset();
    TonemapBlendState.Reset();
}

bool FTonemapPass::Initialize(FFrameResources& FrameResources)
{
    UNREFERENCED_VARIABLE(FrameResources);

    TonemapVertexShader = FShaderCache::Get().GetShader<FFullscreenVS>();
    if (!TonemapVertexShader)
    {
        DEBUG_BREAK();
        return false;
    }

    TonemapShader = FShaderCache::Get().GetShader<FTonemappingPS>();
    if (!TonemapShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::Always;
    DepthStencilStateDesc.bDepthEnable      = false;
    DepthStencilStateDesc.bDepthWriteEnable = false;

    TonemapDepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
    if (!TonemapDepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    TonemapRasterizerState = RHI::CreateRasterizerState(RasterizerStateDesc);
    if (!TonemapRasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.NumRenderTargets = 1;

    TonemapBlendState = RHI::CreateBlendState(BlendStateDesc);
    if (!TonemapBlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FTonemapPass::PreparePipelineStateForFormat(EFormat OutputFormat)
{
    if (TonemapPSO && TonemapPSOFormat == OutputFormat)
    {
        return;
    }

    FRHIGraphicsPipelineStateDesc PSODesc;
    PSODesc.InputLayout                                    = nullptr;
    PSODesc.BlendState                                     = TonemapBlendState.Get();
    PSODesc.DepthStencilState                              = TonemapDepthStencilState.Get();
    PSODesc.RasterizerState                                = TonemapRasterizerState.Get();
    PSODesc.VertexShader                                   = TonemapVertexShader.Get();
    PSODesc.PixelShader                                    = TonemapShader.Get();
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

    TonemapPSO       = NewPSO;
    TonemapPSOFormat = OutputFormat;
}

void FTonemapPass::Record(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRHITexture* OutputTarget, bool bOutputSRGB)
{
    // Function to return a enum from the tonemap cvar
    const auto GetTonemappingFunctionCVar = []()
    {
        const int32 Function = GTonemappingFunction;
        switch (Function)
        {
            case 0:
            case 1: return ETonemappingType::ACES;
            case 2: return ETonemappingType::Reinhard;
            case 3: return ETonemappingType::Uncharted2;

            // Default to ACES
            default: return ETonemappingType::ACES;
        }
    };

    if (!OutputTarget)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "Tonemapping");

    TRACE_SCOPE("Tonemapping");

    GPU_TRACE_SCOPE(CommandList, "Tonemapping");

    const float RenderWidth  = static_cast<float>(FrameResources.CurrentRenderWidth);
    const float RenderHeight = static_cast<float>(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    CommandList.SetGraphicsPipelineState(TonemapPSO.Get());

    FRHIShaderResourceView* SceneTargetSRV = FrameResources.SceneTarget->GetShaderResourceView();
    CommandList.SetShaderResourceView(TonemapShader.Get(), SceneTargetSRV, 0);
    CommandList.SetSamplerState(TonemapShader.Get(), FrameResources.GBufferSampler.Get(), 0);

    FTonemapInfoHLSL TonemapInfo;
    TonemapInfo.TonemappingType   = GetTonemappingFunctionCVar();
    TonemapInfo.bOutputSRGB       = bOutputSRGB ? 1 : 0;
    TonemapInfo.ReinhardIntensity = Math::Clamp<float>(GTonemappingReinhardIntensity, 0.1f, 10.0f);
    TonemapInfo.Padding0          = 0.0f;

    constexpr uint32 NumConstants = sizeof(FTonemapInfoHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(TonemapShader.Get(), &TonemapInfo, NumConstants);

    CommandList.DrawInstanced(3, 1, 0, 0);
}

void FTonemapPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context, FRHITexture* OutputTarget, bool bOutputSRGB)
{
    FRenderGraphTexture* OutputGraphTexture = Context.TonemappedTarget;
    if (OutputTarget != nullptr)
    {
        OutputGraphTexture = Context.BackBuffer;
    }

    GraphBuilder.AddPass("Tonemap", ERenderGraphPassFlags::Raster, true,
        [&Context, OutputGraphTexture](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.SceneTarget, ERHIResourceState::PixelShaderResource);
            PassBuilder.SetRenderTarget(0, OutputGraphTexture, EAttachmentLoadAction::DontCare);
        },
        [this, Context, OutputTarget, bOutputSRGB, OutputGraphTexture](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);

            FRHITexture* ResolvedOutput = OutputTarget ? PassResources.Get(OutputGraphTexture) : PassResources.Get(OutputGraphTexture);
            Record(PassCommandList, *Context.FrameResources, ResolvedOutput, bOutputSRGB);
        });
}

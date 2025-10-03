#include "RHI/ShaderCompiler.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Renderer/PostProcessing.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "RendererCore/RenderSettings.h"

static TAutoConsoleVariable<bool> CVarFXAADebug(
    "Renderer.Debug.FXAADebug",
    "Enables FXAA (Anti-Aliasing) Debugging mode",
    false);

static TAutoConsoleVariable<int32> CVarTonemappingFunction(
    "Renderer.Tonemapping.Function",
    "Select function to use during tonemapping. 0: Default 1: ACES 2: Reinhard 3: Uncharted 2",
    1,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<float> CVarTonemappingReinhardIntensity(
    "Renderer.Tonemapping.ReinhardIntensity",
    "Intensity/\"Exposure\" when using Reinhard tonemapping",
    1.0f,
    EConsoleVariableFlags::Default);

FTonemapPass::FTonemapPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , TonemapPSO(nullptr)
    , TonemapShader(nullptr)
{
}

FTonemapPass::~FTonemapPass()
{
    TonemapPSO.Reset();
    TonemapShader.Reset();
}

bool FTonemapPass::Initialize(const FFrameResources& FrameResources)
{
    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/FullscreenVS.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIVertexShaderRef VShader = FRHI::Get()->CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        DEBUG_BREAK();
        return false;
    }

    CompileInfo = FShaderCompileInfo("TonemappingPS", EShaderModel::SM_6_2, EShaderStage::Pixel);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/Tonemapping.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    TonemapShader = FRHI::Get()->CreatePixelShader(ShaderCode);
    if (!TonemapShader)
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

    FRHIRasterizerStateInitializer RasterizerInitializer;
    RasterizerInitializer.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerInitializer);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInitializer;
    BlendStateInitializer.NumRenderTargets = 1;

    FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateInitializer);
    if (!BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateInitializer PSOInitializer;
    PSOInitializer.VertexInputLayout                      = nullptr;
    PSOInitializer.BlendState                             = BlendState.Get();
    PSOInitializer.DepthStencilState                      = DepthStencilState.Get();
    PSOInitializer.RasterizerState                        = RasterizerState.Get();
    PSOInitializer.ShaderState.VertexShader               = VShader.Get();
    PSOInitializer.ShaderState.PixelShader                = TonemapShader.Get();
    PSOInitializer.PrimitiveTopology                      = EPrimitiveTopology::TriangleList;
    PSOInitializer.PipelineFormats.RenderTargetFormats[0] = RenderSettings::GetBackBufferFormat();
    PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
    PSOInitializer.PipelineFormats.DepthStencilFormat     = EFormat::Unknown;

    TonemapPSO = FRHI::Get()->CreateGraphicsPipelineState(PSOInitializer);
    if (!TonemapPSO)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FTonemapPass::Execute(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources)
{
    // Function to return a enum from the tonemap cvar
    const auto GetTonemappingFunctionCVar = []()
    {
        const int32 Function = CVarTonemappingFunction.GetValue();
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

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Tonemapping and BackBuffer-Blit");

    TRACE_SCOPE("Tonemapping and BackBuffer-Blit");

    const float RenderWidth  = static_cast<float>(FrameResources.CurrentRenderWidth);
    const float RenderHeight = static_cast<float>(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.NumRenderTargets            = 1;
    RenderPass.RenderTargets[0]            = FRHIRenderTargetView(SceneRenderView.RenderTarget, EAttachmentLoadAction::Load);
    RenderPass.RenderTargets[0].ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);

    CommandList.BeginRenderPass(RenderPass);

    CommandList.SetGraphicsPipelineState(TonemapPSO.Get());

    FRHIShaderResourceView* FinalTargetSRV = FrameResources.FinalTarget->GetShaderResourceView();
    CommandList.SetShaderResourceView(TonemapShader.Get(), FinalTargetSRV, 0);
    CommandList.SetSamplerState(TonemapShader.Get(), FrameResources.GBufferSampler.Get(), 0);

    FTonemapInfoHLSL TonemapInfo;
    TonemapInfo.TonemappingType   = GetTonemappingFunctionCVar();
    TonemapInfo.ReinhardIntensity = Math::Clamp<float>(CVarTonemappingReinhardIntensity.GetValue(), 0.1f, 10.0f);
    TonemapInfo.Padding0          = 0.0f;
    TonemapInfo.Padding1          = 0.0f;

    constexpr uint32 NumConstants = sizeof(FTonemapInfoHLSL) / sizeof(uint32);
    CommandList.Set32BitShaderConstants(TonemapShader.Get(), &TonemapInfo, NumConstants);

    CommandList.DrawInstanced(3, 1, 0, 0);

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End Tonemapping and BackBuffer-Blit");
}

FFXAAPass::FFXAAPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , FXAAPSO(nullptr)
    , FXAAShader(nullptr)
    , FXAADebugPSO(nullptr)
    , FXAADebugShader(nullptr)
{
}

FFXAAPass::~FFXAAPass()
{
    FXAAPSO.Reset();
    FXAAShader.Reset();
    FXAADebugPSO.Reset();
    FXAADebugShader.Reset();
}

bool FFXAAPass::Initialize(FFrameResources& FrameResources)
{
    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/FullscreenVS.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIVertexShaderRef VShader = FRHI::Get()->CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        DEBUG_BREAK();
        return false;
    }

    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/FXAA_PS.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    FXAAShader = FRHI::Get()->CreatePixelShader(ShaderCode);
    if (!FXAAShader)
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

    FRHIRasterizerStateInitializer RasterizerInitializer;
    RasterizerInitializer.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerInitializer);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInitializer;
    BlendStateInitializer.NumRenderTargets = 1;

    FRHIBlendStateRef BlendState = FRHI::Get()->CreateBlendState(BlendStateInitializer);
    if (!BlendState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIGraphicsPipelineStateInitializer PSOInitializer;
    PSOInitializer.VertexInputLayout                      = nullptr;
    PSOInitializer.BlendState                             = BlendState.Get();
    PSOInitializer.DepthStencilState                      = DepthStencilState.Get();
    PSOInitializer.RasterizerState                        = RasterizerState.Get();
    PSOInitializer.ShaderState.VertexShader               = VShader.Get();
    PSOInitializer.ShaderState.PixelShader                = FXAAShader.Get();
    PSOInitializer.PrimitiveTopology                      = EPrimitiveTopology::TriangleList;
    PSOInitializer.PipelineFormats.RenderTargetFormats[0] = RenderSettings::GetBackBufferFormat();
    PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
    PSOInitializer.PipelineFormats.DepthStencilFormat     = EFormat::Unknown;

    FXAAPSO = FRHI::Get()->CreateGraphicsPipelineState(PSOInitializer);
    if (!FXAAPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        FXAAPSO->SetDebugName("FXAA PipelineState");
    }

    TArray<FShaderDefine> Defines =
    {
        { "ENABLE_DEBUG", "(1)" }
    };

    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Pixel, Defines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/FXAA_PS.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    FXAADebugShader = FRHI::Get()->CreatePixelShader(ShaderCode);
    if (!FXAADebugShader)
    {
        DEBUG_BREAK();
        return false;
    }

    PSOInitializer.ShaderState.PixelShader = FXAADebugShader.Get();

    FXAADebugPSO = FRHI::Get()->CreateGraphicsPipelineState(PSOInitializer);
    if (!FXAADebugPSO)
    {
        DEBUG_BREAK();
        return false;
    }

    // FXAA
    FRHISamplerStateInfo SamplerInfo;
    SamplerInfo.AddressU = ESamplerMode::Clamp;
    SamplerInfo.AddressV = ESamplerMode::Clamp;
    SamplerInfo.AddressW = ESamplerMode::Clamp;
    SamplerInfo.Filter   = ESamplerFilter::MinMagMipLinear;

    FrameResources.FXAASampler = FRHI::Get()->CreateSamplerState(SamplerInfo);
    if (!FrameResources.FXAASampler)
    {
        return false;
    }

    return true;
}

void FFXAAPass::Execute(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin FXAA");

    TRACE_SCOPE("FXAA");

    GPU_TRACE_SCOPE(CommandList, "FXAA");

    struct FFXAASettings
    {
        float Width;
        float Height;
    } Settings;

    Settings.Width  = static_cast<float>(FrameResources.CurrentRenderWidth);
    Settings.Height = static_cast<float>(FrameResources.CurrentRenderHeight);

    FViewportRegion ViewportRegion(Settings.Width, Settings.Height, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(Settings.Width, Settings.Height, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.NumRenderTargets            = 1;
    RenderPass.RenderTargets[0]            = FRHIRenderTargetView(SceneRenderView.RenderTarget, EAttachmentLoadAction::Clear);
    RenderPass.RenderTargets[0].ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);

    CommandList.BeginRenderPass(RenderPass);

    FRHIShaderResourceView* FinalTargetSRV = FrameResources.FinalTarget->GetShaderResourceView();
    if (CVarFXAADebug.GetValue())
    {
        CommandList.SetGraphicsPipelineState(FXAADebugPSO.Get());
        CommandList.SetShaderResourceView(FXAADebugShader.Get(), FinalTargetSRV, 0);
        CommandList.SetSamplerState(FXAADebugShader.Get(), FrameResources.FXAASampler.Get(), 0);
        CommandList.Set32BitShaderConstants(FXAADebugShader.Get(), &Settings, 2);
    }
    else
    {
        CommandList.SetGraphicsPipelineState(FXAAPSO.Get());
        CommandList.SetShaderResourceView(FXAAShader.Get(), FinalTargetSRV, 0);
        CommandList.SetSamplerState(FXAAShader.Get(), FrameResources.FXAASampler.Get(), 0);
        CommandList.Set32BitShaderConstants(FXAAShader.Get(), &Settings, 2);
    }

    CommandList.DrawInstanced(3, 1, 0, 0);

    CommandList.EndRenderPass();

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End FXAA");
}
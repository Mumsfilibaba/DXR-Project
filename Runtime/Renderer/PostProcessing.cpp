#include "PostProcessing.h"
#include "Debug/GPUProfiler.h"
#include "RHI/ShaderCompiler.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"

static TAutoConsoleVariable<bool> CVarFXAADebug(
    "Renderer.Debug.FXAADebug",
    "Enables FXAA (Anti-Aliasing) Debugging mode",
    false);

FTonemapPass::FTonemapPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , TonemapPSO(nullptr)
    , TonemapShader(nullptr)
{
}

FTonemapPass::~FTonemapPass()
{
    Release();
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

    FRHIVertexShaderRef VShader = RHICreateVertexShader(ShaderCode);
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

    TonemapShader = RHICreatePixelShader(ShaderCode);
    if (!TonemapShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateInitializer DepthStencilInitializer;
    DepthStencilInitializer.DepthFunc         = EComparisonFunc::Always;
    DepthStencilInitializer.bDepthEnable      = false;
    DepthStencilInitializer.bDepthWriteEnable = false;

    FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerInitializer;
    RasterizerInitializer.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerInitializer);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInitializer;
    BlendStateInitializer.NumRenderTargets = 1;

    FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
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
    PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FrameResources.BackBufferFormat;
    PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
    PSOInitializer.PipelineFormats.DepthStencilFormat     = EFormat::Unknown;

    TonemapPSO = RHICreateGraphicsPipelineState(PSOInitializer);
    if (!TonemapPSO)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FTonemapPass::Release()
{
    TonemapPSO.Reset();
    TonemapShader.Reset();
}

void FTonemapPass::Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin Tonemapping and BackBuffer-Blit");

    TRACE_SCOPE("Tonemapping and BackBuffer-Blit");

    const float RenderWidth  = static_cast<float>(FrameResources.BackBuffer->GetWidth());
    const float RenderHeight = static_cast<float>(FrameResources.BackBuffer->GetHeight());

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.NumRenderTargets            = 1;
    RenderPass.RenderTargets[0]            = FRHIRenderTargetView(FrameResources.BackBuffer, EAttachmentLoadAction::Load);
    RenderPass.RenderTargets[0].ClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f);

    CommandList.BeginRenderPass(RenderPass);

    CommandList.SetGraphicsPipelineState(TonemapPSO.Get());

    FRHIShaderResourceView* FinalTargetSRV = FrameResources.FinalTarget->GetShaderResourceView();
    CommandList.SetShaderResourceView(TonemapShader.Get(), FinalTargetSRV, 0);
    CommandList.SetSamplerState(TonemapShader.Get(), FrameResources.GBufferSampler.Get(), 0);

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
    Release();
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

    FRHIVertexShaderRef VShader = RHICreateVertexShader(ShaderCode);
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

    FXAAShader = RHICreatePixelShader(ShaderCode);
    if (!FXAAShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateInitializer DepthStencilInitializer;
    DepthStencilInitializer.DepthFunc         = EComparisonFunc::Always;
    DepthStencilInitializer.bDepthEnable      = false;
    DepthStencilInitializer.bDepthWriteEnable = false;

    FRHIDepthStencilStateRef DepthStencilState = RHICreateDepthStencilState(DepthStencilInitializer);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateInitializer RasterizerInitializer;
    RasterizerInitializer.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = RHICreateRasterizerState(RasterizerInitializer);
    if (!RasterizerState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIBlendStateInitializer BlendStateInitializer;
    BlendStateInitializer.NumRenderTargets = 1;

    FRHIBlendStateRef BlendState = RHICreateBlendState(BlendStateInitializer);
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
    PSOInitializer.PipelineFormats.RenderTargetFormats[0] = FrameResources.BackBufferFormat;
    PSOInitializer.PipelineFormats.NumRenderTargets       = 1;
    PSOInitializer.PipelineFormats.DepthStencilFormat     = EFormat::Unknown;

    FXAAPSO = RHICreateGraphicsPipelineState(PSOInitializer);
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

    FXAADebugShader = RHICreatePixelShader(ShaderCode);
    if (!FXAADebugShader)
    {
        DEBUG_BREAK();
        return false;
    }

    PSOInitializer.ShaderState.PixelShader = FXAADebugShader.Get();

    FXAADebugPSO = RHICreateGraphicsPipelineState(PSOInitializer);
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

    FrameResources.FXAASampler = RHICreateSamplerState(SamplerInfo);
    if (!FrameResources.FXAASampler)
    {
        return false;
    }

    return true;
}

void FFXAAPass::Release()
{
    FXAAPSO.Reset();
    FXAAShader.Reset();
    FXAADebugPSO.Reset();
    FXAADebugShader.Reset();
}

void FFXAAPass::Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FScene* Scene)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin FXAA");

    TRACE_SCOPE("FXAA");

    GPU_TRACE_SCOPE(CommandList, "FXAA");

    struct FFXAASettings
    {
        float Width;
        float Height;
    } Settings;

    const float RenderWidth  = static_cast<float>(FrameResources.CurrentWidth);
    const float RenderHeight = static_cast<float>(FrameResources.CurrentHeight);

    Settings.Width = RenderWidth;
    Settings.Height = RenderHeight;

    FViewportRegion ViewportRegion(RenderWidth, RenderHeight, 0.0f, 0.0f, 0.0f, 1.0f);
    CommandList.SetViewport(ViewportRegion);

    FScissorRegion ScissorRegion(RenderWidth, RenderHeight, 0, 0);
    CommandList.SetScissorRect(ScissorRegion);

    FRHIBeginRenderPassInfo RenderPass;
    RenderPass.NumRenderTargets            = 1;
    RenderPass.RenderTargets[0]            = FRHIRenderTargetView(FrameResources.BackBuffer, EAttachmentLoadAction::Clear);
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
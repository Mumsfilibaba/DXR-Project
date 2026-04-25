#include "RHI/ShaderCompiler.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Renderer/PostProcessing.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/EditorGridSettings.h"
#include "Renderer/SelectionOutlineSettings.h"
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
    , TonemapPSO_Linear(nullptr)
    , TonemapPSO_BackBuffer(nullptr)
    , TonemapShader(nullptr)
{
}

FTonemapPass::~FTonemapPass()
{
    TonemapPSO_Linear.Reset();
    TonemapPSO_BackBuffer.Reset();
    TonemapShader.Reset();
}

bool FTonemapPass::Initialize(FFrameResources& FrameResources)
{
    if (!CreateResources(FrameResources, FrameResources.CurrentRenderWidth, FrameResources.CurrentRenderHeight))
    {
        return false;
    }

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

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::Always;
    DepthStencilStateDesc.bDepthEnable      = false;
    DepthStencilStateDesc.bDepthWriteEnable = false;

    FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateDesc);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateDesc);
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
    PSODesc.VertexShader                                   = VShader.Get();
    PSODesc.PixelShader                                    = TonemapShader.Get();
    PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
    PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSODesc.RasterizerOutputFormats.DepthStencilFormat     = EFormat::Unknown;

    // Linear output (float HDR->LDR target)
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = FGlobalTextureFormats::FinalTargetFormat;
    TonemapPSO_Linear = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
    if (!TonemapPSO_Linear)
    {
        DEBUG_BREAK();
        return false;
    }

    // BackBuffer output (runtime path)
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = RenderSettings::GetBackBufferFormat();
    TonemapPSO_BackBuffer = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
    if (!TonemapPSO_BackBuffer)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

bool FTonemapPass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
#if !EDITOR_BUILD
    (void)FrameResources;
    (void)Width;
    (void)Height;
    return true;
#else
    if (Width <= 0 || Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture;
    const FClearValue ClearValue(FGlobalTextureFormats::FinalTargetFormat, 0.0f, 0.0f, 0.0f, 1.0f);
    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(FGlobalTextureFormats::FinalTargetFormat, Width, Height, 1, 1, Usage, ClearValue);

    FrameResources.TonemappedTarget = FRHI::Get()->CreateTexture(TextureDesc, EResourceAccess::PixelShaderResource);
    if (!FrameResources.TonemappedTarget)
    {
        return false;
    }

    FrameResources.TonemappedTarget->SetDebugName("Tonemapped Target");
    return true;
#endif
}

void FTonemapPass::Execute(FRHICommandList& CommandList, const FFrameResources& FrameResources, FRHITexture* OutputTarget, bool bOutputSRGB)
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

    const bool bNeedsTransition = !OutputTarget->GetDesc().IsPresentable();
    if (bNeedsTransition)
    {
        CommandList.TransitionTextureState(OutputTarget, FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::RenderTarget));
    }

    FRHIRenderTargetView* RenderTargetView = OutputTarget->GetRenderTargetView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.NumRenderTargets            = 1;
    RenderPassDesc.RenderTargets[0]            = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::DontCare);

    CommandList.BeginRenderPass(RenderPassDesc);

    const FRHIGraphicsPipelineStateRef& PSO = (OutputTarget->GetFormat() == RenderSettings::GetBackBufferFormat()) ? TonemapPSO_BackBuffer : TonemapPSO_Linear;
    CommandList.SetGraphicsPipelineState(PSO.Get());

    FRHIShaderResourceView* FinalTargetSRV = FrameResources.FinalTarget->GetShaderResourceView();
    CommandList.SetShaderResourceView(TonemapShader.Get(), FinalTargetSRV, 0);
    CommandList.SetSamplerState(TonemapShader.Get(), FrameResources.GBufferSampler.Get(), 0);

    FTonemapInfoHLSL TonemapInfo;
    TonemapInfo.TonemappingType   = GetTonemappingFunctionCVar();
    TonemapInfo.bOutputSRGB       = bOutputSRGB ? 1 : 0;
    TonemapInfo.ReinhardIntensity = Math::Clamp<float>(CVarTonemappingReinhardIntensity.GetValue(), 0.1f, 10.0f);
    TonemapInfo.Padding0          = 0.0f;

    constexpr uint32 NumConstants = sizeof(FTonemapInfoHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(TonemapShader.Get(), &TonemapInfo, NumConstants);

    CommandList.DrawInstanced(3, 1, 0, 0);

    CommandList.EndRenderPass();

    if (bNeedsTransition)
    {
        CommandList.TransitionTextureState(OutputTarget, FRHITextureTransition::Make(EResourceAccess::RenderTarget, EResourceAccess::PixelShaderResource));
    }
}

#if EDITOR_BUILD
FFinalCompositePass::FFinalCompositePass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , CompositePSO(nullptr)
    , CompositeShader(nullptr)
{
}

FFinalCompositePass::~FFinalCompositePass()
{
    CompositePSO.Reset();
    CompositeShader.Reset();
}

bool FFinalCompositePass::Initialize(const FFrameResources& /*FrameResources*/)
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
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/FinalComposite.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    CompositeShader = FRHI::Get()->CreatePixelShader(ShaderCode);
    if (!CompositeShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::Always;
    DepthStencilStateDesc.bDepthEnable      = false;
    DepthStencilStateDesc.bDepthWriteEnable = false;

    FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateDesc);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateDesc);
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
    PSODesc.VertexShader                                   = VShader.Get();
    PSODesc.PixelShader                                    = CompositeShader.Get();
    PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = RenderSettings::GetBackBufferFormat();
    PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSODesc.RasterizerOutputFormats.DepthStencilFormat     = EFormat::Unknown;

    CompositePSO = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
    if (!CompositePSO)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

void FFinalCompositePass::Execute(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources)
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

    CommandList.RequireTextureState(SceneRenderView.RenderTarget, FRHIRequiredTextureState::Make(EResourceAccess::RenderTarget));

    FRHIRenderTargetView* RenderTargetView = SceneRenderView.RenderTarget->GetRenderTargetView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.NumRenderTargets            = 1;
    RenderPassDesc.RenderTargets[0]            = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::DontCare);

    CommandList.BeginRenderPass(RenderPassDesc);

    CommandList.SetGraphicsPipelineState(CompositePSO.Get());

    CommandList.SetShaderResourceView(CompositeShader.Get(), FrameResources.TonemappedTarget->GetShaderResourceView(), 0);
    CommandList.SetConstantBuffer(CompositeShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    const FSelectionOutlineSettings OutlineSettings = GetSelectionOutlineSettings();
    const FEditorGridSettings GridSettings = GetEditorGridSettings();
#if EDITOR_BUILD
    FRHITexture* SelectionRingTexture = GetRenderer()->GetSelectionRingTexture();
    const bool bCanUseSelectionOutline = OutlineSettings.bEnabled && (SelectionRingTexture != nullptr);
    const bool bCanUseGrid = GridSettings.bEnabled && (FrameResources.EditorNoJitterDepth != nullptr);
#else
    FRHITexture* SelectionRingTexture = nullptr;
    const bool bCanUseSelectionOutline = false;
    const bool bCanUseGrid = false;
#endif
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

    CommandList.EndRenderPass();

    CommandList.RequireTextureState(SceneRenderView.RenderTarget, FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
}
#endif

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

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.DepthFunc         = EComparisonFunc::Always;
    DepthStencilStateDesc.bDepthEnable      = false;
    DepthStencilStateDesc.bDepthWriteEnable = false;

    FRHIDepthStencilStateRef DepthStencilState = FRHI::Get()->CreateDepthStencilState(DepthStencilStateDesc);
    if (!DepthStencilState)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    FRHIRasterizerStateRef RasterizerState = FRHI::Get()->CreateRasterizerState(RasterizerStateDesc);
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
    PSODesc.VertexShader                                   = VShader.Get();
    PSODesc.PixelShader                                    = FXAAShader.Get();
    PSODesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;
    PSODesc.RasterizerOutputFormats.RenderTargetFormats[0] = RenderSettings::GetBackBufferFormat();
    PSODesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PSODesc.RasterizerOutputFormats.DepthStencilFormat     = EFormat::Unknown;

    FXAAPSO = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
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

    PSODesc.PixelShader = FXAADebugShader.Get();

    FXAADebugPSO = FRHI::Get()->CreateGraphicsPipelineState(PSODesc);
    if (!FXAADebugPSO)
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

    FrameResources.FXAASampler = FRHI::Get()->CreateSamplerState(SamplerDesc);
    if (!FrameResources.FXAASampler)
    {
        return false;
    }

    return true;
}

void FFXAAPass::Execute(FRHICommandList& CommandList, const FSceneRenderView& SceneRenderView, const FFrameResources& FrameResources)
{
    RHI_EVENT_SCOPE(CommandList, "FXAA");

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

    CommandList.RequireTextureState(SceneRenderView.RenderTarget, FRHIRequiredTextureState::Make(EResourceAccess::RenderTarget));

    FRHIRenderTargetView* RenderTargetView = SceneRenderView.RenderTarget->GetRenderTargetView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.NumRenderTargets            = 1;
    RenderPassDesc.RenderTargets[0]            = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Clear, EAttachmentStoreAction::Store, FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));

    CommandList.BeginRenderPass(RenderPassDesc);

    FRHIShaderResourceView* FinalTargetSRV = FrameResources.FinalTarget->GetShaderResourceView();
    if (CVarFXAADebug.GetValue())
    {
        CommandList.SetGraphicsPipelineState(FXAADebugPSO.Get());
        CommandList.SetShaderResourceView(FXAADebugShader.Get(), FinalTargetSRV, 0);
        CommandList.SetSamplerState(FXAADebugShader.Get(), FrameResources.FXAASampler.Get(), 0);
        CommandList.SetShaderConstants(FXAADebugShader.Get(), &Settings, 2);
    }
    else
    {
        CommandList.SetGraphicsPipelineState(FXAAPSO.Get());
        CommandList.SetShaderResourceView(FXAAShader.Get(), FinalTargetSRV, 0);
        CommandList.SetSamplerState(FXAAShader.Get(), FrameResources.FXAASampler.Get(), 0);
        CommandList.SetShaderConstants(FXAAShader.Get(), &Settings, 2);
    }

    CommandList.DrawInstanced(3, 1, 0, 0);

    CommandList.EndRenderPass();

    CommandList.RequireTextureState(SceneRenderView.RenderTarget, FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
}

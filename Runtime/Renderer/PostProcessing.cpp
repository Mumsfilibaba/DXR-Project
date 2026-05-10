#include "RHI/ShaderCompiler.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Renderer/PostProcessing.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/EditorGridSettings.h"
#include "Renderer/SelectionOutlineSettings.h"

static bool GFXAADebug = false;
static FAutoConsoleVariableRef CVarFXAADebug(
    "Renderer.Debug.FXAADebug",
    "Enables FXAA (Anti-Aliasing) Debugging mode",
    GFXAADebug);

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

    TonemapVertexShader = RHI::CreateVertexShader(ShaderCode);
    if (!TonemapVertexShader)
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

    TonemapShader = RHI::CreatePixelShader(ShaderCode);
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

void FTonemapPass::PreparePipelineState(EFormat OutputFormat)
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

bool FTonemapPass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
#if !EDITOR_BUILD
    UNREFERENCED_VARIABLE(FrameResources);
    UNREFERENCED_VARIABLE(Width);
    UNREFERENCED_VARIABLE(Height);
    return true;
#else
    if (Width <= 0 || Height <= 0)
    {
        return true;
    }

    const ETextureUsageFlags Usage = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::ShaderResourceTexture;
    const FClearValue ClearValue(FGlobalTextureFormats::SceneTargetFormat, 0.0f, 0.0f, 0.0f, 1.0f);
    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(FGlobalTextureFormats::SceneTargetFormat, Width, Height, 1, 1, Usage, ClearValue);

    FrameResources.TonemappedTarget = RHI::CreateTexture(TextureDesc, EResourceAccess::PixelShaderResource);
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

    const bool bNeedsTransition = !OutputTarget->GetDesc().IsPresentable();
    if (bNeedsTransition)
    {
        CommandList.TransitionTextureState(OutputTarget, FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::RenderTarget));
    }

    FRHIRenderTargetView* RenderTargetView = OutputTarget->GetRenderTargetView();

    FRHIBeginRenderPassDesc RenderPassDesc;
    RenderPassDesc.NumRenderTargets = 1;
    RenderPassDesc.RenderTargets[0] = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::DontCare);

    CommandList.BeginRenderPass(RenderPassDesc);

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
    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/FullscreenVS.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    CompositeVertexShader = RHI::CreateVertexShader(ShaderCode);
    if (!CompositeVertexShader)
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

    CompositeShader = RHI::CreatePixelShader(ShaderCode);
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

void FFinalCompositePass::PreparePipelineState(EFormat OutputFormat)
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
    RenderPassDesc.NumRenderTargets = 1;
    RenderPassDesc.RenderTargets[0] = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::DontCare);

    CommandList.BeginRenderPass(RenderPassDesc);

    CommandList.SetGraphicsPipelineState(CompositePSO.Get());

    CommandList.SetShaderResourceView(CompositeShader.Get(), FrameResources.TonemappedTarget->GetShaderResourceView(), 0);
    CommandList.SetConstantBuffer(CompositeShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    const FEditorGridSettings       GridSettings    = GetEditorGridSettings();
    const FSelectionOutlineSettings OutlineSettings = GetSelectionOutlineSettings();

    #if EDITOR_BUILD
    FRHITexture* SelectionRingTexture = GetRenderer()->GetSelectionRingTexture();
    const bool bCanUseGrid             = GridSettings.bEnabled && (FrameResources.EditorNoJitterDepth != nullptr);
    const bool bCanUseSelectionOutline = OutlineSettings.bEnabled && (SelectionRingTexture != nullptr);
#else
    FRHITexture* SelectionRingTexture = nullptr;
    const bool bCanUseGrid             = false;
    const bool bCanUseSelectionOutline = false;
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
    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/FullscreenVS.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    FXAAVertexShader = RHI::CreateVertexShader(ShaderCode);
    if (!FXAAVertexShader)
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

    FXAAShader = RHI::CreatePixelShader(ShaderCode);
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

    FXAADebugShader = RHI::CreatePixelShader(ShaderCode);
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

void FFXAAPass::PreparePipelineState(EFormat OutputFormat)
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
    RenderPassDesc.NumRenderTargets = 1;
    RenderPassDesc.RenderTargets[0] = FRHIRenderPassAttachment(RenderTargetView, EAttachmentLoadAction::Clear, EAttachmentStoreAction::Store, FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));

    CommandList.BeginRenderPass(RenderPassDesc);

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

    CommandList.EndRenderPass();

    CommandList.RequireTextureState(SceneRenderView.RenderTarget, FRHIRequiredTextureState::Make(EResourceAccess::PixelShaderResource));
}

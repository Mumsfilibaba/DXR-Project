#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/IntVector2.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/Shaders/CommonShaders.h"
#include "Renderer/Passes/ScreenSpaceOcclusionRendering.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

static float GSSAORadius = 0.2f;
static FAutoConsoleVariableRef CVarSSAORadius(
    "Renderer.SSAO.Radius",
    "Specifies the radius of the Screen-Space Ray-Trace in SSAO",
    GSSAORadius);

static float GSSAOBias = 0.04f;
static FAutoConsoleVariableRef CVarSSAOBias(
    "Renderer.SSAO.Bias",
    "Specifies the bias when testing the Screen-Space Rays against the depth-buffer",
    GSSAOBias);

static int32 GSSAOKernelSize = 8;
static FAutoConsoleVariableRef CVarSSAOKernelSize(
    "Renderer.SSAO.KernelSize",
    "Specifies the number of samples for each pixel",
    GSSAOKernelSize);

struct FSSAOSettingsHLSL
{
    Vector2    ScreenSize;
    Vector2    NoiseSize;
    IntVector2 GBufferSize;
    float      Radius;
    float      Bias;
    uint32     KernelSize;
    uint32     FrameIndex;
};

class FSSAOCS
{
    DECLARE_SHADER_TYPE(FSSAOCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FSSAOCS, "Shaders/SSAO.hlsl", "Main", EShaderModel::SM_6_2);

class FBlurHorizontal : SHADER_PERMUTATION_BOOL("HORIZONTAL_PASS");

class FBlurCS
{
    DECLARE_SHADER_TYPE(FBlurCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<FBlurHorizontal>;
};

IMPLEMENT_SHADER_TYPE(FBlurCS, "Shaders/Blur.hlsl", "Main", EShaderModel::SM_6_2);

FScreenSpaceOcclusionPass::FScreenSpaceOcclusionPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
{
}

FScreenSpaceOcclusionPass::~FScreenSpaceOcclusionPass()
{
    PipelineState.Reset();
    BlurHorizontalPSO.Reset();
    BlurVerticalPSO.Reset();
    SSAOShader.Reset();
    BlurHorizontalShader.Reset();
    BlurVerticalShader.Reset();
}

bool FScreenSpaceOcclusionPass::Initialize(FFrameResources& FrameResources)
{
    UNREFERENCED_VARIABLE(FrameResources);

    SSAOShader = FShaderCache::Get().GetShader<FSSAOCS>();
    if (!SSAOShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader = SSAOShader.Get();

    PipelineState = RHI::CreateComputePipelineState(PSODesc);
    if (!PipelineState)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PipelineState->SetDebugName("SSAO PipelineState");
    }

    FBlurCS::FPermutation BlurPermutation;
    BlurPermutation.Set<FBlurHorizontal>(true);

    BlurHorizontalShader = FShaderCache::Get().GetShader<FBlurCS>(BlurPermutation);
    if (!BlurHorizontalShader)
    {
        DEBUG_BREAK();
        return false;
    }

    PSODesc.Shader = BlurHorizontalShader.Get();
    BlurHorizontalPSO = RHI::CreateComputePipelineState(PSODesc);

    if (!BlurHorizontalPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        BlurHorizontalPSO->SetDebugName("SSAO Horizontal Blur PSO");
    }

    BlurPermutation.Set<FBlurHorizontal>(false);

    BlurVerticalShader = FShaderCache::Get().GetShader<FBlurCS>(BlurPermutation);
    if (!BlurVerticalShader)
    {
        DEBUG_BREAK();
        return false;
    }

    PSODesc.Shader = BlurVerticalShader.Get();

    BlurVerticalPSO = RHI::CreateComputePipelineState(PSODesc);
    if (!BlurVerticalPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        BlurVerticalPSO->SetDebugName("SSAO Vertical Blur PSO");
    }

    return true;
}

void FScreenSpaceOcclusionPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    GraphBuilder.AddPass("SSAO", ERenderGraphPassFlags::Compute, GEnableSSAO,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.GBufferNormal, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.GBufferDepth, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.WriteTexture(Context.SSAOBuffer, ERHIResourceState::UnorderedAccess);
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            Record(PassCommandList, Context.CreatePassResources(PassResources));
        });

    GraphBuilder.AddPass("SSAOClear", ERenderGraphPassFlags::Compute, !GEnableSSAO,
        [Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.WriteTexture(Context.SSAOBuffer, ERHIResourceState::UnorderedAccess);
        },
        [Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FRHITexture* SSAOTexture = PassResources.Get(Context.SSAOBuffer);
            PassCommandList.ClearUnorderedAccessViewFloat(SSAOTexture->GetUnorderedAccessView(), Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        });
}

void FScreenSpaceOcclusionPass::Record(FRHICommandList& CommandList, const FPassResources& PassResources)
{
    if (!PassResources.SSAOBuffer || PassResources.SSAOBuffer->GetDesc().Extent.X == 0 || PassResources.SSAOBuffer->GetDesc().Extent.Y == 0)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "SSAO");

    TRACE_SCOPE("SSAO");

    FSSAOSettingsHLSL SSAOSettings;

    const uint32 Width         = PassResources.SSAOBuffer->GetDesc().Extent.X;
    const uint32 Height        = PassResources.SSAOBuffer->GetDesc().Extent.Y;
    const uint32 GBufferWidth  = PassResources.GBufferDepth->GetDesc().Extent.X;
    const uint32 GBufferHeight = PassResources.GBufferDepth->GetDesc().Extent.Y;

    SSAOSettings.ScreenSize  = Vector2(float(Width), float(Height));
    SSAOSettings.NoiseSize   = Vector2(4.0f, 4.0f);
    SSAOSettings.GBufferSize = IntVector2(GBufferWidth, GBufferHeight);
    SSAOSettings.Radius      = GSSAORadius;
    SSAOSettings.KernelSize  = GSSAOKernelSize;
    SSAOSettings.Bias        = GSSAOBias;
    SSAOSettings.FrameIndex  = GetRenderer()->GetFrameCounter().GetFrameIndex();

    const FFrameResources& FrameResources = *PassResources.FrameResources;

    CommandList.SetComputePipelineState(PipelineState.Get());
    CommandList.SetConstantBuffer(SSAOShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    CommandList.SetShaderResourceView(SSAOShader.Get(), PassResources.GBufferNormal->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(SSAOShader.Get(), PassResources.GBufferDepth->GetShaderResourceView(), 1);

    CommandList.SetSamplerState(SSAOShader.Get(), FrameResources.GBufferSampler.Get(), 0);

    FRHIUnorderedAccessView* SSAOBufferUAV = PassResources.SSAOBuffer->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(SSAOShader.Get(), SSAOBufferUAV, 0);

    constexpr uint32 NumConstants = sizeof(FSSAOSettingsHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(SSAOShader.Get(), &SSAOSettings, NumConstants);

    constexpr uint32 ThreadCount = 16;
    const uint32 DispatchWidth   = Math::DivideByMultiple<uint32>(Width, ThreadCount);
    const uint32 DispatchHeight  = Math::DivideByMultiple<uint32>(Height, ThreadCount);

    {
        GPU_TRACE_SCOPE(CommandList, "SSAO Tracing");
        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);

        CommandList.UnorderedAccessBarrier(PassResources.SSAOBuffer);
    }

    {
        GPU_TRACE_SCOPE(CommandList, "SSAO Horizontal blur");
        CommandList.SetComputePipelineState(BlurHorizontalPSO.Get());
        CommandList.SetUnorderedAccessView(SSAOShader.Get(), SSAOBufferUAV, 0);
        CommandList.SetShaderConstants(BlurHorizontalShader.Get(), &SSAOSettings.ScreenSize, 2);
        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
        CommandList.UnorderedAccessBarrier(PassResources.SSAOBuffer);
    }

    {
        GPU_TRACE_SCOPE(CommandList, "SSAO Vertical blur");
        CommandList.SetComputePipelineState(BlurVerticalPSO.Get());
        CommandList.SetUnorderedAccessView(SSAOShader.Get(), SSAOBufferUAV, 0);
        CommandList.SetShaderConstants(BlurVerticalShader.Get(), &SSAOSettings.ScreenSize, 2);
        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
    }
}

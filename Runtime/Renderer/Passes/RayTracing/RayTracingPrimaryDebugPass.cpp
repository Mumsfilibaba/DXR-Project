#include "RHI/RHI.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "Renderer/Graph/SceneRenderGraphContext.h"
#include "Renderer/Graph/PassResources.h"
#include "Renderer/Passes/RayTracing/RayTracingPrimaryDebugPass.h"
#include "Renderer/Shaders/RayTracingShaders.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

FRayTracingPrimaryDebugPass::FRayTracingPrimaryDebugPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
{
}

FRayTracingPrimaryDebugPass::~FRayTracingPrimaryDebugPass()
{
    Release();
}

bool FRayTracingPrimaryDebugPass::Initialize(FFrameResources& /*Resources*/)
{
    if (!RHI::bSupportsInlineRayTracing)
    {
        return true;
    }

    PrimaryRayDebugShader = FShaderCache::Get().GetShader<FPrimaryRayDebugCS>();
    if (PrimaryRayDebugShader)
    {
        FRHIComputePipelineStateDesc PSODesc;
        PSODesc.Shader = PrimaryRayDebugShader.Get();

        PrimaryRayDebugPipeline = RHI::CreateComputePipelineState(PSODesc);
        if (PrimaryRayDebugPipeline)
        {
            PrimaryRayDebugPipeline->SetDebugName("RT Primary-Ray Debug PSO");
        }
        else
        {
            PrimaryRayDebugShader.Reset();
        }
    }

    if (!PrimaryRayDebugPipeline)
    {
        LOG_WARNING("[RayTracingPrimaryDebug]: Primary-ray debug pipeline unavailable. The RT primary-ID debug view will be disabled");
    }

    return true;
}

void FRayTracingPrimaryDebugPass::Release()
{
    PrimaryRayDebugPipeline.Reset();
    PrimaryRayDebugShader.Reset();
}

bool FRayTracingPrimaryDebugPass::IsEnabled(const FSceneRenderView& SceneRenderView) const
{
    return RHI::bSupportsRayTracing && GRayTracingEnabled && PrimaryRayDebugPipeline
        && SceneRenderView.DebugView == FSceneRenderView::EDebugView::RayTracingPrimaryID;
}

void FRayTracingPrimaryDebugPass::Record(FRHICommandList& CommandList, FFrameResources& Resources)
{
    if (!Resources.RayTracingScene || !Resources.RayTracingOutput)
    {
        return;
    }

    FRHITexture* Output = Resources.RayTracingOutput.Get();

    CommandList.SetComputePipelineState(PrimaryRayDebugPipeline.Get());
    CommandList.SetConstantBuffer(PrimaryRayDebugShader.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetShaderResourceView(PrimaryRayDebugShader.Get(), Resources.RayTracingScene->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(PrimaryRayDebugShader.Get(), Output->GetUnorderedAccessView(), 0);

    const uint32 Width  = Output->GetDesc().Extent.X;
    const uint32 Height = Output->GetDesc().Extent.Y;

    constexpr uint32 ThreadCount = 8;
    const uint32 DispatchWidth   = Math::DivideByMultiple<uint32>(Width, ThreadCount);
    const uint32 DispatchHeight  = Math::DivideByMultiple<uint32>(Height, ThreadCount);

    CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
}

void FRayTracingPrimaryDebugPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    const bool bEnablePass =
        Context.View &&
        IsEnabled(*Context.View) &&
        Context.RayTracingOutput;

    GraphBuilder.AddPass("RayTracingPrimaryDebug", ERenderGraphPassFlags::Compute, bEnablePass,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            if (Context.RayTracingOutput)
            {
                PassBuilder.WriteTexture(Context.RayTracingOutput, ERHIResourceState::UnorderedAccess);
            }
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            Record(PassCommandList, *Context.FrameResources);
        });
}

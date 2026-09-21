#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "Renderer/Passes/DepthReducePass.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Settings/ShadowSettings.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

struct FReductionConstants
{
    Matrix4 CamProjection;
    float   NearPlane;
    float   FarPlane;
};

class FDepthReductionInitialCS
{
    DECLARE_SHADER_TYPE(FDepthReductionInitialCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

class FDepthReductionCS
{
    DECLARE_SHADER_TYPE(FDepthReductionCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FDepthReductionInitialCS, "Shaders/DepthReduction.hlsl", "ReductionMainInital", EShaderModel::SM_6_2);
IMPLEMENT_SHADER_TYPE(FDepthReductionCS,        "Shaders/DepthReduction.hlsl", "ReductionMain",       EShaderModel::SM_6_2);

/** Every step folds a 16x16 tile into a single texel, so the dispatch shrinks once per step already taken */
constexpr uint32 REDUCTION_TILE_SIZE = 16;

static void GetReductionDispatchSize(const FFrameResources& FrameResources, int32 NumStepsTaken, uint32& OutThreadsX, uint32& OutThreadsY)
{
    OutThreadsX = FrameResources.ReducedDepthBuffer[0]->GetDesc().Extent.X;
    OutThreadsY = FrameResources.ReducedDepthBuffer[0]->GetDesc().Extent.Y;

    for (int32 Step = 0; Step < NumStepsTaken; ++Step)
    {
        OutThreadsX = Math::DivideByMultiple(OutThreadsX, REDUCTION_TILE_SIZE);
        OutThreadsY = Math::DivideByMultiple(OutThreadsY, REDUCTION_TILE_SIZE);
    }
}

FDepthReducePass::FDepthReducePass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , ReduceDepthInitalPSO(nullptr)
    , ReduceDepthInitalShader(nullptr)
    , ReduceDepthPSO(nullptr)
    , ReduceDepthShader(nullptr)
{
}

FDepthReducePass::~FDepthReducePass()
{
    ReduceDepthInitalPSO.Reset();
    ReduceDepthInitalShader.Reset();
    ReduceDepthPSO.Reset();
    ReduceDepthShader.Reset();
}

bool FDepthReducePass::Initialize(FFrameResources& FrameResources)
{
    UNREFERENCED_VARIABLE(FrameResources);

    ReduceDepthInitalShader = FShaderCache::Get().GetShader<FDepthReductionInitialCS>();
    if (!ReduceDepthInitalShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader = ReduceDepthInitalShader.Get();

    ReduceDepthInitalPSO = RHI::CreateComputePipelineState(PSODesc);
    if (!ReduceDepthInitalPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ReduceDepthInitalPSO->SetDebugName("Initial DepthReduction PipelineState");
    }


    ReduceDepthShader = FShaderCache::Get().GetShader<FDepthReductionCS>();
    if (!ReduceDepthShader)
    {
        DEBUG_BREAK();
        return false;
    }

    PSODesc.Shader = ReduceDepthShader.Get();

    ReduceDepthPSO = RHI::CreateComputePipelineState(PSODesc);
    if (!ReduceDepthPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        ReduceDepthPSO->SetDebugName("DepthReduction PipelineState");
    }

    return true;
}

void FDepthReducePass::RecordInitialReduction(FRHICommandList& CommandList, FFrameResources& FrameResources, FScene* Scene)
{
    FRHITexture* Destination = FrameResources.ReducedDepthBuffer[0].Get();
    if (!Destination || Destination->GetDesc().Extent.X == 0 || Destination->GetDesc().Extent.Y == 0)
    {
        return;
    }

    TRACE_SCOPE("Depth Reduction Initial");

    FReductionConstants ReductionConstants;

    FSceneCamera* Camera = Scene->GetCamera();
    ReductionConstants.CamProjection = Camera->Snapshot.Projection;
    ReductionConstants.NearPlane     = Camera->Snapshot.NearPlane;
    ReductionConstants.FarPlane      = Camera->Snapshot.FarPlane;

    CommandList.SetComputePipelineState(ReduceDepthInitalPSO.Get());

    CommandList.SetShaderResourceView(ReduceDepthInitalShader.Get(), FrameResources.GBuffer[EGBufferIndex::Depth]->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthInitalShader.Get(), Destination->GetUnorderedAccessView(), 0);

    constexpr uint32 NumConstants = sizeof(FReductionConstants) / sizeof(uint32);
    CommandList.SetShaderConstants(ReduceDepthInitalShader.Get(), &ReductionConstants, NumConstants);

    uint32 ThreadsX = 0;
    uint32 ThreadsY = 0;
    GetReductionDispatchSize(FrameResources, 0, ThreadsX, ThreadsY);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);
}

void FDepthReducePass::RecordReduction(FRHICommandList& CommandList, FFrameResources& FrameResources, int32 SourceIndex, int32 DestinationIndex, int32 NumStepsTaken)
{
    FRHITexture* Source      = FrameResources.ReducedDepthBuffer[SourceIndex].Get();
    FRHITexture* Destination = FrameResources.ReducedDepthBuffer[DestinationIndex].Get();
    if (!Source || !Destination || Source->GetDesc().Extent.X == 0 || Source->GetDesc().Extent.Y == 0)
    {
        return;
    }

    TRACE_SCOPE("Depth Reduction");

    CommandList.SetComputePipelineState(ReduceDepthPSO.Get());

    CommandList.SetShaderResourceView(ReduceDepthShader.Get(), Source->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(ReduceDepthShader.Get(), Destination->GetUnorderedAccessView(), 0);

    uint32 ThreadsX = 0;
    uint32 ThreadsY = 0;
    GetReductionDispatchSize(FrameResources, NumStepsTaken, ThreadsX, ThreadsY);
    CommandList.Dispatch(ThreadsX, ThreadsY, 1);
}


void FDepthReducePass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    GraphBuilder.AddPass("DepthReduceInitial", ERenderGraphPassFlags::Compute, GCSMTightFrustum,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.GBufferDepth, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.WriteTexture(Context.ReducedDepthBuffer0, ERHIResourceState::UnorderedAccess);
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            RecordInitialReduction(PassCommandList, *Context.FrameResources, Context.Scene);
        });

    GraphBuilder.AddPass("DepthReduceStep1", ERenderGraphPassFlags::Compute, GCSMTightFrustum,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.ReducedDepthBuffer0, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.WriteTexture(Context.ReducedDepthBuffer1, ERHIResourceState::UnorderedAccess);
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            RecordReduction(PassCommandList, *Context.FrameResources, 0, 1, 1);
        });

    GraphBuilder.AddPass("DepthReduceStep2", ERenderGraphPassFlags::Compute, GCSMTightFrustum,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.ReducedDepthBuffer1, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.WriteTexture(Context.ReducedDepthBuffer0, ERHIResourceState::UnorderedAccess);
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            RecordReduction(PassCommandList, *Context.FrameResources, 1, 0, 2);
        });
}

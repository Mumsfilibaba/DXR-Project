#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "Renderer/Passes/CascadeGenerationPass.h"
#include "Renderer/Settings/RenderFeatureSettings.h"
#include "Renderer/Settings/ShadowSettings.h"
#include "Renderer/Shaders/ShadowShaders.h"
#include "Renderer/Performance/GPUProfiler.h"
#include "Renderer/Scene/Scene.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"

bool GCSMStableCascades = true;
static FAutoConsoleVariableRef CVarCSMStableCascades(
    "Renderer.CSM.StableCascades",
    "Set to true to enable stable cascades when generating shadow cascade matrices",
    GCSMStableCascades,
    EConsoleVariableFlags::Default);

FCascadeGenerationPass::FCascadeGenerationPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , CascadeGen()
    , CascadeGenShader()
{
}

FCascadeGenerationPass::~FCascadeGenerationPass()
{
    CascadeGen.Reset();
    CascadeGenShader.Reset();
}

bool FCascadeGenerationPass::Initialize(FFrameResources& Resources)
{
    CascadeGenShader = FShaderCache::Get().GetShader<FCascadeMatrixGenCS>();
    if (!CascadeGenShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc CascadeMatrixGenPSODesc;
    CascadeMatrixGenPSODesc.Shader = CascadeGenShader.Get();

    CascadeGen = RHI::CreateComputePipelineState(CascadeMatrixGenPSODesc);
    if (!CascadeGen)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        CascadeGen->SetDebugName("CascadeGen PSO");
    }

    const FRHIBufferDesc CascadeMatrixBufferDesc = FRHIBufferDesc::CreateStructuredBuffer(sizeof(FCascadeMatricesHLSL), NUM_SHADOW_CASCADES,
        EBufferFlags::Default | EBufferFlags::UnorderedAccessBuffer);
    Resources.CascadeMatrixBuffer = RHI::CreateBuffer(CascadeMatrixBufferDesc, ERHIResourceState::NonPixelShaderResource, nullptr);

    if (!Resources.CascadeMatrixBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.CascadeMatrixBuffer->SetDebugName("Cascade Matrices Buffer");
    }

    const FRHIBufferDesc CascadeSplitsBufferDesc = FRHIBufferDesc::CreateStructuredBuffer(sizeof(FCascadeSplitHLSL), NUM_SHADOW_CASCADES,
        EBufferFlags::Default | EBufferFlags::UnorderedAccessBuffer);
    Resources.CascadeSplitsBuffer = RHI::CreateBuffer(CascadeSplitsBufferDesc, ERHIResourceState::NonPixelShaderResource, nullptr);
    
    if (!Resources.CascadeSplitsBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Resources.CascadeSplitsBuffer->SetDebugName("Cascade SplitBuffer");
    }

    return true;
}

void FCascadeGenerationPass::Record(FRHICommandList& CommandList, FFrameResources& Resources)
{
    GPU_TRACE_SCOPE(CommandList, "Generate Cascade Matrices");

    CommandList.SetComputePipelineState(CascadeGen.Get());

    CommandList.SetConstantBuffer(CascadeGenShader.Get(), Resources.CameraBuffer.Get(), 0);
    CommandList.SetConstantBuffer(CascadeGenShader.Get(), Resources.CascadeGenerationDataBuffer.Get(), 1);

    CommandList.SetUnorderedAccessView(CascadeGenShader.Get(), Resources.CascadeMatrixBufferUAV.Get(), 0);
    CommandList.SetUnorderedAccessView(CascadeGenShader.Get(), Resources.CascadeSplitsBufferUAV.Get(), 1);

    CommandList.SetShaderResourceView(CascadeGenShader.Get(), Resources.ReducedDepthBuffer[0]->GetShaderResourceView(), 0);

    CommandList.Dispatch(1, 1, 1);
}

void FCascadeGenerationPass::AddRenderGraphPass(FRenderGraphBuilder& GraphBuilder, const FSceneRenderGraphContext& Context)
{
    FSceneDirectionalLight* DirectionalLight = Context.Scene ? Context.Scene->GetDirectionalLight() : nullptr;

    const bool bEnableSunShadows = GSunShadowsEnabled && (!DirectionalLight || DirectionalLight->bCastShadows);
    const bool bEnablePass       = GShadowsEnabled && bEnableSunShadows && !GFreezeRendering;

    GraphBuilder.AddPass("CascadeGeneration", ERenderGraphPassFlags::Compute, bEnablePass,
        [&Context](FRenderGraphPassBuilder& PassBuilder)
        {
            PassBuilder.ReadTexture(Context.GBufferDepth, ERHIResourceState::NonPixelShaderResource);
            PassBuilder.ReadTexture(Context.ReducedDepthBuffer0, ERHIResourceState::NonPixelShaderResource);

            if (Context.CascadeMatrixBufferUAV)
            {
                PassBuilder.Write(Context.CascadeMatrixBufferUAV);
            }

            if (Context.CascadeSplitsBufferUAV)
            {
                PassBuilder.Write(Context.CascadeSplitsBufferUAV);
            }
        },
        [this, Context](FRHICommandList& PassCommandList, const FRenderGraphPassResources& PassResources)
        {
            FPassResources Resolved = Context.CreatePassResources(PassResources);
            PassResourceSync::SyncToFrameResources(Resolved, *Context.FrameResources);
            Record(PassCommandList, *Context.FrameResources);
        });
}

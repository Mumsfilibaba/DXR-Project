#include "VulkanRHI/VulkanCommandContextState.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanRHI.h"

static VkImageLayout GetDepthStencilAttachmentLayout(const FVulkanDepthStencilViewRHI* DepthStencilView)
{
    if (DepthStencilView->IsReadOnly())
    {
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }
    else if (DepthStencilView->IsDepthReadOnly())
    {
        return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
    }
    else if (DepthStencilView->IsStencilReadOnly())
    {
        return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
    }

    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

#if VULKAN_VALIDATE_IMAGE_LAYOUTS
static void AddRenderPassAttachmentLayouts(FVulkanCommandBuffer& CommandBuffer, const FVulkanRenderTargetState& RenderTargetState,
    const VkRenderingAttachmentInfoKHR* ColorAttachments, const VkRenderingAttachmentInfoKHR* DepthStencilAttachment)
{
    const auto AddView = [&CommandBuffer](const FVulkanResourceView* View, VkImageLayout Layout)
    {
        if (!View)
        {
            return;
        }

        if (const FVulkanTextureRHI* Texture = static_cast<const FVulkanTextureRHI*>(View->GetOwnerResource()))
        {
            CommandBuffer.AddImageLayoutForValidation(Texture->GetVkImage(), Layout, Layout, true, "BeginRendering");
        }
    };

    for (uint32 Index = 0; Index < RenderTargetState.NumRenderTargets; Index++)
    {
        AddView(RenderTargetState.RenderTargetViews[Index], ColorAttachments[Index].imageLayout);
    }

    if (DepthStencilAttachment)
    {
        AddView(RenderTargetState.DepthStencilView, DepthStencilAttachment->imageLayout);
    }
}
#endif

FVulkanCommandContextState::FVulkanCommandContextState(FVulkanDevice* InDevice, FVulkanCommandContext& InContext)
    : FVulkanDeviceChild(InDevice)
    , GraphicsState()
    , ComputeState()
    , MeshletState()
    , CommonState()
    , Context(InContext)
    , CurrentFrame(0)
    , ContextPhase(ECommandContextPhase::Finished)
    , bMeshletPipelineActive(false)
{
}

FVulkanCommandContextState::~FVulkanCommandContextState()
{
    for (auto Entry : ComputeState.DescriptorStates)
    {
        delete Entry.Second.State;
    }

    for (auto Entry : GraphicsState.DescriptorStates)
    {
        delete Entry.Second.State;
    }

    for (auto Entry : MeshletState.DescriptorStates)
    {
        delete Entry.Second.State;
    }

    for (auto Entry : RayTracingState.DescriptorStates)
    {
        delete Entry.Second.State;
    }

    ComputeState.DescriptorStates.Clear();
    GraphicsState.DescriptorStates.Clear();
    MeshletState.DescriptorStates.Clear();
    RayTracingState.DescriptorStates.Clear();
}

bool FVulkanCommandContextState::Initialize()
{
    ResetState();
    return true;
}

void FVulkanCommandContextState::PrepareGraphicsState()
{
    if (!GraphicsState.PipelineState)
    {
        return;
    }

    CHECK(CommonGraphicsState.ViewInstancingState == GraphicsState.PipelineState->GetViewInstancingState());
    MAYBE_UNUSED FVulkanPipelineLayout* PipelineLayout = GraphicsState.PipelineState->GetPipelineLayout();
    CHECK(PipelineLayout != nullptr);

    CHECK(PipelineLayout == GraphicsState.CurrentDescriptorState->GetLayout());
    ResolveSampledImageLayouts(GraphicsState.CurrentDescriptorState);

    if (GraphicsState.CurrentDescriptorState->IsResourcesDirty())
    {
        GraphicsState.CurrentDescriptorState->UpdateDescriptorSets(Context.GetTransientDescriptorAllocator());
        GraphicsState.CurrentDescriptorState->ClearResourcesDirty();
    }

    GraphicsState.CurrentDescriptorState->TransitionBoundResources(Context);
    TransitionVertexAndIndexBuffers();

    if (Context.GetBarrierBatcher().HasPendingBarriers())
    {
        if (IsInsideRenderPass())
        {
            PauseRenderPass();
        }

        Context.GetBarrierBatcher().FlushBarriers(Context.GetCommandBuffer());
    }

    if (IsRenderPassPaused())
    {
        ResumeRenderPass();
    }
}

void FVulkanCommandContextState::ResolveSampledImageLayouts(FVulkanDescriptorState* DescriptorState)
{
    FVulkanTextureRHI* ReadOnlyDepthTexture = nullptr;
    if (FVulkanResourceView* DepthStencilView = CommonGraphicsState.RenderTargetState.DepthStencilView)
    {
        if (GetDepthStencilAttachmentLayout(static_cast<const FVulkanDepthStencilViewRHI*>(DepthStencilView)) == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        {
            ReadOnlyDepthTexture = static_cast<FVulkanTextureRHI*>(DepthStencilView->GetOwnerResource());
        }
    }

    DescriptorState->ResolveSampledImageLayouts(ReadOnlyDepthTexture, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
}

void FVulkanCommandContextState::TransitionVertexAndIndexBuffers()
{
    const FVulkanVertexBufferCache& VertexBufferCache = GraphicsState.VertexBufferCache;
    for (uint32 Index = 0; Index < VertexBufferCache.NumVertexBuffers; Index++)
    {
        if (FVulkanBufferRHI* VertexBuffer = VertexBufferCache.BufferResources[Index])
        {
            Context.RequireBufferState(VertexBuffer, ERHIResourceState::VertexBuffer);
        }
    }

    if (FVulkanBufferRHI* IndexBuffer = GraphicsState.IndexBufferCache.BufferResource)
    {
        Context.RequireBufferState(IndexBuffer, ERHIResourceState::IndexBuffer);
    }

#if VK_EXT_transform_feedback
    if (GVulkanSupportsTransformFeedback)
    {
        const FVulkanStreamOutputCache& StreamOutputCache = GraphicsState.StreamOutputCache;
        for (uint32 Index = 0; Index < StreamOutputCache.NumBuffers; Index++)
        {
            if (FVulkanBufferRHI* StreamOutputBuffer = StreamOutputCache.BufferResources[Index])
            {
                Context.RequireBufferState(StreamOutputBuffer, ERHIResourceState::StreamOutput);
            }
        }
    }
#endif
}

void FVulkanCommandContextState::BindGraphicsState()
{
    if (!GraphicsState.PipelineState)
    {
        return;
    }

    if (GraphicsState.bBindPipelineState || GVulkanForceBinding)
    {
        VkPipeline Pipeline = GraphicsState.PipelineState->GetVkPipeline();
        Context.GetCommandBuffer()->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
        GraphicsState.bBindPipelineState = false;
    }
    
    if (GraphicsState.CurrentDescriptorState->IsDescriptorSetDirty() || GVulkanForceBinding)
    {
        GraphicsState.CurrentDescriptorState->BindGraphicsDescriptorSets(Context.GetCommandBuffer());
        GraphicsState.CurrentDescriptorState->ClearDescriptorSetDirty();
    }
    
    FVulkanPipelineLayout* PipelineLayout = GraphicsState.PipelineState->GetPipelineLayout();
    if (GraphicsState.bBindPushConstants || GVulkanForceBinding)
    {
        BindPushConstants(PipelineLayout, EPushConstantsPipeline::Graphics);
        GraphicsState.bBindPushConstants = false;
    }

    if (GraphicsState.bBindVertexBuffers || GVulkanForceBinding)
    {
        FVulkanVertexBufferCache& VertexBufferCache = GraphicsState.VertexBufferCache;
        if (VertexBufferCache.NumVertexBuffers > 0)
        {
            Context.GetCommandBuffer()->BindVertexBuffers(0, VertexBufferCache.NumVertexBuffers, VertexBufferCache.VertexBuffers, VertexBufferCache.VertexBufferOffsets);
        }

        GraphicsState.bBindVertexBuffers = false;
    }

    if (GraphicsState.bBindIndexBuffer || GVulkanForceBinding)
    {
        FVulkanIndexBufferCache& IndexBufferCache = GraphicsState.IndexBufferCache;
        Context.GetCommandBuffer()->BindIndexBuffer(IndexBufferCache.IndexBuffer, IndexBufferCache.Offset, IndexBufferCache.IndexType);
        GraphicsState.bBindIndexBuffer = false;
    }

    if (CommonGraphicsState.bBindViewports || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetViewport(0, CommonGraphicsState.NumViewports, CommonGraphicsState.Viewports);
        CommonGraphicsState.bBindViewports = false;
    }

    if (CommonGraphicsState.bBindScissorRects || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetScissor(0, CommonGraphicsState.NumScissorRects, CommonGraphicsState.ScissorRects);
        CommonGraphicsState.bBindScissorRects = false;
    }

    if (CommonGraphicsState.bBindBlendFactor || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetBlendConstants(CommonGraphicsState.BlendFactor);
        CommonGraphicsState.bBindBlendFactor = false;
    }

    if (CommonGraphicsState.bBindStencilRef || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetStencilReference(VK_STENCIL_FACE_FRONT_AND_BACK, CommonGraphicsState.StencilRef);
        CommonGraphicsState.bBindStencilRef = false;
    }

    if (CommonGraphicsState.bBindDepthBias || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetDepthBias(CommonGraphicsState.DepthBias[0], CommonGraphicsState.DepthBias[1], CommonGraphicsState.DepthBias[2]);
        CommonGraphicsState.bBindDepthBias = false;
    }

    if (GraphicsState.PipelineState->IsDepthBoundsTestEnabled() && (CommonGraphicsState.bBindDepthBounds || GVulkanForceBinding))
    {
        Context.GetCommandBuffer()->SetDepthBounds(CommonGraphicsState.DepthBounds[0], CommonGraphicsState.DepthBounds[1]);
        CommonGraphicsState.bBindDepthBounds = false;
    }

#if VK_EXT_sample_locations
    if (GraphicsState.PipelineState->UsesSampleLocations() && (CommonGraphicsState.bBindSampleLocations || GVulkanForceBinding))
    {
        Context.GetCommandBuffer()->SetSampleLocations(&CommonGraphicsState.SampleLocationsInfo);
        CommonGraphicsState.bBindSampleLocations = false;
    }
#endif

#if VK_EXT_transform_feedback
    if (GraphicsState.bBindStreamOutputTargets && GVulkanSupportsTransformFeedback)
    {
        Context.GetCommandBuffer()->BindTransformFeedbackBuffers(
            0, 
            GraphicsState.StreamOutputCache.NumBuffers, 
            GraphicsState.StreamOutputCache.Buffers, 
            GraphicsState.StreamOutputCache.Offsets, 
            GraphicsState.StreamOutputCache.Sizes);

        GraphicsState.bBindStreamOutputTargets = false;
    }
#endif
}

void FVulkanCommandContextState::PrepareComputeState()
{
    if (!ComputeState.PipelineState)
    {
        return;
    }

    MAYBE_UNUSED FVulkanPipelineLayout* PipelineLayout = ComputeState.PipelineState->GetPipelineLayout();
    CHECK(PipelineLayout == ComputeState.CurrentDescriptorState->GetLayout());
    if (ComputeState.CurrentDescriptorState->IsResourcesDirty())
    {
        ComputeState.CurrentDescriptorState->UpdateDescriptorSets(Context.GetTransientDescriptorAllocator());
        ComputeState.CurrentDescriptorState->ClearResourcesDirty();
    }

    ComputeState.CurrentDescriptorState->TransitionBoundResources(Context);

    Context.GetBarrierBatcher().FlushBarriers(Context.GetCommandBuffer());
}

void FVulkanCommandContextState::BindComputeState()
{
    if (!ComputeState.PipelineState)
    {
        return;
    }

    if (ComputeState.bBindPipelineState || GVulkanForceBinding)
    {
        VkPipeline Pipeline = ComputeState.PipelineState->GetVkPipeline();
        Context.GetCommandBuffer()->BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);
        ComputeState.bBindPipelineState = false;
    }
    
    if (ComputeState.CurrentDescriptorState->IsDescriptorSetDirty() || GVulkanForceBinding)
    {
        ComputeState.CurrentDescriptorState->BindComputeDescriptorSets(Context.GetCommandBuffer());
        ComputeState.CurrentDescriptorState->ClearDescriptorSetDirty();
    }
    
    FVulkanPipelineLayout* PipelineLayout = ComputeState.PipelineState->GetPipelineLayout();
    if (ComputeState.bBindPushConstants || GVulkanForceBinding)
    {
        BindPushConstants(PipelineLayout, EPushConstantsPipeline::Compute);
        ComputeState.bBindPushConstants = false;
    }
}

void FVulkanCommandContextState::PrepareMeshletState()
{
    if (!MeshletState.PipelineState)
    {
        return;
    }

    CHECK(CommonGraphicsState.ViewInstancingState == MeshletState.PipelineState->GetViewInstancingState());
    MAYBE_UNUSED FVulkanPipelineLayout* PipelineLayout = MeshletState.PipelineState->GetPipelineLayout();
    CHECK(PipelineLayout != nullptr);

    CHECK(PipelineLayout == MeshletState.CurrentDescriptorState->GetLayout());
    ResolveSampledImageLayouts(MeshletState.CurrentDescriptorState);

    if (MeshletState.CurrentDescriptorState->IsResourcesDirty())
    {
        MeshletState.CurrentDescriptorState->UpdateDescriptorSets(Context.GetTransientDescriptorAllocator());
        MeshletState.CurrentDescriptorState->ClearResourcesDirty();
    }

    MeshletState.CurrentDescriptorState->TransitionBoundResources(Context);
    TransitionVertexAndIndexBuffers();

    if (Context.GetBarrierBatcher().HasPendingBarriers())
    {
        if (IsInsideRenderPass())
        {
            PauseRenderPass();
        }

        Context.GetBarrierBatcher().FlushBarriers(Context.GetCommandBuffer());
    }

    if (IsRenderPassPaused())
    {
        ResumeRenderPass();
    }
}

void FVulkanCommandContextState::BindMeshletState()
{
    if (!MeshletState.PipelineState)
    {
        return;
    }

    if (MeshletState.bBindPipelineState || GVulkanForceBinding)
    {
        VkPipeline Pipeline = MeshletState.PipelineState->GetVkPipeline();
        Context.GetCommandBuffer()->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
        MeshletState.bBindPipelineState = false;
    }

    if (MeshletState.CurrentDescriptorState->IsDescriptorSetDirty() || GVulkanForceBinding)
    {
        MeshletState.CurrentDescriptorState->BindGraphicsDescriptorSets(Context.GetCommandBuffer());
        MeshletState.CurrentDescriptorState->ClearDescriptorSetDirty();
    }

    FVulkanPipelineLayout* PipelineLayout = MeshletState.PipelineState->GetPipelineLayout();
    if (MeshletState.bBindPushConstants || GVulkanForceBinding)
    {
        BindPushConstants(PipelineLayout, EPushConstantsPipeline::Graphics);
        MeshletState.bBindPushConstants = false;
    }

    if (CommonGraphicsState.bBindViewports || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetViewport(0, CommonGraphicsState.NumViewports, CommonGraphicsState.Viewports);
        CommonGraphicsState.bBindViewports = false;
    }

    if (CommonGraphicsState.bBindScissorRects || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetScissor(0, CommonGraphicsState.NumScissorRects, CommonGraphicsState.ScissorRects);
        CommonGraphicsState.bBindScissorRects = false;
    }

    if (CommonGraphicsState.bBindBlendFactor || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetBlendConstants(CommonGraphicsState.BlendFactor);
        CommonGraphicsState.bBindBlendFactor = false;
    }

    if (CommonGraphicsState.bBindStencilRef || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetStencilReference(VK_STENCIL_FACE_FRONT_AND_BACK, CommonGraphicsState.StencilRef);
        CommonGraphicsState.bBindStencilRef = false;
    }

    if (CommonGraphicsState.bBindDepthBias || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetDepthBias(CommonGraphicsState.DepthBias[0], CommonGraphicsState.DepthBias[1], CommonGraphicsState.DepthBias[2]);
        CommonGraphicsState.bBindDepthBias = false;
    }

    if (MeshletState.PipelineState->IsDepthBoundsTestEnabled() && (CommonGraphicsState.bBindDepthBounds || GVulkanForceBinding))
    {
        Context.GetCommandBuffer()->SetDepthBounds(CommonGraphicsState.DepthBounds[0], CommonGraphicsState.DepthBounds[1]);
        CommonGraphicsState.bBindDepthBounds = false;
    }

#if VK_EXT_sample_locations
    if (MeshletState.PipelineState->UsesSampleLocations() && (CommonGraphicsState.bBindSampleLocations || GVulkanForceBinding))
    {
        Context.GetCommandBuffer()->SetSampleLocations(&CommonGraphicsState.SampleLocationsInfo);
        CommonGraphicsState.bBindSampleLocations = false;
    }
#endif
}

void FVulkanCommandContextState::BindPushConstants(FVulkanPipelineLayout* PipelineLayout, EPushConstantsPipeline::Type Pipeline)
{
    FPushConstantsInfo ConstantsInfo = PipelineLayout->GetConstantsInfo();
    if (ConstantsInfo.NumConstants > 0)
    {
        CHECK(ConstantsInfo.NumConstants <= VULKAN_MAX_NUM_PUSH_CONSTANTS);
        Context.GetCommandBuffer()->PushConstants(
            PipelineLayout->GetVkPipelineLayout(), 
            ConstantsInfo.StageFlags, 
            0, 
            ConstantsInfo.NumConstants * sizeof(uint32), 
            CommonState.PushConstantsCache[Pipeline].Constants);
    }
}

void FVulkanCommandContextState::DirtyPushConstants()
{
    GraphicsState.bBindPushConstants   = true;
    ComputeState.bBindPushConstants    = true;
    MeshletState.bBindPushConstants    = true;
    RayTracingState.bBindPushConstants = true;
}

void FVulkanCommandContextState::ResetState()
{
    for (FVulkanPushConstantsCache& ConstantCache : CommonState.PushConstantsCache)
    {
        ConstantCache.Clear();
    }

    GraphicsState.VertexBufferCache.Clear();
    GraphicsState.IndexBufferCache.Clear();

    Memory::Memzero(CommonGraphicsState.BlendFactor, sizeof(CommonGraphicsState.BlendFactor));
    Memory::Memzero(CommonGraphicsState.DepthBias, sizeof(CommonGraphicsState.DepthBias));

    CommonGraphicsState.DepthBounds[0] = 0.0f;
    CommonGraphicsState.DepthBounds[1] = 1.0f;

    GraphicsState.StreamOutputCache.Clear();
    CommonGraphicsState.StencilRef = 0;
    
    Memory::Memzero(CommonGraphicsState.Viewports, sizeof(CommonGraphicsState.Viewports));
    CommonGraphicsState.NumViewports = 0;

    Memory::Memzero(CommonGraphicsState.ScissorRects, sizeof(CommonGraphicsState.ScissorRects));
    CommonGraphicsState.NumScissorRects = 0;
    
    GraphicsState.PipelineState            = nullptr;
    GraphicsState.CurrentDescriptorState   = nullptr;
    GraphicsState.CurrentLayout            = nullptr;
    GraphicsState.bBindIndexBuffer         = true;
    CommonGraphicsState.bBindBlendFactor   = true;
    CommonGraphicsState.bBindStencilRef    = true;
    CommonGraphicsState.bBindDepthBias     = true;
    CommonGraphicsState.bBindDepthBounds   = true;
    GraphicsState.bBindPipelineState       = true;
    CommonGraphicsState.bBindScissorRects  = true;
    CommonGraphicsState.bBindViewports     = true;
    GraphicsState.bBindVertexBuffers       = true;
    GraphicsState.bBindPushConstants       = true;
    GraphicsState.bBindStreamOutputTargets = false;
    ComputeState.PipelineState             = nullptr;
    ComputeState.CurrentDescriptorState    = nullptr;
    ComputeState.CurrentLayout             = nullptr;
    ComputeState.bBindPipelineState        = true;
    ComputeState.bBindPushConstants        = true;
    MeshletState.PipelineState             = nullptr;
    MeshletState.CurrentDescriptorState    = nullptr;
    MeshletState.CurrentLayout             = nullptr;
    MeshletState.bBindPipelineState        = true;
    MeshletState.bBindPushConstants        = true;
    RayTracingState.PipelineState          = nullptr;
    RayTracingState.CurrentDescriptorState = nullptr;
    RayTracingState.CurrentLayout          = nullptr;
    RayTracingState.bBindPipelineState     = true;
    RayTracingState.bBindPushConstants     = true;
    bMeshletPipelineActive                 = false;

#if VK_EXT_sample_locations
    Memory::Memzero(CommonGraphicsState.SampleLocations, sizeof(CommonGraphicsState.SampleLocations));
    Memory::Memzero(&CommonGraphicsState.SampleLocationsInfo, sizeof(CommonGraphicsState.SampleLocationsInfo));

    CommonGraphicsState.bUsingCustomSampleLocations = false;
    CommonGraphicsState.bBindSampleLocations        = false;

    SetSamplePositions(FRHISamplePositionsDesc());
#endif
}

void FVulkanCommandContextState::BeginCommandBuffer()
{
    GraphicsState.bBindIndexBuffer         = true;
    CommonGraphicsState.bBindBlendFactor   = true;
    CommonGraphicsState.bBindStencilRef    = true;
    CommonGraphicsState.bBindDepthBias     = true;
    CommonGraphicsState.bBindDepthBounds   = true;
    GraphicsState.bBindPipelineState       = true;
    CommonGraphicsState.bBindScissorRects  = true;
    CommonGraphicsState.bBindViewports     = true;
    GraphicsState.bBindVertexBuffers       = true;
    GraphicsState.bBindPushConstants       = true;
    GraphicsState.bBindStreamOutputTargets = (GraphicsState.StreamOutputCache.NumBuffers > 0);
    ComputeState.bBindPipelineState        = true;
    ComputeState.bBindPushConstants        = true;
    MeshletState.bBindPipelineState        = true;
    MeshletState.bBindPushConstants        = true;
    RayTracingState.bBindPipelineState     = true;
    RayTracingState.bBindPushConstants     = true;

#if VK_EXT_sample_locations
    CommonGraphicsState.bBindSampleLocations = GVulkanSupportsSampleLocations && CommonGraphicsState.SampleLocationsInfo.sampleLocationsCount > 0;
#endif

    if (GraphicsState.CurrentDescriptorState)
    {
        GraphicsState.CurrentDescriptorState->DirtyDescriptorSet();
    }

    if (ComputeState.CurrentDescriptorState)
    {
        ComputeState.CurrentDescriptorState->DirtyDescriptorSet();
    }

    if (MeshletState.CurrentDescriptorState)
    {
        MeshletState.CurrentDescriptorState->DirtyDescriptorSet();
    }

    if (RayTracingState.CurrentDescriptorState)
    {
        RayTracingState.CurrentDescriptorState->DirtyDescriptorSet();
    }
}

void FVulkanCommandContextState::EndCommandBuffer()
{
    // Nothing needs closing out yet; the hook exists as the counterpart to BeginCommandBuffer.
}

#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
FVulkanRenderPassKey FVulkanCommandContextState::BuildRenderPassKey(const FVulkanRenderTargetState& RenderTargetState) const
{
    FVulkanRenderPassKey RenderPassKey;
    RenderPassKey.NumRenderTargets = static_cast<uint8>(RenderTargetState.NumRenderTargets);

    uint8 NumSamples = 0;
    for (uint32 Index = 0; Index < RenderTargetState.NumRenderTargets; Index++)
    {
        FVulkanResourceView* View = RenderTargetState.RenderTargetViews[Index];
        if (!View)
        {
            continue;
        }

        FVulkanTextureRHI* Texture = static_cast<FVulkanTextureRHI*>(View->GetOwnerResource());
        RenderPassKey.RenderTargetFormats[Index]             = Texture->GetDesc().Format;
        RenderPassKey.RenderTargetActions[Index].LoadAction  = EAttachmentLoadAction::Load;
        RenderPassKey.RenderTargetActions[Index].StoreAction = RenderTargetState.ColorStoreActions[Index];
        NumSamples = Math::Max<uint8>(static_cast<uint8>(Texture->GetDesc().NumSamples), NumSamples);
    }

    if (FVulkanResourceView* DepthView = RenderTargetState.DepthStencilView)
    {
        FVulkanTextureRHI*                Texture          = static_cast<FVulkanTextureRHI*>(DepthView->GetOwnerResource());
        const FVulkanDepthStencilViewRHI* DepthStencilView = static_cast<const FVulkanDepthStencilViewRHI*>(DepthView);

        RenderPassKey.DepthStencilFormat              = Texture->GetDesc().Format;
        RenderPassKey.DepthStencilActions.LoadAction  = EAttachmentLoadAction::Load;
        RenderPassKey.DepthStencilActions.StoreAction = RenderTargetState.DepthStencilStoreAction;
        RenderPassKey.DepthStencilFlags               = DepthStencilView->GetFlags();
        NumSamples = Math::Max<uint8>(static_cast<uint8>(Texture->GetDesc().NumSamples), NumSamples);
    }

    RenderPassKey.NumSamples = NumSamples;

    if (CommonGraphicsState.ViewInstancingState.bEnableViewInstancing)
    {
        RenderPassKey.ViewInstancingState = CommonGraphicsState.ViewInstancingState;
    }

    return RenderPassKey;
}
#endif // VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH

void FVulkanCommandContextState::BeginRenderPass(const FRHIBeginRenderPassDesc& RenderPassDesc)
{
    CHECK(ContextPhase == ECommandContextPhase::Recording);

    CommonGraphicsState.ViewInstancingState = RenderPassDesc.ViewInstancingState;

    FVulkanRenderTargetState& RenderTargetState = CommonGraphicsState.RenderTargetState;
    RenderTargetState.Clear();
    RenderTargetState.NumRenderTargets = RenderPassDesc.NumRenderTargets;

    uint32 Width          = TNumericLimits<uint32>::Max();
    uint32 Height         = TNumericLimits<uint32>::Max();
    uint32 NumArrayLayers = 0;
    uint8  NumSamples     = 0;

    VkClearValue ColorClearValues[RHI_MAX_RENDER_TARGETS] = {};

#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    FVulkanRenderPassKey RenderPassKey;
    RenderPassKey.NumRenderTargets = static_cast<uint8>(RenderPassDesc.NumRenderTargets);
#endif

    for (uint32 Index = 0; Index < RenderPassDesc.NumRenderTargets; Index++)
    {
        const FRHIRenderPassAttachment& Attachment = RenderPassDesc.RenderTargets[Index];
        FVulkanRenderTargetViewRHI* VulkanRenderTargetView = FVulkanDeviceRHI::ResourceCast(Attachment.View);
        if (!VulkanRenderTargetView)
        {
            VULKAN_ERROR("BeginRenderPass: RenderTargetView at slot %u is null (NumRenderTargets=%u)",
                Index, RenderPassDesc.NumRenderTargets);
            continue;
        }

        FVulkanTextureRHI* VulkanTexture = static_cast<FVulkanTextureRHI*>(VulkanRenderTargetView->GetOwnerResource());
        if (!VulkanTexture)
        {
            VULKAN_ERROR("BeginRenderPass: RenderTargetView at slot %u has no OwnerResource (source texture was destroyed)", Index);
            continue;
        }

        const FVulkanResourceView::FImageView& ImageViewInfo = VulkanRenderTargetView->GetImageViewInfo();

        Width          = Math::Min<uint32>(VulkanTexture->GetDesc().Extent.X, Width);
        Height         = Math::Min<uint32>(VulkanTexture->GetDesc().Extent.Y, Height);
        NumArrayLayers = Math::Max<uint32>(ImageViewInfo.SubresourceRange.layerCount, NumArrayLayers);
        NumSamples     = Math::Max<uint8>(static_cast<uint8>(VulkanTexture->GetDesc().NumSamples), NumSamples);

        RenderTargetState.RenderTargetViews[Index] = VulkanRenderTargetView;
        RenderTargetState.ColorStoreActions[Index] = Attachment.StoreAction;

    #if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
        RenderPassKey.RenderTargetFormats[Index]             = VulkanTexture->GetDesc().Format;
        RenderPassKey.RenderTargetActions[Index].LoadAction  = Attachment.LoadAction;
        RenderPassKey.RenderTargetActions[Index].StoreAction = Attachment.StoreAction;
    #endif

        Memory::Memcpy(ColorClearValues[Index].color.float32, Attachment.ClearValue.RGBA, sizeof(ColorClearValues[Index].color.float32));
    }

    VkClearValue DepthStencilClearValue = {};

    const FRHIDepthStencilAttachment& DepthStencilAttachment = RenderPassDesc.DepthStencilAttachment;
    FVulkanDepthStencilViewRHI* VulkanDepthStencilView = FVulkanDeviceRHI::ResourceCast(DepthStencilAttachment.View);
    if (VulkanDepthStencilView)
    {
        FVulkanTextureRHI* VulkanTexture = static_cast<FVulkanTextureRHI*>(VulkanDepthStencilView->GetOwnerResource());
        if (!VulkanTexture)
        {
            VULKAN_ERROR("BeginRenderPass: DepthStencilView has no OwnerResource (source texture was destroyed)");
        }
        if (VulkanTexture)
        {
            const FVulkanResourceView::FImageView& ImageViewInfo = VulkanDepthStencilView->GetImageViewInfo();

            Width          = Math::Min<uint32>(VulkanTexture->GetDesc().Extent.X, Width);
            Height         = Math::Min<uint32>(VulkanTexture->GetDesc().Extent.Y, Height);
            NumArrayLayers = Math::Max<uint32>(ImageViewInfo.SubresourceRange.layerCount, NumArrayLayers);
            NumSamples     = Math::Max<uint8>(static_cast<uint8>(VulkanTexture->GetDesc().NumSamples), NumSamples);

            RenderTargetState.DepthStencilView        = VulkanDepthStencilView;
            RenderTargetState.DepthStencilStoreAction = DepthStencilAttachment.StoreAction;

        #if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
            RenderPassKey.DepthStencilFormat              = VulkanTexture->GetDesc().Format;
            RenderPassKey.DepthStencilActions.LoadAction  = DepthStencilAttachment.LoadAction;
            RenderPassKey.DepthStencilActions.StoreAction = DepthStencilAttachment.StoreAction;
            RenderPassKey.DepthStencilFlags               = VulkanDepthStencilView->GetFlags();
        #endif

            DepthStencilClearValue.depthStencil.depth   = DepthStencilAttachment.ClearValue.Depth;
            DepthStencilClearValue.depthStencil.stencil = DepthStencilAttachment.ClearValue.Stencil;
        }
    }

    if (RenderPassDesc.ViewInstancingState.bEnableViewInstancing)
    {
        NumArrayLayers = 1;
    }

    RenderTargetState.RenderAreaWidth     = (Width  != TNumericLimits<uint32>::Max()) ? Width  : 0;
    RenderTargetState.RenderAreaHeight    = (Height != TNumericLimits<uint32>::Max()) ? Height : 0;
    RenderTargetState.RenderingLayerCount = Math::Max(NumArrayLayers, 1u);

    if (RenderTargetState.RenderAreaWidth == 0 || RenderTargetState.RenderAreaHeight == 0)
    {
        VULKAN_ERROR("FVulkanCommandContextState::BeginRenderPass produced a zero-sized render area (W=%u H=%u NumRenderTargets=%u)",
            RenderTargetState.RenderAreaWidth, RenderTargetState.RenderAreaHeight, RenderPassDesc.NumRenderTargets);
    }

#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    RenderPassKey.NumSamples = NumSamples;
    if (RenderPassDesc.ViewInstancingState.bEnableViewInstancing)
    {
        RenderPassKey.ViewInstancingState = RenderPassDesc.ViewInstancingState;
    }
#endif

    if (GVulkanSupportsMultiviews && RenderPassDesc.ViewInstancingState.bEnableViewInstancing)
    {
        constexpr uint32 MaxArraySlices = 32;
        const uint32 NumViews = Math::Min<uint32>(RenderPassDesc.ViewInstancingState.NumArraySlices, MaxArraySlices);

        uint32 ViewMask = 0;
        for (uint32 i = 0; i < NumViews; i++)
        {
            const uint32 BitIndex = RenderPassDesc.ViewInstancingState.StartRenderTargetArrayIndex + i;
            CHECK(BitIndex < 32);
            ViewMask |= (1u << BitIndex);
        }

        RenderTargetState.RenderingViewMask = ViewMask;
    }

    TransitionRenderPassAttachments(RenderTargetState);

    Context.GetBarrierBatcher().FlushBarriers(Context.GetCommandBuffer());

    if (GVulkanUseDynamicRendering)
    {
        VkRenderingAttachmentInfoKHR ColorAttachments[RHI_MAX_RENDER_TARGETS] = {};
        for (uint32 i = 0; i < RenderTargetState.NumRenderTargets; i++)
        {
            const FRHIRenderPassAttachment& ColorAttachment = RenderPassDesc.RenderTargets[i];
            ColorAttachments[i].sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            ColorAttachments[i].imageView   = RenderTargetState.RenderTargetViews[i] ? RenderTargetState.RenderTargetViews[i]->GetImageViewInfo().ImageView : VK_NULL_HANDLE;
            ColorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ColorAttachments[i].loadOp      = ConvertLoadAction(ColorAttachment.LoadAction);
            ColorAttachments[i].storeOp     = ConvertStoreAction(ColorAttachment.StoreAction);
            ColorAttachments[i].clearValue  = ColorClearValues[i];
        }

        VkRenderingAttachmentInfoKHR DepthStencilAttachmentInfo = {};
        VkRenderingAttachmentInfoKHR StencilAttachmentInfo      = {};

        bool bHasStencil = false;

        const bool bHasDepthStencil = (RenderTargetState.DepthStencilView != nullptr);
        if (bHasDepthStencil)
        {
            const FVulkanDepthStencilViewRHI* DepthStencilView = static_cast<const FVulkanDepthStencilViewRHI*>(RenderTargetState.DepthStencilView);
            bHasStencil = DepthStencilView->HasStencilFormat();

            DepthStencilAttachmentInfo.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            DepthStencilAttachmentInfo.imageView   = RenderTargetState.DepthStencilView->GetImageViewInfo().ImageView;
            DepthStencilAttachmentInfo.imageLayout = GetDepthStencilAttachmentLayout(DepthStencilView);
            DepthStencilAttachmentInfo.loadOp      = ConvertLoadAction(DepthStencilAttachment.LoadAction);
            DepthStencilAttachmentInfo.storeOp     = ConvertStoreAction(DepthStencilAttachment.StoreAction);
            DepthStencilAttachmentInfo.clearValue  = DepthStencilClearValue;

            StencilAttachmentInfo = DepthStencilAttachmentInfo;
        }

        VkRenderingInfoKHR RenderingInfo = {};
        RenderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        RenderingInfo.renderArea           = { {0, 0}, {RenderTargetState.RenderAreaWidth, RenderTargetState.RenderAreaHeight} };
        RenderingInfo.layerCount           = RenderTargetState.RenderingLayerCount;
        RenderingInfo.colorAttachmentCount = RenderTargetState.NumRenderTargets;
        RenderingInfo.pColorAttachments    = ColorAttachments;
        RenderingInfo.pDepthAttachment     = bHasDepthStencil ? &DepthStencilAttachmentInfo : nullptr;
        RenderingInfo.pStencilAttachment   = (bHasDepthStencil && bHasStencil) ? &StencilAttachmentInfo : nullptr;
        RenderingInfo.viewMask             = RenderTargetState.RenderingViewMask;

    #if VULKAN_VALIDATE_IMAGE_LAYOUTS
        AddRenderPassAttachmentLayouts(Context.GetCommandBuffer(), RenderTargetState, ColorAttachments, bHasDepthStencil ? &DepthStencilAttachmentInfo : nullptr);
    #endif

        Context.GetCommandBuffer()->BeginRendering(&RenderingInfo);
    }
#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    else
    {
        const VkRenderPass RenderPass = GetDevice()->GetRenderPassCache().GetRenderPass(RenderPassKey);
        if (!VULKAN_CHECK_HANDLE(RenderPass))
        {
            DEBUG_BREAK();
        }

        VkImageView AttachmentViews[RHI_MAX_RENDER_TARGETS + 1] = {};
        
        uint16 NumAttachmentViews = 0;
        for (uint32 i = 0; i < RenderTargetState.NumRenderTargets; i++)
        {
            if (FVulkanResourceView* View = RenderTargetState.RenderTargetViews[i])
            {
                AttachmentViews[NumAttachmentViews++] = View->GetImageViewInfo().ImageView;
            }
        }

        if (FVulkanResourceView* DepthView = RenderTargetState.DepthStencilView)
        {
            AttachmentViews[NumAttachmentViews++] = DepthView->GetImageViewInfo().ImageView;
        }

        FVulkanFramebufferKey FramebufferKey;
        FramebufferKey.RenderPass         = RenderPass;
        FramebufferKey.Width              = static_cast<uint16>(RenderTargetState.RenderAreaWidth);
        FramebufferKey.Height             = static_cast<uint16>(RenderTargetState.RenderAreaHeight);
        FramebufferKey.NumArrayLayers     = static_cast<uint16>(RenderTargetState.RenderingLayerCount);
        FramebufferKey.NumAttachmentViews = NumAttachmentViews;

        Memory::Memcpy(FramebufferKey.AttachmentViews, AttachmentViews, sizeof(FramebufferKey.AttachmentViews));

        RenderTargetState.Framebuffer = GetDevice()->GetRenderPassCache().GetFramebuffer(FramebufferKey);
        if (!VULKAN_CHECK_HANDLE(RenderTargetState.Framebuffer))
        {
            DEBUG_BREAK();
        }

        VkClearValue ClearValues[RHI_MAX_RENDER_TARGETS + 1] = {};
        uint32 NumClearValues = 0;
        for (uint32 i = 0; i < RenderTargetState.NumRenderTargets; i++)
        {
            ClearValues[NumClearValues++] = ColorClearValues[i];
        }

        if (RenderTargetState.DepthStencilView)
        {
            ClearValues[NumClearValues++] = DepthStencilClearValue;
        }

        VkRenderPassBeginInfo RenderPassBeginInfo = {};
        RenderPassBeginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassBeginInfo.renderPass               = RenderPass;
        RenderPassBeginInfo.framebuffer              = RenderTargetState.Framebuffer;
        RenderPassBeginInfo.renderArea.extent.width  = RenderTargetState.RenderAreaWidth;
        RenderPassBeginInfo.renderArea.extent.height = RenderTargetState.RenderAreaHeight;
        RenderPassBeginInfo.clearValueCount          = NumClearValues;
        RenderPassBeginInfo.pClearValues             = ClearValues;

        Context.GetCommandBuffer()->BeginRenderPass(&RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }
#endif // VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH

    ContextPhase = ECommandContextPhase::InsideRenderPass;
}

void FVulkanCommandContextState::TransitionRenderPassAttachments(const FVulkanRenderTargetState& RenderTargetState)
{
    for (uint32 Index = 0; Index < RenderTargetState.NumRenderTargets; Index++)
    {
        TransitionAttachmentLayout(RenderTargetState.RenderTargetViews[Index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (FVulkanResourceView* DepthStencilView = RenderTargetState.DepthStencilView)
    {
        const VkImageLayout DepthStencilLayout = GetDepthStencilAttachmentLayout(static_cast<const FVulkanDepthStencilViewRHI*>(DepthStencilView));
        TransitionAttachmentLayout(DepthStencilView, DepthStencilLayout);
    }
}

void FVulkanCommandContextState::TransitionAttachmentLayout(FVulkanResourceView* View, VkImageLayout Layout)
{
    if (!View)
    {
        return;
    }

    FVulkanTextureRHI* Texture = static_cast<FVulkanTextureRHI*>(View->GetOwnerResource());
    if (!Texture)
    {
        return;
    }

    const VkImageSubresourceRange& Range = View->GetImageViewInfo().SubresourceRange;
    Context.TransitionImageLayout(Texture, Layout, Range.baseMipLevel, Range.levelCount, Range.baseArrayLayer, Range.layerCount);
}

void FVulkanCommandContextState::EndRenderPass()
{
    CHECK(IsInsideRenderPass());

    if (GVulkanUseDynamicRendering)
    {
        Context.GetCommandBuffer()->EndRendering();
    }
#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    else
    {
        Context.GetCommandBuffer()->EndRenderPass();
    }
#endif

    ContextPhase = ECommandContextPhase::Recording;
}

void FVulkanCommandContextState::PauseRenderPass()
{
    CHECK(IsInsideRenderPass());

    if (GVulkanUseDynamicRendering)
    {
        Context.GetCommandBuffer()->EndRendering();
    }
#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    else
    {
        Context.GetCommandBuffer()->EndRenderPass();
    }
#endif

    ContextPhase = ECommandContextPhase::RenderPassPaused;
}

void FVulkanCommandContextState::ResumeRenderPass()
{
    CHECK(IsRenderPassPaused());

    const FVulkanRenderTargetState& RenderTargetState = CommonGraphicsState.RenderTargetState;
    TransitionRenderPassAttachments(RenderTargetState);

    Context.GetBarrierBatcher().FlushBarriers(Context.GetCommandBuffer());

    if (GVulkanUseDynamicRendering)
    {
        VkRenderingAttachmentInfoKHR ColorAttachments[RHI_MAX_RENDER_TARGETS] = {};
        for (uint32 i = 0; i < RenderTargetState.NumRenderTargets; i++)
        {
            ColorAttachments[i].sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            ColorAttachments[i].imageView   = RenderTargetState.RenderTargetViews[i] ? RenderTargetState.RenderTargetViews[i]->GetImageViewInfo().ImageView : VK_NULL_HANDLE;
            ColorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ColorAttachments[i].loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
            ColorAttachments[i].storeOp     = ConvertStoreAction(RenderTargetState.ColorStoreActions[i]);
        }

        VkRenderingAttachmentInfoKHR DepthStencilAttachment = {};
        VkRenderingAttachmentInfoKHR StencilAttachment      = {};

        bool bHasStencil = false;
        
        const bool bHasDepthStencil = (RenderTargetState.DepthStencilView != nullptr);
        if (bHasDepthStencil)
        {
            const FVulkanDepthStencilViewRHI* DepthStencilView = static_cast<const FVulkanDepthStencilViewRHI*>(RenderTargetState.DepthStencilView);
            bHasStencil = DepthStencilView->HasStencilFormat();

            DepthStencilAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            DepthStencilAttachment.imageView   = RenderTargetState.DepthStencilView->GetImageViewInfo().ImageView;
            DepthStencilAttachment.imageLayout = GetDepthStencilAttachmentLayout(DepthStencilView);
            DepthStencilAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
            DepthStencilAttachment.storeOp     = ConvertStoreAction(RenderTargetState.DepthStencilStoreAction);

            StencilAttachment = DepthStencilAttachment;
        }

        VkRenderingInfoKHR RenderingInfo = {};
        RenderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        RenderingInfo.renderArea           = { {0, 0}, {RenderTargetState.RenderAreaWidth, RenderTargetState.RenderAreaHeight} };
        RenderingInfo.layerCount           = RenderTargetState.RenderingLayerCount;
        RenderingInfo.colorAttachmentCount = RenderTargetState.NumRenderTargets;
        RenderingInfo.pColorAttachments    = ColorAttachments;
        RenderingInfo.pDepthAttachment     = bHasDepthStencil ? &DepthStencilAttachment : nullptr;
        RenderingInfo.pStencilAttachment   = (bHasDepthStencil && bHasStencil) ? &StencilAttachment : nullptr;
        RenderingInfo.viewMask             = RenderTargetState.RenderingViewMask;

    #if VULKAN_VALIDATE_IMAGE_LAYOUTS
        AddRenderPassAttachmentLayouts(Context.GetCommandBuffer(), RenderTargetState, ColorAttachments, bHasDepthStencil ? &DepthStencilAttachment : nullptr);
    #endif

        Context.GetCommandBuffer()->BeginRendering(&RenderingInfo);
    }
#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    else
    {
        const FVulkanRenderPassKey RenderPassKey = BuildRenderPassKey(RenderTargetState);
        const VkRenderPass RenderPass = GetDevice()->GetRenderPassCache().GetRenderPass(RenderPassKey);
        if (!VULKAN_CHECK_HANDLE(RenderPass))
        {
            DEBUG_BREAK();
        }

        VkRenderPassBeginInfo RenderPassBeginInfo = {};
        RenderPassBeginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassBeginInfo.renderPass               = RenderPass;
        RenderPassBeginInfo.framebuffer              = RenderTargetState.Framebuffer;
        RenderPassBeginInfo.renderArea.extent.width  = RenderTargetState.RenderAreaWidth;
        RenderPassBeginInfo.renderArea.extent.height = RenderTargetState.RenderAreaHeight;
        RenderPassBeginInfo.clearValueCount          = 0;
        RenderPassBeginInfo.pClearValues             = nullptr;

        Context.GetCommandBuffer()->BeginRenderPass(&RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }
#endif // VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH

    ContextPhase = ECommandContextPhase::InsideRenderPass;
}

void FVulkanCommandContextState::SetGraphicsPipelineState(FVulkanGraphicsPipelineStateRHI* InGraphicsPipelineState)
{
    bMeshletPipelineActive = false;

    FVulkanGraphicsPipelineStateRHI* CurrentGraphicsPipelineState = GraphicsState.PipelineState.Get();
    if (CurrentGraphicsPipelineState != InGraphicsPipelineState || GVulkanForceBinding)
    {
        GraphicsState.PipelineState      = MakeSharedRef<FVulkanGraphicsPipelineStateRHI>(InGraphicsPipelineState);
        GraphicsState.bBindPipelineState = true;

        if (InGraphicsPipelineState)
        {
            GraphicsState.CurrentLayout = InGraphicsPipelineState->GetPipelineLayout();
            
            if (FCachedDescriptorState* Cached = GraphicsState.DescriptorStates.Find(InGraphicsPipelineState))
            {
                Cached->LastUsedFrame                = CurrentFrame;
                GraphicsState.CurrentDescriptorState = Cached->State;

                if (GVulkanForceBinding)
                {
                    GraphicsState.CurrentDescriptorState->Reset();
                }
            }
            else
            {
                FVulkanDescriptorState* NewState = new FVulkanDescriptorState(GetDevice(), GraphicsState.CurrentLayout, GetDevice()->GetDefaultResources());
                
                FCachedDescriptorState NewEntry;
                NewEntry.State         = NewState;
                NewEntry.LastUsedFrame = CurrentFrame;
                
                GraphicsState.DescriptorStates.Add(InGraphicsPipelineState, NewEntry);
                GraphicsState.CurrentDescriptorState = NewState;
            }
        }
        else
        {
            GraphicsState.CurrentLayout          = nullptr;
            GraphicsState.CurrentDescriptorState = nullptr;
        }

        // NOTE: When we change PipelineLayout/PipelineState we need to ensure that PushConstants are also bound
        GraphicsState.bBindPushConstants = true;

        CommonGraphicsState.DepthBias[0]   = 0.0f;
        CommonGraphicsState.DepthBias[1]   = 0.0f;
        CommonGraphicsState.DepthBias[2]   = 0.0f;
        CommonGraphicsState.bBindDepthBias = true;

        CommonGraphicsState.DepthBounds[0]   = 0.0f;
        CommonGraphicsState.DepthBounds[1]   = 1.0f;
        CommonGraphicsState.bBindDepthBounds = true;
    }
}

void FVulkanCommandContextState::SetComputePipelineState(FVulkanComputePipelineStateRHI* InComputePipelineState)
{
    FVulkanComputePipelineStateRHI* CurrentComputePipelineState = ComputeState.PipelineState.Get();
    if (CurrentComputePipelineState != InComputePipelineState || GVulkanForceBinding)
    {
        ComputeState.PipelineState      = MakeSharedRef<FVulkanComputePipelineStateRHI>(InComputePipelineState);
        ComputeState.bBindPipelineState = true;

        if (InComputePipelineState)
        {
            ComputeState.CurrentLayout = InComputePipelineState->GetPipelineLayout();

            if (FCachedDescriptorState* Cached = ComputeState.DescriptorStates.Find(InComputePipelineState))
            {
                Cached->LastUsedFrame = CurrentFrame;
                ComputeState.CurrentDescriptorState = Cached->State;
                if (GVulkanForceBinding)
                {
                    ComputeState.CurrentDescriptorState->Reset();
                }
            }
            else
            {
                FVulkanDescriptorState* NewState = new FVulkanDescriptorState(GetDevice(), ComputeState.CurrentLayout, GetDevice()->GetDefaultResources());
                
                FCachedDescriptorState NewEntry;
                NewEntry.State         = NewState;
                NewEntry.LastUsedFrame = CurrentFrame;
                
                ComputeState.DescriptorStates.Add(InComputePipelineState, NewEntry);
                ComputeState.CurrentDescriptorState = NewState;
            }
        }
        else
        {
            ComputeState.CurrentLayout          = nullptr;
            ComputeState.CurrentDescriptorState = nullptr;
        }

        // NOTE: When we change PipelineLayout/PipelineState we need to ensure that PushConstants are also bound
        ComputeState.bBindPushConstants = true;
    }
}

void FVulkanCommandContextState::SetMeshletPipelineState(FVulkanMeshletPipelineStateRHI* InMeshletPipelineState)
{
    bMeshletPipelineActive = true;

    FVulkanMeshletPipelineStateRHI* CurrentMeshletPipelineState = MeshletState.PipelineState.Get();
    if (CurrentMeshletPipelineState != InMeshletPipelineState || GVulkanForceBinding)
    {
        MeshletState.PipelineState      = MakeSharedRef<FVulkanMeshletPipelineStateRHI>(InMeshletPipelineState);
        MeshletState.bBindPipelineState = true;

        if (InMeshletPipelineState)
        {
            MeshletState.CurrentLayout = InMeshletPipelineState->GetPipelineLayout();

            if (FCachedDescriptorState* Cached = MeshletState.DescriptorStates.Find(InMeshletPipelineState))
            {
                Cached->LastUsedFrame                = CurrentFrame;
                MeshletState.CurrentDescriptorState  = Cached->State;

                if (GVulkanForceBinding)
                {
                    MeshletState.CurrentDescriptorState->Reset();
                }
            }
            else
            {
                FVulkanDescriptorState* NewState = new FVulkanDescriptorState(GetDevice(), MeshletState.CurrentLayout, GetDevice()->GetDefaultResources());

                FCachedDescriptorState NewEntry;
                NewEntry.State         = NewState;
                NewEntry.LastUsedFrame = CurrentFrame;

                MeshletState.DescriptorStates.Add(InMeshletPipelineState, NewEntry);
                MeshletState.CurrentDescriptorState = NewState;
            }
        }
        else
        {
            MeshletState.CurrentLayout          = nullptr;
            MeshletState.CurrentDescriptorState = nullptr;
        }

        // NOTE: When we change PipelineLayout/PipelineState we need to ensure that PushConstants are also bound
        MeshletState.bBindPushConstants = true;
    }
}

void FVulkanCommandContextState::SetViewports(VkViewport* Viewports, uint32 NumViewports)
{
    CHECK(NumViewports < VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT);

    const uint32 ViewportArraySize = sizeof(VkViewport) * NumViewports;
    if (CommonGraphicsState.NumViewports != NumViewports || 
        Memory::Memcmp(CommonGraphicsState.Viewports, Viewports, ViewportArraySize) != 0 || GVulkanForceBinding)
    {
        Memory::Memcpy(CommonGraphicsState.Viewports, Viewports, ViewportArraySize);

        CommonGraphicsState.NumViewports   = NumViewports;
        CommonGraphicsState.bBindViewports = true;
    }
}

void FVulkanCommandContextState::SetScissorRects(VkRect2D* ScissorRects, uint32 NumScissorRects)
{
    CHECK(NumScissorRects < VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT);

    const uint32 ScissorRectArraySize = sizeof(VkRect2D) * NumScissorRects;
    if (CommonGraphicsState.NumScissorRects != NumScissorRects || 
        Memory::Memcmp(CommonGraphicsState.ScissorRects, ScissorRects, ScissorRectArraySize) != 0 || GVulkanForceBinding)
    {
        Memory::Memcpy(CommonGraphicsState.ScissorRects, ScissorRects, ScissorRectArraySize);

        CommonGraphicsState.NumScissorRects   = NumScissorRects;
        CommonGraphicsState.bBindScissorRects = true;
    }
}

void FVulkanCommandContextState::SetBlendFactor(const float BlendFactor[4])
{
    if (Memory::Memcmp(CommonGraphicsState.BlendFactor, BlendFactor, sizeof(CommonGraphicsState.BlendFactor)) != 0 || GVulkanForceBinding)
    {
        Memory::Memcpy(CommonGraphicsState.BlendFactor, BlendFactor, sizeof(CommonGraphicsState.BlendFactor));
        CommonGraphicsState.bBindBlendFactor = true;
    }
}

void FVulkanCommandContextState::SetStencilRef(uint32 InStencilRef)
{
    if (CommonGraphicsState.StencilRef != InStencilRef || GVulkanForceBinding)
    {
        CommonGraphicsState.StencilRef      = InStencilRef;
        CommonGraphicsState.bBindStencilRef = true;
    }
}

void FVulkanCommandContextState::SetDepthBias(float InDepthBias, float InDepthBiasClamp, float InSlopeScaledDepthBias)
{
    const float NewValues[3] = 
    { 
        InDepthBias, 
        InDepthBiasClamp, 
        InSlopeScaledDepthBias
    };

    if (Memory::Memcmp(CommonGraphicsState.DepthBias, NewValues, sizeof(NewValues)) != 0 || GVulkanForceBinding)
    {
        Memory::Memcpy(CommonGraphicsState.DepthBias, NewValues, sizeof(NewValues));
        CommonGraphicsState.bBindDepthBias = true;
    }
}

void FVulkanCommandContextState::SetDepthBounds(float InMinDepth, float InMaxDepth)
{
    const float NewValues[2] = 
    { 
        InMinDepth, 
        InMaxDepth
    };

    if (Memory::Memcmp(CommonGraphicsState.DepthBounds, NewValues, sizeof(NewValues)) != 0 || GVulkanForceBinding)
    {
        Memory::Memcpy(CommonGraphicsState.DepthBounds, NewValues, sizeof(NewValues));
        CommonGraphicsState.bBindDepthBounds = true;
    }
}

void FVulkanCommandContextState::SetSamplePositions(const FRHISamplePositionsDesc& SamplePositionsDesc)
{
#if VK_EXT_sample_locations
    if (!GVulkanSupportsSampleLocations)
    {
        return;
    }

    const bool bCustom = SamplePositionsDesc.NumSamplesPerPixel > 0;

    // Vulkan places the pixel center at (0.5, 0.5), so an offset of zero maps to the standard 1x location.
    const uint32 NumSamplesPerPixel = bCustom ? SamplePositionsDesc.NumSamplesPerPixel : 1;

    if ((GVulkanSampleLocationSampleCounts & ConvertSampleCount(NumSamplesPerPixel)) == 0)
    {
        return;
    }

    const uint32 GridWidth          = bCustom ? SamplePositionsDesc.GridWidth  : 1;
    const uint32 GridHeight         = bCustom ? SamplePositionsDesc.GridHeight : 1;
    const uint32 NumLocations       = NumSamplesPerPixel * GridWidth * GridHeight;
    CHECK(NumLocations <= RHI_MAX_SAMPLE_POSITIONS);

    VkSampleLocationEXT NewLocations[RHI_MAX_SAMPLE_POSITIONS] = { };
    for (uint32 Index = 0; Index < NumLocations; ++Index)
    {
        const FRHISamplePosition Position = bCustom ? SamplePositionsDesc.Positions[Index] : FRHISamplePosition();
        NewLocations[Index].x = Math::Clamp(0.5f + Position.X, GVulkanSampleLocationCoordinateRange[0], GVulkanSampleLocationCoordinateRange[1]);
        NewLocations[Index].y = Math::Clamp(0.5f + Position.Y, GVulkanSampleLocationCoordinateRange[0], GVulkanSampleLocationCoordinateRange[1]);
    }

    const uint32 LocationArraySize = sizeof(VkSampleLocationEXT) * NumLocations;
    if (CommonGraphicsState.SampleLocationsInfo.sampleLocationsCount != NumLocations ||
        Memory::Memcmp(CommonGraphicsState.SampleLocations, NewLocations, LocationArraySize) != 0)
    {
        Memory::Memcpy(CommonGraphicsState.SampleLocations, NewLocations, LocationArraySize);

        CommonGraphicsState.SampleLocationsInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT;
        CommonGraphicsState.SampleLocationsInfo.pNext                   = nullptr;
        CommonGraphicsState.SampleLocationsInfo.sampleLocationsPerPixel = ConvertSampleCount(NumSamplesPerPixel);
        CommonGraphicsState.SampleLocationsInfo.sampleLocationGridSize  = VkExtent2D{ GridWidth, GridHeight };
        CommonGraphicsState.SampleLocationsInfo.sampleLocationsCount    = NumLocations;
        CommonGraphicsState.SampleLocationsInfo.pSampleLocations        = CommonGraphicsState.SampleLocations;

        CommonGraphicsState.bBindSampleLocations = true;
    }

    CommonGraphicsState.bUsingCustomSampleLocations = bCustom;
#else
    UNREFERENCED_VARIABLE(SamplePositionsDesc);
#endif
}

void FVulkanCommandContextState::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
    GraphicsState.StreamOutputCache.NumBuffers = Math::Min(static_cast<uint32>(Buffers.Size()), static_cast<uint32>(VULKAN_MAX_STREAM_OUTPUT_BUFFER_COUNT));
    for (uint32 Index = 0; Index < GraphicsState.StreamOutputCache.NumBuffers; ++Index)
    {
        FVulkanBufferRHI* VulkanBuffer = FVulkanDeviceRHI::ResourceCast(Buffers[Index]);
        if (VulkanBuffer)
        {
            GraphicsState.StreamOutputCache.Buffers[Index]         = VulkanBuffer->GetVkBuffer();
            GraphicsState.StreamOutputCache.Offsets[Index]         = Offsets ? Offsets[Index] : 0;
            GraphicsState.StreamOutputCache.Sizes[Index]           = VulkanBuffer->GetDesc().Size;
            GraphicsState.StreamOutputCache.BufferResources[Index] = VulkanBuffer;
        }
        else
        {
            GraphicsState.StreamOutputCache.Buffers[Index]         = VK_NULL_HANDLE;
            GraphicsState.StreamOutputCache.Offsets[Index]         = 0;
            GraphicsState.StreamOutputCache.Sizes[Index]           = 0;
            GraphicsState.StreamOutputCache.BufferResources[Index] = nullptr;
        }
    }

    GraphicsState.bBindStreamOutputTargets = true;
}

void FVulkanCommandContextState::SetVertexBuffer(FVulkanBufferRHI* VertexBuffer, uint32 VertexBufferSlot)
{
    CHECK(VertexBufferSlot < VULKAN_MAX_VERTEX_BUFFER_SLOTS);
    
    VkBuffer     Buffer;
    VkDeviceSize Offset;

    if (VertexBuffer)
    {
        Buffer = VertexBuffer->GetBindVkBuffer();
        Offset = VertexBuffer->GetBindOffset();
    }
    else
    {
        Buffer = VK_NULL_HANDLE;
        Offset = 0;
    }

    VkBuffer     CurrentBuffer = GraphicsState.VertexBufferCache.VertexBuffers[VertexBufferSlot];
    VkDeviceSize CurrentOffset = GraphicsState.VertexBufferCache.VertexBufferOffsets[VertexBufferSlot];

    GraphicsState.VertexBufferCache.BufferResources[VertexBufferSlot] = VertexBuffer;

    if (Buffer != CurrentBuffer || Offset != CurrentOffset || GVulkanForceBinding)
    {
        GraphicsState.VertexBufferCache.VertexBuffers[VertexBufferSlot]       = Buffer;
        GraphicsState.VertexBufferCache.VertexBufferOffsets[VertexBufferSlot] = Offset;
        GraphicsState.VertexBufferCache.NumVertexBuffers                      = Math::Max(GraphicsState.VertexBufferCache.NumVertexBuffers, VertexBufferSlot + 1);
        GraphicsState.bBindVertexBuffers                                      = true;
    }
}

void FVulkanCommandContextState::SetIndexBuffer(FVulkanBufferRHI* IndexBuffer, VkIndexType IndexType)
{
    VkBuffer     Buffer;
    VkDeviceSize Offset;

    if (IndexBuffer)
    {
        Buffer = IndexBuffer->GetBindVkBuffer();
        Offset = IndexBuffer->GetBindOffset();
    }
    else
    {
        Buffer = VK_NULL_HANDLE;
        Offset = 0;
    }

    VkBuffer     CurrentBuffer    = GraphicsState.IndexBufferCache.IndexBuffer;
    VkDeviceSize CurrentOffset    = GraphicsState.IndexBufferCache.Offset;
    VkIndexType  CurrentIndexType = GraphicsState.IndexBufferCache.IndexType;

    GraphicsState.IndexBufferCache.BufferResource = IndexBuffer;

    if (Buffer != CurrentBuffer || Offset != CurrentOffset || IndexType != CurrentIndexType || GVulkanForceBinding)
    {
        GraphicsState.IndexBufferCache.IndexBuffer = Buffer;
        GraphicsState.IndexBufferCache.Offset      = Offset;
        GraphicsState.IndexBufferCache.IndexType   = IndexType;
        GraphicsState.bBindIndexBuffer             = true;
    }
}

void FVulkanCommandContextState::SetPushConstants(EShaderStage ShaderStage, const uint32* ShaderConstants, uint32 NumShaderConstants)
{
    const EPushConstantsPipeline::Type Pipeline = GetPushConstantsPipeline(ShaderStage);

    FVulkanPushConstantsCache& ConstantCache = CommonState.PushConstantsCache[Pipeline];
    if (NumShaderConstants != ConstantCache.NumConstants || 
        Memory::Memcmp(ShaderConstants, ConstantCache.Constants, sizeof(uint32) * NumShaderConstants) != 0 || GVulkanForceBinding)
    {
        Memory::Memcpy(ConstantCache.Constants, ShaderConstants, sizeof(uint32) * NumShaderConstants);

        ConstantCache.NumConstants = NumShaderConstants;

        DirtyPushConstants();
    }
}

void FVulkanCommandContextState::SetSRV(FVulkanShaderResourceViewRHI* ShaderResourceView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex)
{
    CHECK(ResourceIndex < VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);

    FVulkanPipelineLayout*  Layout          = nullptr;
    FVulkanDescriptorState* DescriptorState = nullptr;

    if (ShaderStage == EShaderVisibility::Compute)
    {
        Layout          = ComputeState.CurrentLayout;
        DescriptorState = ComputeState.CurrentDescriptorState;
    }
    else if (ShaderStage == EShaderVisibility::RayTracing)
    {
        Layout          = RayTracingState.CurrentLayout;
        DescriptorState = RayTracingState.CurrentDescriptorState;
    }
    else if (bMeshletPipelineActive)
    {
        Layout          = MeshletState.CurrentLayout;
        DescriptorState = MeshletState.CurrentDescriptorState;
    }
    else
    {
        Layout          = GraphicsState.CurrentLayout;
        DescriptorState = GraphicsState.CurrentDescriptorState;
    }
    
    if (!Layout || !DescriptorState)
    {
        VULKAN_WARNING("Binding a ShaderResource without having a PipelineState set, this does not have any effect");
        DEBUG_BREAK();
        return;
    }
    
    uint32 BindingIndex;
    uint32 DescriptorSetIndex;

    if (!Layout->GetDescriptorBinding(ShaderStage, EResourceType::SRV, ResourceIndex, DescriptorSetIndex, BindingIndex))
    {
    #if VULKAN_ENABLE_BINDING_VALIDATION
        VULKAN_WARNING("SetShaderResourceView: Slot %u does not exist in %s shader", ResourceIndex, ToString(ShaderStage));
    #endif
        return;
    }
    
    DescriptorState->SetSRV(ShaderResourceView, DescriptorSetIndex, BindingIndex);
}

void FVulkanCommandContextState::SetUAV(FVulkanUnorderedAccessViewRHI* UnorderedAccessView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex)
{
    CHECK(ResourceIndex < VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);

    FVulkanPipelineLayout*  Layout          = nullptr;
    FVulkanDescriptorState* DescriptorState = nullptr;

    if (ShaderStage == EShaderVisibility::Compute)
    {
        Layout          = ComputeState.CurrentLayout;
        DescriptorState = ComputeState.CurrentDescriptorState;
    }
    else if (ShaderStage == EShaderVisibility::RayTracing)
    {
        Layout          = RayTracingState.CurrentLayout;
        DescriptorState = RayTracingState.CurrentDescriptorState;
    }
    else if (bMeshletPipelineActive)
    {
        Layout          = MeshletState.CurrentLayout;
        DescriptorState = MeshletState.CurrentDescriptorState;
    }
    else
    {
        Layout          = GraphicsState.CurrentLayout;
        DescriptorState = GraphicsState.CurrentDescriptorState;
    }
    
    if (!Layout || !DescriptorState)
    {
        VULKAN_WARNING("Binding a ShaderResource without having a PipelineState set, this does not have any effect");
        DEBUG_BREAK();
        return;
    }
    
    uint32 BindingIndex;
    uint32 DescriptorSetIndex;

    if (!Layout->GetDescriptorBinding(ShaderStage, EResourceType::UAV, ResourceIndex, DescriptorSetIndex, BindingIndex))
    {
    #if VULKAN_ENABLE_BINDING_VALIDATION
        VULKAN_WARNING("SetUnorderedAccessView: Slot %u does not exist in %s shader", ResourceIndex, ToString(ShaderStage));
    #endif
        return;
    }
    
    DescriptorState->SetUAV(UnorderedAccessView, DescriptorSetIndex, BindingIndex);
}

void FVulkanCommandContextState::SetUniformBuffer(FVulkanBufferRHI* UniformBuffer, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex)
{
    CHECK(ResourceIndex < VULKAN_DEFAULT_UNIFORM_BUFFER_COUNT);

    FVulkanPipelineLayout*  Layout          = nullptr;
    FVulkanDescriptorState* DescriptorState = nullptr;

    if (ShaderStage == EShaderVisibility::Compute)
    {
        Layout          = ComputeState.CurrentLayout;
        DescriptorState = ComputeState.CurrentDescriptorState;
    }
    else if (ShaderStage == EShaderVisibility::RayTracing)
    {
        Layout          = RayTracingState.CurrentLayout;
        DescriptorState = RayTracingState.CurrentDescriptorState;
    }
    else if (bMeshletPipelineActive)
    {
        Layout          = MeshletState.CurrentLayout;
        DescriptorState = MeshletState.CurrentDescriptorState;
    }
    else
    {
        Layout          = GraphicsState.CurrentLayout;
        DescriptorState = GraphicsState.CurrentDescriptorState;
    }
    
    if (!Layout || !DescriptorState)
    {
        VULKAN_WARNING("Binding a ShaderResource without having a PipelineState set, this does not have any effect");
        DEBUG_BREAK();
        return;
    }
    
    uint32 BindingIndex;
    uint32 DescriptorSetIndex;

    if (!Layout->GetDescriptorBinding(ShaderStage, EResourceType::UniformBuffer, ResourceIndex, DescriptorSetIndex, BindingIndex))
    {
    #if VULKAN_ENABLE_BINDING_VALIDATION
        VULKAN_WARNING("SetConstantBuffer: Slot %u does not exist in %s shader", ResourceIndex, ToString(ShaderStage));
    #endif
        return;
    }
    
    DescriptorState->SetUniformBuffer(UniformBuffer, DescriptorSetIndex, BindingIndex);
}

void FVulkanCommandContextState::SetSampler(FVulkanSamplerStateRHI* SamplerState, EShaderVisibility::Type ShaderStage, uint32 SamplerIndex)
{
    CHECK(SamplerIndex < VULKAN_DEFAULT_SAMPLER_STATE_COUNT);

    FVulkanPipelineLayout*  Layout          = nullptr;
    FVulkanDescriptorState* DescriptorState = nullptr;

    if (ShaderStage == EShaderVisibility::Compute)
    {
        Layout          = ComputeState.CurrentLayout;
        DescriptorState = ComputeState.CurrentDescriptorState;
    }
    else if (ShaderStage == EShaderVisibility::RayTracing)
    {
        Layout          = RayTracingState.CurrentLayout;
        DescriptorState = RayTracingState.CurrentDescriptorState;
    }
    else if (bMeshletPipelineActive)
    {
        Layout          = MeshletState.CurrentLayout;
        DescriptorState = MeshletState.CurrentDescriptorState;
    }
    else
    {
        Layout          = GraphicsState.CurrentLayout;
        DescriptorState = GraphicsState.CurrentDescriptorState;
    }
    
    if (!Layout || !DescriptorState)
    {
        VULKAN_WARNING("Binding a ShaderResource without having a PipelineState set, this does not have any effect");
        DEBUG_BREAK();
        return;
    }
    
    uint32 BindingIndex;
    uint32 DescriptorSetIndex;

    if (!Layout->GetDescriptorBinding(ShaderStage, EResourceType::Sampler, SamplerIndex, DescriptorSetIndex, BindingIndex))
    {
    #if VULKAN_ENABLE_BINDING_VALIDATION
        VULKAN_WARNING("SetSamplerState: Slot %u does not exist in %s shader", SamplerIndex, ToString(ShaderStage));
    #endif
        return;
    }
    
    DescriptorState->SetSampler(SamplerState, DescriptorSetIndex, BindingIndex);
}

void FVulkanCommandContextState::SetRayTracingPipelineState(FVulkanRayTracingPipelineStateRHI* InRayTracingPipelineState)
{
    if (RayTracingState.PipelineState.Get() != InRayTracingPipelineState || GVulkanForceBinding)
    {
        RayTracingState.PipelineState      = MakeSharedRef<FVulkanRayTracingPipelineStateRHI>(InRayTracingPipelineState);
        RayTracingState.bBindPipelineState = true;

        if (InRayTracingPipelineState)
        {
            RayTracingState.CurrentLayout = InRayTracingPipelineState->GetPipelineLayout();

            if (FCachedDescriptorState* Cached = RayTracingState.DescriptorStates.Find(InRayTracingPipelineState))
            {
                Cached->LastUsedFrame                  = CurrentFrame;
                RayTracingState.CurrentDescriptorState = Cached->State;

                if (GVulkanForceBinding)
                {
                    RayTracingState.CurrentDescriptorState->Reset();
                }
            }
            else
            {
                FVulkanDescriptorState* NewState = new FVulkanDescriptorState(GetDevice(), RayTracingState.CurrentLayout, GetDevice()->GetDefaultResources());

                FCachedDescriptorState NewEntry;
                NewEntry.State         = NewState;
                NewEntry.LastUsedFrame = CurrentFrame;

                RayTracingState.DescriptorStates.Add(InRayTracingPipelineState, NewEntry);
                RayTracingState.CurrentDescriptorState = NewState;
            }
        }
        else
        {
            RayTracingState.CurrentLayout          = nullptr;
            RayTracingState.CurrentDescriptorState = nullptr;
        }

        RayTracingState.bBindPushConstants = true;
    }
}

void FVulkanCommandContextState::PrepareRayTracingState()
{
    if (!RayTracingState.PipelineState)
    {
        return;
    }

    if (RayTracingState.CurrentDescriptorState->IsResourcesDirty())
    {
        RayTracingState.CurrentDescriptorState->UpdateDescriptorSets(Context.GetTransientDescriptorAllocator());
        RayTracingState.CurrentDescriptorState->ClearResourcesDirty();
    }

    RayTracingState.CurrentDescriptorState->TransitionBoundResources(Context);
    Context.GetBarrierBatcher().FlushBarriers(Context.GetCommandBuffer());
}

void FVulkanCommandContextState::BindRayTracingState()
{
#if VK_KHR_ray_tracing_pipeline
    if (!RayTracingState.PipelineState)
    {
        return;
    }

    if (RayTracingState.bBindPipelineState || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->BindPipeline(VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, RayTracingState.PipelineState->GetVkPipeline());
        RayTracingState.bBindPipelineState = false;
    }

    if (RayTracingState.CurrentDescriptorState->IsDescriptorSetDirty() || GVulkanForceBinding)
    {
        RayTracingState.CurrentDescriptorState->BindRayTracingDescriptorSets(Context.GetCommandBuffer());
        RayTracingState.CurrentDescriptorState->ClearDescriptorSetDirty();
    }

    FVulkanPipelineLayout* PipelineLayout = RayTracingState.PipelineState->GetPipelineLayout();
    if (RayTracingState.bBindPushConstants || GVulkanForceBinding)
    {
        BindPushConstants(PipelineLayout, EPushConstantsPipeline::RayTracing);
        RayTracingState.bBindPushConstants = false;
    }
#endif
}

void FVulkanCommandContextState::EvictStaleDescriptorStates()
{
    constexpr uint64 StaleFrameThreshold = 8;
    
    CurrentFrame++;

    if (CurrentFrame <= StaleFrameThreshold)
    {
        return;
    }

    const uint64 EvictionCutoff = CurrentFrame - StaleFrameThreshold;
    int32 EvictedCount = 0;

    // Evict stale graphics descriptor states
    {
        TArray<FVulkanGraphicsPipelineStateRHI*> StaleKeys;
        GraphicsState.DescriptorStates.Foreach([&StaleKeys, EvictionCutoff](FVulkanGraphicsPipelineStateRHI* const& Key, const FCachedDescriptorState& Cached)
        {
            if (Cached.LastUsedFrame < EvictionCutoff)
            {
                StaleKeys.Add(Key);
            }
        });

        for (FVulkanGraphicsPipelineStateRHI* Key : StaleKeys)
        {
            FCachedDescriptorState Cached;
            if (GraphicsState.DescriptorStates.RemoveKey(Key, &Cached))
            {
                if (GraphicsState.CurrentDescriptorState == Cached.State)
                {
                    GraphicsState.CurrentDescriptorState = nullptr;
                }

                delete Cached.State;
                EvictedCount++;
            }
        }
    }

    // Evict stale compute descriptor states
    {
        TArray<FVulkanComputePipelineStateRHI*> StaleKeys;
        ComputeState.DescriptorStates.Foreach([&StaleKeys, EvictionCutoff](FVulkanComputePipelineStateRHI* const& Key, const FCachedDescriptorState& Cached)
        {
            if (Cached.LastUsedFrame < EvictionCutoff)
            {
                StaleKeys.Add(Key);
            }
        });

        for (FVulkanComputePipelineStateRHI* Key : StaleKeys)
        {
            FCachedDescriptorState Cached;
            if (ComputeState.DescriptorStates.RemoveKey(Key, &Cached))
            {
                if (ComputeState.CurrentDescriptorState == Cached.State)
                {
                    ComputeState.CurrentDescriptorState = nullptr;
                }

                delete Cached.State;
                EvictedCount++;
            }
        }
    }

    // Evict stale meshlet descriptor states
    {
        TArray<FVulkanMeshletPipelineStateRHI*> StaleKeys;
        MeshletState.DescriptorStates.Foreach([&StaleKeys, EvictionCutoff](FVulkanMeshletPipelineStateRHI* const& Key, const FCachedDescriptorState& Cached)
        {
            if (Cached.LastUsedFrame < EvictionCutoff)
            {
                StaleKeys.Add(Key);
            }
        });

        for (FVulkanMeshletPipelineStateRHI* Key : StaleKeys)
        {
            FCachedDescriptorState Cached;
            if (MeshletState.DescriptorStates.RemoveKey(Key, &Cached))
            {
                if (MeshletState.CurrentDescriptorState == Cached.State)
                {
                    MeshletState.CurrentDescriptorState = nullptr;
                }

                delete Cached.State;
                EvictedCount++;
            }
        }
    }

    // Evict stale ray tracing descriptor states
    {
        TArray<FVulkanRayTracingPipelineStateRHI*> StaleKeys;
        RayTracingState.DescriptorStates.Foreach([&StaleKeys, EvictionCutoff](FVulkanRayTracingPipelineStateRHI* const& Key, const FCachedDescriptorState& Cached)
        {
            if (Cached.LastUsedFrame < EvictionCutoff)
            {
                StaleKeys.Add(Key);
            }
        });

        for (FVulkanRayTracingPipelineStateRHI* Key : StaleKeys)
        {
            FCachedDescriptorState Cached;
            if (RayTracingState.DescriptorStates.RemoveKey(Key, &Cached))
            {
                if (RayTracingState.CurrentDescriptorState == Cached.State)
                {
                    RayTracingState.CurrentDescriptorState = nullptr;
                }

                delete Cached.State;
                EvictedCount++;
            }
        }
    }

    if (EvictedCount > 0)
    {
        VULKAN_INFO("Evicted %d stale descriptor state(s)", EvictedCount);
    }
}

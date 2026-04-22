#include "VulkanRHI/VulkanCommandContextState.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanResourceViews.h"

FVulkanCommandContextState::FVulkanCommandContextState(FVulkanDevice* InDevice, FVulkanCommandContext& InContext)
    : FVulkanDeviceChild(InDevice)
    , GraphicsState()
    , ComputeState()
    , CommonState()
    , Context(InContext)
    , CurrentFrame(0)
    , ContextPhase(ECommandContextPhase::Finished)
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

    ComputeState.DescriptorStates.Clear();
    GraphicsState.DescriptorStates.Clear();
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

    CHECK(GraphicsState.ViewInstancingState == GraphicsState.PipelineState->GetViewInstancingState());
    FVulkanPipelineLayout* PipelineLayout = GraphicsState.PipelineState->GetPipelineLayout();
    CHECK(PipelineLayout != nullptr);

    CHECK(PipelineLayout == GraphicsState.CurrentDescriptorState->GetLayout());
    if (GraphicsState.CurrentDescriptorState->IsResourcesDirty())
    {
        GraphicsState.CurrentDescriptorState->UpdateDescriptorSets(Context.GetTransientDescriptorAllocator());
        GraphicsState.CurrentDescriptorState->ClearResourcesDirty();
    }

    GraphicsState.CurrentDescriptorState->TransitionBoundResources(Context);

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
        BindPushConstants(PipelineLayout);
        GraphicsState.bBindPushConstants = false;
    }

    if (GraphicsState.bBindVertexBuffers || GVulkanForceBinding)
    {
        FVulkanVertexBufferCache& VertexBufferCache = GraphicsState.VertexBufferCache;
        Context.GetCommandBuffer()->BindVertexBuffers(0, VertexBufferCache.NumVertexBuffers, VertexBufferCache.VertexBuffers, VertexBufferCache.VertexBufferOffsets);
        GraphicsState.bBindVertexBuffers = false;
    }

    if (GraphicsState.bBindIndexBuffer || GVulkanForceBinding)
    {
        FVulkanIndexBufferCache& IndexBufferCache = GraphicsState.IndexBufferCache;
        Context.GetCommandBuffer()->BindIndexBuffer(IndexBufferCache.IndexBuffer, IndexBufferCache.Offset, IndexBufferCache.IndexType);
        GraphicsState.bBindIndexBuffer = false;
    }

    if (GraphicsState.bBindViewports || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetViewport(0, GraphicsState.NumViewports, GraphicsState.Viewports);
        GraphicsState.bBindViewports = false;
    }

    if (GraphicsState.bBindScissorRects || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetScissor(0, GraphicsState.NumScissorRects, GraphicsState.ScissorRects);
        GraphicsState.bBindScissorRects = false;
    }

    if (GraphicsState.bBindBlendFactor || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetBlendConstants(GraphicsState.BlendFactor);
        GraphicsState.bBindBlendFactor = false;
    }

    if (GraphicsState.bBindStencilRef || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetStencilReference(VK_STENCIL_FACE_FRONT_AND_BACK, GraphicsState.StencilRef);
        GraphicsState.bBindStencilRef = false;
    }

    if (GraphicsState.bBindDepthBias || GVulkanForceBinding)
    {
        Context.GetCommandBuffer()->SetDepthBias(GraphicsState.DepthBias[0], GraphicsState.DepthBias[1], GraphicsState.DepthBias[2]);
        GraphicsState.bBindDepthBias = false;
    }

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

    FVulkanPipelineLayout* PipelineLayout = ComputeState.PipelineState->GetPipelineLayout();
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
        BindPushConstants(PipelineLayout);
        ComputeState.bBindPushConstants = false;
    }
}

void FVulkanCommandContextState::BindPushConstants(FVulkanPipelineLayout* PipelineLayout)
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
            CommonState.PushConstantsCache.Constants);
    }
}

void FVulkanCommandContextState::ResetState()
{
    CommonState.PushConstantsCache.Clear();

    GraphicsState.VertexBufferCache.Clear();
    GraphicsState.IndexBufferCache.Clear();

    FMemory::Memzero(GraphicsState.BlendFactor, sizeof(GraphicsState.BlendFactor));
    FMemory::Memzero(GraphicsState.DepthBias, sizeof(GraphicsState.DepthBias));

    GraphicsState.StreamOutputCache.Clear();
    GraphicsState.StencilRef = 0;
    
    FMemory::Memzero(GraphicsState.Viewports, sizeof(GraphicsState.Viewports));
    GraphicsState.NumViewports = 0;

    FMemory::Memzero(GraphicsState.ScissorRects, sizeof(GraphicsState.ScissorRects));
    GraphicsState.NumScissorRects = 0;
    
    GraphicsState.PipelineState            = nullptr;
    GraphicsState.CurrentDescriptorState   = nullptr;
    GraphicsState.CurrentLayout            = nullptr;
    GraphicsState.bBindIndexBuffer         = true;
    GraphicsState.bBindBlendFactor         = true;
    GraphicsState.bBindStencilRef          = true;
    GraphicsState.bBindDepthBias           = true;
    GraphicsState.bBindPipelineState       = true;
    GraphicsState.bBindScissorRects        = true;
    GraphicsState.bBindViewports           = true;
    GraphicsState.bBindVertexBuffers       = true;
    GraphicsState.bBindPushConstants       = true;
    GraphicsState.bBindStreamOutputTargets = false;
    ComputeState.PipelineState             = nullptr;
    ComputeState.CurrentDescriptorState    = nullptr;
    ComputeState.CurrentLayout             = nullptr;
    ComputeState.bBindPipelineState        = true;
    ComputeState.bBindPushConstants        = true;
}

void FVulkanCommandContextState::ResetStateForNewCommandBuffer()
{
    GraphicsState.bBindIndexBuffer         = true;
    GraphicsState.bBindBlendFactor         = true;
    GraphicsState.bBindStencilRef          = true;
    GraphicsState.bBindDepthBias           = true;
    GraphicsState.bBindPipelineState       = true;
    GraphicsState.bBindScissorRects        = true;
    GraphicsState.bBindViewports           = true;
    GraphicsState.bBindVertexBuffers       = true;
    GraphicsState.bBindPushConstants       = true;
    GraphicsState.bBindStreamOutputTargets = (GraphicsState.StreamOutputCache.NumBuffers > 0);
    ComputeState.bBindPipelineState        = true;
    ComputeState.bBindPushConstants        = true;

    if (GraphicsState.CurrentDescriptorState)
    {
        GraphicsState.CurrentDescriptorState->DirtyDescriptorSet();
    }

    if (ComputeState.CurrentDescriptorState)
    {
        ComputeState.CurrentDescriptorState->DirtyDescriptorSet();
    }
}

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

        FVulkanTexture* Texture = static_cast<FVulkanTexture*>(View->GetOwnerResource());
        RenderPassKey.RenderTargetFormats[Index]             = Texture->GetInfo().Format;
        RenderPassKey.RenderTargetActions[Index].LoadAction  = EAttachmentLoadAction::Load;
        RenderPassKey.RenderTargetActions[Index].StoreAction = RenderTargetState.ColorStoreActions[Index];
        NumSamples = Math::Max<uint8>(static_cast<uint8>(Texture->GetNumSamples()), NumSamples);
    }

    if (FVulkanResourceView* DepthView = RenderTargetState.DepthStencilView)
    {
        FVulkanTexture* Texture = static_cast<FVulkanTexture*>(DepthView->GetOwnerResource());
        RenderPassKey.DepthStencilFormat              = Texture->GetInfo().Format;
        RenderPassKey.DepthStencilActions.LoadAction  = EAttachmentLoadAction::Load;
        RenderPassKey.DepthStencilActions.StoreAction = RenderTargetState.DepthStencilStoreAction;
        NumSamples = Math::Max<uint8>(static_cast<uint8>(Texture->GetNumSamples()), NumSamples);
    }

    RenderPassKey.NumSamples = NumSamples;

    if (GraphicsState.ViewInstancingState.bEnableViewInstancing)
    {
        RenderPassKey.ViewInstancingState = GraphicsState.ViewInstancingState;
    }

    return RenderPassKey;
}

void FVulkanCommandContextState::BeginRenderPass(const FRHIBeginRenderPassInfo& RenderPassInfo)
{
    CHECK(ContextPhase == ECommandContextPhase::Recording);

    GraphicsState.ViewInstancingState = RenderPassInfo.ViewInstancingState;

    FVulkanRenderTargetState& RenderTargetState = GraphicsState.RenderTargetState;
    RenderTargetState.Clear();
    RenderTargetState.NumRenderTargets = RenderPassInfo.NumRenderTargets;

    uint32 Width          = TNumericLimits<uint32>::Max();
    uint32 Height         = TNumericLimits<uint32>::Max();
    uint32 NumArrayLayers = 0;
    uint8  NumSamples     = 0;

    VkClearValue ColorClearValues[RHI_MAX_RENDER_TARGETS] = {};

    FVulkanRenderPassKey RenderPassKey;
    RenderPassKey.NumRenderTargets = static_cast<uint8>(RenderPassInfo.NumRenderTargets);

    for (uint32 Index = 0; Index < RenderPassInfo.NumRenderTargets; Index++)
    {
        const FRHIRenderTargetView& RenderTargetView = RenderPassInfo.RenderTargets[Index];
        FVulkanTexture* VulkanTexture = FVulkanTexture::Cast(&Context, RenderTargetView.Texture);
        if (!VulkanTexture)
        {
            continue;
        }

        Width          = Math::Min<uint32>(VulkanTexture->GetWidth(), Width);
        Height         = Math::Min<uint32>(VulkanTexture->GetHeight(), Height);
        NumArrayLayers = Math::Max<uint32>(RenderTargetView.NumArraySlices, NumArrayLayers);
        NumSamples     = Math::Max<uint8>(static_cast<uint8>(VulkanTexture->GetNumSamples()), NumSamples);

        FVulkanHashableImageView HashableImageView;
        HashableImageView.ArrayIndex     = RenderTargetView.ArrayIndex;
        HashableImageView.NumArraySlices = RenderTargetView.NumArraySlices;
        HashableImageView.Format         = RenderTargetView.Format;
        HashableImageView.MipLevel       = RenderTargetView.MipLevel;

        FVulkanResourceView* ImageView = VulkanTexture->GetOrCreateImageView(HashableImageView);
        RenderTargetState.RenderTargetViews[Index] = ImageView;
        RenderTargetState.ColorStoreActions[Index] = RenderTargetView.StoreAction;

        RenderPassKey.RenderTargetFormats[Index]             = RenderTargetView.Format;
        RenderPassKey.RenderTargetActions[Index].LoadAction  = RenderTargetView.LoadAction;
        RenderPassKey.RenderTargetActions[Index].StoreAction = RenderTargetView.StoreAction;

        FMemory::Memcpy(ColorClearValues[Index].color.float32, RenderTargetView.ClearValue.RGBA, sizeof(ColorClearValues[Index].color.float32));
    }

    VkClearValue DepthStencilClearValue = {};

    const FRHIDepthStencilView& DepthStencilView = RenderPassInfo.DepthStencilView;
    if (FVulkanTexture* VulkanTexture = FVulkanTexture::Cast(&Context, DepthStencilView.Texture))
    {
        Width          = Math::Min<uint32>(VulkanTexture->GetWidth(), Width);
        Height         = Math::Min<uint32>(VulkanTexture->GetHeight(), Height);
        NumArrayLayers = Math::Max<uint32>(DepthStencilView.NumArraySlices, NumArrayLayers);
        NumSamples     = Math::Max<uint8>(static_cast<uint8>(VulkanTexture->GetNumSamples()), NumSamples);

        FVulkanHashableImageView HashableImageView;
        HashableImageView.ArrayIndex     = DepthStencilView.ArrayIndex;
        HashableImageView.NumArraySlices = DepthStencilView.NumArraySlices;
        HashableImageView.Format         = DepthStencilView.Format;
        HashableImageView.MipLevel       = DepthStencilView.MipLevel;

        FVulkanResourceView* DepthImageView = VulkanTexture->GetOrCreateImageView(HashableImageView);
        RenderTargetState.DepthStencilView        = DepthImageView;
        RenderTargetState.DepthStencilStoreAction = DepthStencilView.StoreAction;

        RenderPassKey.DepthStencilFormat              = DepthStencilView.Format;
        RenderPassKey.DepthStencilActions.LoadAction  = DepthStencilView.LoadAction;
        RenderPassKey.DepthStencilActions.StoreAction = DepthStencilView.StoreAction;

        DepthStencilClearValue.depthStencil.depth   = DepthStencilView.ClearValue.Depth;
        DepthStencilClearValue.depthStencil.stencil = DepthStencilView.ClearValue.Stencil;
    }

    if (RenderPassInfo.ViewInstancingState.bEnableViewInstancing)
    {
        NumArrayLayers = 1;
    }

    RenderTargetState.RenderAreaWidth     = (Width  != TNumericLimits<uint32>::Max()) ? Width  : 0;
    RenderTargetState.RenderAreaHeight    = (Height != TNumericLimits<uint32>::Max()) ? Height : 0;
    RenderTargetState.RenderingLayerCount = Math::Max(NumArrayLayers, 1u);

    RenderPassKey.NumSamples = NumSamples;
    if (RenderPassInfo.ViewInstancingState.bEnableViewInstancing)
    {
        RenderPassKey.ViewInstancingState = RenderPassInfo.ViewInstancingState;
    }

    if (GVulkanSupportsMultiviews && RenderPassInfo.ViewInstancingState.bEnableViewInstancing)
    {
        constexpr uint32 MaxArraySlices = 32;
        const uint32 NumViews = Math::Min<uint32>(RenderPassInfo.ViewInstancingState.NumArraySlices, MaxArraySlices);
        uint32 ViewMask = 0;
        for (uint32 i = 0; i < NumViews; i++)
        {
            const uint32 BitIndex = RenderPassInfo.ViewInstancingState.StartRenderTargetArrayIndex + i;
            CHECK(BitIndex < 32);
            ViewMask |= (1u << BitIndex);
        }

        RenderTargetState.RenderingViewMask = ViewMask;
    }

    Context.GetBarrierBatcher().FlushBarriers(Context.GetCommandBuffer());

    if (GVulkanUseDynamicRendering)
    {
        VkRenderingAttachmentInfo ColorAttachments[RHI_MAX_RENDER_TARGETS] = {};
        for (uint32 i = 0; i < RenderTargetState.NumRenderTargets; i++)
        {
            const FRHIRenderTargetView& RenderTargetView = RenderPassInfo.RenderTargets[i];
            ColorAttachments[i].sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            ColorAttachments[i].imageView   = RenderTargetState.RenderTargetViews[i] ? RenderTargetState.RenderTargetViews[i]->GetImageViewInfo().ImageView : VK_NULL_HANDLE;
            ColorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ColorAttachments[i].loadOp      = ConvertLoadAction(RenderTargetView.LoadAction);
            ColorAttachments[i].storeOp     = ConvertStoreAction(RenderTargetView.StoreAction);
            ColorAttachments[i].clearValue  = ColorClearValues[i];
        }

        VkRenderingAttachmentInfo DepthStencilAttachment = {};
        const bool bHasDepthStencil = (RenderTargetState.DepthStencilView != nullptr);
        bool bHasStencil = false;
        if (bHasDepthStencil)
        {
            const VkFormat DepthStencilVkFormat = RenderTargetState.DepthStencilView->GetImageViewInfo().Format;
            bHasStencil = IsStencilFormat(DepthStencilVkFormat);

            DepthStencilAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            DepthStencilAttachment.imageView   = RenderTargetState.DepthStencilView->GetImageViewInfo().ImageView;
            DepthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            DepthStencilAttachment.loadOp      = ConvertLoadAction(DepthStencilView.LoadAction);
            DepthStencilAttachment.storeOp     = ConvertStoreAction(DepthStencilView.StoreAction);
            DepthStencilAttachment.clearValue  = DepthStencilClearValue;
        }

        VkRenderingInfo RenderingInfo = {};
        RenderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        RenderingInfo.renderArea           = { {0, 0}, {RenderTargetState.RenderAreaWidth, RenderTargetState.RenderAreaHeight} };
        RenderingInfo.layerCount           = RenderTargetState.RenderingLayerCount;
        RenderingInfo.colorAttachmentCount = RenderTargetState.NumRenderTargets;
        RenderingInfo.pColorAttachments    = ColorAttachments;
        RenderingInfo.pDepthAttachment     = bHasDepthStencil ? &DepthStencilAttachment : nullptr;
        RenderingInfo.pStencilAttachment   = (bHasDepthStencil && bHasStencil) ? &DepthStencilAttachment : nullptr;
        RenderingInfo.viewMask             = RenderTargetState.RenderingViewMask;

        Context.GetCommandBuffer()->BeginRendering(&RenderingInfo);
    }
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

        FMemory::Memcpy(FramebufferKey.AttachmentViews, AttachmentViews, sizeof(FramebufferKey.AttachmentViews));

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

    ContextPhase = ECommandContextPhase::InsideRenderPass;
}

void FVulkanCommandContextState::EndRenderPass()
{
    CHECK(IsInsideRenderPass());

    if (GVulkanUseDynamicRendering)
    {
        Context.GetCommandBuffer()->EndRendering();
    }
    else
    {
        Context.GetCommandBuffer()->EndRenderPass();
    }

    ContextPhase = ECommandContextPhase::Recording;
}

void FVulkanCommandContextState::PauseRenderPass()
{
    CHECK(IsInsideRenderPass());

    if (GVulkanUseDynamicRendering)
    {
        Context.GetCommandBuffer()->EndRendering();
    }
    else
    {
        Context.GetCommandBuffer()->EndRenderPass();
    }

    ContextPhase = ECommandContextPhase::RenderPassPaused;
}

void FVulkanCommandContextState::ResumeRenderPass()
{
    CHECK(IsRenderPassPaused());

    Context.GetBarrierBatcher().FlushBarriers(Context.GetCommandBuffer());

    const FVulkanRenderTargetState& RenderTargetState = GraphicsState.RenderTargetState;
    if (GVulkanUseDynamicRendering)
    {
        VkRenderingAttachmentInfo ColorAttachments[RHI_MAX_RENDER_TARGETS] = {};
        for (uint32 i = 0; i < RenderTargetState.NumRenderTargets; i++)
        {
            ColorAttachments[i].sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            ColorAttachments[i].imageView   = RenderTargetState.RenderTargetViews[i] ? RenderTargetState.RenderTargetViews[i]->GetImageViewInfo().ImageView : VK_NULL_HANDLE;
            ColorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ColorAttachments[i].loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
            ColorAttachments[i].storeOp     = ConvertStoreAction(RenderTargetState.ColorStoreActions[i]);
        }

        VkRenderingAttachmentInfo DepthStencilAttachment = {};
        const bool bHasDepthStencil = (RenderTargetState.DepthStencilView != nullptr);
        bool bHasStencil = false;
        if (bHasDepthStencil)
        {
            const VkFormat DepthStencilVkFormat = RenderTargetState.DepthStencilView->GetImageViewInfo().Format;
            bHasStencil = IsStencilFormat(DepthStencilVkFormat);

            DepthStencilAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            DepthStencilAttachment.imageView   = RenderTargetState.DepthStencilView->GetImageViewInfo().ImageView;
            DepthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            DepthStencilAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
            DepthStencilAttachment.storeOp     = ConvertStoreAction(RenderTargetState.DepthStencilStoreAction);
        }

        VkRenderingInfo RenderingInfo = {};
        RenderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        RenderingInfo.renderArea           = { {0, 0}, {RenderTargetState.RenderAreaWidth, RenderTargetState.RenderAreaHeight} };
        RenderingInfo.layerCount           = RenderTargetState.RenderingLayerCount;
        RenderingInfo.colorAttachmentCount = RenderTargetState.NumRenderTargets;
        RenderingInfo.pColorAttachments    = ColorAttachments;
        RenderingInfo.pDepthAttachment     = bHasDepthStencil ? &DepthStencilAttachment : nullptr;
        RenderingInfo.pStencilAttachment   = (bHasDepthStencil && bHasStencil) ? &DepthStencilAttachment : nullptr;
        RenderingInfo.viewMask             = RenderTargetState.RenderingViewMask;

        Context.GetCommandBuffer()->BeginRendering(&RenderingInfo);
    }
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

    ContextPhase = ECommandContextPhase::InsideRenderPass;
}

void FVulkanCommandContextState::SetGraphicsPipelineState(FVulkanGraphicsPipelineState* InGraphicsPipelineState)
{
    FVulkanGraphicsPipelineState* CurrentGraphicsPipelineState = GraphicsState.PipelineState.Get();
    if (CurrentGraphicsPipelineState != InGraphicsPipelineState || GVulkanForceBinding)
    {
        GraphicsState.PipelineState      = MakeSharedRef<FVulkanGraphicsPipelineState>(InGraphicsPipelineState);
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

        GraphicsState.DepthBias[0]   = 0.0f;
        GraphicsState.DepthBias[1]   = 0.0f;
        GraphicsState.DepthBias[2]   = 0.0f;
        GraphicsState.bBindDepthBias = true;
    }
}

void FVulkanCommandContextState::SetComputePipelineState(FVulkanComputePipelineState* InComputePipelineState)
{
    FVulkanComputePipelineState* CurrentComputePipelineState = ComputeState.PipelineState.Get();
    if (CurrentComputePipelineState != InComputePipelineState || GVulkanForceBinding)
    {
        ComputeState.PipelineState      = MakeSharedRef<FVulkanComputePipelineState>(InComputePipelineState);
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

void FVulkanCommandContextState::SetViewports(VkViewport* Viewports, uint32 NumViewports)
{
    CHECK(NumViewports < VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT);

    const uint32 ViewportArraySize = sizeof(VkViewport) * NumViewports;
    if (GraphicsState.NumViewports != NumViewports || FMemory::Memcmp(GraphicsState.Viewports, Viewports, ViewportArraySize) != 0 || GVulkanForceBinding)
    {
        FMemory::Memcpy(GraphicsState.Viewports, Viewports, ViewportArraySize);

        GraphicsState.NumViewports   = NumViewports;
        GraphicsState.bBindViewports = true;
    }
}

void FVulkanCommandContextState::SetScissorRects(VkRect2D* ScissorRects, uint32 NumScissorRects)
{
    CHECK(NumScissorRects < VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT);

    const uint32 ScissorRectArraySize = sizeof(VkRect2D) * NumScissorRects;
    if (GraphicsState.NumScissorRects != NumScissorRects || FMemory::Memcmp(GraphicsState.ScissorRects, ScissorRects, ScissorRectArraySize) != 0 || GVulkanForceBinding)
    {
        FMemory::Memcpy(GraphicsState.ScissorRects, ScissorRects, ScissorRectArraySize);

        GraphicsState.NumScissorRects   = NumScissorRects;
        GraphicsState.bBindScissorRects = true;
    }
}

void FVulkanCommandContextState::SetBlendFactor(const float BlendFactor[4])
{
    if (FMemory::Memcmp(GraphicsState.BlendFactor, BlendFactor, sizeof(GraphicsState.BlendFactor)) != 0 || GVulkanForceBinding)
    {
        FMemory::Memcpy(GraphicsState.BlendFactor, BlendFactor, sizeof(GraphicsState.BlendFactor));
        GraphicsState.bBindBlendFactor = true;
    }
}

void FVulkanCommandContextState::SetStencilRef(uint32 InStencilRef)
{
    if (GraphicsState.StencilRef != InStencilRef || GVulkanForceBinding)
    {
        GraphicsState.StencilRef      = InStencilRef;
        GraphicsState.bBindStencilRef = true;
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

    if (FMemory::Memcmp(GraphicsState.DepthBias, NewValues, sizeof(NewValues)) != 0 || GVulkanForceBinding)
    {
        FMemory::Memcpy(GraphicsState.DepthBias, NewValues, sizeof(NewValues));
        GraphicsState.bBindDepthBias = true;
    }
}

void FVulkanCommandContextState::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
    GraphicsState.StreamOutputCache.NumBuffers = Math::Min(static_cast<uint32>(Buffers.Size()), static_cast<uint32>(VULKAN_MAX_STREAM_OUTPUT_BUFFER_COUNT));
    for (uint32 Index = 0; Index < GraphicsState.StreamOutputCache.NumBuffers; ++Index)
    {
        FVulkanBuffer* VulkanBuffer = static_cast<FVulkanBuffer*>(Buffers[Index]);
        if (VulkanBuffer)
        {
            GraphicsState.StreamOutputCache.Buffers[Index] = VulkanBuffer->GetVkBuffer();
            GraphicsState.StreamOutputCache.Offsets[Index] = Offsets ? Offsets[Index] : 0;
            GraphicsState.StreamOutputCache.Sizes[Index]   = VulkanBuffer->GetInfo().Size;
        }
        else
        {
            GraphicsState.StreamOutputCache.Buffers[Index] = VK_NULL_HANDLE;
            GraphicsState.StreamOutputCache.Offsets[Index] = 0;
            GraphicsState.StreamOutputCache.Sizes[Index]   = 0;
        }
    }

    GraphicsState.bBindStreamOutputTargets = true;
}

void FVulkanCommandContextState::SetVertexBuffer(FVulkanBuffer* VertexBuffer, uint32 VertexBufferSlot)
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

    if (Buffer != CurrentBuffer || Offset != CurrentOffset || GVulkanForceBinding)
    {
        GraphicsState.VertexBufferCache.VertexBuffers[VertexBufferSlot]       = Buffer;
        GraphicsState.VertexBufferCache.VertexBufferOffsets[VertexBufferSlot] = Offset;
        GraphicsState.VertexBufferCache.NumVertexBuffers                      = Math::Max(GraphicsState.VertexBufferCache.NumVertexBuffers, VertexBufferSlot + 1);
        GraphicsState.bBindVertexBuffers                                      = true;
    }
}

void FVulkanCommandContextState::SetIndexBuffer(FVulkanBuffer* IndexBuffer, VkIndexType IndexType)
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

    if (Buffer != CurrentBuffer || Offset != CurrentOffset || IndexType != CurrentIndexType || GVulkanForceBinding)
    {
        GraphicsState.IndexBufferCache.IndexBuffer = Buffer;
        GraphicsState.IndexBufferCache.Offset      = Offset;
        GraphicsState.IndexBufferCache.IndexType   = IndexType;
        GraphicsState.bBindIndexBuffer             = true;
    }
}

void FVulkanCommandContextState::SetPushConstants(const uint32* ShaderConstants, uint32 NumShaderConstants)
{
    FVulkanPushConstantsCache& ConstantCache = CommonState.PushConstantsCache;
    if (NumShaderConstants != ConstantCache.NumConstants || FMemory::Memcmp(ShaderConstants, ConstantCache.Constants, sizeof(uint32) * NumShaderConstants) != 0 || GVulkanForceBinding)
    {
        FMemory::Memcpy(ConstantCache.Constants, ShaderConstants, sizeof(uint32) * NumShaderConstants);

        ConstantCache.NumConstants       = NumShaderConstants;
        GraphicsState.bBindPushConstants = true;
        ComputeState.bBindPushConstants  = true;
    }
}

void FVulkanCommandContextState::SetSRV(FVulkanShaderResourceView* ShaderResourceView, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    CHECK(ResourceIndex < VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);
    
    FVulkanPipelineLayout*  Layout          = nullptr;
    FVulkanDescriptorState* DescriptorState = nullptr;

    if (ShaderStage == ShaderVisibility_Compute)
    {
        Layout          = ComputeState.CurrentLayout;
        DescriptorState = ComputeState.CurrentDescriptorState;
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

    if (!Layout->GetDescriptorBinding(ShaderStage, ResourceType_SRV, ResourceIndex, DescriptorSetIndex, BindingIndex))
    {
    #if VULKAN_ENABLE_BINDING_VALIDATION
        VULKAN_WARNING("SetShaderResourceView: Slot %u does not exist in %s shader", ResourceIndex, ToString(ShaderStage));
    #endif
        return;
    }
    
    DescriptorState->SetSRV(ShaderResourceView, DescriptorSetIndex, BindingIndex);
}

void FVulkanCommandContextState::SetUAV(FVulkanUnorderedAccessView* UnorderedAccessView, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    CHECK(ResourceIndex < VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);

    FVulkanPipelineLayout*  Layout          = nullptr;
    FVulkanDescriptorState* DescriptorState = nullptr;

    if (ShaderStage == ShaderVisibility_Compute)
    {
        Layout          = ComputeState.CurrentLayout;
        DescriptorState = ComputeState.CurrentDescriptorState;
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

    if (!Layout->GetDescriptorBinding(ShaderStage, ResourceType_UAV, ResourceIndex, DescriptorSetIndex, BindingIndex))
    {
    #if VULKAN_ENABLE_BINDING_VALIDATION
        VULKAN_WARNING("SetUnorderedAccessView: Slot %u does not exist in %s shader", ResourceIndex, ToString(ShaderStage));
    #endif
        return;
    }
    
    DescriptorState->SetUAV(UnorderedAccessView, DescriptorSetIndex, BindingIndex);
}

void FVulkanCommandContextState::SetUniformBuffer(FVulkanBuffer* UniformBuffer, EShaderVisibility ShaderStage, uint32 ResourceIndex)
{
    CHECK(ResourceIndex < VULKAN_DEFAULT_UNIFORM_BUFFER_COUNT);
    
    FVulkanPipelineLayout*  Layout          = nullptr;
    FVulkanDescriptorState* DescriptorState = nullptr;

    if (ShaderStage == ShaderVisibility_Compute)
    {
        Layout          = ComputeState.CurrentLayout;
        DescriptorState = ComputeState.CurrentDescriptorState;
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

    if (!Layout->GetDescriptorBinding(ShaderStage, ResourceType_UniformBuffer, ResourceIndex, DescriptorSetIndex, BindingIndex))
    {
    #if VULKAN_ENABLE_BINDING_VALIDATION
        VULKAN_WARNING("SetConstantBuffer: Slot %u does not exist in %s shader", ResourceIndex, ToString(ShaderStage));
    #endif
        return;
    }
    
    DescriptorState->SetUniformBuffer(UniformBuffer, DescriptorSetIndex, BindingIndex);
}

void FVulkanCommandContextState::SetSampler(FVulkanSamplerState* SamplerState, EShaderVisibility ShaderStage, uint32 SamplerIndex)
{
    CHECK(SamplerIndex < VULKAN_DEFAULT_SAMPLER_STATE_COUNT);

    FVulkanPipelineLayout*  Layout          = nullptr;
    FVulkanDescriptorState* DescriptorState = nullptr;

    if (ShaderStage == ShaderVisibility_Compute)
    {
        Layout          = ComputeState.CurrentLayout;
        DescriptorState = ComputeState.CurrentDescriptorState;
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

    if (!Layout->GetDescriptorBinding(ShaderStage, ResourceType_Sampler, SamplerIndex, DescriptorSetIndex, BindingIndex))
    {
    #if VULKAN_ENABLE_BINDING_VALIDATION
        VULKAN_WARNING("SetSamplerState: Slot %u does not exist in %s shader", SamplerIndex, ToString(ShaderStage));
    #endif
        return;
    }
    
    DescriptorState->SetSampler(SamplerState, DescriptorSetIndex, BindingIndex);
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
        TArray<FVulkanGraphicsPipelineState*> StaleKeys;
        GraphicsState.DescriptorStates.Foreach([&StaleKeys, EvictionCutoff](FVulkanGraphicsPipelineState* const& Key, const FCachedDescriptorState& Cached)
        {
            if (Cached.LastUsedFrame < EvictionCutoff)
            {
                StaleKeys.Add(Key);
            }
        });

        for (FVulkanGraphicsPipelineState* Key : StaleKeys)
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
        TArray<FVulkanComputePipelineState*> StaleKeys;
        ComputeState.DescriptorStates.Foreach([&StaleKeys, EvictionCutoff](FVulkanComputePipelineState* const& Key, const FCachedDescriptorState& Cached)
        {
            if (Cached.LastUsedFrame < EvictionCutoff)
            {
                StaleKeys.Add(Key);
            }
        });

        for (FVulkanComputePipelineState* Key : StaleKeys)
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

    if (EvictedCount > 0)
    {
        VULKAN_INFO("Evicted %d stale descriptor state(s)", EvictedCount);
    }
}

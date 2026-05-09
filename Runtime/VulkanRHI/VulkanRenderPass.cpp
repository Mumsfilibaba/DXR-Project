#include "VulkanRHI/VulkanRenderPass.h"
#include "VulkanRHI/VulkanDevice.h"

#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH

static void DestroyFramebuffer(VkDevice Device, VkFramebuffer Framebuffer)
{
    if (VULKAN_CHECK_HANDLE(Framebuffer))
    {
        vkDestroyFramebuffer(Device, Framebuffer, nullptr);
    }
    else
    {
        DEBUG_BREAK();
    }
}

FVulkanRenderPassCache::FVulkanRenderPassCache(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , RenderPasses()
    , Framebuffers()
    , CurrentFrame(0)
{
}

FVulkanRenderPassCache::~FVulkanRenderPassCache()
{
    {
        SCOPED_LOCK(FramebuffersCS);

        for (auto Entry : Framebuffers)
        {
            DestroyFramebuffer(GetDevice()->GetVkDevice(), Entry.Second.Handle);
        }

        Framebuffers.Clear();
    }

    {
        SCOPED_LOCK(RenderPassesCS);

        // Destroy all RenderPasses
        for (auto RenderPass : RenderPasses)
        {
            if (VULKAN_CHECK_HANDLE(RenderPass.Second))
            {
                vkDestroyRenderPass(GetDevice()->GetVkDevice(), RenderPass.Second, nullptr);
            }
        }

        // Clear all
        RenderPasses.Clear();
    }
}

VkRenderPass FVulkanRenderPassCache::GetRenderPass(const FVulkanRenderPassKey& Key)
{
    SCOPED_LOCK(RenderPassesCS);

    // Check if the RenderPass exists
    if (VkRenderPass* ExistingPass = RenderPasses.Find(Key))
    {
        return *ExistingPass;
    }

    // Setup ColorAttachments
    TArray<VkAttachmentReference>   ColorAttachents;
    TArray<VkAttachmentDescription> Attachments;

    // Number of samples (MSAA)
    const VkSampleCountFlagBits SampleCount = ConvertSampleCount(Key.NumSamples);
    if (SampleCount < VK_SAMPLE_COUNT_1_BIT)
    {
        VULKAN_ERROR_CRITICAL("Invalid SampleCount");
        return VK_NULL_HANDLE;
    }

    // Setup ColorAttachments
    for (uint8 Index = 0; Index < Key.NumRenderTargets; Index++)
    {
        // Setup Attachments
        VkAttachmentDescription ColorAttachment = {};
        ColorAttachment.format         = ConvertFormat(Key.RenderTargetFormats[Index]);
        ColorAttachment.samples        = SampleCount;
        ColorAttachment.loadOp         = ConvertLoadAction(Key.RenderTargetActions[Index].LoadAction);
        ColorAttachment.storeOp        = ConvertStoreAction(Key.RenderTargetActions[Index].StoreAction);
        ColorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ColorAttachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ColorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        Attachments.Add(ColorAttachment);

        VkAttachmentReference ColorAttachmentRef = {};
        ColorAttachmentRef.attachment = Index;
        ColorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ColorAttachents.Add(ColorAttachmentRef);
    }

    // Setup Subpass
    VkSubpassDescription Subpass = {};
    Subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpass.colorAttachmentCount = ColorAttachents.Size();
    Subpass.pColorAttachments    = ColorAttachents.Data();

    // Setup DepthStencil
    VkAttachmentReference DepthAttachmentRef = {};
    if (Key.DepthStencilFormat != EFormat::Unknown)
    {
        const bool bIsReadOnlyDepth   = IsEnumFlagSet(Key.DepthStencilFlags, EDepthStencilViewFlags::ReadOnlyDepth);
        const bool bIsReadOnlyStencil = IsEnumFlagSet(Key.DepthStencilFlags, EDepthStencilViewFlags::ReadOnlyStencil);

        VkImageLayout DepthStencilLayout;
        if (bIsReadOnlyDepth && bIsReadOnlyStencil)
        {
            DepthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        }
        else if (bIsReadOnlyDepth)
        {
            DepthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        }
        else if (bIsReadOnlyStencil)
        {
            DepthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
        }
        else
        {
            DepthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentDescription DepthAttachment = {};
        DepthAttachment.format         = ConvertFormat(Key.DepthStencilFormat);
        DepthAttachment.samples        = SampleCount;
        DepthAttachment.loadOp         = ConvertLoadAction(Key.DepthStencilActions.LoadAction);
        DepthAttachment.storeOp        = ConvertStoreAction(Key.DepthStencilActions.StoreAction);
        DepthAttachment.stencilLoadOp  = DepthAttachment.loadOp;
        DepthAttachment.stencilStoreOp = DepthAttachment.storeOp;
        DepthAttachment.initialLayout  = DepthStencilLayout;
        DepthAttachment.finalLayout    = DepthStencilLayout;

        DepthAttachmentRef.attachment = Attachments.Size();
        DepthAttachmentRef.layout     = DepthStencilLayout;
        Attachments.Add(DepthAttachment);
        
        Subpass.pDepthStencilAttachment = &DepthAttachmentRef;
    }

    // Create RenderPass
    VkRenderPassCreateInfo RenderPassCreateInfo = {};
    RenderPassCreateInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassCreateInfo.attachmentCount = Attachments.Size();
    RenderPassCreateInfo.pAttachments    = Attachments.Data();
    RenderPassCreateInfo.subpassCount    = 1;
    RenderPassCreateInfo.pSubpasses      = &Subpass;

    // Multi-view extension
    uint32 ViewMask;
    uint32 CorrelationMask;

    VkRenderPassMultiviewCreateInfo MultiviewCreateInfo;
    if (GVulkanSupportsMultiviews && Key.ViewInstancingState.bEnableViewInstancing)
    {
        constexpr uint32 MaxArraySlices = 32;
        ViewMask = 0;
        CorrelationMask = 0;

        // Limit to the number of bits in a uint32
        const uint32 NumViews = Math::Min<uint32>(Key.ViewInstancingState.NumArraySlices, MaxArraySlices);
        for (uint32 Index = 0; Index < NumViews; Index++)
        {
		    const uint32 BitIndex = Key.ViewInstancingState.StartRenderTargetArrayIndex + Index;
		    CHECK(BitIndex < 32);
		    ViewMask |= (1u << BitIndex);
	    }

        CorrelationMask = ViewMask;

        FMemory::Memzero(&MultiviewCreateInfo);
        MultiviewCreateInfo.sType                = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
        MultiviewCreateInfo.subpassCount         = 1;
        MultiviewCreateInfo.pViewMasks           = &ViewMask;
        MultiviewCreateInfo.correlationMaskCount = 1;
        MultiviewCreateInfo.pCorrelationMasks    = &CorrelationMask;

        AddToStructChain(RenderPassCreateInfo, MultiviewCreateInfo);
    }

    // Create the RenderPass
    VkRenderPass RenderPass = VK_NULL_HANDLE;
    VkResult Result = vkCreateRenderPass(GetDevice()->GetVkDevice(), &RenderPassCreateInfo, nullptr, &RenderPass);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create renderpass");
        return VK_NULL_HANDLE;
    }
    else
    {
        VULKAN_INFO("Created new renderpass");
        RenderPasses.Add(Key, RenderPass);
        return RenderPass;
    }
}

VkFramebuffer FVulkanRenderPassCache::GetFramebuffer(const FVulkanFramebufferKey& FrameBufferKey)
{
    SCOPED_LOCK(FramebuffersCS);

    if (FCachedFramebuffer* Existing = Framebuffers.Find(FrameBufferKey))
    {
        Existing->LastUsedFrame = CurrentFrame;
        return Existing->Handle;
    }

    VkFramebufferCreateInfo FramebufferCreateInfo = {};
    FramebufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCreateInfo.renderPass      = FrameBufferKey.RenderPass;
    FramebufferCreateInfo.attachmentCount = FrameBufferKey.NumAttachmentViews;
    FramebufferCreateInfo.pAttachments    = FrameBufferKey.AttachmentViews;
    FramebufferCreateInfo.width           = FrameBufferKey.Width;
    FramebufferCreateInfo.height          = FrameBufferKey.Height;
    FramebufferCreateInfo.layers          = FrameBufferKey.NumArrayLayers;

    VkFramebuffer Framebuffer = VK_NULL_HANDLE;
    VkResult Result = vkCreateFramebuffer(GetDevice()->GetVkDevice(), &FramebufferCreateInfo, nullptr, &Framebuffer);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Framebuffer");
        return VK_NULL_HANDLE;
    }
    else
    {
        VULKAN_INFO("Created new Framebuffer");

        FCachedFramebuffer Cached;
        Cached.Handle        = Framebuffer;
        Cached.LastUsedFrame = CurrentFrame;
        Framebuffers.Add(FrameBufferKey, Cached);
        return Framebuffer;
    }
}

void FVulkanRenderPassCache::OnReleaseImageView(VkImageView View)
{
    SCOPED_LOCK(FramebuffersCS);

    TArray<FVulkanFramebufferKey> Keys = Framebuffers.GetKeys();
    for (const FVulkanFramebufferKey& Key : Keys)
    {
        if (Key.ContainsImageView(View))
        {
            if (FCachedFramebuffer* Cached = Framebuffers.Find(Key))
            {
                DestroyFramebuffer(GetDevice()->GetVkDevice(), Cached->Handle);
            }
            
            Framebuffers.Remove(Key);
        }
    }
}

void FVulkanRenderPassCache::OnReleaseRenderPass(VkRenderPass RenderPass)
{
    SCOPED_LOCK(FramebuffersCS);

    TArray<FVulkanFramebufferKey> Keys = Framebuffers.GetKeys();
    for (const FVulkanFramebufferKey& Key : Keys)
    {
        if (Key.ContainsRenderPass(RenderPass))
        {
            if (FCachedFramebuffer* Cached = Framebuffers.Find(Key))
            {
                DestroyFramebuffer(GetDevice()->GetVkDevice(), Cached->Handle);
            }
            
            Framebuffers.Remove(Key);
        }
    }
}

void FVulkanRenderPassCache::EvictStaleFramebuffers()
{
    constexpr uint64 StaleFrameThreshold = 8;

    SCOPED_LOCK(FramebuffersCS);

    CurrentFrame++;

    if (CurrentFrame <= StaleFrameThreshold)
    {
        return;
    }

    const uint64 EvictionCutoff = CurrentFrame - StaleFrameThreshold;

    TArray<FVulkanFramebufferKey> StaleKeys;
    Framebuffers.Foreach([&StaleKeys, EvictionCutoff](const FVulkanFramebufferKey& Key, const FCachedFramebuffer& Cached)
    {
        if (Cached.LastUsedFrame < EvictionCutoff)
        {
            StaleKeys.Add(Key);
        }
    });

    for (const FVulkanFramebufferKey& Key : StaleKeys)
    {
        if (FCachedFramebuffer* Cached = Framebuffers.Find(Key))
        {
            DestroyFramebuffer(GetDevice()->GetVkDevice(), Cached->Handle);
        }

        Framebuffers.Remove(Key);
    }

    if (StaleKeys.Size() > 0)
    {
        VULKAN_INFO("Evicted %d stale framebuffer(s)", StaleKeys.Size());
    }
}

#endif // VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH

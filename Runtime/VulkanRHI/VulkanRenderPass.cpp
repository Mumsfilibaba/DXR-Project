#include "VulkanRenderPass.h"
#include "VulkanDevice.h"

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
{
}

FVulkanRenderPassCache::~FVulkanRenderPassCache()
{
    ReleaseAll();
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
        VULKAN_ERROR("Invalid SampleCount");
        return VK_NULL_HANDLE;
    }

    // Setup ColorAttachments
    for (uint8 Index = 0; Index < Key.NumRenderTargets; Index++)
    {
        // Setup Attachments
        VkAttachmentDescription ColorAttachment;
        FMemory::Memzero(&ColorAttachment);

        ColorAttachment.format         = ConvertFormat(Key.RenderTargetFormats[Index]);
        ColorAttachment.samples        = SampleCount;
        ColorAttachment.loadOp         = ConvertLoadAction(Key.RenderTargetActions[Index].LoadAction);
        ColorAttachment.storeOp        = ConvertStoreAction(Key.RenderTargetActions[Index].StoreAction);
        ColorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ColorAttachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ColorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        Attachments.Add(ColorAttachment);

        VkAttachmentReference ColorAttachmentRef;
        FMemory::Memzero(&ColorAttachmentRef);

        ColorAttachmentRef.attachment = Index;
        ColorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ColorAttachents.Add(ColorAttachmentRef);
    }

    // Setup Subpass
    VkSubpassDescription Subpass;
    FMemory::Memzero(&Subpass);

    Subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpass.colorAttachmentCount = ColorAttachents.Size();
    Subpass.pColorAttachments    = ColorAttachents.Data();

    // Setup DepthStencil
    VkAttachmentReference DepthAttachmentRef;
    FMemory::Memzero(&DepthAttachmentRef);

    if (Key.DepthStencilFormat != EFormat::Unknown)
    {
        VkAttachmentDescription DepthAttachment;
        FMemory::Memzero(&DepthAttachment);

        DepthAttachment.format         = ConvertFormat(Key.DepthStencilFormat);
        DepthAttachment.samples        = SampleCount;
        DepthAttachment.loadOp         = ConvertLoadAction(Key.DepthStencilActions.LoadAction);
        DepthAttachment.storeOp        = ConvertStoreAction(Key.DepthStencilActions.StoreAction);
        DepthAttachment.stencilLoadOp  = DepthAttachment.loadOp;
        DepthAttachment.stencilStoreOp = DepthAttachment.storeOp;
        DepthAttachment.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        DepthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        DepthAttachmentRef.attachment = Attachments.Size();
        DepthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        Attachments.Add(DepthAttachment);
        
        Subpass.pDepthStencilAttachment = &DepthAttachmentRef;
    }

    // Create RenderPass
    VkRenderPassCreateInfo RenderPassCreateInfo;
    FMemory::Memzero(&RenderPassCreateInfo);
    
    RenderPassCreateInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassCreateInfo.attachmentCount = Attachments.Size();
    RenderPassCreateInfo.pAttachments    = Attachments.Data();
    RenderPassCreateInfo.subpassCount    = 1;
    RenderPassCreateInfo.pSubpasses      = &Subpass;

    VkRenderPass RenderPass = VK_NULL_HANDLE;
    VkResult Result = vkCreateRenderPass(GetDevice()->GetVkDevice(), &RenderPassCreateInfo, nullptr, &RenderPass);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create renderpass");
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

    // Check if a Framebuffer exists
    if (VkFramebuffer* ExistingFramebuffer = Framebuffers.Find(FrameBufferKey))
    {
        return *ExistingFramebuffer;
    }

    //Create new Framebuffer
    VkFramebufferCreateInfo FramebufferCreateInfo;
    FMemory::Memzero(&FramebufferCreateInfo);
    
    FramebufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCreateInfo.renderPass      = FrameBufferKey.RenderPass;
    FramebufferCreateInfo.attachmentCount = FrameBufferKey.NumAttachmentViews;
    FramebufferCreateInfo.pAttachments    = FrameBufferKey.AttachmentViews;
    FramebufferCreateInfo.width           = FrameBufferKey.Width;
    FramebufferCreateInfo.height          = FrameBufferKey.Height;
    FramebufferCreateInfo.layers          = 1;

    VkFramebuffer Framebuffer = VK_NULL_HANDLE;
    VkResult Result = vkCreateFramebuffer(GetDevice()->GetVkDevice(), &FramebufferCreateInfo, nullptr, &Framebuffer);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Framebuffer");
        return VK_NULL_HANDLE;
    }
    else
    {
        VULKAN_INFO("Created new Framebuffer");
        Framebuffers.Add(FrameBufferKey, Framebuffer);
        return Framebuffer;
    }
}

void FVulkanRenderPassCache::OnReleaseImageView(VkImageView View)
{
    SCOPED_LOCK(FramebuffersCS);
    
    // TODO: Iterate with an iterator instead
    TArray<FVulkanFramebufferKey> Keys = Framebuffers.GetKeys();
    
    // Find all framebuffers containing this renderpass
    for (const FVulkanFramebufferKey& Key : Keys)
    {
        if (Key.ContainsImageView(View))
        {
            // Destroy Framebuffer
            if (VkFramebuffer* Framebuffer = Framebuffers.Find(Key))
            {
                DestroyFramebuffer(GetDevice()->GetVkDevice(), *Framebuffer);
            }
            
            Framebuffers.Remove(Key);
        }
    }
}

void FVulkanRenderPassCache::OnReleaseRenderPass(VkRenderPass RenderPass)
{
    SCOPED_LOCK(FramebuffersCS);

    // TODO: Iterate with an iterator instead
    TArray<FVulkanFramebufferKey> Keys = Framebuffers.GetKeys();
    
    // Find all framebuffers containing this renderpass
    for (const FVulkanFramebufferKey& Key : Keys)
    {
        if (Key.ContainsRenderPass(RenderPass))
        {
            // Destroy Framebuffer
            if (VkFramebuffer* Framebuffer = Framebuffers.Find(Key))
            {
                DestroyFramebuffer(GetDevice()->GetVkDevice(), *Framebuffer);
            }
            
            Framebuffers.Remove(Key);
        }
    }
}

void FVulkanRenderPassCache::ReleaseAll()
{
    {
        SCOPED_LOCK(FramebuffersCS);

        // Destroy all framebuffers
        for (auto Framebuffer : Framebuffers)
        {
            DestroyFramebuffer(GetDevice()->GetVkDevice(), Framebuffer.Second);
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

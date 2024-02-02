#include "VulkanFramebuffer.h"
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


FVulkanFramebufferCache::FVulkanFramebufferCache(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Framebuffers()
{
}

FVulkanFramebufferCache::~FVulkanFramebufferCache()
{
    ReleaseAll();
}

VkFramebuffer FVulkanFramebufferCache::GetFramebuffer(const FVulkanFramebufferKey& FrameBufferKey)
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

void FVulkanFramebufferCache::OnReleaseImageView(VkImageView View)
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

void FVulkanFramebufferCache::OnReleaseRenderPass(VkRenderPass RenderPass)
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

void FVulkanFramebufferCache::ReleaseAll()
{
    SCOPED_LOCK(FramebuffersCS);

    // Destroy all framebuffers
    for (auto Framebuffer : Framebuffers)
    {
        DestroyFramebuffer(GetDevice()->GetVkDevice(), Framebuffer.Second);
    }

    Framebuffers.Clear();
}

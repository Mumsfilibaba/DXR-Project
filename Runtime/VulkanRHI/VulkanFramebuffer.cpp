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
    : FVulkanDeviceObject(InDevice)
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

    //Check if a Framebuffer exists
    auto FrameBufferIt = Framebuffers.find(FrameBufferKey);
    if (FrameBufferIt != Framebuffers.end())
    {
        return FrameBufferIt->second;
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
        Framebuffers.insert(std::pair<FVulkanFramebufferKey, VkFramebuffer>(FrameBufferKey, Framebuffer));
        return Framebuffer;
    }
}


void FVulkanFramebufferCache::OnReleaseImageView(VkImageView View)
{
    SCOPED_LOCK(FramebuffersCS);

    //Find all FrameBuffers containing this texture
    for (auto FrameBufferIt = Framebuffers.begin(); FrameBufferIt != Framebuffers.end();)
    {
        if (FrameBufferIt->first.ContainsImageView(View))
        {
            //Destroy Framebuffer
            DestroyFramebuffer(GetDevice()->GetVkDevice(), FrameBufferIt->second);
            FrameBufferIt = Framebuffers.erase(FrameBufferIt);
        }
        else
        {
            FrameBufferIt++;
        }
    }
}


void FVulkanFramebufferCache::OnReleaseRenderPass(VkRenderPass renderpass)
{
    SCOPED_LOCK(FramebuffersCS);

    //Find all framebuffers containing this renderpass
    for (auto FrameBufferIt = Framebuffers.begin(); FrameBufferIt != Framebuffers.end();)
    {
        if (FrameBufferIt->first.ContainsRenderPass(renderpass))
        {
            //Destroy Framebuffer
            DestroyFramebuffer(GetDevice()->GetVkDevice(), FrameBufferIt->second);
            FrameBufferIt = Framebuffers.erase(FrameBufferIt);
        }
        else
        {
            FrameBufferIt++;
        }
    }
}


void FVulkanFramebufferCache::ReleaseAll()
{
    SCOPED_LOCK(FramebuffersCS);

    //Destroy all framebuffers
    for (auto& Framebuffer : Framebuffers)
    {
        DestroyFramebuffer(GetDevice()->GetVkDevice(), Framebuffer.second);
    }

    Framebuffers.clear();
}

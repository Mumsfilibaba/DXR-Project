#pragma once
#include "VulkanDeviceChild.h"
#include "Core/Containers/Map.h"

struct FVulkanFramebufferKey
{
    FVulkanFramebufferKey()
        : RenderPass(VK_NULL_HANDLE)
        , Width(0)
        , Height(0)
        , NumAttachmentViews(0)
    {
        FMemory::Memzero(AttachmentViews, sizeof(AttachmentViews));
    }

    bool ContainsImageView(VkImageView InView) const
    {
        for (uint32 Index = 0; Index < NumAttachmentViews; Index++)
        {
            if (AttachmentViews[Index] == InView)
            {
                return true;
            }
        }

        return false;
    }

    bool ContainsRenderPass(VkRenderPass InRenderpass) const
    {
        return RenderPass == InRenderpass;
    }

    bool operator==(const FVulkanFramebufferKey& Other) const
    {
        if (RenderPass != Other.RenderPass || NumAttachmentViews != Other.NumAttachmentViews || Width != Other.Width || Height != Other.Height)
            return false;

        for (uint32 Index = 0; Index < NumAttachmentViews; Index++)
        {
            if (AttachmentViews[Index] != Other.AttachmentViews[Index])
                return false;
        }

        return true;
    }

    bool operator!=(const FVulkanFramebufferKey& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FVulkanFramebufferKey& Key)
    {
        uint64 Hash = reinterpret_cast<uint64>(Key.RenderPass);
        HashCombine(Hash, Key.Width);
        HashCombine(Hash, Key.Height);

        for (uint32 Index = 0; Index < Key.NumAttachmentViews; Index++)
            HashCombine(Hash, Key.AttachmentViews[Index]);

        return Hash;
    }

    VkRenderPass RenderPass;
    uint16       Width;
    uint16       Height;
    uint32       NumAttachmentViews;
    VkImageView  AttachmentViews[RHI_MAX_RENDER_TARGETS + 1];
};

class FVulkanFramebufferCache : public FVulkanDeviceChild
{
public:
    FVulkanFramebufferCache(FVulkanDevice* InDevice);
    ~FVulkanFramebufferCache();

    VkFramebuffer GetFramebuffer(const FVulkanFramebufferKey& Key);
    void ReleaseAll();
    void OnReleaseImageView(VkImageView View);
    void OnReleaseRenderPass(VkRenderPass RenderPass);

private:
    TMap<FVulkanFramebufferKey, VkFramebuffer> Framebuffers;
    FCriticalSection                           FramebuffersCS;
};

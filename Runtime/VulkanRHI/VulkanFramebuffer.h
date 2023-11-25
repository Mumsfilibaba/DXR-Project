#pragma once
#include "VulkanDeviceObject.h"
#include "Core/Containers/Map.h"

struct FVulkanFramebufferKey
{
    FVulkanFramebufferKey()
        : RenderPass(VK_NULL_HANDLE)
        , NumAttachmentViews(0)
        , Width(0)
        , Height(0)
    {
        FMemory::Memzero(AttachmentViews, sizeof(AttachmentViews));
    }

    uint64 GetHash() const
    {
        uint64 Hash = reinterpret_cast<uint64>(RenderPass);
        HashCombine(Hash, Width);
        HashCombine(Hash, Height);

        for (uint32 Index = 0; Index < NumAttachmentViews; Index++)
        {
            HashCombine(Hash, AttachmentViews[Index]);
        }

        return Hash;
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
        {
            return false;
        }

        for (uint32 Index = 0; Index < NumAttachmentViews; Index++)
        {
            if (AttachmentViews[Index] != Other.AttachmentViews[Index])
            {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const FVulkanFramebufferKey& Other) const
    {
        return !(*this == Other);
    }

    VkRenderPass RenderPass;
    uint16       Width;
    uint16       Height;
    uint32       NumAttachmentViews;
    VkImageView  AttachmentViews[FRHILimits::MaxRenderTargets + 1];
};


struct FVulkanFramebufferKeyHasher
{
    uint64 operator()(const FVulkanFramebufferKey& Key) const
    {
        return Key.GetHash();
    }
};


class FVulkanFramebufferCache : public FVulkanDeviceObject
{
public:
    FVulkanFramebufferCache(FVulkanDevice* InDevice);
    ~FVulkanFramebufferCache();

    VkFramebuffer GetFramebuffer(const FVulkanFramebufferKey& Key);

    void ReleaseAll();

    void OnReleaseImageView(VkImageView View);

    void OnReleaseRenderPass(VkRenderPass Renderpass);

private:
    TMap<FVulkanFramebufferKey, VkFramebuffer, FVulkanFramebufferKeyHasher> Framebuffers;
    FCriticalSection FramebuffersCS;
};

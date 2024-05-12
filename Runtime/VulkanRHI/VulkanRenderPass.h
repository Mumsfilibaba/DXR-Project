#pragma once
#include "VulkanDeviceChild.h"
#include "Core/Containers/Map.h"

struct FVulkanRenderPassActions
{
    EAttachmentLoadAction  LoadAction  : 4;
    EAttachmentStoreAction StoreAction : 4;
};

struct FVulkanRenderPassKey
{
    FVulkanRenderPassKey()
        : Key0(0)
        , Key1(0)
        , Key2(0)
    {
    }

    bool operator==(const FVulkanRenderPassKey& Other) const
    {
        return FMemory::Memcmp(this, &Other, sizeof(FVulkanRenderPassKey)) == 0;
    }

    bool operator!=(const FVulkanRenderPassKey& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FVulkanRenderPassKey& Key)
    {
        uint64 Hash = Key.Key0;
        HashCombine(Hash, Key.Key1);
        HashCombine(Hash, Key.Key2);
        return Hash;
    }

    union
    {
        struct
        {
            EFormat                  DepthStencilFormat;
            FVulkanRenderPassActions DepthStencilActions;
            
            uint8 NumSamples       : 4;
            uint8 NumRenderTargets : 4;
            
            EFormat                  RenderTargetFormats[RHI_MAX_RENDER_TARGETS];
            FVulkanRenderPassActions RenderTargetActions[RHI_MAX_RENDER_TARGETS];
        };
        
        struct
        {
            uint64 Key0;
            uint64 Key1;
            uint64 Key2;
        };
    };
};

static_assert(sizeof(FVulkanRenderPassKey) == sizeof(uint64[3]), "Size of FVulkanRenderPassKey is invalid");

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

class FVulkanRenderPassCache : public FVulkanDeviceChild
{
public:
    FVulkanRenderPassCache(FVulkanDevice* InDevice);
    ~FVulkanRenderPassCache();

    VkRenderPass GetRenderPass(const FVulkanRenderPassKey& Key);
    VkFramebuffer GetFramebuffer(const FVulkanFramebufferKey& Key);
    void OnReleaseImageView(VkImageView View);
    void OnReleaseRenderPass(VkRenderPass RenderPass);

private:
    TMap<FVulkanRenderPassKey, VkRenderPass>   RenderPasses;
    FCriticalSection                           RenderPassesCS;
    TMap<FVulkanFramebufferKey, VkFramebuffer> Framebuffers;
    FCriticalSection                           FramebuffersCS;
};

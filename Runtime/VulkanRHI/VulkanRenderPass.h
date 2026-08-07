#pragma once
#include "Core/Containers/Map.h"
#include "VulkanRHI/VulkanDeviceChild.h"

#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH

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
        return Memory::Memcmp(this, &Other, sizeof(FVulkanRenderPassKey)) == 0;
    }

    bool operator!=(const FVulkanRenderPassKey& Other) const
    {
        return !(*this == Other);
    }

    union
    {
        struct
        {
            EFormat                  DepthStencilFormat;
            FVulkanRenderPassActions DepthStencilActions;
            EDepthStencilViewFlags   DepthStencilFlags;
            uint8                    SampleCountLog2 : 4; // 0 = 1x ... 6 = 64x
            uint8                    NumRenderTargets : 4;
            FRHIViewInstancingState  ViewInstancingState;
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

template<>
struct THash<FVulkanRenderPassKey>
{
    static uint64 GetHash(const FVulkanRenderPassKey& Key)
    {
        uint64 Result = Key.Key0;
        HashCombine(Result, Key.Key1);
        HashCombine(Result, Key.Key2);
        return Result;
    }
};

struct FVulkanFramebufferKey
{
    FVulkanFramebufferKey()
        : Width(0)
        , Height(0)
        , NumArrayLayers(0)
        , NumAttachmentViews(0)
        , RenderPass(VK_NULL_HANDLE)
    {
        Memory::Memzero(AttachmentViews, sizeof(AttachmentViews));
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
        if (RenderPass != Other.RenderPass || Width != Other.Width || Height != Other.Height ||
            NumAttachmentViews != Other.NumAttachmentViews || NumArrayLayers != Other.NumArrayLayers)
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

    uint16       Width;
    uint16       Height;
    uint16       NumArrayLayers;
    uint16       NumAttachmentViews;
    VkRenderPass RenderPass;
    VkImageView  AttachmentViews[RHI_MAX_RENDER_TARGETS + 1];
};

template<>
struct THash<FVulkanFramebufferKey>
{
    static uint64 GetHash(const FVulkanFramebufferKey& Key)
    {
        uint64 Result = reinterpret_cast<uint64>(Key.RenderPass);
        HashCombine(Result, Key.Width);
        HashCombine(Result, Key.Height);
        HashCombine(Result, Key.NumArrayLayers);
        HashCombine(Result, Key.NumAttachmentViews);

        for (uint32 Index = 0; Index < Key.NumAttachmentViews; Index++)
        {
            HashCombine(Result, Key.AttachmentViews[Index]);
        }

        return Result;
    }
};

class FVulkanRenderPassCache : public FVulkanDeviceChild
{
public:
    FVulkanRenderPassCache(FVulkanDevice* InDevice);
    ~FVulkanRenderPassCache();

    VkRenderPass  GetRenderPass(const FVulkanRenderPassKey& Key);
    VkFramebuffer GetFramebuffer(const FVulkanFramebufferKey& Key);

    void OnReleaseImageView(VkImageView View);
    void OnReleaseRenderPass(VkRenderPass RenderPass);
    void EvictStaleFramebuffers();

private:
    struct FCachedFramebuffer
    {
        VkFramebuffer Handle;
        uint64        LastUsedFrame;
    };

    TMap<FVulkanRenderPassKey, VkRenderPass>        RenderPasses;
    FCriticalSection                                RenderPassesCS;
    TMap<FVulkanFramebufferKey, FCachedFramebuffer> Framebuffers;
    FCriticalSection                                FramebuffersCS;
    uint64                                          CurrentFrame;
};

#endif // VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH

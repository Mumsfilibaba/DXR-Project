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

    
    union
    {
        struct
        {
            EFormat                  DepthStencilFormat;
            FVulkanRenderPassActions DepthStencilActions;
            
            uint8 NumSamples       : 4;
            uint8 NumRenderTargets : 4;
            
            EFormat                  RenderTargetFormats[FRHILimits::MaxRenderTargets];
            FVulkanRenderPassActions RenderTargetActions[FRHILimits::MaxRenderTargets];
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

inline uint64 HashType(const FVulkanRenderPassKey& Key)
{
    uint64 Hash = Key.Key0;
    HashCombine(Hash, Key.Key1);
    HashCombine(Hash, Key.Key2);
    return Hash;
}


class FVulkanRenderPassCache : public FVulkanDeviceChild
{
public:
    FVulkanRenderPassCache(FVulkanDevice* InDevice);
    ~FVulkanRenderPassCache();

    VkRenderPass GetRenderPass(const FVulkanRenderPassKey& Key);

    void ReleaseAll();

private:
    TMap<FVulkanRenderPassKey, VkRenderPass> RenderPasses;
    FCriticalSection                         RenderPassesCS;
};

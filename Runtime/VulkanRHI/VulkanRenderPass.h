#pragma once
#include "VulkanDeviceObject.h"
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

    uint64 GetHash() const
    {
        uint64 Hash = Key0;
        HashCombine(Hash, Key1);
        HashCombine(Hash, Key2);
        return Hash;
    }
    
    bool operator==(const FVulkanRenderPassKey& Other) const
    {
        return Key0 == Other.Key0 && Key1 == Other.Key1 && Key2 == Other.Key2;
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


struct FVulkanRenderPassKeyHasher
{
    uint64 operator()(const FVulkanRenderPassKey& Key) const
    {
        return Key.GetHash();
    }
};


class FVulkanRenderPassCache : public FVulkanDeviceObject
{
public:
    FVulkanRenderPassCache(FVulkanDevice* InDevice);
    ~FVulkanRenderPassCache();

    VkRenderPass GetRenderPass(const FVulkanRenderPassKey& Key);

    void ReleaseAll();

private:
    TMap<FVulkanRenderPassKey, VkRenderPass, FVulkanRenderPassKeyHasher> RenderPasses;
};

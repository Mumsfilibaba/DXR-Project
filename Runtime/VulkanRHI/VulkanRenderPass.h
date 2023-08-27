#pragma once
#include "VulkanDeviceObject.h"
#include "Core/Containers/Map.h"

struct FVulkanRenderPassKey
{
    FVulkanRenderPassKey()
        : NumSamples(0)
        , DepthStencilFormat(EFormat::Unknown)
        , NumRenderTargets(0)
        , RenderTargetFormats()
    {
        FMemory::Memzero(RenderTargetFormats, sizeof(RenderTargetFormats));
    }

    uint64 GetHash() const
    {
        uint64 Hash = NumSamples;
        HashCombine(Hash, ToUnderlying(DepthStencilFormat));
        for (uint8 Index = 0; Index < NumRenderTargets; Index++)
        {
            HashCombine(Hash, ToUnderlying(RenderTargetFormats[Index]));
        }

        return Hash;
    }
    
    bool operator==(const FVulkanRenderPassKey& Other) const
    {
        if (NumRenderTargets != Other.NumRenderTargets || GetHash() != Other.GetHash() ||
            NumSamples != Other.NumSamples || DepthStencilFormat != Other.DepthStencilFormat)
        {
            return false;
        }

        for (uint8 Index = 0; Index < NumRenderTargets; Index++)
        {
            if (RenderTargetFormats[Index] != Other.RenderTargetFormats[Index])
            {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const FVulkanRenderPassKey& Other) const
    {
        return !(*this == Other);
    }

    uint8   NumSamples;
    EFormat DepthStencilFormat;
    uint8   NumRenderTargets;
    EFormat RenderTargetFormats[FRHILimits::MaxRenderTargets];
};


struct FVulkanRenderPassKeyHasher
{
    size_t operator()(const FVulkanRenderPassKey& Key) const
    {
        return Key.GetHash();
    }
};


class FVulkanRenderPassCache : public FVulkanDeviceObject
{
public:
    FVulkanRenderPassCache(FVulkanDevice* InDevice);
    ~FVulkanRenderPassCache();

    VkRenderPass GetRenderPass(const FVulkanRenderPassKey& key);
    
    void ReleaseAll();

private:
    TMap<FVulkanRenderPassKey, VkRenderPass, FVulkanRenderPassKeyHasher> RenderPasses;
};

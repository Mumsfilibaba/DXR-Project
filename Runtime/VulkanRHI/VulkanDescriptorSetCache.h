#pragma once
#include "VulkanDescriptorSet.h"
#include "Core/Containers/Map.h"
#include "Core/Platform/CriticalSection.h"

class FVulkanBuffer;

class FVulkanDescriptorSetCache : public FVulkanDeviceChild
{
    class FCachedPool : public FVulkanDeviceChild
    {
    public:
        FCachedPool(FVulkanDevice* InDevice, const FVulkanDescriptorPoolInfo& InPoolInfo);
        ~FCachedPool();

        bool AllocateDescriptorSet(VkDescriptorSetLayout SetLayout, VkDescriptorSet& OutDescriptorSet);
        void ReleaseAll();

    private:
        FVulkanDescriptorPool*         CurrentDescriptorPool;
        TArray<FVulkanDescriptorPool*> DescriptorPools;
        FVulkanDescriptorPoolInfo      PoolInfo;
    };

public:
    FVulkanDescriptorSetCache(FVulkanDevice* InDevice);
    ~FVulkanDescriptorSetCache();

    bool FindOrCreateDescriptorSet(const FVulkanDescriptorPoolInfo& PoolInfo, FVulkanDescriptorSetBuilder& DSBuilder, VkDescriptorSet& OutDescriptorSet);
    void ReleaseDescriptorSets();
    void Release();

private:
    TMap<FVulkanDescriptorPoolInfo, FCachedPool*>  Caches;
    TMap<FVulkanDescriptorSetKey, VkDescriptorSet> DescriptorSets;
    FCriticalSection                               CacheCS;
};

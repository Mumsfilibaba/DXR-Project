#pragma once
#include "VulkanDescriptorSet.h"
#include "Core/Containers/Map.h"
#include "Core/Platform/CriticalSection.h"

class FVulkanBuffer;

class FVulkanDescriptorSetCache : public FVulkanDeviceChild
{
public:
    FVulkanDescriptorSetCache(FVulkanDevice* InDevice);
    ~FVulkanDescriptorSetCache();

    bool FindOrCreateDescriptorSet(VkDescriptorSetLayout SetLayout, FVulkanDescriptorSetBuilder& DSBuilder, VkDescriptorSet& OutDescriptorSet);
    
    bool Initialize();
    void Release();

private:
    FVulkanDescriptorPool*         CurrentDescriptorPool;
    TArray<FVulkanDescriptorPool*> DescriptorPools;

    TMap<FVulkanDescriptorSetKey, VkDescriptorSet> DescriptorSets;
    FCriticalSection DescriptorSetsCS;
};

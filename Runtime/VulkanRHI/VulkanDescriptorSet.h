#pragma once
#include "VulkanDeviceChild.h"
#include "VulkanRefCounted.h"

class FVulkanDescriptorPool;

class FVulkanDescriptorPool : public FVulkanDeviceChild
{
public:
    FVulkanDescriptorPool(const FVulkanDescriptorPool&) = delete;
    FVulkanDescriptorPool& operator=(const FVulkanDescriptorPool&) = delete;

    FVulkanDescriptorPool(FVulkanDevice* InDevice);
    ~FVulkanDescriptorPool();
    
    bool Initialize();
    
    bool AllocateDescriptorSet(VkDescriptorSetLayout DescriptorSetLayout, VkDescriptorSet& OuDescriptorSet);
    
    void Reset();

private:
    VkDescriptorPool DescriptorPool;
};

class FVulkanDescriptorPoolManager : public FVulkanDeviceChild
{
public:
    FVulkanDescriptorPoolManager(FVulkanDevice* InDevice);
    ~FVulkanDescriptorPoolManager();
    
    FVulkanDescriptorPool* ObtainPool();
    void RecyclePool(FVulkanDescriptorPool* InDescriptorPool);
    
    void ReleaseAll();
    
private:
    TArray<FVulkanDescriptorPool*> DescriptorPools;
    FCriticalSection               DescriptorPoolsCS;
};

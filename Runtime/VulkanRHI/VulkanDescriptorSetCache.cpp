#include "VulkanDescriptorSetCache.h"
#include "VulkanDevice.h"

FVulkanDescriptorSetCache::FVulkanDescriptorSetCache(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , CurrentDescriptorPool(nullptr)
    , DescriptorSets()
    , DescriptorSetsCS()
{
}

FVulkanDescriptorSetCache::~FVulkanDescriptorSetCache()
{
    Release();
}

bool FVulkanDescriptorSetCache::Initialize()
{   
    CurrentDescriptorPool = GetDevice()->GetDescriptorPoolManager().ObtainPool();
    if (!CurrentDescriptorPool)
    {
        return false;
    }
    
    return true;
}

void FVulkanDescriptorSetCache::Release()
{
    if (CurrentDescriptorPool)
    {
        GetDevice()->GetDescriptorPoolManager().RecyclePool(CurrentDescriptorPool);
        CurrentDescriptorPool = nullptr;
    }
    
    for (FVulkanDescriptorPool* DescriptorPool : DescriptorPools)
    {
        GetDevice()->GetDescriptorPoolManager().RecyclePool(DescriptorPool);
    }
    
    DescriptorSets.Clear();
    DescriptorPools.Clear();
}

bool FVulkanDescriptorSetCache::FindOrCreateDescriptorSet(VkDescriptorSetLayout SetLayout, FVulkanDescriptorSetBuilder& DSBuilder, VkDescriptorSet& OutDescriptorSet)
{
        // Get or Create a DescriptorSet
    if (VkDescriptorSet* DescriptorSet = DescriptorSets.Find(DSBuilder.GetKey()))
    {
        OutDescriptorSet = *DescriptorSet;
    }
    else
    {
        // Create a new DescriptorSet
        if (!CurrentDescriptorPool->AllocateDescriptorSet(SetLayout, OutDescriptorSet))
        {
            DescriptorPools.Add(CurrentDescriptorPool);
            
            //FVulkanRHI::GetRHI()->GetDeletionQueue().Emplace(DescriptorPool);
            //DescriptorPool = nullptr;

            CurrentDescriptorPool = GetDevice()->GetDescriptorPoolManager().ObtainPool();
            if (!CurrentDescriptorPool)
            {
                return false;
            }
            
            if (!CurrentDescriptorPool->AllocateDescriptorSet(SetLayout, OutDescriptorSet))
            {
                return false;
            }
        }
        
        CHECK(OutDescriptorSet != VK_NULL_HANDLE);
        DescriptorSets.Add(DSBuilder.GetKey(), OutDescriptorSet);
        
        DSBuilder.SetDescriptorSet(OutDescriptorSet);
        DSBuilder.UpdateDescriptorSet(GetDevice()->GetVkDevice());
    }

    return true;
}

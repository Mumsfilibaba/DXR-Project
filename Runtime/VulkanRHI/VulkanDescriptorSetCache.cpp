#include "VulkanDescriptorSetCache.h"
#include "VulkanDevice.h"

FVulkanDescriptorSetCache::FCachedPool::FCachedPool(FVulkanDevice* InDevice, const FVulkanDescriptorPoolInfo& InPoolInfo)
    : FVulkanDeviceChild(InDevice)
    , CurrentDescriptorPool(nullptr)
    , DescriptorPools()
    , PoolInfo(InPoolInfo)
{
}

FVulkanDescriptorSetCache::FCachedPool::~FCachedPool()
{
    ReleaseAll();
}

bool FVulkanDescriptorSetCache::FCachedPool::AllocateDescriptorSet(VkDescriptorSetLayout SetLayout, VkDescriptorSet& OutDescriptorSet)
{
    VkDescriptorSetAllocateInfo AllocateInfo;
    FMemory::Memzero(&AllocateInfo, sizeof(VkDescriptorSetAllocateInfo));

    AllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    AllocateInfo.pSetLayouts        = &SetLayout;
    AllocateInfo.descriptorSetCount = 1;

    if (CurrentDescriptorPool)
    {
        if (CurrentDescriptorPool->CanAllocateDescriptorSet())
        {
            return CurrentDescriptorPool->AllocateDescriptorSet(AllocateInfo, &OutDescriptorSet);
        }

        DescriptorPools.Add(CurrentDescriptorPool);
    }

    CurrentDescriptorPool = new FVulkanDescriptorPool(GetDevice());
    if (!CurrentDescriptorPool->Initialize(PoolInfo))
    {
        return false;
    }

    return CurrentDescriptorPool->AllocateDescriptorSet(AllocateInfo, &OutDescriptorSet);
}

void FVulkanDescriptorSetCache::FCachedPool::ReleaseAll()
{
    SAFE_DELETE(CurrentDescriptorPool);

    // TODO: Look into putting the pools in the deferred deletion queue
    for (FVulkanDescriptorPool* DescriptorPool : DescriptorPools)
    {
        delete DescriptorPool;
    }

    DescriptorPools.Clear();
}


FVulkanDescriptorSetCache::FVulkanDescriptorSetCache(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Caches()
    , DescriptorSets()
    , CacheCS()
{
}

FVulkanDescriptorSetCache::~FVulkanDescriptorSetCache()
{
    Release();
}

void FVulkanDescriptorSetCache::Release()
{
    TScopedLock Lock(CacheCS);
    DescriptorSets.Clear();
    Caches.Clear();
}

bool FVulkanDescriptorSetCache::FindOrCreateDescriptorSet(const FVulkanDescriptorPoolInfo& PoolInfo, FVulkanDescriptorSetBuilder& DSBuilder, VkDescriptorSet& OutDescriptorSet)
{
    TScopedLock Lock(CacheCS);

    // Get or Create a DescriptorSet
    if (VkDescriptorSet* DescriptorSet = DescriptorSets.Find(DSBuilder.GetKey()))
    {
        OutDescriptorSet = *DescriptorSet;
    }
    else
    {
        FCachedPool* Pool = nullptr;
        if (FCachedPool* ExistingPool = Caches.Find(PoolInfo))
        {
            Pool = ExistingPool;
        }
        else
        {
            FCachedPool NewPool(GetDevice(), PoolInfo);
            Pool = &Caches.Add(Move(PoolInfo), Move(NewPool));
        }

        if (!Pool)
        {
            return false;
        }

        // Create a new DescriptorSet
        if (!Pool->AllocateDescriptorSet(PoolInfo.DescriptorSetLayout, OutDescriptorSet))
        {
            return false;
        }

        CHECK(OutDescriptorSet != VK_NULL_HANDLE);
        DescriptorSets.Add(DSBuilder.GetKey(), OutDescriptorSet);

        DSBuilder.SetDescriptorSet(OutDescriptorSet);
        DSBuilder.UpdateDescriptorSet(GetDevice()->GetVkDevice());
    }

    return true;
}

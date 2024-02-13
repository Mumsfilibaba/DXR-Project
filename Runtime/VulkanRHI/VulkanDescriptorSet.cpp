#include "VulkanDescriptorSet.h"

FVulkanDescriptorPool::FVulkanDescriptorPool(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , DescriptorPool(VK_NULL_HANDLE)
{
}

FVulkanDescriptorPool::~FVulkanDescriptorPool()
{
    if (VULKAN_CHECK_HANDLE(DescriptorPool))
    {
        VkDevice VulkanDevice = GetDevice()->GetVkDevice();
        vkResetDescriptorPool(VulkanDevice, DescriptorPool, 0);
        vkDestroyDescriptorPool(VulkanDevice, DescriptorPool, nullptr);
        DescriptorPool = VK_NULL_HANDLE;
    }
}

bool FVulkanDescriptorPool::Initialize()
{
    const VkDescriptorType DescriptorTypes[] =
    {
        // ConstantBuffers
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        // SRV + UAV Buffers
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        // Samplers
        VK_DESCRIPTOR_TYPE_SAMPLER,
        // UAV Textures
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        // Textures
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    };
    
    TArray<VkDescriptorPoolSize> PoolSizes;
    for (VkDescriptorType DescriptorType : DescriptorTypes)
    {
        VkDescriptorPoolSize NewPoolSize;
        NewPoolSize.type            = DescriptorType;
        NewPoolSize.descriptorCount = 1024;
        PoolSizes.Add(NewPoolSize);
    }
    
    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo;
    FMemory::Memzero(&DescriptorPoolCreateInfo);
    
    DescriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCreateInfo.maxSets       = 1024;
    DescriptorPoolCreateInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    DescriptorPoolCreateInfo.poolSizeCount = PoolSizes.Size();
    DescriptorPoolCreateInfo.pPoolSizes    = PoolSizes.Data();
    
    VkResult Result = vkCreateDescriptorPool(GetDevice()->GetVkDevice(), &DescriptorPoolCreateInfo, nullptr, &DescriptorPool);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create DescriptorPool");
        return false;
    }
    else
    {
        return true;
    }
}

bool FVulkanDescriptorPool::AllocateDescriptorSet(VkDescriptorSetLayout DescriptorSetLayout, VkDescriptorSet& OutDescriptorSet)
{
    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
    FMemory::Memzero(&DescriptorSetAllocateInfo);
    
    DescriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescriptorSetAllocateInfo.descriptorPool     = DescriptorPool;
    DescriptorSetAllocateInfo.descriptorSetCount = 1;
    DescriptorSetAllocateInfo.pSetLayouts        = &DescriptorSetLayout;
    
    // Initalize the DescriptorSet to Null
    OutDescriptorSet = VK_NULL_HANDLE;
    
    VkResult Result = vkAllocateDescriptorSets(GetDevice()->GetVkDevice(), &DescriptorSetAllocateInfo, &OutDescriptorSet);
    if (Result == VK_ERROR_OUT_OF_POOL_MEMORY)
    {
        return false;
    }
    
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to allocate descriptorset");
        return false;
    }
    else
    {
        return true;
    }
}

void FVulkanDescriptorPool::Reset()
{
    vkResetDescriptorPool(GetDevice()->GetVkDevice(), DescriptorPool, 0);
}


FVulkanDescriptorPoolManager::FVulkanDescriptorPoolManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , DescriptorPools()
    , DescriptorPoolsCS()
{
}

FVulkanDescriptorPoolManager::~FVulkanDescriptorPoolManager()
{
    ReleaseAll();
}

FVulkanDescriptorPool* FVulkanDescriptorPoolManager::ObtainPool()
{
    {
        SCOPED_LOCK(DescriptorPoolsCS);
        
        if (!DescriptorPools.IsEmpty())
        {
            FVulkanDescriptorPool* DescriptorPool = DescriptorPools.LastElement();
            DescriptorPools.Pop();

            // Reset the memory of the DescriptorPool
            DescriptorPool->Reset();
            return DescriptorPool;
        }
    }
    
    FVulkanDescriptorPool* DescriptorPool = new FVulkanDescriptorPool(GetDevice());
    if (!DescriptorPool->Initialize())
    {
        DEBUG_BREAK();
        return nullptr;
    }
    else
    {
        return DescriptorPool;
    }
}

void FVulkanDescriptorPoolManager::RecyclePool(FVulkanDescriptorPool* InDescriptorPool)
{
    if (InDescriptorPool)
    {
        SCOPED_LOCK(DescriptorPoolsCS);
        DescriptorPools.Add(InDescriptorPool);
    }
    else
    {
        LOG_WARNING("Trying to Recycle an invalid Fence");
    }
}

void FVulkanDescriptorPoolManager::ReleaseAll()
{
    SCOPED_LOCK(DescriptorPoolsCS);
    
    for (FVulkanDescriptorPool* DescriptorPool : DescriptorPools)
    {
        delete DescriptorPool;
    }
    
    DescriptorPools.Clear();
}

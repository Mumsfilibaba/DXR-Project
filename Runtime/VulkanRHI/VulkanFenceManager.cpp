#include "VulkanFenceManager.h"
#include "VulkanFence.h"

FVulkanFenceManager::FVulkanFenceManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Fences()
    , FencesCS()
{
}

FVulkanFenceManager::~FVulkanFenceManager()
{
    ReleaseAll();
}

FVulkanFence* FVulkanFenceManager::ObtainFence()
{
    {
        SCOPED_LOCK(FencesCS);
        
        if (!Fences.IsEmpty())
        {
            FVulkanFence* Fence = Fences.LastElement();
            Fences.Pop();
            
            // Reset the fence to not be signaled before we return it
            Fence->Reset();
            return Fence;
        }
    }
    
    FVulkanFence* Fence = new FVulkanFence(GetDevice());
    if (!Fence->Initialize(false))
    {
        DEBUG_BREAK();
        return nullptr;
    }
    else
    {
        return Fence;
    }
}

void FVulkanFenceManager::RecycleFence(FVulkanFence* InFence)
{
    if (InFence)
    {
        SCOPED_LOCK(FencesCS);
        Fences.Add(InFence);
    }
    else
    {
        LOG_WARNING("Trying to Recycle an invalid Fence");
    }
}

void FVulkanFenceManager::ReleaseAll()
{
    SCOPED_LOCK(FencesCS);
    
    for (FVulkanFence* Fence : Fences)
    {
        delete Fence;
    }
    
    Fences.Clear();
}

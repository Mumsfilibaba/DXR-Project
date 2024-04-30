#include "VulkanFenceManager.h"
#include "VulkanFence.h"

FVulkanFenceManager::FVulkanFenceManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , AvailableFences()
    , FencesCS()
{
}

FVulkanFenceManager::~FVulkanFenceManager()
{
    ReleaseAll();
}

FVulkanFence* FVulkanFenceManager::ObtainFence()
{
    SCOPED_LOCK(FencesCS);

    FVulkanFence* Fence = nullptr;
    if (AvailableFences.Dequeue(Fence))
    {
        // Reset the fence to not be signaled before we return it
        Fence->Reset();
        return Fence;
    }
    
    FVulkanFence* NewFence = new FVulkanFence(GetDevice());
    if (!NewFence->Initialize(false))
    {
        DEBUG_BREAK();
        delete NewFence;
        return nullptr;
    }

    Fences.Add(NewFence);
    return NewFence;
}

void FVulkanFenceManager::RecycleFence(FVulkanFence* InFence)
{
    if (InFence)
    {
        SCOPED_LOCK(FencesCS);
        AvailableFences.Enqueue(InFence);
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
    AvailableFences.Clear();
}

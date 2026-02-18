#include "VulkanRHI/VulkanFenceManager.h"
#include "VulkanRHI/VulkanFence.h"

FVulkanFenceManager::FVulkanFenceManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , AvailableFences()
    , FencesCS()
{
}

FVulkanFenceManager::~FVulkanFenceManager()
{
    SCOPED_LOCK(FencesCS);
    SCOPED_LOCK(AvailableFencesCS);

    for (FVulkanFence* Fence : Fences)
    {
        delete Fence;
    }

    Fences.Clear();
    AvailableFences.Clear();
}

FVulkanFence* FVulkanFenceManager::ObtainFence()
{
    FVulkanFence* Fence = nullptr;
    if (FindAvailableFence(&Fence))
    {
        // Reset the fence to not be signaled before we return it
        Fence->Reset();
    }
    else
    {
        FVulkanFence* NewFence = new FVulkanFence(GetDevice());
        if (!NewFence->Initialize(false))
        {
            DEBUG_BREAK();
            delete NewFence;
            return nullptr;
        }
        else
        {
            SCOPED_LOCK(FencesCS);
            Fences.Add(NewFence);
            Fence = NewFence;
        }
    }
    
    Fence->AddRef();
    return Fence;
}

void FVulkanFenceManager::RecycleFence(FVulkanFence* InFence)
{
    if (InFence)
    {
        SCOPED_LOCK(AvailableFencesCS);

        InFence->Release();
        AvailableFences.Add(InFence);
    }
    else
    {
        VULKAN_WARNING("Trying to Recycle an invalid Fence");
    }
}

bool FVulkanFenceManager::FindAvailableFence(FVulkanFence** OutAvailableFence)
{
    SCOPED_LOCK(AvailableFencesCS);

    for (int32 i = 0; i < AvailableFences.Size(); ++i)
    {
        FVulkanFence* CurrentFence = AvailableFences[i];
        if (!CurrentFence->IsReferenced())
        {
            *OutAvailableFence = CurrentFence;
            AvailableFences.RemoveAt(i);
            return true;
        }
    }

    return false;
}

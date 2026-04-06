#include "VulkanRHI/VulkanFenceManager.h"
#include "VulkanRHI/VulkanFence.h"

FVulkanFenceManager::FVulkanFenceManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , AvailableFences()
    , PendingRecycleFences()
    , FencePoolCS()
    , FencesCS()
{
}

FVulkanFenceManager::~FVulkanFenceManager()
{
    SCOPED_LOCK(FencesCS);
    SCOPED_LOCK(FencePoolCS);

    for (FVulkanFence* Fence : Fences)
    {
        Fence->Release();
    }

    Fences.Clear();
    AvailableFences.Clear();
    PendingRecycleFences.Clear();
}

FVulkanFence* FVulkanFenceManager::ObtainFence()
{
    FVulkanFence* Fence = nullptr;
    if (FindAvailableFence(&Fence))
    {
        Fence->Reset();
    }
    else
    {
        FVulkanFence* NewFence = new FVulkanFence(GetDevice());
        if (!NewFence->Initialize(false))
        {
            DEBUG_BREAK();
            NewFence->Release();
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
    if (!InFence)
    {
        VULKAN_WARNING("Trying to Recycle an invalid Fence");
        return;
    }

    SCOPED_LOCK(FencePoolCS);

    const int32 RefCount = InFence->Release();
    if (RefCount == 1)
    {
        AvailableFences.Add(InFence);
    }
    else
    {
        PendingRecycleFences.Add(InFence);
    }
}

bool FVulkanFenceManager::FindAvailableFence(FVulkanFence** OutAvailableFence)
{
    SCOPED_LOCK(FencePoolCS);

    ReclaimReleasedFences();

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

void FVulkanFenceManager::ReclaimReleasedFences()
{
    for (int32 i = PendingRecycleFences.Size() - 1; i >= 0; --i)
    {
        FVulkanFence* Fence = PendingRecycleFences[i];
        if (!Fence->IsReferenced())
        {
            AvailableFences.Add(Fence);
            PendingRecycleFences.RemoveAt(i);
        }
    }
}

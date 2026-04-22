#include "VulkanRHI/VulkanResource.h"
#include "VulkanRHI/VulkanDevice.h"

FVulkanResource::FVulkanResource(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , MemoryStorage(InDevice)
{
    MemoryStorage.SetOwner(this);
}

FVulkanResource::~FVulkanResource()
{
    GetDevice()->GetMemoryManager().CancelPendingDefragMoves(this);
    ResourceRelocated(nullptr);
}

void FVulkanResource::AddResourceRelocatedListener(IVulkanResourceRelocationListener* Listener)
{
    if (!Listener)
    {
        return;
    }

    TScopedLock Lock(ListenersCS);
    Listeners.AddUnique(Listener);
}

void FVulkanResource::RemoveResourceRelocatedListener(IVulkanResourceRelocationListener* Listener)
{
    if (!Listener)
    {
        return;
    }

    TScopedLock Lock(ListenersCS);
    Listeners.Remove(Listener);
}

void FVulkanResource::ResourceRelocated(FVulkanMemoryStorage* NewMemoryStorage)
{
    TScopedLock Lock(ListenersCS);

    for (IVulkanResourceRelocationListener* Listener : Listeners)
    {
        if (Listener)
        {
            Listener->OnResourceRelocated(this, NewMemoryStorage);
        }
    }

    if (!NewMemoryStorage)
    {
        Listeners.Clear();
    }
}

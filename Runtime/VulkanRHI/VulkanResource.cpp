#include "VulkanRHI/VulkanResource.h"
#include "VulkanRHI/VulkanDevice.h"

FVulkanGenericResource::FVulkanGenericResource(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , MemoryStorage(InDevice)
{
    MemoryStorage.SetOwner(this);
}

FVulkanGenericResource::~FVulkanGenericResource()
{
    GetDevice()->GetMemoryManager().CancelPendingDefragMoves(this);
    ResourceRelocated(nullptr);
}

void FVulkanGenericResource::AddResourceRelocatedListener(IVulkanResourceRelocationListener* Listener)
{
    if (!Listener)
    {
        return;
    }

    TScopedLock Lock(ListenersCS);
    Listeners.AddUnique(Listener);
}

void FVulkanGenericResource::RemoveResourceRelocatedListener(IVulkanResourceRelocationListener* Listener)
{
    if (!Listener)
    {
        return;
    }

    TScopedLock Lock(ListenersCS);
    Listeners.Remove(Listener);
}

void FVulkanGenericResource::ResourceRelocated(FVulkanMemoryStorage* NewMemoryStorage)
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

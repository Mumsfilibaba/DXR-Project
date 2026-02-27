#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/ScopedLock.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanMemoryManager.h"

class FVulkanGenericResource;

struct IVulkanResourceRelocationListener
{
    virtual ~IVulkanResourceRelocationListener() = default;
    virtual void OnResourceRelocated(FVulkanGenericResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage) = 0;
};

class FVulkanGenericResource : public FVulkanDeviceChild
{
public:
    FVulkanGenericResource(FVulkanDevice* InDevice);
    virtual ~FVulkanGenericResource();

    void AddResourceRelocatedListener(IVulkanResourceRelocationListener* Listener);
    void RemoveResourceRelocatedListener(IVulkanResourceRelocationListener* Listener);

    void ResourceRelocated(FVulkanMemoryStorage* NewMemoryStorage);

    FVulkanMemoryStorage&       GetMemoryStorage()       { return MemoryStorage; }
    const FVulkanMemoryStorage& GetMemoryStorage() const { return MemoryStorage; }

protected:
    FVulkanMemoryStorage MemoryStorage;

private:
    TArray<IVulkanResourceRelocationListener*> Listeners;
    FCriticalSection                           ListenersCS;
};

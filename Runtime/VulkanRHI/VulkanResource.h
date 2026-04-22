#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/ScopedLock.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanMemoryManager.h"

class FVulkanResource;

struct IVulkanResourceRelocationListener
{
    virtual ~IVulkanResourceRelocationListener() = default;
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage) = 0;
};

class FVulkanResource : public FVulkanDeviceChild
{
public:
    FVulkanResource(FVulkanDevice* InDevice);
    virtual ~FVulkanResource();

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

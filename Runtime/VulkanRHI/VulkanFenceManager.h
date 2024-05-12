#pragma once
#include "VulkanDeviceChild.h"
#include "VulkanLoader.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"

class FVulkanFence;

class FVulkanFenceManager : public FVulkanDeviceChild
{
public:
    FVulkanFenceManager(FVulkanDevice* InDevice);
    ~FVulkanFenceManager();

    FVulkanFence* ObtainFence();
    void RecycleFence(FVulkanFence* InFence);

private:
    TQueue<FVulkanFence*> AvailableFences;
    TArray<FVulkanFence*> Fences;
    FCriticalSection      FencesCS;
};

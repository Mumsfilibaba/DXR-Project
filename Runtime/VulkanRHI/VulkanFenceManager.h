#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanLoader.h"

class FVulkanFence;

class FVulkanFenceManager : public FVulkanDeviceChild
{
public:
    FVulkanFenceManager(FVulkanDevice* InDevice);
    ~FVulkanFenceManager();

    FVulkanFence* ObtainFence();
    void RecycleFence(FVulkanFence* InFence);

private:
    bool FindAvailableFence(FVulkanFence** OutAvailableFence);

    TArray<FVulkanFence*> AvailableFences;
    FCriticalSection AvailableFencesCS;
    TArray<FVulkanFence*> Fences;
    FCriticalSection FencesCS;
};

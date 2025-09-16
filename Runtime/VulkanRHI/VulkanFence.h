#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanLoader.h"

class FVulkanFence : public FVulkanDeviceChild, FNonCopyable
{
public:
    FVulkanFence(FVulkanDevice* InDevice);
    ~FVulkanFence();

    bool Initialize(bool bSignaled);
    bool IsSignaled() const;
    bool Wait(uint64 TimeOut = UINT64_MAX) const;
	bool Reset();
    
    bool IsReferenced() const;
    int64 AddRef() const;
    int64 Release() const;

    VkFence GetVkFence() const
    {
        return Fence;
    }

private:
    VkFence Fence;
    mutable FAtomicInt64 References;
};

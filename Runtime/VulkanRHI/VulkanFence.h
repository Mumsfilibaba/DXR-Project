#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/RefCountedBase.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "RHI/RHIFence.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanLoader.h"

class FVulkanQueue;

class FVulkanFence : public FVulkanDeviceChild, public FRefCountedBase
{
public:
    FVulkanFence(FVulkanDevice* InDevice);
    ~FVulkanFence();

    bool Initialize(bool bSignaled);

    bool Wait(uint64 TimeOut = UINT64_MAX) const;
    bool Reset();
    
    bool IsSignaled() const;

    bool IsReferenced() const
    {
        return GetRefCount() > 1;
    }

    VkFence GetVkFence() const
    {
        return Fence;
    }

private:
    VkFence Fence;
};

class FVulkanTimelineFence : public FVulkanDeviceChild, FNonCopyable
{
public:
    FVulkanTimelineFence(FVulkanDevice* InDevice);
    ~FVulkanTimelineFence();

    bool Initialize();

    uint64 Signal(FVulkanQueue& Queue);
    uint64 GetCompletedValue() const;

    bool WaitForValue(uint64 Value, uint64 TimeoutNs = UINT64_MAX);

    void SetDebugName(const String& Name);
    void GetDebugName(String& OutDebugName) const;

    uint64 GetLastSignaledValue() const
    {
        return LastSignaledValue.Load();
    }

    uint64 GetCurrentValue() const
    {
        return CurrentValue;
    }

    VkSemaphore GetVkSemaphore() const
    {
        return TimelineSemaphore;
    }

private:
    VkSemaphore    TimelineSemaphore;
    mutable uint64 LastCompletedValue;
    uint64         CurrentValue;
    AtomicUInt64   LastSignaledValue;
#if VULKAN_STORE_DEBUG_NAMES
    String         DebugName;
#endif
};

class FVulkanFenceRHI final : public FRHIFence, public FVulkanDeviceChild
{
public:
    FVulkanFenceRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanFenceRHI();

    // FRHIFence Interface
    virtual void* GetRHINativeFence() const override final;
    
    virtual bool IsSignaled()                        const override final;
    virtual bool Wait(uint64 TimeoutNs = UINT64_MAX) const override final;
    
    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize();
 
    // Called by the command context to enqueue a GPU signal at the next submission.
    void EnqueueSignal(FVulkanQueue& Queue);
    
    // Fallback path: use the submission fence from the submission that contained the signal point.
    void SetSubmissionFence(FVulkanFence* InFence);

    bool UsesTimeline() const
    {
        return bUsesTimeline;
    }

    VkSemaphore GetVkTimelineSemaphore() const
    {
        return TimelineSemaphore;
    }

    uint64 GetTargetValue() const
    {
        return TargetValue;
    }

private:
    VkSemaphore   TimelineSemaphore;
    FVulkanFence* SubmissionFence;
    uint64        NextValue;
    uint64        TargetValue;
    AtomicBool    bHasPendingSignal;
    bool          bUsesTimeline;
#if VULKAN_STORE_DEBUG_NAMES
    String        DebugName;
#endif
};

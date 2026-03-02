#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "RHI/RHIFence.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanLoader.h"

class FVulkanQueue;

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

class FVulkanTimelineFence : public FVulkanDeviceChild, FNonCopyable
{
public:
    FVulkanTimelineFence(FVulkanDevice* InDevice);
    ~FVulkanTimelineFence();

    bool Initialize();

    uint64 Signal(FVulkanQueue& Queue);
    uint64 GetCompletedValue() const;
    bool   WaitForValue(uint64 Value, uint64 TimeoutNs = UINT64_MAX);
    void   SetDebugName(const FString& Name);

    uint64 GetLastSignaledValue() const
    {
        return LastSignaledValue;
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
    uint64         LastSignaledValue;
};

class FVulkanGpuFence final : public FRHIGpuFence, public FVulkanDeviceChild
{
public:
    explicit FVulkanGpuFence(FVulkanDevice* InDevice);
    virtual ~FVulkanGpuFence();

    bool Initialize();

    // FRHIGpuFence Interface
    virtual bool IsSignaled() const override final;
    virtual bool Wait(uint64 TimeoutNs = UINT64_MAX) const override final;
    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;
 
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
    FAtomicBool   bHasPendingSignal;
    bool          bUsesTimeline;
    FString       DebugName;
};

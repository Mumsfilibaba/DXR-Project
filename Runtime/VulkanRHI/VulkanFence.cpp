#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanQueue.h"

FVulkanFence::FVulkanFence(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Fence(VK_NULL_HANDLE)
{
}

FVulkanFence::~FVulkanFence()
{
    if (VULKAN_CHECK_HANDLE(Fence))
    {
        vkDestroyFence(GetDevice()->GetVkDevice(), Fence, nullptr);
        Fence = VK_NULL_HANDLE;
    }
}

bool FVulkanFence::Initialize(bool bSignaled)
{
    VkFenceCreateInfo FenceCreateInfo = {};
    FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceCreateInfo.pNext = nullptr;
    FenceCreateInfo.flags = bSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VkResult Result = vkCreateFence(GetDevice()->GetVkDevice(), &FenceCreateInfo, nullptr, &Fence);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Fence");
        return false;
    }
    else
    {
        return true;
    }
}

bool FVulkanFence::IsSignaled() const
{
    VkResult Result = vkGetFenceStatus(GetDevice()->GetVkDevice(), Fence);

#if VULKAN_ENABLE_DEVICE_LOST_CHECK
    if (Result == VK_ERROR_DEVICE_LOST)
    {
        VULKAN_ERROR_CRITICAL("Device Lost");
        return false;
    }
#endif

    return Result == VK_SUCCESS;
}

bool FVulkanFence::Wait(uint64 TimeOut) const
{
    VkResult Result = vkWaitForFences(GetDevice()->GetVkDevice(), 1, &Fence, VK_TRUE, TimeOut);
    if (Result == VK_TIMEOUT)
    {
        // This is valid when the caller asked for a bounded wait.
        return false; 
    }
    
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkWaitForFences Failed");
        return false;
    }

    return true;
}

bool FVulkanFence::Reset()
{
    VkResult Result = vkResetFences(GetDevice()->GetVkDevice(), 1, &Fence);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkResetFences Failed");
        return false;
    }

    return true;
}


FVulkanTimelineFence::FVulkanTimelineFence(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , TimelineSemaphore(VK_NULL_HANDLE)
    , LastCompletedValue(0)
    , CurrentValue(0)
    , LastSignaledValue(0)
{
}

FVulkanTimelineFence::~FVulkanTimelineFence()
{
    if (VULKAN_CHECK_HANDLE(TimelineSemaphore))
    {
        vkDestroySemaphore(GetDevice()->GetVkDevice(), TimelineSemaphore, nullptr);
        TimelineSemaphore = VK_NULL_HANDLE;
    }
}

bool FVulkanTimelineFence::Initialize()
{
    VkSemaphoreTypeCreateInfo TypeInfo = {};
    TypeInfo.sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    TypeInfo.pNext         = nullptr;
    TypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    TypeInfo.initialValue  = 0;

    VkSemaphoreCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    CreateInfo.pNext = &TypeInfo;
    CreateInfo.flags = 0;

    VkResult Result = vkCreateSemaphore(GetDevice()->GetVkDevice(), &CreateInfo, nullptr, &TimelineSemaphore);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create timeline semaphore for FVulkanTimelineFence");
        return false;
    }

    return true;
}

uint64 FVulkanTimelineFence::Signal(FVulkanQueue& Queue)
{
    ++CurrentValue;
    CHECK(LastSignaledValue.Load() != CurrentValue);

    Queue.AddSignalTimelineSemaphore(TimelineSemaphore, CurrentValue);
    LastSignaledValue.Store(CurrentValue);
    return CurrentValue;
}

uint64 FVulkanTimelineFence::GetCompletedValue() const
{
    CHECK(VULKAN_CHECK_HANDLE(TimelineSemaphore));

    uint64 CounterValue = 0;

    VkResult Result = vkGetSemaphoreCounterValue(GetDevice()->GetVkDevice(), TimelineSemaphore, &CounterValue);
#if VULKAN_ENABLE_DEVICE_LOST_CHECK
    if (Result == VK_ERROR_DEVICE_LOST)
    {
        VULKAN_ERROR_CRITICAL("Device Lost");
        return LastCompletedValue;
    }
#endif

    VULKAN_ERROR_COND(Result == VK_SUCCESS, "vkGetSemaphoreCounterValue failed");
    LastCompletedValue = CounterValue;
    return LastCompletedValue;
}

bool FVulkanTimelineFence::WaitForValue(uint64 Value, uint64 TimeoutNs)
{
    CHECK(VULKAN_CHECK_HANDLE(TimelineSemaphore));
    CHECK(Value <= LastSignaledValue.Load());

    uint64 CompletedValue = GetCompletedValue();
    if (Value <= CompletedValue)
    {
        return true;
    }

    VkSemaphoreWaitInfo WaitInfo = {};
    WaitInfo.sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    WaitInfo.pNext          = nullptr;
    WaitInfo.flags          = 0;
    WaitInfo.semaphoreCount = 1;
    WaitInfo.pSemaphores    = &TimelineSemaphore;
    WaitInfo.pValues        = &Value;

    VkResult Result = vkWaitSemaphores(GetDevice()->GetVkDevice(), &WaitInfo, TimeoutNs);
    if (Result == VK_TIMEOUT)
    {
        return false;
    }

    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkWaitSemaphores failed in FVulkanTimelineFence");
        return false;
    }

    return true;
}

void FVulkanTimelineFence::SetDebugName(const String& Name)
{
    if (VULKAN_CHECK_HANDLE(TimelineSemaphore))
    {
        VulkanSetObjectName(GetDevice()->GetVkDevice(), *Name, TimelineSemaphore, VK_OBJECT_TYPE_SEMAPHORE);
    }

#if VULKAN_STORE_DEBUG_NAMES
    DebugName = Name;
#endif
}

void FVulkanTimelineFence::GetDebugName(String& OutDebugName) const
{
#if VULKAN_STORE_DEBUG_NAMES
    OutDebugName = DebugName;
#else
    OutDebugName.Clear();
#endif
}

FVulkanFenceRHI::FVulkanFenceRHI(FVulkanDevice* InDevice)
    : FRHIFence()
    , FVulkanDeviceChild(InDevice)
    , bUsesTimeline(false)
    , TimelineSemaphore(VK_NULL_HANDLE)
    , NextValue(0)
    , TargetValue(0)
    , bHasPendingSignal(false)
    , SubmissionFence(nullptr)
#if VULKAN_STORE_DEBUG_NAMES
    , DebugName()
#endif
{
}

FVulkanFenceRHI::~FVulkanFenceRHI()
{
    if (SubmissionFence)
    {
        SubmissionFence->Release();
        SubmissionFence = nullptr;
    }

    if (bUsesTimeline && VULKAN_CHECK_HANDLE(TimelineSemaphore))
    {
        vkDestroySemaphore(GetDevice()->GetVkDevice(), TimelineSemaphore, nullptr);
        TimelineSemaphore = VK_NULL_HANDLE;
    }
}

bool FVulkanFenceRHI::Initialize()
{
    const VkPhysicalDeviceVulkan12Features& Features12 = GetDevice()->GetPhysicalDevice()->GetFeaturesVulkan12();
    if (Features12.timelineSemaphore != VK_TRUE)
    {
        VULKAN_ERROR_CRITICAL("Timeline semaphores are required but not supported by this device.");
        return false;
    }

    bUsesTimeline = true;

    VkSemaphoreTypeCreateInfo TypeInfo = {};
    TypeInfo.sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    TypeInfo.pNext         = nullptr;
    TypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    TypeInfo.initialValue  = 0;

    VkSemaphoreCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    CreateInfo.pNext = &TypeInfo;
    CreateInfo.flags = 0;

    VkResult Result = vkCreateSemaphore(GetDevice()->GetVkDevice(), &CreateInfo, nullptr, &TimelineSemaphore);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create timeline semaphore");
        return false;
    }

    return true;
}

bool FVulkanFenceRHI::IsSignaled() const
{
    if (!bHasPendingSignal.Load())
    {
        // A fence that has never been signaled should not be treated as completed.
        return false;
    }

    if (bUsesTimeline)
    {
        uint64 CounterValue = 0;

        VkResult Result = vkGetSemaphoreCounterValue(GetDevice()->GetVkDevice(), TimelineSemaphore, &CounterValue);
    #if VULKAN_ENABLE_DEVICE_LOST_CHECK
        if (Result == VK_ERROR_DEVICE_LOST)
        {
            VULKAN_ERROR_CRITICAL("Device Lost");
            return false;
        }
    #endif

        VULKAN_ERROR_COND(Result == VK_SUCCESS, "vkGetSemaphoreCounterValue failed");
        return CounterValue >= TargetValue;
    }

    return SubmissionFence ? SubmissionFence->IsSignaled() : false;
}

bool FVulkanFenceRHI::Wait(uint64 TimeoutNs) const
{
    if (!bHasPendingSignal.Load())
    {
        // Nothing has been enqueued to signal this fence yet.
        return false;
    }

    if (IsSignaled())
    {
        return true;
    }

    if (bUsesTimeline)
    {
        VkSemaphoreWaitInfo WaitInfo = {};
        WaitInfo.sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        WaitInfo.pNext          = nullptr;
        WaitInfo.flags          = 0;
        WaitInfo.semaphoreCount = 1;
        WaitInfo.pSemaphores    = &TimelineSemaphore;
        WaitInfo.pValues        = &TargetValue;

        VkResult Result = vkWaitSemaphores(GetDevice()->GetVkDevice(), &WaitInfo, TimeoutNs);
        if (Result == VK_TIMEOUT)
        {
            return false;
        }
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("vkWaitSemaphores failed");
            return false;
        }

        return true;
    }

    return SubmissionFence ? SubmissionFence->Wait(TimeoutNs) : false;
}

void* FVulkanFenceRHI::GetRHINativeFence() const
{
    return reinterpret_cast<void*>(TimelineSemaphore);
}

void FVulkanFenceRHI::SetDebugName(const String& InName)
{
#if VULKAN_STORE_DEBUG_NAMES
    DebugName = InName;
#endif

    if (bUsesTimeline && VULKAN_CHECK_HANDLE(TimelineSemaphore))
    {
        VulkanSetObjectName(GetDevice()->GetVkDevice(), *InName, TimelineSemaphore, VK_OBJECT_TYPE_SEMAPHORE);
    }
}

void FVulkanFenceRHI::GetDebugName(String& OutDebugName) const
{
#if VULKAN_STORE_DEBUG_NAMES
    OutDebugName = DebugName;
#else
    OutDebugName.Clear();
#endif
}

void FVulkanFenceRHI::EnqueueSignal(FVulkanQueue& Queue)
{
    if (bUsesTimeline)
    {
        TargetValue = ++NextValue;
        bHasPendingSignal.Store(true);
        Queue.AddSignalTimelineSemaphore(TimelineSemaphore, TargetValue);
    }
}

void FVulkanFenceRHI::SetSubmissionFence(FVulkanFence* InFence)
{
    CHECK(!bUsesTimeline);

    if (SubmissionFence)
    {
        SubmissionFence->Release();
        SubmissionFence = nullptr;
    }

    SubmissionFence = InFence;
    if (SubmissionFence)
    {
        SubmissionFence->AddRef();
    }
    
    bHasPendingSignal.Store(SubmissionFence != nullptr);
}

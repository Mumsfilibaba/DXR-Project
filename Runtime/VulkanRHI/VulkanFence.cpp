#include "VulkanRHI/VulkanFence.h"

FVulkanFence::FVulkanFence(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Fence(VK_NULL_HANDLE)
    , References(0)
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
	if (Result == VK_ERROR_DEVICE_LOST)
	{
		VULKAN_ERROR_CRITICAL("Device Lost");
		return false;
	}

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

bool FVulkanFence::IsReferenced() const
{
	const int64 RefCount = References.Load();
	CHECK(RefCount >= 0);
	return RefCount > 0;
}

int64 FVulkanFence::AddRef() const
{
	CHECK(References.Load() >= 0);
	++References;
	return References.Load();
}

int64 FVulkanFence::Release() const
{
	const int64 RefCount = --References;
	CHECK(RefCount >= 0);
	return RefCount;
}

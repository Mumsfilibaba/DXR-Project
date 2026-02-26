#pragma once
#include "Core/Platform/CriticalSection.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanCore.h"
#include "VulkanRHI/VulkanRefCounted.h"
#include "VulkanRHI/VulkanMemoryManager.h"

class FRHIResource;
class FVulkanDevice;

struct FVulkanDeferredObject
{
    static void ProcessItems(FVulkanDevice* Device, TArray<FVulkanDeferredObject>& Items);

    enum class EType
    {
        RHIResource         = 1,
        VulkanResource      = 2,
        BuddyAllocatorBlock = 3,
        PoolAllocatorBlock  = 4,
        DedicatedAllocation = 5,
    };

    FVulkanDeferredObject(FRHIResource* InResource)
        : Type(EType::RHIResource)
    {
        CHECK(InResource != nullptr);
        RHIResource = InResource;
    }

    FVulkanDeferredObject(FVulkanRefCounted* InResource)
        : Type(EType::VulkanResource)
    {
        CHECK(InResource != nullptr);
        VulkanResource = InResource;
        InResource->AddRef();
    }

    FVulkanDeferredObject(FVulkanBuddyAllocator* InAllocator, const FVulkanBuddyAllocatorAllocationData& InAllocationData)
        : Type(EType::BuddyAllocatorBlock)
    {
        CHECK(InAllocator != nullptr);
        BuddyAllocatorBlock.Allocator      = InAllocator;
        BuddyAllocatorBlock.AllocationData = InAllocationData;
    }

    FVulkanDeferredObject(FVulkanPoolAllocator* InAllocator, const FVulkanPoolAllocatorAllocationData& InAllocationData)
        : Type(EType::PoolAllocatorBlock)
    {
        CHECK(InAllocator != nullptr);
        PoolAllocatorBlock.Allocator      = InAllocator;
        PoolAllocatorBlock.AllocationData = InAllocationData;
    }

    FVulkanDeferredObject(VkDeviceMemory InMemory, VkBuffer InBuffer)
        : Type(EType::DedicatedAllocation)
    {
        DedicatedAllocation.Memory = InMemory;
        DedicatedAllocation.Buffer = InBuffer;
    }

    EType const Type;

    struct FBuddyAllocatorBlockData
    {
        FVulkanBuddyAllocator*              Allocator      = nullptr;
        FVulkanBuddyAllocatorAllocationData AllocationData = {};
    };

    struct FPoolAllocatorBlockData
    {
        FVulkanPoolAllocator*              Allocator      = nullptr;
        FVulkanPoolAllocatorAllocationData AllocationData = {};
    };

    struct FDedicatedAllocationData
    {
        VkBuffer       Buffer = VK_NULL_HANDLE;
        VkDeviceMemory Memory = VK_NULL_HANDLE;
    };

    union
    {
        FRHIResource*            RHIResource;
        FVulkanRefCounted*       VulkanResource;
        FBuddyAllocatorBlockData BuddyAllocatorBlock;
        FPoolAllocatorBlockData  PoolAllocatorBlock;
        FDedicatedAllocationData DedicatedAllocation;
    };
};

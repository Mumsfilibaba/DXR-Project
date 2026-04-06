#pragma once
#include "Core/Platform/CriticalSection.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanCore.h"
#include "Core/RefCountedBase.h"
#include "VulkanRHI/VulkanMemoryManager.h"

class FRHIResource;
class FVulkanDevice;
class FVulkanDescriptorPool;
class FVulkanDescriptorPoolManager;

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
        LinearAllocatorPage = 6,
        DescriptorPool      = 7,
    };

    FVulkanDeferredObject(FRHIResource* InResource)
        : Type(EType::RHIResource)
    {
        CHECK(InResource != nullptr);
        RHIResource = InResource;
    }

    FVulkanDeferredObject(FRefCountedBase* InResource)
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

    FVulkanDeferredObject(FVulkanLinearAllocator* InAllocator, FVulkanLinearAllocatorPage* InPage)
        : Type(EType::LinearAllocatorPage)
    {
        CHECK(InAllocator != nullptr);
        CHECK(InPage != nullptr);
        LinearAllocatorPage.Allocator = InAllocator;
        LinearAllocatorPage.Page      = InPage;
    }

    FVulkanDeferredObject(FVulkanDescriptorPoolManager* InPoolManager, FVulkanDescriptorPool* InPool)
        : Type(EType::DescriptorPool)
    {
        CHECK(InPoolManager != nullptr);
        CHECK(InPool != nullptr);
        DescriptorPoolData.PoolManager = InPoolManager;
        DescriptorPoolData.Pool        = InPool;
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

    struct FLinearAllocatorPageData
    {
        FVulkanLinearAllocator*     Allocator = nullptr;
        FVulkanLinearAllocatorPage* Page      = nullptr;
    };

    struct FDescriptorPoolData
    {
        FVulkanDescriptorPoolManager* PoolManager = nullptr;
        FVulkanDescriptorPool*        Pool        = nullptr;
    };

    union
    {
        FRHIResource*            RHIResource;
        FRefCountedBase*         VulkanResource;
        FBuddyAllocatorBlockData BuddyAllocatorBlock;
        FPoolAllocatorBlockData  PoolAllocatorBlock;
        FDedicatedAllocationData DedicatedAllocation;
        FLinearAllocatorPageData LinearAllocatorPage;
        FDescriptorPoolData      DescriptorPoolData;
    };
};

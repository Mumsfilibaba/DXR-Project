#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanMemory.h"
#include "VulkanRHI/VulkanResourceState.h"
#include "Core/Containers/UniquePtr.h"

typedef TSharedRef<class FVulkanBufferRHI> FVulkanBufferRHIRef;
class FVulkanCommandContext;

class FVulkanBufferRHI : public FRHIBuffer, public FVulkanDeviceChild
{
public:
    FVulkanBufferRHI(FVulkanDevice* InDevice, const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState = EResourceAccess::Common);
    ~FVulkanBufferRHI();

    bool Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData);

    // FRHIBuffer Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetVkBuffer()); }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final;
    virtual void Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final;

    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

    void EnableStateTracking(EResourceAccess InitialState);
    void DisableStateTracking(FVulkanCommandContext* CommandContext = nullptr);

    // Resource State Tracking
    VkBuffer GetVkBuffer() const
    {
        return Buffer;
    }

    FVulkanBufferState* GetBufferState() const
    {
        return BufferState.Get();
    }

    VkDeviceMemory GetVkDeviceMemory() const
    {
        return MemoryAllocation.Memory;
    }

    VkDeviceAddress GetDeviceAddress() const
    {
        return MemoryAllocation.DeviceAddress;
    }

    VkDeviceSize GetRequiredAlignment() const
    {
        return RequiredAlignment;
    }

protected:
    VkBuffer                       Buffer;
    FVulkanMemoryAllocation        MemoryAllocation;
    VkDeviceSize                   RequiredAlignment;
    FString                        DebugName;
    TUniquePtr<FVulkanBufferState> BufferState;
};

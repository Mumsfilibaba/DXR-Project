#pragma once
#include "VulkanCore.h"
#include "VulkanLoader.h"
#include "VulkanPhysicalDevice.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/StringView.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/Optional.h"
#include "Core/Threading/AtomicInt.h"

class FVulkanInstance;
class FVulkanPhysicalDevice;

enum class EVulkanCommandQueueType
{
    Unknown  = 0, 
    Graphics = 1, 
    Copy     = 2, 
    Compute  = 3, 
};

struct FVulkanDeviceDesc
{
    FVulkanDeviceDesc()
        : RequiredExtensionNames()
        , OptionalExtensionNames()
        , RequiredFeatures()
    {
        FMemory::Memzero(&RequiredFeatures);
    }

    TArray<const CHAR*>      RequiredExtensionNames;
    TArray<const CHAR*>      OptionalExtensionNames;

    VkPhysicalDeviceFeatures RequiredFeatures;
};

class FVulkanDevice : public FVulkanRefCounted
{
public:
    FVulkanDevice(FVulkanInstance* InInstance, FVulkanPhysicalDevice* InAdapter);
    ~FVulkanDevice();

    bool Initialize(const FVulkanDeviceDesc& DeviceDesc);

    bool AllocateMemory(const VkMemoryAllocateInfo& MemoryAllocationInfo, VkDeviceMemory* OutDeviceMemory);

    void FreeMemory(VkDeviceMemory* OutDeviceMemory);
    
    uint32 GetCommandQueueIndexFromType(EVulkanCommandQueueType Type) const;

    FORCEINLINE bool IsLayerEnabled(const FString& LayerName)
    {
        return LayerNames.find(LayerName) != LayerNames.end();
    }

    FORCEINLINE bool IsExtensionEnabled(const FString& ExtensionName)
    {
        return ExtensionNames.find(ExtensionName) != ExtensionNames.end();
    }

    FORCEINLINE FVulkanInstance* GetInstance() const
    {
        return Instance;
    }
    
    FORCEINLINE FVulkanPhysicalDevice* GetPhysicalDevice() const
    {
        return PhysicalDevice;
    }

    FORCEINLINE VkDevice GetVkDevice() const
    {
        return Device;
    }

    FORCEINLINE TOptional<FVulkanQueueFamilyIndices> GetQueueIndicies() const
    {
        return QueueIndicies;
    }

private:
    FVulkanInstance*       Instance;
    FVulkanPhysicalDevice* PhysicalDevice;
    VkDevice               Device;

    TOptional<FVulkanQueueFamilyIndices> QueueIndicies;

    FAtomicInt32  NumAllocations = 0;
    
    TSet<FString> ExtensionNames;
    TSet<FString> LayerNames;
};

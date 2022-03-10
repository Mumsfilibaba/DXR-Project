#pragma once
#include "VulkanCore.h"
#include "VulkanLoader.h"
#include "VulkanPhysicalDevice.h"

#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/Optional.h"

class CVulkanInstance;
class CVulkanPhysicalDevice;

typedef TSharedRef<CVulkanDevice> CVulkanDeviceRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EVulkanCommandQueueType

enum class EVulkanCommandQueueType
{
    Unknown  = 0, 
    Graphics = 1, 
    Copy     = 2, 
    Compute  = 3, 
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SVulkanDeviceDesc

struct SVulkanDeviceDesc
{
    TArray<const char*> RequiredExtensionNames;
    TArray<const char*> OptionalExtensionNames;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDevice

class CVulkanDevice : public CRefCounted
{
public:

    /* Creates a new wrapper for VkDevice */
    static CVulkanDeviceRef CreateDevice(CVulkanInstance* InInstance, CVulkanPhysicalDevice* InAdapter, const SVulkanDeviceDesc& DeviceDesc);

    uint32 GetCommandQueueIndexFromType(EVulkanCommandQueueType Type) const;

    FORCEINLINE bool IsLayerEnabled(const String& LayerName)
    {
        return LayerNames.find(LayerName) != LayerNames.end();
    }

    FORCEINLINE bool IsExtensionEnabled(const String& ExtensionName)
    {
        return ExtensionNames.find(ExtensionName) != ExtensionNames.end();
    }

    FORCEINLINE CVulkanInstance* GetInstance() const
    {
        return Instance;
    }
    
    FORCEINLINE CVulkanPhysicalDevice* GetPhysicalDevice() const
    {
        return Adapter;
    }

    FORCEINLINE VkDevice GetVkDevice() const
    {
        return Device;
    }

    FORCEINLINE TOptional<SVulkanQueueFamilyIndices> GetQueueIndicies() const
    {
        return QueueIndicies;
    }

private:

    CVulkanDevice(CVulkanInstance* InInstance, CVulkanPhysicalDevice* InAdapter);
    ~CVulkanDevice();

    bool Initialize(const SVulkanDeviceDesc& DeviceDesc);

    CVulkanInstance* Instance;
    CVulkanPhysicalDevice* Adapter;
    VkDevice               Device;

    TOptional<SVulkanQueueFamilyIndices> QueueIndicies;

    TSet<String> ExtensionNames;
    TSet<String> LayerNames;
};

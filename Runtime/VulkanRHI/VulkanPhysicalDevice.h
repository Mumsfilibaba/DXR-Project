#pragma once 
#include "VulkanCore.h"

#include "Core/RefCounted.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Set.h"

class CVulkanDriverInstance;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanPhysicalDevice

struct SVulkanPhysicalDeviceDesc
{
    TArray<const char*> RequiredDeviceExtensionNames;
    TArray<const char*> RequiredDeviceLayerNames;
    TArray<const char*> OptionalDeviceExtensionNames;
    TArray<const char*> OptionalDeviceLayerNames;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanPhysicalDevice

class CVulkanPhysicalDevice : public CRefCounted
{
public:

    static TSharedRef<CVulkanPhysicalDevice> QueryAdapter(CVulkanDriverInstance* InInstance, const SVulkanPhysicalDeviceDesc& AdapterDesc);

    bool IsLayerEnabled(const String& LayerName);
    bool IsExtensionEnabled(const String& ExtensionName);

    FORCEINLINE CVulkanDriverInstance* GetInstance() const
    {
        return Instance;
    }

    FORCEINLINE VkPhysicalDevice GetPhysicalDevice() const
    {
        return PhysicalDevice;
    }

private:

	CVulkanPhysicalDevice(CVulkanDriverInstance* InInstance);
    ~CVulkanPhysicalDevice();

    bool Initialize(const SVulkanPhysicalDeviceDesc& AdapterDesc);

    CVulkanDriverInstance* Instance;
    VkPhysicalDevice       PhysicalDevice;

    TSet<String> ExtensionNames;
    TSet<String> LayerNames;
};

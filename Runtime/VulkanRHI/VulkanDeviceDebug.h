#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanMemoryManager.h"

class FVulkanCommandBuffer;
class FVulkanQueue;

// -------------------------------------------------------------------------------------------
// Debug Utils (VK_EXT_debug_utils)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool GVulkanSupportsDebugUtils;

inline VkResult VulkanSetObjectName(VkDevice Device, const CHAR* Name, uint64 ObjectHandle, VkObjectType ObjectType)
{
#if VULKAN_ENABLE_DEBUG_NAMES && VK_EXT_debug_utils
    if (!GVulkanSupportsDebugUtils)
    {
        return VK_SUCCESS;
    }

    VkDebugUtilsObjectNameInfoEXT DebugUtilsObjectNameInfo = {};
    DebugUtilsObjectNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    DebugUtilsObjectNameInfo.pNext        = nullptr;
    DebugUtilsObjectNameInfo.pObjectName  = Name;
    DebugUtilsObjectNameInfo.objectHandle = ObjectHandle;
    DebugUtilsObjectNameInfo.objectType   = ObjectType;

    return vkSetDebugUtilsObjectNameEXT(Device, &DebugUtilsObjectNameInfo);
#else
    return VK_SUCCESS;
#endif
}

template<typename HandleType>
FORCEINLINE VkResult VulkanSetObjectName(VkDevice Device, const CHAR* Name, HandleType ObjectHandle, VkObjectType ObjectType)
{
    return VulkanSetObjectName(Device, Name, reinterpret_cast<uint64>(ObjectHandle), ObjectType);
}

void VulkanCreateDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT& OutMessenger);
void VulkanDestroyDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT& InOutMessenger);

// -------------------------------------------------------------------------------------------
// Device Lost Check
// -------------------------------------------------------------------------------------------

#if VULKAN_ENABLE_DEVICE_LOST_CHECK
VULKANRHI_API bool VulkanCheckDeviceLost(VkResult Result);
#endif

// -------------------------------------------------------------------------------------------
// Crash Markers
// -------------------------------------------------------------------------------------------

#if VULKAN_ENABLE_CRASH_MARKERS

enum class ECrashMarkerExtension
{
    None,
    AMDBufferMarker,
    NVCheckpoints,
};

class FVulkanCrashMarkers : public FVulkanDeviceChild
{
    static constexpr uint32 GPU_SLOTS      = 256;
    static constexpr uint32 RESERVED_SLOTS = 1;
    static constexpr uint32 DATA_SLOTS     = GPU_SLOTS - RESERVED_SLOTS;
    static constexpr uint32 SLOT_COUNTER   = 0;

public:
    FVulkanCrashMarkers(FVulkanDevice* InDevice);
    ~FVulkanCrashMarkers();

    bool Initialize(FVulkanQueue& InGraphicsQueue);
    void Release();

    void ResetMarkers(FVulkanCommandBuffer& CmdBuf);
    void WriteMarker(FVulkanCommandBuffer& CmdBuf, const FStringView& Name);
    void WriteEndMarker(FVulkanCommandBuffer& CmdBuf);
    void WriteDrawMarker(FVulkanCommandBuffer& CmdBuf, const FStringView& DrawType);
    void WriteSplitMarker(FVulkanCommandBuffer& CmdBuf);
    void DumpCrashMarkers();

private:
    void WriteMarkerInternal(FVulkanCommandBuffer& CmdBuf, const FStringView& Name);
    const FString& ResolveHash(uint32 Hash) const;

    ECrashMarkerExtension Extension;
    FVulkanQueue*         GraphicsQueue;
    FVulkanMemoryLocation MemoryLocation;
    uint32*               MappedData;
    uint32                NextIndex;
    uint32                DrawCounter;
    FString               CurrentRegion;
    TArray<FString>       StringPool;
    TMap<uint32, uint32>  HashToIndex;
};

#endif

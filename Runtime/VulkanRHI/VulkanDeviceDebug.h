#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanMemoryManager.h"

class FVulkanCommandBuffer;
class FVulkanQueue;

#if VULKAN_ENABLE_DEVICE_LOST_CHECK
VULKANRHI_API bool VulkanCheckDeviceLost(VkResult Result);
#endif

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
    FVulkanMemoryStorage  MemoryStorage;
    uint32*               MappedData;
    uint32                NextIndex;
    uint32                DrawCounter;
    FString               CurrentRegion;
    TArray<FString>       StringPool;
    TMap<uint32, uint32>  HashToIndex;
};

#endif

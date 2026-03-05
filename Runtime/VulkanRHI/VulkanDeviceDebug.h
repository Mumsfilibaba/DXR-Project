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
    static constexpr uint32 SENTINEL    = 0xDEADBEEF;
    static constexpr uint32 GPU_SLOTS   = 256;
    static constexpr uint32 DUMP_WINDOW = 64;

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
    TArray<uint32>        FrameHashes;
    TArray<uint32>        PrevFrameHashes;
    uint32                PrevNextIndex;
};

#endif

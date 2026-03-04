#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanMemoryManager.h"

class FVulkanCommandBuffer;

#if VULKAN_ENABLE_DEVICE_LOST_CHECK
VULKANRHI_API bool VulkanCheckDeviceLost(VkResult Result);
#endif

#if VULKAN_ENABLE_BREADCRUMBS

enum class EBreadcrumbBackend
{
    None,
    AMDBufferMarker,
    NVCheckpoints,
};

class FVulkanBreadcrumbs : public FVulkanDeviceChild
{
    static constexpr uint32 MAX_MARKERS = 1024;
    static constexpr uint32 SENTINEL    = 0xDEADBEEF;

public:
    FVulkanBreadcrumbs(FVulkanDevice* InDevice);
    ~FVulkanBreadcrumbs();

    bool Initialize(VkQueue InGraphicsQueue);
    void Release();

    void ResetMarkers(FVulkanCommandBuffer& CmdBuf);
    void WriteMarker(FVulkanCommandBuffer& CmdBuf, const FStringView& Name);
    void WriteDrawMarker(FVulkanCommandBuffer& CmdBuf, const FStringView& DrawType);
    void DumpBreadcrumbTrail();

    EBreadcrumbBackend GetBackend() const { return Backend; }

private:
    void WriteBreadcrumb(FVulkanCommandBuffer& CmdBuf, const FString& Name);

    EBreadcrumbBackend   Backend;
    VkQueue              GraphicsQueue;
    FVulkanMemoryStorage MemoryStorage;
    uint32*              MappedData;
    uint32               NextIndex;
    uint32               DrawCounter;
    FString              CurrentRegion;
    FString              MarkerNames[MAX_MARKERS];
};

#endif

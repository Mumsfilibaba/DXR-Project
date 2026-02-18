#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "VulkanRHI/VulkanQueue.h"

typedef TSharedRef<class FVulkanSurface> FVulkanSurfaceRef;

enum class ESurfaceStatus
{
    Ok,
    ZeroSized,   // Window has been minimized or no drawable size
    SurfaceLost, // Must recreate VkSurfaceKHR
    Error        // Unexpected error (log)
};

constexpr const CHAR* ToString(ESurfaceStatus Status)
{
    switch (Status)
    {
    case ESurfaceStatus::Ok:          return "ESurfaceStatus::Ok";
    case ESurfaceStatus::ZeroSized:   return "ESurfaceStatus::ZeroSized";
    case ESurfaceStatus::SurfaceLost: return "ESurfaceStatus::SurfaceLost";
    case ESurfaceStatus::Error:       return "ESurfaceStatus::Error";
    default:                          return "Unknown";
    }
}

class FVulkanSurface : public FVulkanDeviceChild, public FVulkanRefCounted
{
public:
    FVulkanSurface(FVulkanDevice* InDevice, FVulkanQueue& InQueue, void* InWindowHandle);
    ~FVulkanSurface();

    bool Initialize();

    ESurfaceStatus GetSupportedFormats(TArray<VkSurfaceFormatKHR>& OutSupportedFormats) const;
    ESurfaceStatus GetSupportedPresentModes(TArray<VkPresentModeKHR>& OutPresentModes) const;
    ESurfaceStatus GetCapabilities(VkSurfaceCapabilitiesKHR& OutCapabilities) const;

    const void* GetWindowHandle() const
    {
        return WindowHandle;
    }

    VkSurfaceKHR GetVkSurface() const
    {
        return Surface;
    }
    
private:
    VkSurfaceKHR  Surface;
    void*         WindowHandle;
    FVulkanQueue& Queue;
};

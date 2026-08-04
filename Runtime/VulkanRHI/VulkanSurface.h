#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/RefCountedBase.h"
#include "VulkanRHI/VulkanQueue.h"

typedef TSharedRef<class FVulkanSurface> FVulkanSurfaceRef;

enum class ESurfaceStatus : uint8
{
    Ok,

    /** Window has been minimized or no drawable size */
    ZeroSized,

    /** Must recreate VkSurfaceKHR */
    SurfaceLost,

    /** Unexpected error (log) */
    Error,
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

class FVulkanSurface : public FVulkanDeviceChild, public FRefCountedBase
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

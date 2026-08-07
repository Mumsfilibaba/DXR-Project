#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanLoader.h"

class FVulkanUnorderedAccessViewRHI;

enum class EVulkanBufferClearType : uint8
{
    Float = 0,
    Uint  = 1,
    Sint  = 2,
    Count = 3,
};

struct FVulkanBufferClearRegion
{
    VkBuffer     Buffer;
    VkDeviceSize Offset;
    VkDeviceSize Size;
    bool         bIsValid;
};

struct VulkanClearBufferUAV
{
    static FVulkanBufferClearRegion ResolveRegion(FVulkanUnorderedAccessViewRHI* View);
    static bool PackPattern(EBufferViewType ViewType, EFormat Format, const uint32 Values[4], bool bIsFloat, uint32& OutPattern);
    static bool GetClearType(EFormat Format, EVulkanBufferClearType& OutClearType);
};

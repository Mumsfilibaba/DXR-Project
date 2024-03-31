#include "VulkanRayTracing.h"

FVulkanRayTracingGeometry::FVulkanRayTracingGeometry(FVulkanDevice* InDevice, const FRHIRayTracingGeometryDesc& InDesc)
    : FRHIRayTracingGeometry(InDesc)
    , FVulkanDeviceChild(InDevice)
    , Geometry(VK_NULL_HANDLE)
{
}

FVulkanRayTracingGeometry::~FVulkanRayTracingGeometry()
{
    if (VULKAN_CHECK_HANDLE(Geometry))
    {
        vkDestroyAccelerationStructureKHR(GetDevice()->GetVkDevice(), Geometry, nullptr);
        Geometry = VK_NULL_HANDLE;
    }
}

bool FVulkanRayTracingGeometry::Initialize()
{
    return true;
}
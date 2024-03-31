#pragma once
#include "VulkanResourceViews.h"
#include "VulkanMemory.h"
#include "RHI/RHIRayTracing.h"
#include "Core/Containers/SharedRef.h"

typedef TSharedRef<class FVulkanRayTracingGeometry> FVulkanRayTracingGeometryRef; 

class FVulkanRayTracingGeometry : public FRHIRayTracingGeometry, public FVulkanDeviceChild
{
public:
    FVulkanRayTracingGeometry(FVulkanDevice* InDevice, const FRHIRayTracingGeometryDesc& InDesc);
    ~FVulkanRayTracingGeometry();

    bool Initialize();

    VkAccelerationStructureKHR GetVkAccelerationStructure() const
    {
        return Geometry;
    }

private:
    VkAccelerationStructureKHR Geometry;
};
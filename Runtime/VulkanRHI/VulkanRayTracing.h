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

    virtual void* GetRHIBaseBVHBuffer() override final { return reinterpret_cast<void*>(GeometryBuffer); }
    virtual void* GetRHIBaseAccelerationStructure() override final { return reinterpret_cast<void*>(Geometry); }

    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

    bool Build(FVulkanCommandContext& CmdContext, const FRayTracingGeometryBuildInfo& BuildInfo);

    VkAccelerationStructureKHR GetVkAccelerationStructure() const
    {
        return Geometry;
    }

private:
    VkAccelerationStructureKHR Geometry;
    VkDeviceAddress            GeometryDeviceAddress;
    VkBuffer                   GeometryBuffer;
    FVulkanMemoryAllocation    GeometryMemory;
    VkBuffer                   ScratchBuffer;
    FVulkanMemoryAllocation    ScratchMemory;
    FVulkanBufferRef           VertexBuffer;
    FVulkanBufferRef           IndexBuffer;
    FString                    DebugName;
};
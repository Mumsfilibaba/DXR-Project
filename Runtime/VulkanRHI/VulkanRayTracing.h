#pragma once
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIRayTracing.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanMemory.h"

typedef TSharedRef<class FVulkanRayTracingGeometry> FVulkanRayTracingGeometryRef; 

class FVulkanRayTracingGeometry : public FRHIRayTracingGeometry, public FVulkanDeviceChild
{
public:
    FVulkanRayTracingGeometry(FVulkanDevice* InDevice, const FRHIRayTracingGeometryInfo& InGeometryInfo);
    ~FVulkanRayTracingGeometry();

public:

    // FRHIRayTracingGeometry Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(Geometry); }
    virtual void* GetRHIBaseInterface() override final { return reinterpret_cast<void*>(this); }

    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

public:
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
#pragma once
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIRayTracing.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanMemoryManager.h"

typedef TSharedRef<class FVulkanGeometryAccelerationStructureRHI> FVulkanGeometryAccelerationStructureRHIRef; 

class FVulkanGeometryAccelerationStructureRHI : public FRHIGeometryAccelerationStructure, public FVulkanDeviceChild
{
public:
    FVulkanGeometryAccelerationStructureRHI(FVulkanDevice* InDevice, const FRHIGeometryAccelerationStructureDesc& InGeometryDesc);
    ~FVulkanGeometryAccelerationStructureRHI();
    
    bool Build(FVulkanCommandContext& CmdContext, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc);
    
    // FRHIGeometryAccelerationStructure Interface
    virtual void* GetRHINativeResource() const override final;

    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

    VkAccelerationStructureKHR GetVkAccelerationStructure() const
    {
        return Geometry;
    }

private:
    VkAccelerationStructureKHR Geometry;
    VkDeviceAddress            GeometryDeviceAddress;
    FVulkanMemoryLocation      GeometryLocation;
    FVulkanMemoryLocation      ScratchLocation;
    FVulkanBufferRHIRef        VertexBuffer;
    FVulkanBufferRHIRef        IndexBuffer;
#if VULKAN_STORE_DEBUG_NAMES
    FString                    DebugName;
#endif
};
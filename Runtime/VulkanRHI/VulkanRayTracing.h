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
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(Geometry); }

    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

    VkAccelerationStructureKHR GetVkAccelerationStructure() const
    {
        return Geometry;
    }

private:
    VkAccelerationStructureKHR Geometry;
    VkDeviceAddress            GeometryDeviceAddress;
    FVulkanMemoryStorage       GeometryStorage;
    FVulkanMemoryStorage       ScratchStorage;
    FVulkanBufferRHIRef        VertexBuffer;
    FVulkanBufferRHIRef        IndexBuffer;
    FString                    DebugName;
};
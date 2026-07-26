#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "RHI/RHIRayTracing.h"
#include "RHI/RayTracing/RHIOpacityMicromap.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanMemoryManager.h"

typedef TSharedRef<class FVulkanGeometryAccelerationStructureRHI>         FVulkanGeometryAccelerationStructureRHIRef; 
typedef TSharedRef<class FVulkanSceneAccelerationStructureRHI>            FVulkanSceneAccelerationStructureRHIRef;
typedef TSharedRef<class FVulkanOpacityMicromap>                          FVulkanOpacityMicromapRef;
typedef TSharedRef<class FVulkanClusterAccelerationStructureRHI>          FVulkanClusterAccelerationStructureRHIRef;
typedef TSharedRef<class FVulkanClusterTemplateRHI>                       FVulkanClusterTemplateRHIRef;
typedef TSharedRef<class FVulkanPartitionedSceneAccelerationStructureRHI> FVulkanPartitionedSceneAccelerationStructureRHIRef;

class FVulkanAccelerationStructure : public FVulkanDeviceChild
{
public:
    explicit FVulkanAccelerationStructure(FVulkanDevice* InDevice);
    virtual ~FVulkanAccelerationStructure();

    VkAccelerationStructureKHR GetVkAccelerationStructure() const
    {
        return AccelerationStructure;
    }

    VkDeviceAddress GetDeviceAddress() const
    {
        return DeviceAddress;
    }

protected:
    VkAccelerationStructureKHR AccelerationStructure;
    VkDeviceAddress            DeviceAddress;
};

class FVulkanGeometryAccelerationStructureRHI : public FRHIGeometryAccelerationStructure, public FVulkanAccelerationStructure
{
public:
    FVulkanGeometryAccelerationStructureRHI(FVulkanDevice* InDevice, const FRHIGeometryAccelerationStructureDesc& InGeometryDesc);
    ~FVulkanGeometryAccelerationStructureRHI();
        
    // FRHIGeometryAccelerationStructure Interface
    virtual void* GetRHINativeResource() const override final;
    
    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;
    
    bool Build(FVulkanCommandContext& CmdContext, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc);
    bool CompactInPlace(FVulkanCommandContext& CmdContext, uint64 CompactedSize);

private:
    FVulkanMemoryLocation      GeometryLocation;
    FVulkanMemoryLocation      ScratchLocation;
    FVulkanBufferRHIRef        VertexBuffer;
    FVulkanBufferRHIRef        IndexBuffer;
    VkAccelerationStructureKHR StaleGeometry;
    FVulkanMemoryLocation      StaleGeometryLocation;
    uint64                     TrackedAccelerationStructureMemory;
#if VULKAN_STORE_DEBUG_NAMES
    String                     DebugName;
#endif
};

class FVulkanSceneAccelerationStructureRHI : public FRHISceneAccelerationStructure, public FVulkanAccelerationStructure
{
public:
    FVulkanSceneAccelerationStructureRHI(FVulkanDevice* InDevice, const FRHISceneAccelerationStructureDesc& InSceneDesc);
    ~FVulkanSceneAccelerationStructureRHI();

    bool Build(FVulkanCommandContext& CmdContext, const FRHISceneAccelerationStructureBuildDesc& BuildDesc);
    
    // FRHISceneAccelerationStructure Interface
    virtual void* GetRHINativeResource() const override final;
    
    virtual FRHIShaderResourceView* GetShaderResourceView() const override final;
    virtual FRHIDescriptorHandle    GetBindlessHandle()     const override final;
    
    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

private:
    FVulkanMemoryLocation                             SceneLocation;
    FVulkanMemoryLocation                             ScratchLocation;
    FVulkanMemoryLocation                             InstanceLocation;
    uint32                                            InstanceCapacity;
    FVulkanShaderResourceViewRHIRef                   View;
    TArray<FRHIGeometryAccelerationStructureInstance> Instances;
    uint64                                            TrackedAccelerationStructureMemory;
#if VULKAN_STORE_DEBUG_NAMES
    String DebugName;
#endif
};

class FVulkanOpacityMicromap : public FRHIOpacityMicromap, public FVulkanDeviceChild
{
public:
    FVulkanOpacityMicromap(FVulkanDevice* InDevice, const FRHIOpacityMicromapDesc& InDesc);
    ~FVulkanOpacityMicromap();
    
    bool Build(FVulkanCommandContext& CmdContext, const FRHIOpacityMicromapBuildDesc& BuildDesc);
    
    // FRHIOpacityMicromap Interface
    virtual void* GetRHINativeResource() const override final;

#if VK_EXT_opacity_micromap
    VkMicromapEXT GetVkMicromap() const
    {
        return Micromap;
    }
#endif

private:
    FRHIOpacityMicromapDesc MicromapDesc;
#if VK_EXT_opacity_micromap
    VkMicromapEXT           Micromap;
#endif
    FVulkanMemoryLocation   MicromapLocation;
    FVulkanMemoryLocation   ScratchLocation;
    uint64                  TrackedMicromapMemory;
};

#if VK_NV_cluster_acceleration_structure
struct FVulkanClusterInputScratch
{
    VkClusterAccelerationStructureTriangleClusterInputNV     TriangleClusters;
    VkClusterAccelerationStructureClustersBottomLevelInputNV ClustersBottomLevel;
    VkClusterAccelerationStructureMoveObjectsInputNV         MoveObjects;
};
#endif

class FVulkanClusterAccelerationStructureRHI : public FRHIClusterAccelerationStructure, public FVulkanAccelerationStructure
{
public:
    FVulkanClusterAccelerationStructureRHI(FVulkanDevice* InDevice, const FRHIClusterAccelerationStructureDesc& InDesc);
    ~FVulkanClusterAccelerationStructureRHI();

    bool Initialize();

    // FRHIClusterAccelerationStructure Interface
    virtual void* GetRHINativeResource() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

private:
    FRHIClusterAccelerationStructureDesc ClusterDesc;
    FVulkanMemoryLocation                ResultLocation;
#if VULKAN_STORE_DEBUG_NAMES
    String                               DebugName;
#endif
};

class FVulkanClusterTemplateRHI : public FRHIClusterTemplate, public FVulkanDeviceChild
{
public:
    FVulkanClusterTemplateRHI(FVulkanDevice* InDevice, const FRHIClusterTemplateDesc& InDesc);
    ~FVulkanClusterTemplateRHI();

    bool Initialize();

    // FRHIClusterTemplate Interface
    virtual void* GetRHINativeResource() const override final;

    VkDeviceAddress GetDeviceAddress() const
    {
        return ResultLocation.GetDeviceAddress();
    }

private:
    FRHIClusterTemplateDesc ClusterTemplateDesc;
    FVulkanMemoryLocation   ResultLocation;
};

class FVulkanPartitionedSceneAccelerationStructureRHI : public FRHIPartitionedSceneAccelerationStructure, public FVulkanAccelerationStructure
{
public:
    FVulkanPartitionedSceneAccelerationStructureRHI(FVulkanDevice* InDevice, const FRHIRayTracingAccelerationStructurePartitionedSceneInputs& InInputs);
    ~FVulkanPartitionedSceneAccelerationStructureRHI();

    bool Initialize();

    // FRHIPartitionedSceneAccelerationStructure Interface
    virtual void* GetRHINativeResource() const override final;

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final;
    virtual FRHIDescriptorHandle    GetBindlessHandle()     const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

private:
    FRHIRayTracingAccelerationStructurePartitionedSceneInputs SceneInputs;
    FVulkanMemoryLocation                                     ResultLocation;
    FVulkanShaderResourceViewRHIRef                           View;
#if VULKAN_STORE_DEBUG_NAMES
    String DebugName;
#endif
};

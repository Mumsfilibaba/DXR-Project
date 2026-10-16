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
typedef TSharedRef<class FVulkanOpacityMicromapRHI>                       FVulkanOpacityMicromapRHIRef;
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
    
    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;
    
    bool Build(FVulkanCommandContext& CmdContext, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc);
    bool BuildSerialized(FVulkanCommandContext& CmdContext, const void* SerializedData, uint64 SerializedSizeInBytes);
    bool CompactInPlace(FVulkanCommandContext& CmdContext, uint64 CompactedSize);

private:
    bool AllocateResultStorage(uint64 Size, FVulkanMemoryLocation& OutLocation, VkAccelerationStructureKHR& OutAccelerationStructure, VkDeviceAddress& OutDeviceAddress);

    FORCEINLINE int64 GetCompactionSavings() const
    {
        return UncompactedSize > GeometryLocation.GetSize() ? static_cast<int64>(UncompactedSize - GeometryLocation.GetSize()) : 0;
    }

    FVulkanMemoryLocation                GeometryLocation;
    FVulkanMemoryLocation                ScratchLocation;
    FVulkanBufferRHIRef                  VertexBuffer;
    FVulkanBufferRHIRef                  IndexBuffer;
    FVulkanBufferRHIRef                  AABBBuffer;
    TArray<FVulkanOpacityMicromapRHIRef> OpacityMicromaps;
    TArray<FVulkanBufferRHIRef>          OpacityMicromapIndexBuffers;
    uint64                               TrackedAccelerationStructureMemory;
    uint64                               UncompactedSize;
#if VULKAN_STORE_DEBUG_NAMES
    FString                              DebugName;
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
    
    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

private:
    FVulkanMemoryLocation                             SceneLocation;
    FVulkanMemoryLocation                             ScratchLocation;
    FVulkanMemoryLocation                             InstanceLocation;
    uint32                                            InstanceCapacity;
    FVulkanShaderResourceViewRHIRef                   View;
    TArray<FRHIGeometryAccelerationStructureInstance> Instances;
    uint64                                            TrackedAccelerationStructureMemory;
#if VULKAN_STORE_DEBUG_NAMES
    FString                                           DebugName;
#endif
};

class FVulkanOpacityMicromapRHI : public FRHIOpacityMicromap, public FVulkanAccelerationStructure
{
public:
    FVulkanOpacityMicromapRHI(FVulkanDevice* InDevice, const FRHIOpacityMicromapDesc& InDesc);
    ~FVulkanOpacityMicromapRHI();

    // FRHIOpacityMicromap Interface
    virtual void* GetRHINativeResource() const override final;

    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

    bool Build(FVulkanCommandContext& CmdContext, const FRHIOpacityMicromapBuildDesc& BuildDesc);
    bool BuildSerialized(FVulkanCommandContext& CmdContext, const void* SerializedData, uint64 SerializedSizeInBytes);

    VkAccelerationStructureKHR GetVkMicromap() const
    {
        return AccelerationStructure;
    }

private:
    bool AllocateMicromapStorage(uint64 Size);

    FRHIOpacityMicromapDesc MicromapDesc;
    FVulkanMemoryLocation   MicromapLocation;
    FVulkanMemoryLocation   ScratchLocation;
    uint64                  TrackedMicromapMemory;
#if VULKAN_STORE_DEBUG_NAMES
    FString                 DebugName;
#endif
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

    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

private:
    FRHIClusterAccelerationStructureDesc ClusterDesc;
    FVulkanMemoryLocation                ResultLocation;
#if VULKAN_STORE_DEBUG_NAMES
    FString                              DebugName;
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

    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

private:
    FRHIRayTracingAccelerationStructurePartitionedSceneInputs SceneInputs;
    FVulkanMemoryLocation                                     ResultLocation;
    FVulkanShaderResourceViewRHIRef                           View;
#if VULKAN_STORE_DEBUG_NAMES
    FString                                                   DebugName;
#endif
};

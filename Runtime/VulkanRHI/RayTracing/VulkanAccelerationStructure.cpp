#include "VulkanRHI/RayTracing/VulkanAccelerationStructure.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "RHI/RHIStats.h"

static void VulkanUpdateAccelerationStructureMemoryStat(uint64& Tracked, uint64 NewSize)
{
    if (NewSize > Tracked)
    {
        STAT_ADD(STAT_RHI_AccelerationStructureMemory, NewSize - Tracked);
    }
    else if (NewSize < Tracked)
    {
        STAT_SUBTRACT(STAT_RHI_AccelerationStructureMemory, Tracked - NewSize);
    }

    Tracked = NewSize;
}

#if VK_EXT_opacity_micromap
static VkOpacityMicromapFormatEXT ConvertOpacityMicromapFormat(EOpacityMicromapFormat Format)
{
    return (Format == EOpacityMicromapFormat::OC1_4State)
        ? VK_OPACITY_MICROMAP_FORMAT_4_STATE_EXT
        : VK_OPACITY_MICROMAP_FORMAT_2_STATE_EXT;
}
#endif

FVulkanAccelerationStructure::FVulkanAccelerationStructure(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , AccelerationStructure(VK_NULL_HANDLE)
    , DeviceAddress(0)
{
}

FVulkanAccelerationStructure::~FVulkanAccelerationStructure() = default;

FVulkanGeometryAccelerationStructureRHI::FVulkanGeometryAccelerationStructureRHI(FVulkanDevice* InDevice, const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
    : FRHIGeometryAccelerationStructure(InGeometryDesc)
    , FVulkanAccelerationStructure(InDevice)
    , GeometryLocation(InDevice)
    , ScratchLocation(InDevice)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
    , StaleGeometry(VK_NULL_HANDLE)
    , StaleGeometryLocation(InDevice)
    , TrackedAccelerationStructureMemory(0)
#if VULKAN_STORE_DEBUG_NAMES
    , DebugName()
#endif
{
    STAT_ADD(STAT_RHI_BLASCount, 1);
}

FVulkanGeometryAccelerationStructureRHI::~FVulkanGeometryAccelerationStructureRHI()
{
    if (VULKAN_CHECK_HANDLE(StaleGeometry))
    {
        vkDestroyAccelerationStructureKHR(GetDevice()->GetVkDevice(), StaleGeometry, nullptr);
        StaleGeometry = VK_NULL_HANDLE;
    }

    if (VULKAN_CHECK_HANDLE(AccelerationStructure))
    {
        vkDestroyAccelerationStructureKHR(GetDevice()->GetVkDevice(), AccelerationStructure, nullptr);
        AccelerationStructure = VK_NULL_HANDLE;
    }

    VulkanUpdateAccelerationStructureMemoryStat(TrackedAccelerationStructureMemory, 0);
    STAT_SUBTRACT(STAT_RHI_BLASCount, 1);
}

void* FVulkanGeometryAccelerationStructureRHI::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(AccelerationStructure);
}

void FVulkanGeometryAccelerationStructureRHI::SetDebugName(const String& InName)
{
    if (VULKAN_CHECK_HANDLE(AccelerationStructure))
    {
        VulkanSetObjectName(GetDevice()->GetVkDevice(), InName.Data(), AccelerationStructure, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);
    }

#if VULKAN_STORE_DEBUG_NAMES
    DebugName = InName;
#endif
}

void FVulkanGeometryAccelerationStructureRHI::GetDebugName(String& OutDebugName) const
{
#if VULKAN_STORE_DEBUG_NAMES
    OutDebugName = DebugName;
#else
    OutDebugName.Clear();
#endif
}

bool FVulkanGeometryAccelerationStructureRHI::Build(FVulkanCommandContext& CmdContext, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc)
{
    VertexBuffer = MakeSharedRef<FVulkanBufferRHI>(BuildDesc.VertexBuffer);
    IndexBuffer  = MakeSharedRef<FVulkanBufferRHI>(BuildDesc.IndexBuffer);

    VkDeviceOrHostAddressConstKHR VertexData = {};
    VertexData.deviceAddress = VertexBuffer->GetDeviceAddress();

    VkDeviceOrHostAddressConstKHR IndexData = {};
    IndexData.deviceAddress = IndexBuffer->GetDeviceAddress();

    VkAccelerationStructureGeometryKHR AccelerationStructureGeometry = {};
    AccelerationStructureGeometry.sType                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    AccelerationStructureGeometry.flags                           = VK_GEOMETRY_OPAQUE_BIT_KHR;
    AccelerationStructureGeometry.geometryType                    = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    AccelerationStructureGeometry.geometry.triangles.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    AccelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    AccelerationStructureGeometry.geometry.triangles.maxVertex    = Math::Max<uint32>(BuildDesc.NumVertices - 1, 1);
    AccelerationStructureGeometry.geometry.triangles.vertexStride = VertexBuffer->GetDesc().Stride;
    AccelerationStructureGeometry.geometry.triangles.vertexData   = VertexData;
    AccelerationStructureGeometry.geometry.triangles.indexType    = ConvertIndexFormat(BuildDesc.IndexFormat);
    AccelerationStructureGeometry.geometry.triangles.indexData    = IndexData;

    VkAccelerationStructureBuildGeometryInfoKHR AccelerationStructureBuildGeometryInfo = {};
    AccelerationStructureBuildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    AccelerationStructureBuildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    AccelerationStructureBuildGeometryInfo.flags         = ConvertAccelerationStructureBuildFlags(GetFlags());
    AccelerationStructureBuildGeometryInfo.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    AccelerationStructureBuildGeometryInfo.geometryCount = 1;
    AccelerationStructureBuildGeometryInfo.pGeometries   = &AccelerationStructureGeometry;

    VkAccelerationStructureBuildSizesInfoKHR AccelerationStructureBuildSizesInfo = {};
    AccelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    const uint32 NumTriangles = BuildDesc.NumIndices / 3;
    if ((BuildDesc.NumIndices % 3) != 0)
    {
        VULKAN_WARNING("Creating acceleration structure with an indexcount that is not a multiple of 3");
    }

    vkGetAccelerationStructureBuildSizesKHR(
        GetDevice()->GetVkDevice(), 
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
        &AccelerationStructureBuildGeometryInfo, 
        &NumTriangles, 
        &AccelerationStructureBuildSizesInfo);

    const VkMemoryAllocateFlags AllocateFlags    = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    const VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    const VkBufferUsageFlags    GeometryUsage    = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    const VkBufferUsageFlags    ScratchUsage     = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(MemoryProperties, GeometryUsage, AllocateFlags, 
        AccelerationStructureBuildSizesInfo.accelerationStructureSize, 256, GeometryLocation))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate geometry buffer memory");
        return false;
    }

    VkAccelerationStructureCreateInfoKHR AccelerationStructureCreateInfo = {};
    AccelerationStructureCreateInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    AccelerationStructureCreateInfo.createFlags   = 0;
    AccelerationStructureCreateInfo.buffer        = GeometryLocation.GetBackingBuffer();
    AccelerationStructureCreateInfo.offset        = GeometryLocation.GetBufferOffset();
    AccelerationStructureCreateInfo.size          = AccelerationStructureBuildSizesInfo.accelerationStructureSize;
    AccelerationStructureCreateInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    AccelerationStructureCreateInfo.deviceAddress = 0;

    VkResult Result = vkCreateAccelerationStructureKHR(GetDevice()->GetVkDevice(), &AccelerationStructureCreateInfo, nullptr, &AccelerationStructure);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create AccelerationStructure");
        return false;
    }

    if (!MemoryManager.AllocateBufferMemory(MemoryProperties, ScratchUsage, AllocateFlags, 
        AccelerationStructureBuildSizesInfo.buildScratchSize, 256, ScratchLocation))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate scratch-buffer memory");
        return false;
    }

    VulkanUpdateAccelerationStructureMemoryStat(TrackedAccelerationStructureMemory, GeometryLocation.GetSize() + ScratchLocation.GetSize() + StaleGeometryLocation.GetSize());

    AccelerationStructureBuildGeometryInfo.dstAccelerationStructure  = AccelerationStructure;
    AccelerationStructureBuildGeometryInfo.scratchData.deviceAddress = ScratchLocation.GetDeviceAddress();

    VkAccelerationStructureBuildRangeInfoKHR AccelerationStructureBuildRangeInfo = {};
    AccelerationStructureBuildRangeInfo.primitiveCount  = NumTriangles;
    AccelerationStructureBuildRangeInfo.primitiveOffset = 0;
    AccelerationStructureBuildRangeInfo.firstVertex     = 0;
    AccelerationStructureBuildRangeInfo.transformOffset = 0;

    VkAccelerationStructureBuildRangeInfoKHR* BuildRangeInfos[] = { &AccelerationStructureBuildRangeInfo };
    CmdContext.GetCommandBuffer()->BuildAccelerationStructures(1, &AccelerationStructureBuildGeometryInfo, BuildRangeInfos);

    STAT_ADD(STAT_RHI_AccelerationStructureBuilds, 1);

    VkAccelerationStructureDeviceAddressInfoKHR AccelerationDeviceAddressInfo = {};
    AccelerationDeviceAddressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    AccelerationDeviceAddressInfo.accelerationStructure = AccelerationStructure;

    DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(GetDevice()->GetVkDevice(), &AccelerationDeviceAddressInfo);
    if (DeviceAddress == 0)
    {
        VULKAN_ERROR_CRITICAL("GetAccelerationStructureDeviceAddress returned an invalid DeviceAddress");
        return false;
    }

    return true;
}

bool FVulkanGeometryAccelerationStructureRHI::CompactInPlace(FVulkanCommandContext& CmdContext, uint64 CompactedSize)
{
    if (CompactedSize == 0 || !VULKAN_CHECK_HANDLE(AccelerationStructure) || !vkCmdCopyAccelerationStructureKHR)
    {
        return false;
    }

    const VkMemoryAllocateFlags  AllocateFlags    = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    const VkMemoryPropertyFlags  MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    const VkBufferUsageFlags     GeometryUsage    = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();

    FVulkanMemoryLocation CompactedLocation(GetDevice());
    if (!MemoryManager.AllocateBufferMemory(MemoryProperties, GeometryUsage, AllocateFlags, CompactedSize, 256, CompactedLocation))
    {
        VULKAN_WARNING("CompactInPlace: failed to allocate compacted geometry buffer");
        return false;
    }

    VkAccelerationStructureCreateInfoKHR CreateInfo = {};
    CreateInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    CreateInfo.buffer        = CompactedLocation.GetBackingBuffer();
    CreateInfo.offset        = CompactedLocation.GetBufferOffset();
    CreateInfo.size          = CompactedSize;
    CreateInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    CreateInfo.deviceAddress = 0;

    VkAccelerationStructureKHR CompactedGeometry = VK_NULL_HANDLE;
    if (VULKAN_FAILED(vkCreateAccelerationStructureKHR(GetDevice()->GetVkDevice(), &CreateInfo, nullptr, &CompactedGeometry)))
    {
        CompactedLocation.ReleaseMemory();
        VULKAN_WARNING("CompactInPlace: failed to create compacted acceleration structure");
        return false;
    }

    VkCopyAccelerationStructureInfoKHR CopyInfo = {};
    CopyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
    CopyInfo.src   = AccelerationStructure;
    CopyInfo.dst   = CompactedGeometry;
    CopyInfo.mode  = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;

    CmdContext.GetCommandBuffer()->CopyAccelerationStructure(&CopyInfo);

    VkAccelerationStructureDeviceAddressInfoKHR AddressInfo = {};
    AddressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    AddressInfo.accelerationStructure = CompactedGeometry;
    
    const VkDeviceAddress CompactedAddress = vkGetAccelerationStructureDeviceAddressKHR(GetDevice()->GetVkDevice(), &AddressInfo);
    if (VULKAN_CHECK_HANDLE(StaleGeometry))
    {
        vkDestroyAccelerationStructureKHR(GetDevice()->GetVkDevice(), StaleGeometry, nullptr);
        StaleGeometry = VK_NULL_HANDLE;
    }

    StaleGeometryLocation.ReleaseMemory();

    StaleGeometry = AccelerationStructure;
    StaleGeometryLocation.Swap(GeometryLocation);

    AccelerationStructure = CompactedGeometry;
    GeometryLocation.Swap(CompactedLocation);

    DeviceAddress = CompactedAddress;

    VulkanUpdateAccelerationStructureMemoryStat(TrackedAccelerationStructureMemory, GeometryLocation.GetSize() + ScratchLocation.GetSize() + StaleGeometryLocation.GetSize());
    return true;
}

FVulkanSceneAccelerationStructureRHI::FVulkanSceneAccelerationStructureRHI(FVulkanDevice* InDevice, const FRHISceneAccelerationStructureDesc& InSceneDesc)
    : FRHISceneAccelerationStructure(InSceneDesc)
    , FVulkanAccelerationStructure(InDevice)
    , SceneLocation(InDevice)
    , ScratchLocation(InDevice)
    , InstanceLocation(InDevice)
    , InstanceCapacity(0)
    , View(nullptr)
    , Instances()
    , TrackedAccelerationStructureMemory(0)
#if VULKAN_STORE_DEBUG_NAMES
    , DebugName()
#endif
{
    STAT_ADD(STAT_RHI_TLASCount, 1);
}

FVulkanSceneAccelerationStructureRHI::~FVulkanSceneAccelerationStructureRHI()
{
    if (VULKAN_CHECK_HANDLE(AccelerationStructure))
    {
        vkDestroyAccelerationStructureKHR(GetDevice()->GetVkDevice(), AccelerationStructure, nullptr);
        AccelerationStructure = VK_NULL_HANDLE;
    }

    VulkanUpdateAccelerationStructureMemoryStat(TrackedAccelerationStructureMemory, 0);
    STAT_SUBTRACT(STAT_RHI_TLASCount, 1);
}

void* FVulkanSceneAccelerationStructureRHI::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(AccelerationStructure);
}

FRHIShaderResourceView* FVulkanSceneAccelerationStructureRHI::GetShaderResourceView() const
{
    return View.Get();
}

FRHIDescriptorHandle FVulkanSceneAccelerationStructureRHI::GetBindlessHandle() const
{
    return View ? View->GetBindlessHandle() : FRHIDescriptorHandle();
}

void FVulkanSceneAccelerationStructureRHI::SetDebugName(const String& InName)
{
    if (VULKAN_CHECK_HANDLE(AccelerationStructure))
    {
        VulkanSetObjectName(GetDevice()->GetVkDevice(), InName.Data(), AccelerationStructure, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);
    }

#if VULKAN_STORE_DEBUG_NAMES
    DebugName = InName;
#endif
}

void FVulkanSceneAccelerationStructureRHI::GetDebugName(String& OutDebugName) const
{
#if VULKAN_STORE_DEBUG_NAMES
    OutDebugName = DebugName;
#else
    OutDebugName.Clear();
#endif
}

bool FVulkanSceneAccelerationStructureRHI::Build(FVulkanCommandContext& CmdContext, const FRHISceneAccelerationStructureBuildDesc& BuildDesc)
{
    const uint32 NumInstances = BuildDesc.NumInstances;

    const VkDeviceSize          InstanceBufferSize = Math::Max<VkDeviceSize>(uint64(NumInstances) * sizeof(VkAccelerationStructureInstanceKHR), sizeof(VkAccelerationStructureInstanceKHR));
    const VkMemoryAllocateFlags AllocateFlags      = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    
    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (InstanceCapacity < NumInstances || !InstanceLocation.IsValid())
    {
        InstanceLocation.ReleaseMemory();

        const VkMemoryPropertyFlags InstanceMemoryProperties = 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        const VkBufferUsageFlags InstanceUsage = 
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        if (!MemoryManager.AllocateBufferMemory(InstanceMemoryProperties, InstanceUsage, AllocateFlags, InstanceBufferSize, 16, InstanceLocation))
        {
            VULKAN_ERROR_CRITICAL("Failed to allocate instance-buffer memory");
            return false;
        }

        InstanceCapacity = NumInstances;
    }

    uint32 OutCount = 0;
    if (uint8* MappedInstances = reinterpret_cast<uint8*>(InstanceLocation.GetMappedBaseAddress()))
    {
        VkAccelerationStructureInstanceKHR* InstanceData = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(MappedInstances);
        for (uint32 Index = 0; Index < NumInstances; ++Index)
        {
            const FRHIGeometryAccelerationStructureInstance& Instance = BuildDesc.Instances[Index];
            
            FVulkanGeometryAccelerationStructureRHI* VulkanGeometry = FVulkanDeviceRHI::ResourceCast(Instance.Geometry);
            if (!VulkanGeometry)
            {
                VULKAN_WARNING("TLAS build skipping instance %u with null geometry (no BLAS)", Index);
                continue;
            }

            VkAccelerationStructureInstanceKHR& OutInstance = InstanceData[OutCount++];
            Memory::Memzero(&OutInstance, sizeof(VkAccelerationStructureInstanceKHR));

            Memory::Memcpy(&OutInstance.transform, &Instance.Transform, sizeof(VkTransformMatrixKHR));
            OutInstance.instanceCustomIndex                    = Instance.InstanceIndex & 0xFFFFFF;
            OutInstance.mask                                   = Instance.Mask & 0xFF;
            OutInstance.instanceShaderBindingTableRecordOffset = Instance.HitGroupIndex & 0xFFFFFF;
            OutInstance.flags                                  = ConvertRayTracingInstanceFlags(Instance.Flags) & 0xFF;
            OutInstance.accelerationStructureReference         = VulkanGeometry->GetDeviceAddress();
        }
    }
    else
    {
        VULKAN_ERROR_CRITICAL("Instance-buffer memory is not host-visible");
        return false;
    }

    VkAccelerationStructureGeometryKHR AccelerationStructureGeometry = {};
    AccelerationStructureGeometry.sType                                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    AccelerationStructureGeometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    AccelerationStructureGeometry.flags                                 = VK_GEOMETRY_OPAQUE_BIT_KHR;
    AccelerationStructureGeometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    AccelerationStructureGeometry.geometry.instances.arrayOfPointers    = VK_FALSE;
    AccelerationStructureGeometry.geometry.instances.data.deviceAddress = InstanceLocation.GetDeviceAddress();

    const VkBuildAccelerationStructureModeKHR BuildMode = (BuildDesc.bUpdate && VULKAN_CHECK_HANDLE(AccelerationStructure))
        ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR
        : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

    VkAccelerationStructureBuildGeometryInfoKHR BuildGeometryInfo = {};
    BuildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    BuildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    BuildGeometryInfo.flags         = ConvertAccelerationStructureBuildFlags(GetFlags());
    BuildGeometryInfo.mode          = BuildMode;
    BuildGeometryInfo.geometryCount = 1;
    BuildGeometryInfo.pGeometries   = &AccelerationStructureGeometry;

    VkAccelerationStructureBuildSizesInfoKHR BuildSizesInfo = {};
    BuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    vkGetAccelerationStructureBuildSizesKHR(
        GetDevice()->GetVkDevice(), 
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
        &BuildGeometryInfo, 
        &OutCount, 
        &BuildSizesInfo);

    const VkMemoryPropertyFlags DeviceMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    const VkBufferUsageFlags    SceneUsage             = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    const VkBufferUsageFlags    ScratchUsage           = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (!VULKAN_CHECK_HANDLE(AccelerationStructure) || SceneLocation.GetSize() < BuildSizesInfo.accelerationStructureSize)
    {
        if (VULKAN_CHECK_HANDLE(AccelerationStructure))
        {
            vkDestroyAccelerationStructureKHR(GetDevice()->GetVkDevice(), AccelerationStructure, nullptr);
            AccelerationStructure = VK_NULL_HANDLE;
            SceneLocation.ReleaseMemory();
        }

        if (!MemoryManager.AllocateBufferMemory(DeviceMemoryProperties, SceneUsage, AllocateFlags, BuildSizesInfo.accelerationStructureSize, 256, SceneLocation))
        {
            VULKAN_ERROR_CRITICAL("Failed to allocate scene buffer memory");
            return false;
        }

        VkAccelerationStructureCreateInfoKHR CreateInfo = {};
        CreateInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        CreateInfo.buffer = SceneLocation.GetBackingBuffer();
        CreateInfo.offset = SceneLocation.GetBufferOffset();
        CreateInfo.size   = BuildSizesInfo.accelerationStructureSize;
        CreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

        VkResult Result = vkCreateAccelerationStructureKHR(GetDevice()->GetVkDevice(), &CreateInfo, nullptr, &AccelerationStructure);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("Failed to create scene AccelerationStructure");
            return false;
        }
    }

    const VkDeviceSize RequiredScratchSize = Math::Max<VkDeviceSize>(BuildSizesInfo.buildScratchSize, BuildSizesInfo.updateScratchSize);
    if (ScratchLocation.GetSize() < RequiredScratchSize || !ScratchLocation.IsValid())
    {
        ScratchLocation.ReleaseMemory();
        if (!MemoryManager.AllocateBufferMemory(DeviceMemoryProperties, ScratchUsage, AllocateFlags, RequiredScratchSize, 256, ScratchLocation))
        {
            VULKAN_ERROR_CRITICAL("Failed to allocate scene scratch-buffer memory");
            return false;
        }
    }

    VulkanUpdateAccelerationStructureMemoryStat(TrackedAccelerationStructureMemory, SceneLocation.GetSize() + ScratchLocation.GetSize() + InstanceLocation.GetSize());

    BuildGeometryInfo.dstAccelerationStructure  = AccelerationStructure;
    BuildGeometryInfo.scratchData.deviceAddress = ScratchLocation.GetDeviceAddress();

    if (BuildMode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR)
    {
        BuildGeometryInfo.srcAccelerationStructure = AccelerationStructure;
    }

    VkAccelerationStructureBuildRangeInfoKHR BuildRangeInfo = {};
    BuildRangeInfo.primitiveCount  = OutCount;
    BuildRangeInfo.primitiveOffset = 0;
    BuildRangeInfo.firstVertex     = 0;
    BuildRangeInfo.transformOffset = 0;

    VkAccelerationStructureBuildRangeInfoKHR* BuildRangeInfos[] = { &BuildRangeInfo };
    CmdContext.GetCommandBuffer()->BuildAccelerationStructures(1, &BuildGeometryInfo, BuildRangeInfos);

    STAT_ADD(STAT_RHI_AccelerationStructureBuilds, 1);

    VkAccelerationStructureDeviceAddressInfoKHR AddressInfo = {};
    AddressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    AddressInfo.accelerationStructure = AccelerationStructure;

    DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(GetDevice()->GetVkDevice(), &AddressInfo);

    if (!View)
    {
        View = new FVulkanShaderResourceViewRHI(GetDevice(), this, FRHIShaderResourceViewDesc::CreateAccelerationStructure());
        if (!View->Initialize(this, FRHIShaderResourceViewDesc::CreateAccelerationStructure()))
        {
            VULKAN_ERROR_CRITICAL("Failed to create scene acceleration-structure view");
            return false;
        }
    }

    Instances.Reset(BuildDesc.Instances, NumInstances);
    return true;
}

FVulkanOpacityMicromap::FVulkanOpacityMicromap(FVulkanDevice* InDevice, const FRHIOpacityMicromapDesc& InDesc)
    : FRHIOpacityMicromap(InDesc)
    , FVulkanDeviceChild(InDevice)
    , MicromapDesc(InDesc)
#if VK_EXT_opacity_micromap
    , Micromap(VK_NULL_HANDLE)
#endif
    , MicromapLocation(InDevice)
    , ScratchLocation(InDevice)
    , TrackedMicromapMemory(0)
{
}

FVulkanOpacityMicromap::~FVulkanOpacityMicromap()
{
#if VK_EXT_opacity_micromap
    if (VULKAN_CHECK_HANDLE(Micromap) && vkDestroyMicromapEXT)
    {
        vkDestroyMicromapEXT(GetDevice()->GetVkDevice(), Micromap, nullptr);
        Micromap = VK_NULL_HANDLE;
    }
#endif

    VulkanUpdateAccelerationStructureMemoryStat(TrackedMicromapMemory, 0);
}

void* FVulkanOpacityMicromap::GetRHINativeResource() const
{
#if VK_EXT_opacity_micromap
    return reinterpret_cast<void*>(Micromap);
#else
    return nullptr;
#endif
}

bool FVulkanOpacityMicromap::Build(FVulkanCommandContext& CmdContext, const FRHIOpacityMicromapBuildDesc& BuildDesc)
{
#if VK_EXT_opacity_micromap
    if (!vkGetMicromapBuildSizesEXT || !vkCreateMicromapEXT || !vkCmdBuildMicromapsEXT)
    {
        VULKAN_ERROR_CRITICAL("Opacity-micromap extension functions are not available");
        return false;
    }

    // Usage counts either come from the histogram or from a single uniform entry.
    TArray<VkMicromapUsageEXT> UsageCounts;
    if (BuildDesc.HistogramEntries.Size() > 0)
    {
        UsageCounts.Reserve(BuildDesc.HistogramEntries.Size());

        for (const FRHIOpacityMicromapHistogramEntry& Entry : BuildDesc.HistogramEntries)
        {
            VkMicromapUsageEXT Usage = {};
            Usage.count            = Entry.Count;
            Usage.subdivisionLevel = Entry.SubdivisionLevel;
            Usage.format           = ConvertOpacityMicromapFormat(Entry.Format);

            UsageCounts.Add(Usage);
        }
    }
    else
    {
        VkMicromapUsageEXT Usage = {};
        Usage.count            = BuildDesc.NumOpacityMicromaps;
        Usage.subdivisionLevel = MicromapDesc.SubdivisionLevel;
        Usage.format           = ConvertOpacityMicromapFormat(MicromapDesc.Format);

        UsageCounts.Add(Usage);
    }

    VkMicromapBuildInfoEXT BuildInfo = {};
    BuildInfo.sType            = VK_STRUCTURE_TYPE_MICROMAP_BUILD_INFO_EXT;
    BuildInfo.type             = VK_MICROMAP_TYPE_OPACITY_MICROMAP_EXT;
    BuildInfo.mode             = VK_BUILD_MICROMAP_MODE_BUILD_EXT;
    BuildInfo.usageCountsCount = static_cast<uint32>(UsageCounts.Size());
    BuildInfo.pUsageCounts     = UsageCounts.Data();

    VkMicromapBuildSizesInfoEXT SizesInfo = {};
    SizesInfo.sType = VK_STRUCTURE_TYPE_MICROMAP_BUILD_SIZES_INFO_EXT;

    vkGetMicromapBuildSizesEXT(GetDevice()->GetVkDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &BuildInfo, &SizesInfo);

    const VkMemoryAllocateFlags AllocateFlags    = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    const VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    const VkBufferUsageFlags    MicromapUsage    = VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    const VkBufferUsageFlags    ScratchUsage     = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(MemoryProperties, MicromapUsage, AllocateFlags, SizesInfo.micromapSize, 256, MicromapLocation))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate opacity-micromap buffer memory");
        return false;
    }

    VkMicromapCreateInfoEXT CreateInfo = {};
    CreateInfo.sType  = VK_STRUCTURE_TYPE_MICROMAP_CREATE_INFO_EXT;
    CreateInfo.buffer = MicromapLocation.GetBackingBuffer();
    CreateInfo.offset = MicromapLocation.GetBufferOffset();
    CreateInfo.size   = SizesInfo.micromapSize;
    CreateInfo.type   = VK_MICROMAP_TYPE_OPACITY_MICROMAP_EXT;
    
    if (VULKAN_FAILED(vkCreateMicromapEXT(GetDevice()->GetVkDevice(), &CreateInfo, nullptr, &Micromap)))
    {
        VULKAN_ERROR_CRITICAL("Failed to create opacity micromap");
        return false;
    }

    if (!MemoryManager.AllocateBufferMemory(MemoryProperties, ScratchUsage, AllocateFlags, SizesInfo.buildScratchSize, 256, ScratchLocation))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate opacity-micromap scratch memory");
        return false;
    }

    VulkanUpdateAccelerationStructureMemoryStat(TrackedMicromapMemory, MicromapLocation.GetSize() + ScratchLocation.GetSize());

    VkDeviceOrHostAddressConstKHR MicroTriangleData = {};
    if (FVulkanBufferRHI* DataBuffer = FVulkanDeviceRHI::ResourceCast(BuildDesc.OpacityMicroTriangleDataBuffer))
    {
        MicroTriangleData.deviceAddress = DataBuffer->GetDeviceAddress() + BuildDesc.OpacityMicroTriangleDataBufferOffset;
    }

    VkDeviceOrHostAddressConstKHR TriangleArray = {};
    if (FVulkanBufferRHI* DescriptorBuffer = FVulkanDeviceRHI::ResourceCast(BuildDesc.OMMDescriptorBuffer))
    {
        TriangleArray.deviceAddress = DescriptorBuffer->GetDeviceAddress() + BuildDesc.OMMDescriptorBufferOffset;
    }

    BuildInfo.dstMicromap               = Micromap;
    BuildInfo.data                      = MicroTriangleData;
    BuildInfo.triangleArray             = TriangleArray;
    BuildInfo.triangleArrayStride       = BuildDesc.OMMDescriptorStrideInBytes;
    BuildInfo.scratchData.deviceAddress = ScratchLocation.GetDeviceAddress();

    CmdContext.GetCommandBuffer()->BuildMicromaps(1, &BuildInfo);
    return true;
#else
    UNREFERENCED_VARIABLE(CmdContext);
    UNREFERENCED_VARIABLE(BuildDesc);
    return false;
#endif
}

FVulkanClusterAccelerationStructureRHI::FVulkanClusterAccelerationStructureRHI(FVulkanDevice* InDevice, const FRHIClusterAccelerationStructureDesc& InDesc)
    : FRHIClusterAccelerationStructure(InDesc)
    , FVulkanAccelerationStructure(InDevice)
    , ClusterDesc(InDesc)
    , ResultLocation(InDevice)
{
}

FVulkanClusterAccelerationStructureRHI::~FVulkanClusterAccelerationStructureRHI()
{
    ResultLocation.ReleaseMemory();
}

void* FVulkanClusterAccelerationStructureRHI::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(ResultLocation.GetBackingBuffer());
}

void FVulkanClusterAccelerationStructureRHI::SetDebugName(const String& InName)
{
#if VULKAN_STORE_DEBUG_NAMES
    DebugName = InName;
#else
    UNREFERENCED_VARIABLE(InName);
#endif
}

void FVulkanClusterAccelerationStructureRHI::GetDebugName(String& OutDebugName) const
{
#if VULKAN_STORE_DEBUG_NAMES
    OutDebugName = DebugName;
#else
    OutDebugName.Clear();
#endif
}

bool FVulkanClusterAccelerationStructureRHI::Initialize()
{
#if VK_NV_cluster_acceleration_structure
    if (!vkGetClusterAccelerationStructureBuildSizesNV)
    {
        VULKAN_ERROR_CRITICAL("Cluster acceleration-structure extension functions are not available");
        return false;
    }

    FVulkanClusterInputScratch Scratch = {};
    Scratch.TriangleClusters.sType                         = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_TRIANGLE_CLUSTER_INPUT_NV;
    Scratch.TriangleClusters.vertexFormat                  = VK_FORMAT_R32G32B32_SFLOAT;
    Scratch.TriangleClusters.maxGeometryIndexValue         = ClusterDesc.ClusterLimits.MaxGeometryIndex;
    Scratch.TriangleClusters.maxClusterUniqueGeometryCount = 1;
    Scratch.TriangleClusters.maxClusterTriangleCount       = ClusterDesc.ClusterLimits.MaxTrianglesPerCluster;
    Scratch.TriangleClusters.maxClusterVertexCount         = ClusterDesc.ClusterLimits.MaxVerticesPerCluster;
    Scratch.TriangleClusters.maxTotalTriangleCount         = ClusterDesc.ClusterLimits.MaxTrianglesPerCluster * ClusterDesc.ClusterLimits.MaxClusterCount;
    Scratch.TriangleClusters.maxTotalVertexCount           = ClusterDesc.ClusterLimits.MaxVerticesPerCluster * ClusterDesc.ClusterLimits.MaxClusterCount;
    Scratch.TriangleClusters.minPositionTruncateBitCount   = 0;

    VkClusterAccelerationStructureInputInfoNV InputInfo = {};
    InputInfo.sType                         = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_INPUT_INFO_NV;
    InputInfo.maxAccelerationStructureCount = Math::Max<uint32>(ClusterDesc.ClusterLimits.MaxClusterCount, 1u);
    InputInfo.flags                         = ConvertAccelerationStructureBuildFlags(ClusterDesc.Flags);
    InputInfo.opType                        = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_TRIANGLE_CLUSTER_NV;
    InputInfo.opInput.pTriangleClusters     = &Scratch.TriangleClusters;

    VkAccelerationStructureBuildSizesInfoKHR SizesInfo = {};
    SizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    vkGetClusterAccelerationStructureBuildSizesNV(GetDevice()->GetVkDevice(), &InputInfo, &SizesInfo);

    const VkDeviceSize          ResultSize       = Math::Max<VkDeviceSize>(SizesInfo.accelerationStructureSize, 1);
    const VkMemoryAllocateFlags AllocateFlags    = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    const VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    const VkBufferUsageFlags    Usage            = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (!GetDevice()->GetMemoryManager().AllocateBufferMemory(MemoryProperties, Usage, AllocateFlags, ResultSize, 256, ResultLocation))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate cluster acceleration-structure memory");
        return false;
    }

    DeviceAddress = ResultLocation.GetDeviceAddress();
    return true;
#else
    return false;
#endif
}

FVulkanClusterTemplateRHI::FVulkanClusterTemplateRHI(FVulkanDevice* InDevice, const FRHIClusterTemplateDesc& InDesc)
    : FRHIClusterTemplate(InDesc)
    , FVulkanDeviceChild(InDevice)
    , ClusterTemplateDesc(InDesc)
    , ResultLocation(InDevice)
{
}

FVulkanClusterTemplateRHI::~FVulkanClusterTemplateRHI()
{
    ResultLocation.ReleaseMemory();
}

void* FVulkanClusterTemplateRHI::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(ResultLocation.GetBackingBuffer());
}

bool FVulkanClusterTemplateRHI::Initialize()
{
#if VK_NV_cluster_acceleration_structure
    if (!vkGetClusterAccelerationStructureBuildSizesNV)
    {
        VULKAN_ERROR_CRITICAL("Cluster acceleration-structure extension functions are not available");
        return false;
    }

    FVulkanClusterInputScratch Scratch = {};
    Scratch.TriangleClusters.sType                         = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_TRIANGLE_CLUSTER_INPUT_NV;
    Scratch.TriangleClusters.vertexFormat                  = VK_FORMAT_R32G32B32_SFLOAT;
    Scratch.TriangleClusters.maxGeometryIndexValue         = ClusterTemplateDesc.ClusterLimits.MaxGeometryIndex;
    Scratch.TriangleClusters.maxClusterUniqueGeometryCount = 1;
    Scratch.TriangleClusters.maxClusterTriangleCount       = ClusterTemplateDesc.ClusterLimits.MaxTrianglesPerCluster;
    Scratch.TriangleClusters.maxClusterVertexCount         = ClusterTemplateDesc.ClusterLimits.MaxVerticesPerCluster;
    Scratch.TriangleClusters.maxTotalTriangleCount         = ClusterTemplateDesc.ClusterLimits.MaxTrianglesPerCluster * ClusterTemplateDesc.ClusterLimits.MaxClusterCount;
    Scratch.TriangleClusters.maxTotalVertexCount           = ClusterTemplateDesc.ClusterLimits.MaxVerticesPerCluster * ClusterTemplateDesc.ClusterLimits.MaxClusterCount;
    Scratch.TriangleClusters.minPositionTruncateBitCount   = 0;

    VkClusterAccelerationStructureInputInfoNV InputInfo = {};
    InputInfo.sType                         = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_INPUT_INFO_NV;
    InputInfo.maxAccelerationStructureCount = Math::Max<uint32>(ClusterTemplateDesc.ClusterLimits.MaxClusterCount, 1u);
    InputInfo.flags                         = ConvertAccelerationStructureBuildFlags(ClusterTemplateDesc.Flags);
    InputInfo.opType                        = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_TRIANGLE_CLUSTER_TEMPLATE_NV;
    InputInfo.opInput.pTriangleClusters     = &Scratch.TriangleClusters;

    VkAccelerationStructureBuildSizesInfoKHR SizesInfo = {};
    SizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    vkGetClusterAccelerationStructureBuildSizesNV(GetDevice()->GetVkDevice(), &InputInfo, &SizesInfo);

    const VkDeviceSize          ResultSize       = Math::Max<VkDeviceSize>(SizesInfo.accelerationStructureSize, 1);
    const VkMemoryAllocateFlags AllocateFlags    = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    const VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    const VkBufferUsageFlags    Usage            = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (!GetDevice()->GetMemoryManager().AllocateBufferMemory(MemoryProperties, Usage, AllocateFlags, ResultSize, 256, ResultLocation))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate cluster-template memory");
        return false;
    }

    return true;
#else
    return false;
#endif
}

FVulkanPartitionedSceneAccelerationStructureRHI::FVulkanPartitionedSceneAccelerationStructureRHI(FVulkanDevice* InDevice, const FRHIRayTracingAccelerationStructurePartitionedSceneInputs& InInputs)
    : FRHIPartitionedSceneAccelerationStructure(InInputs.Flags)
    , FVulkanAccelerationStructure(InDevice)
    , SceneInputs(InInputs)
    , ResultLocation(InDevice)
    , View(nullptr)
{
}

FVulkanPartitionedSceneAccelerationStructureRHI::~FVulkanPartitionedSceneAccelerationStructureRHI()
{
    ResultLocation.ReleaseMemory();
}

void* FVulkanPartitionedSceneAccelerationStructureRHI::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(ResultLocation.GetBackingBuffer());
}

FRHIShaderResourceView* FVulkanPartitionedSceneAccelerationStructureRHI::GetShaderResourceView() const
{
    return View.Get();
}

FRHIDescriptorHandle FVulkanPartitionedSceneAccelerationStructureRHI::GetBindlessHandle() const
{
    return View ? View->GetBindlessHandle() : FRHIDescriptorHandle();
}

void FVulkanPartitionedSceneAccelerationStructureRHI::SetDebugName(const String& InName)
{
#if VULKAN_STORE_DEBUG_NAMES
    DebugName = InName;
#else
    UNREFERENCED_VARIABLE(InName);
#endif
}

void FVulkanPartitionedSceneAccelerationStructureRHI::GetDebugName(String& OutDebugName) const
{
#if VULKAN_STORE_DEBUG_NAMES
    OutDebugName = DebugName;
#else
    OutDebugName.Clear();
#endif
}

bool FVulkanPartitionedSceneAccelerationStructureRHI::Initialize()
{
#if VK_NV_partitioned_acceleration_structure
    if (!vkGetPartitionedAccelerationStructuresBuildSizesNV)
    {
        VULKAN_ERROR_CRITICAL("Partitioned acceleration-structure extension functions are not available");
        return false;
    }

    VkPartitionedAccelerationStructureInstancesInputNV InstancesInput = {};
    InstancesInput.sType                             = VK_STRUCTURE_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_INSTANCES_INPUT_NV;
    InstancesInput.flags                             = ConvertAccelerationStructureBuildFlags(SceneInputs.Flags);
    InstancesInput.instanceCount                     = SceneInputs.MaxInstanceCount;
    InstancesInput.maxInstancePerPartitionCount      = SceneInputs.MaxInstanceCount;
    InstancesInput.partitionCount                    = SceneInputs.MaxPartitionCount;
    InstancesInput.maxInstanceInGlobalPartitionCount = SceneInputs.MaxInstanceCount;

    VkAccelerationStructureBuildSizesInfoKHR SizesInfo = {};
    SizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    
    vkGetPartitionedAccelerationStructuresBuildSizesNV(GetDevice()->GetVkDevice(), &InstancesInput, &SizesInfo);

    const VkDeviceSize          ResultSize       = Math::Max<VkDeviceSize>(SizesInfo.accelerationStructureSize, 1);
    const VkMemoryAllocateFlags AllocateFlags    = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    const VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    const VkBufferUsageFlags    Usage            = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (!GetDevice()->GetMemoryManager().AllocateBufferMemory(MemoryProperties, Usage, AllocateFlags, ResultSize, 256, ResultLocation))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate partitioned scene acceleration-structure memory");
        return false;
    }

    DeviceAddress = ResultLocation.GetDeviceAddress();
    return true;
#else
    return false;
#endif
}

#include "VulkanRHI/VulkanRayTracing.h"

FVulkanGeometryAccelerationStructureRHI::FVulkanGeometryAccelerationStructureRHI(FVulkanDevice* InDevice, const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
    : FRHIGeometryAccelerationStructure(InGeometryDesc)
    , FVulkanDeviceChild(InDevice)
    , Geometry(VK_NULL_HANDLE)
    , GeometryBuffer(VK_NULL_HANDLE)
{
}

FVulkanGeometryAccelerationStructureRHI::~FVulkanGeometryAccelerationStructureRHI()
{
    VkDevice DeviceHandle = GetDevice()->GetVkDevice();
    if (VULKAN_CHECK_HANDLE(Geometry))
    {
        vkDestroyAccelerationStructureKHR(DeviceHandle, Geometry, nullptr);
        Geometry = VK_NULL_HANDLE;
    }

    if (VULKAN_CHECK_HANDLE(GeometryBuffer))
    {
        vkDestroyBuffer(DeviceHandle, GeometryBuffer, nullptr);
        GeometryBuffer = VK_NULL_HANDLE;
    }

    if (VULKAN_CHECK_HANDLE(ScratchBuffer))
    {
        vkDestroyBuffer(DeviceHandle, ScratchBuffer, nullptr);
        ScratchBuffer = VK_NULL_HANDLE;
    }

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    MemoryManager.Free(GeometryMemory);
    MemoryManager.Free(ScratchMemory);
}

void FVulkanGeometryAccelerationStructureRHI::SetDebugName(const FString& InName)
{
    if (VULKAN_CHECK_HANDLE(Geometry))
    {
        VulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.Data(), Geometry, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);
    }

    DebugName = InName;
}

FString FVulkanGeometryAccelerationStructureRHI::GetDebugName() const
{
    return DebugName;
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
    AccelerationStructureBuildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    AccelerationStructureBuildGeometryInfo.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    AccelerationStructureBuildGeometryInfo.geometryCount = 1;
    AccelerationStructureBuildGeometryInfo.pGeometries   = &AccelerationStructureGeometry;

    VkAccelerationStructureBuildSizesInfoKHR AccelerationStructureBuildSizesInfo = {};
    AccelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    // TODO: Is there any case when this is not true?
    const uint32_t NumTriangles = BuildDesc.NumIndices / 3;
    if ((BuildDesc.NumIndices % 3) != 0)
    {
        VULKAN_WARNING("Creating acceleration structure with an indexcount that is not a multiple of 3");
    }

    vkGetAccelerationStructureBuildSizesKHR(GetDevice()->GetVkDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &AccelerationStructureBuildGeometryInfo, &NumTriangles, &AccelerationStructureBuildSizesInfo);

    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size        = AccelerationStructureBuildSizesInfo.accelerationStructureSize;
    BufferCreateInfo.usage       = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult Result = vkCreateBuffer(GetDevice()->GetVkDevice(), &BufferCreateInfo, nullptr, &GeometryBuffer);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Buffer");
        return false;
    }

    const VkMemoryAllocateFlags AllocateFlags    = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    const VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(GeometryBuffer, MemoryProperties, AllocateFlags, GVulkanForceDedicatedAllocations, GeometryMemory))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate buffer memory");
        return false;
    }

    VkAccelerationStructureCreateInfoKHR AccelerationStructureCreateInfo = {};
    AccelerationStructureCreateInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    AccelerationStructureCreateInfo.createFlags   = 0;
    AccelerationStructureCreateInfo.buffer        = GeometryBuffer;
    AccelerationStructureCreateInfo.offset        = 0;
    AccelerationStructureCreateInfo.size          = AccelerationStructureBuildSizesInfo.accelerationStructureSize;
    AccelerationStructureCreateInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    AccelerationStructureCreateInfo.deviceAddress = 0;

    Result = vkCreateAccelerationStructureKHR(GetDevice()->GetVkDevice(), &AccelerationStructureCreateInfo, nullptr, &Geometry);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create AccelerationStructure");
        return false;
    }

    // Create ScratchBuffer
    BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size        = AccelerationStructureBuildSizesInfo.buildScratchSize;
    BufferCreateInfo.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Result = vkCreateBuffer(GetDevice()->GetVkDevice(), &BufferCreateInfo, nullptr, &ScratchBuffer);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Buffer");
        return false;
    }

    if (!MemoryManager.AllocateBufferMemory(ScratchBuffer, MemoryProperties, AllocateFlags, GVulkanForceDedicatedAllocations, ScratchMemory))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate scratch-buffer memory");
        return false;
    }

    AccelerationStructureBuildGeometryInfo.dstAccelerationStructure  = Geometry;
    AccelerationStructureBuildGeometryInfo.scratchData.deviceAddress = ScratchMemory.DeviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR AccelerationStructureBuildRangeInfo = {};
    AccelerationStructureBuildRangeInfo.primitiveCount  = NumTriangles;
    AccelerationStructureBuildRangeInfo.primitiveOffset = 0;
    AccelerationStructureBuildRangeInfo.firstVertex     = 0;
    AccelerationStructureBuildRangeInfo.transformOffset = 0;

    VkAccelerationStructureBuildRangeInfoKHR* BuildRangeInfos[] = { &AccelerationStructureBuildRangeInfo };
    CmdContext.GetCommandBuffer()->BuildAccelerationStructures(1, &AccelerationStructureBuildGeometryInfo, BuildRangeInfos);

    VkAccelerationStructureDeviceAddressInfoKHR AccelerationDeviceAddressInfo = {};
    AccelerationDeviceAddressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    AccelerationDeviceAddressInfo.accelerationStructure = Geometry;

    GeometryDeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(GetDevice()->GetVkDevice(), &AccelerationDeviceAddressInfo);
    if (GeometryDeviceAddress == 0)
    {
        VULKAN_ERROR_CRITICAL("GetAccelerationStructureDeviceAddress returned an invalid DeviceAddress");
        return false;
    }

    return true;
}

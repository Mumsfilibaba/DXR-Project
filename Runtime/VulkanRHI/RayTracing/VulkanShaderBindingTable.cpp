#include "VulkanRHI/RayTracing/VulkanShaderBindingTable.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanBuffer.h"

static constexpr uint32 GVulkanSBTLocalRecordBytes = 32;

static FORCEINLINE uint64 VulkanAlignUp(uint64 Value, uint64 Alignment)
{
    return Alignment ? ((Value + (Alignment - 1)) & ~(Alignment - 1)) : Value;
}

FVulkanShaderBindingTable::FVulkanShaderBindingTable(FVulkanDevice* InDevice, const FRHIShaderBindingTableDesc& InDesc)
    : FRHIShaderBindingTable(InDesc)
    , FVulkanDeviceChild(InDevice)
    , Pipeline(FVulkanDeviceRHI::ResourceCast(InDesc.Pipeline))
    , NumRayGen(Math::Max<uint32>(InDesc.NumRayGenerationShaders, 1u))
    , NumMiss(InDesc.NumMissShaders)
    , NumCallable(InDesc.NumCallableShaders)
    , NumHitGroup(InDesc.NumHitGroupRecords)
    , HandleSize(0)
    , RecordStride(0)
    , RayGenOffset(0)
    , MissOffset(0)
    , HitGroupOffset(0)
    , CallableOffset(0)
    , CpuShadow()
    , TableLocation(InDevice)
{
    if (Pipeline)
    {
        Pipeline->AddRef();
    }

    HandleSize = Pipeline ? Pipeline->GetShaderGroupHandleSize() : 0;

    const uint32 HandleAlignment = Pipeline ? Pipeline->GetShaderGroupHandleAlignment() : 1;
    const uint32 BaseAlignment   = Pipeline ? Pipeline->GetShaderGroupBaseAlignment() : 1;

    RecordStride = static_cast<uint32>(VulkanAlignUp(uint64(HandleSize) + GVulkanSBTLocalRecordBytes, Math::Max<uint32>(HandleAlignment, 1u)));

    MAYBE_UNUSED const uint32 MaxShaderGroupStride = Pipeline ? Pipeline->GetMaxShaderGroupStride() : 0;
    CHECK(MaxShaderGroupStride == 0 || RecordStride <= MaxShaderGroupStride);

    RayGenOffset   = 0;
    MissOffset     = VulkanAlignUp(RayGenOffset + uint64(NumRayGen) * RecordStride, BaseAlignment);
    HitGroupOffset = VulkanAlignUp(MissOffset + uint64(NumMiss) * RecordStride, BaseAlignment);
    CallableOffset = VulkanAlignUp(HitGroupOffset + uint64(NumHitGroup) * RecordStride, BaseAlignment);

    const uint64 TotalSize = Math::Max<uint64>(CallableOffset + uint64(NumCallable) * RecordStride, RecordStride);
    CpuShadow.Resize(int32(TotalSize));

    Memory::Memzero(CpuShadow.Data(), CpuShadow.SizeInBytes());
}

FVulkanShaderBindingTable::~FVulkanShaderBindingTable()
{
    TableLocation.ReleaseMemory();
}

void* FVulkanShaderBindingTable::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(TableLocation.GetBackingBuffer());
}

bool FVulkanShaderBindingTable::Initialize()
{
    const uint64 RequiredSize = uint64(CpuShadow.SizeInBytes());
    if (RequiredSize == 0)
    {
        return false;
    }

    const VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const VkBufferUsageFlags    Usage            = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    const VkMemoryAllocateFlags AllocateFlags    = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    const uint32 BaseAlignment = Pipeline ? Pipeline->GetShaderGroupBaseAlignment() : 256;
    return GetDevice()->GetMemoryManager().AllocateBufferMemory(MemoryProperties, Usage, AllocateFlags, RequiredSize, BaseAlignment, TableLocation);
}

FRHIShaderBindingTableAddressInfo FVulkanShaderBindingTable::GetAddressInfo() const
{
    FRHIShaderBindingTableAddressInfo AddressInfo = {};
    if (!TableLocation.IsValid())
    {
        return AddressInfo;
    }

    const VkStridedDeviceAddressRegionKHR RayGen = GetRayGenRegion();
    AddressInfo.RayGeneration = { RayGen.deviceAddress, RayGen.size, RayGen.stride };

    const VkStridedDeviceAddressRegionKHR Miss = GetMissRegion();
    AddressInfo.Miss = { Miss.deviceAddress, Miss.size, Miss.stride };

    const VkStridedDeviceAddressRegionKHR HitGroup = GetHitGroupRegion();
    AddressInfo.HitGroup = { HitGroup.deviceAddress, HitGroup.size, HitGroup.stride };

    const VkStridedDeviceAddressRegionKHR Callable = GetCallableRegion();
    AddressInfo.Callable = { Callable.deviceAddress, Callable.size, Callable.stride };
    return AddressInfo;
}

uint64 FVulkanShaderBindingTable::GetRegionBaseOffset(ERayTracingShaderRecordKind RecordKind) const
{
    switch (RecordKind)
    {
        case ERayTracingShaderRecordKind::RayGeneration: return RayGenOffset;
        case ERayTracingShaderRecordKind::Miss:          return MissOffset;
        case ERayTracingShaderRecordKind::HitGroup:      return HitGroupOffset;
        case ERayTracingShaderRecordKind::Callable:      return CallableOffset;
        default:                                         return 0;
    }
}

void FVulkanShaderBindingTable::PopulateRecord(uint8* OutRecord, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings)
{
    String ExportName;
    if (Pipeline)
    {
        Pipeline->GetExportName(RecordKind, RecordIndex, ExportName);
    }

    const uint8* GroupHandle = (Pipeline && !ExportName.IsEmpty()) ? Pipeline->GetShaderGroupHandle(ExportName) : nullptr;

    if (GroupHandle)
    {
        Memory::Memcpy(OutRecord, GroupHandle, HandleSize);
    }
    else
    {
        VULKAN_ERROR("[FVulkanShaderBindingTable]: Failed to resolve shader-group handle for record kind '%s' index %u (export name '%s'). The record would be written with a zeroed handle and its shader would never execute; verify the pipeline registered a group for this shader.",
            ToString(RecordKind), RecordIndex, ExportName.IsEmpty() ? "<empty>" : *ExportName);
    }

    if (!Bindings || NumBindings == 0)
    {
        return;
    }

    uint8*           LocalData     = OutRecord + HandleSize;
    const uint32     MaxLocalBytes = RecordStride - HandleSize;
    VkDeviceAddress* Addresses     = reinterpret_cast<VkDeviceAddress*>(LocalData);
    const uint32     MaxSlots      = MaxLocalBytes / uint32(sizeof(VkDeviceAddress));

    for (uint32 i = 0; i < NumBindings; ++i)
    {
        const FRHIHitGroupLocalShaderBinding& Binding = Bindings[i];
        if (Binding.Type != ERayTracingLocalBindingType::ConstantBuffer)
        {
            continue;
        }

        const uint32 Slot = Binding.RegisterIndex;
        if (Slot < MaxSlots)
        {
            FVulkanBufferRHI* VulkanBuffer = FVulkanDeviceRHI::ResourceCast(Binding.Buffer);
            Addresses[Slot] = VulkanBuffer ? VulkanBuffer->GetDeviceAddress() : 0;
        }
    }
}

void FVulkanShaderBindingTable::SetBindings(ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings)
{
    const uint64 RegionBase = GetRegionBaseOffset(RecordKind);
    const uint64 ByteOffset = RegionBase + uint64(RecordIndex) * RecordStride;

    if (ByteOffset + RecordStride <= uint64(CpuShadow.SizeInBytes()))
    {
        PopulateRecord(CpuShadow.Data() + ByteOffset, RecordKind, RecordIndex, Bindings, NumBindings);
    }
}

void FVulkanShaderBindingTable::ClearTableRecords()
{
    Memory::Memzero(CpuShadow.Data(), CpuShadow.SizeInBytes());
}

void FVulkanShaderBindingTable::Build()
{
    const uint64 RequiredSize = uint64(CpuShadow.SizeInBytes());
    if (RequiredSize == 0)
    {
        return;
    }

    CHECK(TableLocation.IsValid() && TableLocation.GetSize() >= RequiredSize);

    if (void* Mapped = TableLocation.GetMappedBaseAddress())
    {
        Memory::Memcpy(Mapped, CpuShadow.Data(), CpuShadow.SizeInBytes());
    }
    else
    {
        VULKAN_ERROR("Failed to map shader-binding-table memory");
    }
}

VkStridedDeviceAddressRegionKHR FVulkanShaderBindingTable::GetRayGenRegion() const
{
    VkStridedDeviceAddressRegionKHR Region = {};
    Region.deviceAddress = TableLocation.GetDeviceAddress() + RayGenOffset;
    Region.stride        = RecordStride;
    Region.size          = RecordStride;
    return Region;
}

VkStridedDeviceAddressRegionKHR FVulkanShaderBindingTable::GetMissRegion() const
{
    VkStridedDeviceAddressRegionKHR Region = {};
    if (NumMiss > 0)
    {
        Region.deviceAddress = TableLocation.GetDeviceAddress() + MissOffset;
        Region.stride        = RecordStride;
        Region.size          = uint64(NumMiss) * RecordStride;
    }
    
    return Region;
}

VkStridedDeviceAddressRegionKHR FVulkanShaderBindingTable::GetHitGroupRegion() const
{
    VkStridedDeviceAddressRegionKHR Region = {};
    if (NumHitGroup > 0)
    {
        Region.deviceAddress = TableLocation.GetDeviceAddress() + HitGroupOffset;
        Region.stride        = RecordStride;
        Region.size          = uint64(NumHitGroup) * RecordStride;
    }

    return Region;
}

VkStridedDeviceAddressRegionKHR FVulkanShaderBindingTable::GetCallableRegion() const
{
    VkStridedDeviceAddressRegionKHR Region = {};
    if (NumCallable > 0)
    {
        Region.deviceAddress = TableLocation.GetDeviceAddress() + CallableOffset;
        Region.stride        = RecordStride;
        Region.size          = uint64(NumCallable) * RecordStride;
    }

    return Region;
}

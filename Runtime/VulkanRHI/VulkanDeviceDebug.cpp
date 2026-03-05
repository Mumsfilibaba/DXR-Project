#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanRHI.h"
#include "Core/Misc/CRC.h"

#if VULKAN_ENABLE_CRASH_MARKERS

FVulkanCrashMarkers::FVulkanCrashMarkers(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Extension(ECrashMarkerExtension::None)
    , GraphicsQueue(nullptr)
    , MemoryStorage(InDevice)
    , MappedData(nullptr)
    , NextIndex(0)
    , DrawCounter(0)
    , CurrentRegion()
    , PrevNextIndex(0)
{
}

FVulkanCrashMarkers::~FVulkanCrashMarkers()
{
    Release();
}

bool FVulkanCrashMarkers::Initialize(FVulkanQueue& InGraphicsQueue)
{
    GraphicsQueue = &InGraphicsQueue;

#if VK_AMD_buffer_marker
    if (GetDevice()->IsExtensionEnabled(VK_AMD_BUFFER_MARKER_EXTENSION_NAME))
    {
        Extension = ECrashMarkerExtension::AMDBufferMarker;

        constexpr VkDeviceSize          BufferSize       = GPU_SLOTS * sizeof(uint32);
        constexpr VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        constexpr VkBufferUsageFlags    BufferUsage      = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
        if (!MemoryManager.AllocateBufferMemory(MemoryProperties, BufferUsage, 0, BufferSize, sizeof(uint32), MemoryStorage))
        {
            VULKAN_ERROR("FVulkanCrashMarkers: Failed to allocate marker buffer");
            Extension = ECrashMarkerExtension::None;
            return false;
        }

        MappedData = reinterpret_cast<uint32*>(MemoryStorage.GetMappedBaseAddress());
        if (!MappedData)
        {
            VULKAN_ERROR("FVulkanCrashMarkers: Marker buffer is not host-mapped");
            MemoryStorage.ReleaseMemory();
            Extension = ECrashMarkerExtension::None;
            return false;
        }

        for (uint32 i = 0; i < GPU_SLOTS; i++)
        {
            MappedData[i] = SENTINEL;
        }

        VULKAN_INFO("GPU crash marker tracking initialized with VK_AMD_buffer_marker (%u GPU slots)", GPU_SLOTS);
        return true;
    }
#endif

#if VK_NV_device_diagnostic_checkpoints
    if (GetDevice()->IsExtensionEnabled(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME))
    {
        Extension = ECrashMarkerExtension::NVCheckpoints;
        VULKAN_INFO("GPU crash marker tracking initialized with VK_NV_device_diagnostic_checkpoints");
        return true;
    }
#endif

    VULKAN_WARNING("FVulkanCrashMarkers: No supported extension available (VK_AMD_buffer_marker or VK_NV_device_diagnostic_checkpoints)");
    return false;
}

void FVulkanCrashMarkers::Release()
{
#if VK_AMD_buffer_marker
    if (Extension == ECrashMarkerExtension::AMDBufferMarker)
    {
        MemoryStorage.ReleaseMemory();
        MappedData = nullptr;
    }
#endif

    Extension = ECrashMarkerExtension::None;
}

void FVulkanCrashMarkers::ResetMarkers(FVulkanCommandBuffer& CmdBuf)
{
    PrevFrameHashes = Move(FrameHashes);
    PrevNextIndex   = NextIndex;

    NextIndex   = 0;
    DrawCounter = 0;
    CurrentRegion.Clear();

#if VK_AMD_buffer_marker
    if (Extension == ECrashMarkerExtension::AMDBufferMarker && MappedData)
    {
        const VkDeviceSize BufferSize = GPU_SLOTS * sizeof(uint32);
        CmdBuf->FillBuffer(MemoryStorage.GetBackingBuffer(), MemoryStorage.GetBufferOffset(), BufferSize, SENTINEL);
    }
#endif
}

void FVulkanCrashMarkers::WriteMarker(FVulkanCommandBuffer& CmdBuf, const FStringView& Name)
{
    CurrentRegion = Name.Data();
    DrawCounter   = 0;

    WriteMarkerInternal(CmdBuf, Name);
}

void FVulkanCrashMarkers::WriteEndMarker(FVulkanCommandBuffer& CmdBuf)
{
    if (!CurrentRegion.IsEmpty())
    {
        FString Name = FString::CreateFormatted("%s [END]", *CurrentRegion);
        WriteMarkerInternal(CmdBuf, *Name);
    }
}

void FVulkanCrashMarkers::WriteDrawMarker(FVulkanCommandBuffer& CmdBuf, const FStringView& DrawType)
{
    FString Name;
    if (!CurrentRegion.IsEmpty())
    {
        Name = FString::CreateFormatted("%s > %s #%u", *CurrentRegion, DrawType.Data(), DrawCounter);
    }
    else
    {
        Name = FString::CreateFormatted("%s #%u", DrawType.Data(), DrawCounter);
    }

    DrawCounter++;
    WriteMarkerInternal(CmdBuf, *Name);
}

void FVulkanCrashMarkers::WriteSplitMarker(FVulkanCommandBuffer& CmdBuf)
{
    WriteMarkerInternal(CmdBuf, "========== CommandBuffer Split ==========");
}

void FVulkanCrashMarkers::WriteMarkerInternal(FVulkanCommandBuffer& CmdBuf, const FStringView& Name)
{
    if (Extension == ECrashMarkerExtension::None)
    {
        return;
    }

    const uint32 Hash = CRC32::Generate(Name.Data(), static_cast<uint64>(Name.Length()) * sizeof(CHAR));
    if (!HashToIndex.Contains(Hash))
    {
        HashToIndex.Add(Hash, static_cast<uint32>(StringPool.Size()));
        StringPool.Add(FString(Name.Data()));
    }

    FrameHashes.Add(Hash);

    const uint32 Slot = NextIndex % GPU_SLOTS;
    if (Extension == ECrashMarkerExtension::AMDBufferMarker)
    {
    #if VK_AMD_buffer_marker
        const VkDeviceSize Offset = MemoryStorage.GetBufferOffset() + static_cast<VkDeviceSize>(Slot) * sizeof(uint32);
        vkCmdWriteBufferMarkerAMD(CmdBuf.GetVkCommandBuffer(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MemoryStorage.GetBackingBuffer(), Offset, Hash);
    #endif
    }
    else if (Extension == ECrashMarkerExtension::NVCheckpoints)
    {
    #if VK_NV_device_diagnostic_checkpoints
        const uintptr_t Marker = (static_cast<uintptr_t>(Hash) << 32) | static_cast<uintptr_t>(NextIndex);
        vkCmdSetCheckpointNV(CmdBuf.GetVkCommandBuffer(), reinterpret_cast<void*>(Marker));
    #endif
    }

    NextIndex++;
}

const FString& FVulkanCrashMarkers::ResolveHash(uint32 Hash) const
{
    static const FString Unknown("???");
    
    const uint32* Index = HashToIndex.Find(Hash);
    if (Index && *Index < static_cast<uint32>(StringPool.Size()))
    {
        return StringPool[*Index];
    }

    return Unknown;
}

void FVulkanCrashMarkers::DumpCrashMarkers()
{
    LOG_ERROR("==========================================================");
    LOG_ERROR("[VulkanRHI] GPU CRASH MARKERS (VK_ERROR_DEVICE_LOST)");
    LOG_ERROR("==========================================================");

    if (NextIndex == 0 && PrevNextIndex == 0)
    {
        LOG_ERROR("[VulkanRHI]   No crash markers were recorded.");
        LOG_ERROR("==========================================================");
        return;
    }

    LOG_ERROR("[VulkanRHI]   %u markers this frame, %u previous frame.",
        NextIndex, PrevNextIndex);

    if (Extension == ECrashMarkerExtension::AMDBufferMarker)
    {
    #if VK_AMD_buffer_marker
        if (!MappedData)
        {
            LOG_ERROR("[VulkanRHI]   AMD buffer marker: mapped data is null.");
            LOG_ERROR("==========================================================");
            return;
        }

        uint32 HighestCompleted = UINT32_MAX;
        bool bUsingPrevFrame = false;
        const TArray<uint32>* ActiveHashes = &FrameHashes;
        uint32 ActiveTotal = NextIndex;

        if (ActiveTotal > 0)
        {
            for (uint32 i = ActiveTotal; i > 0; i--)
            {
                const uint32 Idx  = i - 1;
                const uint32 Slot = Idx % GPU_SLOTS;
                if (MappedData[Slot] == (*ActiveHashes)[Idx])
                {
                    HighestCompleted = Idx;
                    break;
                }
            }
        }

        if (HighestCompleted == UINT32_MAX && PrevNextIndex > 0)
        {
            bUsingPrevFrame = true;
            ActiveHashes    = &PrevFrameHashes;
            ActiveTotal     = PrevNextIndex;

            for (uint32 i = ActiveTotal; i > 0; i--)
            {
                const uint32 Idx  = i - 1;
                const uint32 Slot = Idx % GPU_SLOTS;
                if (MappedData[Slot] == (*ActiveHashes)[Idx])
                {
                    HighestCompleted = Idx;
                    break;
                }
            }
        }

        if (bUsingPrevFrame)
        {
            LOG_ERROR("[VulkanRHI]   GPU buffer contains stale data (vkCmdFillBuffer reset did not execute).");
            LOG_ERROR("[VulkanRHI]   Analyzing previous frame's %u markers.", ActiveTotal);
        }

        if (HighestCompleted == UINT32_MAX)
        {
            LOG_ERROR("[VulkanRHI]   GPU did not complete any marker.");
            LOG_ERROR("----------------------------------------------------------");

            const uint32 ShowCount = (ActiveTotal < DUMP_WINDOW) ? ActiveTotal : DUMP_WINDOW;
            for (uint32 i = 0; i < ShowCount; i++)
            {
                LOG_ERROR("[VulkanRHI]   [NOT REACHED] [%4u] %s", i, *ResolveHash((*ActiveHashes)[i]));
            }

            if (ActiveTotal > ShowCount)
            {
                LOG_ERROR("[VulkanRHI]   ... (%u more NOT REACHED entries omitted)", ActiveTotal - ShowCount);
            }
        }
        else if (HighestCompleted + 1 >= ActiveTotal)
        {
            LOG_ERROR("[VulkanRHI]   All %u markers completed. Crash likely after last marker.", ActiveTotal);
            LOG_ERROR("----------------------------------------------------------");

            const uint32 ShowStart = (ActiveTotal > DUMP_WINDOW) ? (ActiveTotal - DUMP_WINDOW) : 0;
            for (uint32 i = ShowStart; i < ActiveTotal; i++)
            {
                LOG_ERROR("[VulkanRHI]   [DONE] [%4u] %s", i, *ResolveHash((*ActiveHashes)[i]));
            }
        }
        else
        {
            const uint32 FirstNotReached = HighestCompleted + 1;
            LOG_ERROR("[VulkanRHI]   Last completed  : [%u] %s", HighestCompleted, *ResolveHash((*ActiveHashes)[HighestCompleted]));
            LOG_ERROR("[VulkanRHI]   First not reached: [%u] %s", FirstNotReached, *ResolveHash((*ActiveHashes)[FirstNotReached]));
            LOG_ERROR("----------------------------------------------------------");

            const uint32 WindowBefore = DUMP_WINDOW / 2;
            const uint32 WindowAfter  = DUMP_WINDOW / 2;
            const uint32 DumpStart    = (HighestCompleted >= WindowBefore) ? (HighestCompleted - WindowBefore) : 0;
            const uint32 DumpEnd      = ((FirstNotReached + WindowAfter) < ActiveTotal) ? (FirstNotReached + WindowAfter) : ActiveTotal;

            if (DumpStart > 0)
            {
                LOG_ERROR("[VulkanRHI]   ... (%u earlier entries omitted, all DONE)", DumpStart);
            }

            for (uint32 i = DumpStart; i < DumpEnd; i++)
            {
                const bool bDone = (i <= HighestCompleted);
                LOG_ERROR("[VulkanRHI]   [%s] [%4u] %s", bDone ? "DONE" : "NOT REACHED", i, *ResolveHash((*ActiveHashes)[i]));
            }

            if (DumpEnd < ActiveTotal)
            {
                LOG_ERROR("[VulkanRHI]   ... (%u later entries omitted, all NOT REACHED)", ActiveTotal - DumpEnd);
            }
        }
    #else
        LOG_ERROR("[VulkanRHI]   AMD buffer marker: extension not compiled in.");
    #endif
    }
    else if (Extension == ECrashMarkerExtension::NVCheckpoints)
    {
    #if VK_NV_device_diagnostic_checkpoints
        if (!GraphicsQueue)
        {
            LOG_ERROR("[VulkanRHI]   NV checkpoints: no graphics queue available.");
            LOG_ERROR("==========================================================");
            return;
        }

        uint32 CheckpointCount = 0;
        vkGetQueueCheckpointDataNV(GraphicsQueue->GetVkQueue(), &CheckpointCount, nullptr);

        if (CheckpointCount == 0)
        {
            LOG_ERROR("[VulkanRHI]   NV checkpoints: no checkpoint data returned by driver.");
            LOG_ERROR("==========================================================");
            return;
        }

        TArray<VkCheckpointDataNV> Checkpoints(CheckpointCount);
        for (uint32 i = 0; i < CheckpointCount; i++)
        {
            Checkpoints[i].sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
            Checkpoints[i].pNext = nullptr;
        }

        vkGetQueueCheckpointDataNV(GraphicsQueue->GetVkQueue(), &CheckpointCount, Checkpoints.Data());

        uint32 HighestReached = UINT32_MAX;
        for (uint32 j = 0; j < CheckpointCount; j++)
        {
            const uintptr_t Marker = reinterpret_cast<uintptr_t>(Checkpoints[j].pCheckpointMarker);
            const uint32    Index  = static_cast<uint32>(Marker & 0xFFFFFFFF);

            if (HighestReached == UINT32_MAX || Index > HighestReached)
            {
                HighestReached = Index;
            }
        }

        bool bNVPrevFrame = false;
        const TArray<uint32>* NVHashes = &FrameHashes;
        uint32 NVTotal = NextIndex;

        if (HighestReached != UINT32_MAX && HighestReached >= NextIndex && PrevNextIndex > 0)
        {
            bNVPrevFrame = true;
            NVHashes     = &PrevFrameHashes;
            NVTotal      = PrevNextIndex;
        }

        if (bNVPrevFrame)
        {
            LOG_ERROR("[VulkanRHI]   Checkpoint data is from a previous frame.");
            LOG_ERROR("[VulkanRHI]   Analyzing previous frame's %u markers.", NVTotal);
        }

        if (HighestReached == UINT32_MAX || HighestReached + 1 >= NVTotal)
        {
            LOG_ERROR("[VulkanRHI]   %u checkpoint(s) returned. %s",
                CheckpointCount, (HighestReached == UINT32_MAX) ? "GPU did not reach any marker." : "All markers reached.");
            LOG_ERROR("----------------------------------------------------------");

            if (NVTotal > 0 && HighestReached != UINT32_MAX)
            {
                const uint32 ShowStart = (NVTotal > DUMP_WINDOW) ? (NVTotal - DUMP_WINDOW) : 0;
                for (uint32 i = ShowStart; i < NVTotal; i++)
                {
                    LOG_ERROR("[VulkanRHI]   [REACHED    ] [%4u] %s", i, *ResolveHash((*NVHashes)[i]));
                }
            }
        }
        else
        {
            const uint32 FirstNotReached = HighestReached + 1;
            LOG_ERROR("[VulkanRHI]   Last reached    : [%u] %s", HighestReached, *ResolveHash((*NVHashes)[HighestReached]));
            LOG_ERROR("[VulkanRHI]   First not reached: [%u] %s", FirstNotReached, *ResolveHash((*NVHashes)[FirstNotReached]));
            LOG_ERROR("----------------------------------------------------------");

            const uint32 WindowBefore = DUMP_WINDOW / 2;
            const uint32 WindowAfter  = DUMP_WINDOW / 2;

            const uint32 DumpStart = (HighestReached >= WindowBefore) ? (HighestReached - WindowBefore) : 0;
            const uint32 DumpEnd   = ((FirstNotReached + WindowAfter) < NVTotal) ? (FirstNotReached + WindowAfter) : NVTotal;

            if (DumpStart > 0)
            {
                LOG_ERROR("[VulkanRHI]   ... (%u earlier entries omitted, all REACHED)", DumpStart);
            }

            for (uint32 i = DumpStart; i < DumpEnd; i++)
            {
                const bool bReached = (i <= HighestReached);
                LOG_ERROR("[VulkanRHI]   [%s] [%4u] %s", bReached ? "REACHED    " : "NOT REACHED", i, *ResolveHash((*NVHashes)[i]));
            }

            if (DumpEnd < NVTotal)
            {
                LOG_ERROR("[VulkanRHI]   ... (%u later entries omitted, all NOT REACHED)", NVTotal - DumpEnd);
            }
        }

        LOG_ERROR("[VulkanRHI]   %u checkpoint(s) returned by driver.", CheckpointCount);
    #else
        LOG_ERROR("[VulkanRHI]   NV checkpoints: extension not compiled in.");
    #endif
    }

    LOG_ERROR("==========================================================");
}

#endif // VULKAN_ENABLE_CRASH_MARKERS

#if VULKAN_ENABLE_DEVICE_LOST_CHECK

bool VulkanCheckDeviceLost(VkResult Result)
{
    if (Result == VK_ERROR_DEVICE_LOST)
    {
    #if VULKAN_ENABLE_CRASH_MARKERS
        if (FVulkanRHI* RHI = FVulkanRHI::Get())
        {
            if (FVulkanCrashMarkers* CrashMarkers = RHI->GetCrashMarkers())
            {
                CrashMarkers->DumpCrashMarkers();
            }
        }
    #endif

    #if VK_EXT_device_fault
        if (FVulkanRHI* RHI = FVulkanRHI::Get())
        {
            FVulkanDevice* Device = RHI->GetDevice();
            if (Device && Device->IsExtensionEnabled(VK_EXT_DEVICE_FAULT_EXTENSION_NAME) && vkGetDeviceFaultInfoEXT)
            {
                VkDeviceFaultCountsEXT FaultCounts = {};
                FaultCounts.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT;

                VkResult FaultResult = vkGetDeviceFaultInfoEXT(Device->GetVkDevice(), &FaultCounts, nullptr);
                if (FaultResult == VK_SUCCESS)
                {
                    LOG_ERROR("==========================================================");
                    LOG_ERROR("[VulkanRHI] VK_EXT_device_fault: %u address info(s), %u vendor info(s), %llu bytes vendor binary",
                        FaultCounts.addressInfoCount, FaultCounts.vendorInfoCount, FaultCounts.vendorBinarySize);

                    if (FaultCounts.addressInfoCount > 0 || FaultCounts.vendorInfoCount > 0)
                    {
                        TArray<VkDeviceFaultAddressInfoEXT> AddressInfos(FaultCounts.addressInfoCount);
                        FMemory::Memzero(AddressInfos.Data(), AddressInfos.Size() * sizeof(VkDeviceFaultAddressInfoEXT));
                        
                        TArray<VkDeviceFaultVendorInfoEXT> VendorInfos(FaultCounts.vendorInfoCount);
                        FMemory::Memzero(VendorInfos.Data(), VendorInfos.Size() * sizeof(VkDeviceFaultVendorInfoEXT));

                        VkDeviceFaultInfoEXT FaultInfo = {};
                        FaultInfo.sType             = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT;
                        FaultInfo.pAddressInfos     = AddressInfos.IsEmpty() ? nullptr : AddressInfos.Data();
                        FaultInfo.pVendorInfos      = VendorInfos.IsEmpty() ? nullptr : VendorInfos.Data();
                        FaultInfo.pVendorBinaryData = nullptr;

                        FaultResult = vkGetDeviceFaultInfoEXT(Device->GetVkDevice(), &FaultCounts, &FaultInfo);
                        if (FaultResult == VK_SUCCESS)
                        {
                            if (FaultInfo.description[0] != '\0')
                            {
                                LOG_ERROR("[VulkanRHI]   Description: %s", FaultInfo.description);
                            }

                            for (uint32 i = 0; i < FaultCounts.addressInfoCount; i++)
                            {
                                const VkDeviceFaultAddressInfoEXT& Addr = AddressInfos[i];

                                const CHAR* TypeStr = "UNKNOWN";
                                switch (Addr.addressType)
                                {
                                    case VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_EXT:                         TypeStr = "NONE";                    break;
                                    case VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT:                 TypeStr = "READ_INVALID";            break;
                                    case VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT:                TypeStr = "WRITE_INVALID";           break;
                                    case VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT:              TypeStr = "EXECUTE_INVALID";         break;
                                    case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT:  TypeStr = "INSTRUCTION_PTR_UNKNOWN"; break;
                                    case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT:  TypeStr = "INSTRUCTION_PTR_INVALID"; break;
                                    case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT:    TypeStr = "INSTRUCTION_PTR_FAULT";   break;

                                    default: break;
                                }

                                LOG_ERROR("[VulkanRHI]   Address[%u]: type=%s addr=0x%llX precision=0x%llX",
                                    i, TypeStr, Addr.reportedAddress, Addr.addressPrecision);
                            }

                            for (uint32 i = 0; i < FaultCounts.vendorInfoCount; i++)
                            {
                                const VkDeviceFaultVendorInfoEXT& Vendor = VendorInfos[i];
                                LOG_ERROR("[VulkanRHI]   Vendor[%u]: '%s' code=0x%llX data=0x%llX",
                                    i, Vendor.description, Vendor.vendorFaultCode, Vendor.vendorFaultData);
                            }
                        }
                    }

                    LOG_ERROR("==========================================================");
                }
                else
                {
                    LOG_ERROR("[VulkanRHI] VK_EXT_device_fault: vkGetDeviceFaultInfoEXT returned %s", ToString(FaultResult));
                }
            }
        }
    #endif
    }

    return true;
}

#endif // VULKAN_ENABLE_DEVICE_LOST_CHECK

#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/CRC.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanRHI.h"

VULKANRHI_API bool GVulkanSupportsDebugUtils = false;

static TAutoConsoleVariable<bool> CVarBreakOnValidationError(
    "VulkanRHI.BreakOnValidationError",
    "Enables breakpoints when the validation-layer encounters an error",
    true);

DISABLE_UNREFERENCED_VARIABLE_WARNING

static bool VulkanIsMessageSuppressed(const VkDebugUtilsMessengerCallbackDataEXT* CallbackData)
{
    if (!CallbackData->pMessageIdName)
    {
        return false;
    }

    static const CHAR* SuppressedMessageIds[] =
    {
        "VUID-vkDestroyDevice-device-05137",
    };

    for (const CHAR* SuppressedId : SuppressedMessageIds)
    {
        if (CString::Strcmp(CallbackData->pMessageIdName, SuppressedId) == 0)
        {
            return true;
        }
    }

    return false;
}

static bool VulkanIsMessageSilenced(const VkDebugUtilsMessengerCallbackDataEXT* CallbackData)
{
    if (!CallbackData->pMessage)
    {
        return false;
    }

    static const CHAR* SilencedMessageText[] =
    {
        "Blending is enabled for attachment with format",
    };

    for (const CHAR* SilencedText : SilencedMessageText)
    {
        if (CString::Strstr(CallbackData->pMessage, SilencedText) != nullptr)
        {
            return true;
        }
    }

    return false;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugLayerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity, VkDebugUtilsMessageTypeFlagsEXT Type,
    const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData)
{
    if (VulkanIsMessageSilenced(CallbackData))
    {
        return VK_FALSE;
    }

    if (Severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        LOG_ERROR("[Vulkan Validation layer] %s", CallbackData->pMessage);

        if (CVarBreakOnValidationError.GetValue() && !VulkanIsMessageSuppressed(CallbackData))
        {
            DEBUG_BREAK();
        }
    }
    else if (Severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LOG_WARNING("[Vulkan Validation layer] %s", CallbackData->pMessage);
    }

    return VK_FALSE;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

void VulkanCreateDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT& OutMessenger)
{
#if VK_EXT_debug_utils
    if (!GVulkanSupportsDebugUtils)
    {
        return;
    }

    bool bEnableDebugLayer = false;
    if (IConsoleVariable* CVarEnableDebugLayer = FConsoleManager::Get().FindConsoleVariable("RHI.EnableDebugLayer"))
    {
        bEnableDebugLayer = CVarEnableDebugLayer->GetBool();
    }

    if (!bEnableDebugLayer)
    {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo = {};
    DebugMessengerCreateInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    DebugMessengerCreateInfo.flags           = 0;
    DebugMessengerCreateInfo.pNext           = nullptr;
    DebugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    DebugMessengerCreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    DebugMessengerCreateInfo.pfnUserCallback = VulkanDebugLayerCallback;
    DebugMessengerCreateInfo.pUserData       = nullptr;

    VkResult Result = vkCreateDebugUtilsMessengerEXT(Instance, &DebugMessengerCreateInfo, nullptr, &OutMessenger);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create DebugMessenger");
    }
#endif
}

void VulkanDestroyDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT& InOutMessenger)
{
#if VK_EXT_debug_utils
    if (VULKAN_CHECK_HANDLE(InOutMessenger))
    {
        vkDestroyDebugUtilsMessengerEXT(Instance, InOutMessenger, nullptr);
        InOutMessenger = VK_NULL_HANDLE;
    }
#endif
}

#if VULKAN_ENABLE_CRASH_MARKERS

FVulkanCrashMarkers::FVulkanCrashMarkers(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Extension(ECrashMarkerExtension::None)
    , GraphicsQueue(nullptr)
    , MemoryLocation(InDevice)
    , MappedData(nullptr)
    , NextIndex(0)
    , DrawCounter(0)
    , CurrentRegion()
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
    if (GetDevice()->IsAMDBufferMarkerEnabled())
    {
        Extension = ECrashMarkerExtension::AMDBufferMarker;

        constexpr VkDeviceSize          BufferSize       = GPU_SLOTS * sizeof(uint32);
        constexpr VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        constexpr VkBufferUsageFlags    BufferUsage      = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
        if (!MemoryManager.AllocateBufferMemory(MemoryProperties, BufferUsage, 0, BufferSize, sizeof(uint32), MemoryLocation))
        {
            VULKAN_ERROR("FVulkanCrashMarkers: Failed to allocate marker buffer");
            Extension = ECrashMarkerExtension::None;
            return false;
        }

        MappedData = reinterpret_cast<uint32*>(MemoryLocation.GetMappedBaseAddress());
        if (!MappedData)
        {
            VULKAN_ERROR("FVulkanCrashMarkers: Marker buffer is not host-mapped");
            MemoryLocation.ReleaseMemory();
            Extension = ECrashMarkerExtension::None;
            return false;
        }

        MappedData[SLOT_COUNTER] = 0;

        VULKAN_INFO("GPU crash marker tracking initialized with VK_AMD_buffer_marker (%u data slots)", DATA_SLOTS);
        return true;
    }
#endif

#if VK_NV_device_diagnostic_checkpoints
    if (GetDevice()->IsNVDiagnosticCheckpointsEnabled())
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
        MemoryLocation.ReleaseMemory();
        MappedData = nullptr;
    }
#endif

    Extension = ECrashMarkerExtension::None;
}

void FVulkanCrashMarkers::ResetMarkers(FVulkanCommandBuffer& /*CmdBuf*/)
{
    DrawCounter = 0;
    CurrentRegion.Clear();
}

void FVulkanCrashMarkers::WriteMarker(FVulkanCommandBuffer& CmdBuf, const StringView& Name)
{
    DrawCounter   = 0;
    CurrentRegion = Name.Data();

    WriteMarkerInternal(CmdBuf, Name);
}

void FVulkanCrashMarkers::WriteEndMarker(FVulkanCommandBuffer& CmdBuf)
{
    if (!CurrentRegion.IsEmpty())
    {
        String Name = String::Printf("%s [END]", *CurrentRegion);
        WriteMarkerInternal(CmdBuf, *Name);
    }
}

void FVulkanCrashMarkers::WriteDrawMarker(FVulkanCommandBuffer& CmdBuf, const StringView& DrawType)
{
    String Name;
    if (!CurrentRegion.IsEmpty())
    {
        Name = String::Printf("%s > %s #%u", *CurrentRegion, DrawType.Data(), DrawCounter);
    }
    else
    {
        Name = String::Printf("%s #%u", DrawType.Data(), DrawCounter);
    }

    DrawCounter++;
    WriteMarkerInternal(CmdBuf, *Name);
}

void FVulkanCrashMarkers::WriteSplitMarker(FVulkanCommandBuffer& CmdBuf)
{
    WriteMarkerInternal(CmdBuf, "---------- CommandBuffer Split ----------");
}

void FVulkanCrashMarkers::WriteMarkerInternal(FVulkanCommandBuffer& CmdBuf, const StringView& Name)
{
    if (Extension == ECrashMarkerExtension::None)
    {
        return;
    }

    const uint32 Hash = CRC32::Generate(Name.Data(), static_cast<uint64>(Name.Length()) * sizeof(CHAR));
    if (!HashToIndex.Contains(Hash))
    {
        HashToIndex.Add(Hash, static_cast<uint32>(StringPool.Size()));
        StringPool.Add(String(Name.Data()));
    }

    if (Extension == ECrashMarkerExtension::AMDBufferMarker)
    {
    #if VK_AMD_buffer_marker
        const uint32       RingSlot      = RESERVED_SLOTS + (NextIndex % DATA_SLOTS);
        const VkDeviceSize HashOffset    = MemoryLocation.GetBufferOffset() + static_cast<VkDeviceSize>(RingSlot) * sizeof(uint32);
        const VkDeviceSize CounterOffset = MemoryLocation.GetBufferOffset() + static_cast<VkDeviceSize>(SLOT_COUNTER) * sizeof(uint32);

        vkCmdWriteBufferMarkerAMD(CmdBuf.GetVkCommandBuffer(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MemoryLocation.GetBackingBuffer(), HashOffset, Hash);
        vkCmdWriteBufferMarkerAMD(CmdBuf.GetVkCommandBuffer(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MemoryLocation.GetBackingBuffer(), CounterOffset, NextIndex + 1);
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

const String& FVulkanCrashMarkers::ResolveHash(uint32 Hash) const
{
    static const String Unknown("???");
    
    const uint32* Index = HashToIndex.Find(Hash);
    if (Index && *Index < static_cast<uint32>(StringPool.Size()))
    {
        return StringPool[*Index];
    }

    return Unknown;
}

void FVulkanCrashMarkers::DumpCrashMarkers()
{
    LOG_ERROR("----------------------------------------------------------");
    LOG_ERROR("[VulkanRHI] GPU CRASH MARKERS (VK_ERROR_DEVICE_LOST)");
    LOG_ERROR("----------------------------------------------------------");

    if (Extension == ECrashMarkerExtension::AMDBufferMarker)
    {
    #if VK_AMD_buffer_marker
        if (!MappedData)
        {
            LOG_ERROR("[VulkanRHI]   AMD buffer marker: mapped data is null.");
            LOG_ERROR("----------------------------------------------------------");
            return;
        }

        const uint32 GPUCount = MappedData[SLOT_COUNTER];
        if (GPUCount == 0)
        {
            LOG_ERROR("[VulkanRHI]   No markers were executed by the GPU.");
            LOG_ERROR("----------------------------------------------------------");
            return;
        }

        const uint32 ValidCount = (GPUCount < DATA_SLOTS) ? GPUCount : DATA_SLOTS;

        LOG_ERROR("[VulkanRHI]   Showing last %u executed markers:", ValidCount);
        LOG_ERROR("----------------------------------------------------------");

        for (uint32 i = 0; i < ValidCount; i++)
        {
            const uint32 MarkerIdx = GPUCount - ValidCount + i;
            const uint32 RingSlot  = RESERVED_SLOTS + (MarkerIdx % DATA_SLOTS);
            LOG_ERROR("[VulkanRHI]   [%4u] %s", i, *ResolveHash(MappedData[RingSlot]));
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
            LOG_ERROR("----------------------------------------------------------");
            return;
        }

        uint32 CheckpointCount = 0;
        vkGetQueueCheckpointDataNV(GraphicsQueue->GetVkQueue(), &CheckpointCount, nullptr);

        if (CheckpointCount == 0)
        {
            LOG_ERROR("[VulkanRHI]   NV checkpoints: no checkpoint data returned by driver.");
            LOG_ERROR("----------------------------------------------------------");
            return;
        }

        TArray<VkCheckpointDataNV> Checkpoints(CheckpointCount);
        for (uint32 i = 0; i < CheckpointCount; i++)
        {
            Checkpoints[i].sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
            Checkpoints[i].pNext = nullptr;
        }

        vkGetQueueCheckpointDataNV(GraphicsQueue->GetVkQueue(), &CheckpointCount, Checkpoints.Data());

        uint32 HighestReached = 0;
        for (uint32 j = 0; j < CheckpointCount; j++)
        {
            const uintptr_t Marker = reinterpret_cast<uintptr_t>(Checkpoints[j].pCheckpointMarker);
            const uint32    Index  = static_cast<uint32>(Marker & 0xFFFFFFFF);

            if (Index > HighestReached)
            {
                HighestReached = Index;
            }
        }

        LOG_ERROR("[VulkanRHI]   %u checkpoint(s) returned by driver (highest index: %u):", CheckpointCount, HighestReached);
        LOG_ERROR("----------------------------------------------------------");

        // Sort checkpoints by index for display (driver order is unspecified)
        for (uint32 i = 0; i < CheckpointCount; i++)
        {
            for (uint32 j = i + 1; j < CheckpointCount; j++)
            {
                const uint32 IdxI = static_cast<uint32>(reinterpret_cast<uintptr_t>(Checkpoints[i].pCheckpointMarker) & 0xFFFFFFFF);
                const uint32 IdxJ = static_cast<uint32>(reinterpret_cast<uintptr_t>(Checkpoints[j].pCheckpointMarker) & 0xFFFFFFFF);
                if (IdxJ < IdxI)
                {
                    VkCheckpointDataNV Temp = Checkpoints[i];
                    Checkpoints[i] = Checkpoints[j];
                    Checkpoints[j] = Temp;
                }
            }
        }

        for (uint32 i = 0; i < CheckpointCount; i++)
        {
            const uintptr_t Marker = reinterpret_cast<uintptr_t>(Checkpoints[i].pCheckpointMarker);
            const uint32    Hash   = static_cast<uint32>(Marker >> 32);
            LOG_ERROR("[VulkanRHI]   [%4u] %s", i, *ResolveHash(Hash));
        }
    #else
        LOG_ERROR("[VulkanRHI]   NV checkpoints: extension not compiled in.");
    #endif
    }

    LOG_ERROR("----------------------------------------------------------");
}

#endif // VULKAN_ENABLE_CRASH_MARKERS

#if VULKAN_ENABLE_DEVICE_LOST_CHECK

bool VulkanCheckDeviceLost(VkResult Result)
{
    if (Result == VK_ERROR_DEVICE_LOST)
    {
    #if VULKAN_ENABLE_CRASH_MARKERS
        if (FVulkanDeviceRHI* RHI = FVulkanDeviceRHI::Get())
        {
            if (FVulkanCrashMarkers* CrashMarkers = RHI->GetCrashMarkers())
            {
                CrashMarkers->DumpCrashMarkers();
            }
        }
    #endif

    #if VK_EXT_device_fault
        if (FVulkanDeviceRHI* RHI = FVulkanDeviceRHI::Get())
        {
            FVulkanDevice* Device = RHI->GetDevice();
            if (Device && Device->IsExtensionEnabled(VK_EXT_DEVICE_FAULT_EXTENSION_NAME) && vkGetDeviceFaultInfoEXT)
            {
                VkDeviceFaultCountsEXT FaultCounts = {};
                FaultCounts.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT;

                VkResult FaultResult = vkGetDeviceFaultInfoEXT(Device->GetVkDevice(), &FaultCounts, nullptr);
                if (FaultResult == VK_SUCCESS)
                {
                    LOG_ERROR("----------------------------------------------------------");
                    LOG_ERROR("[VulkanRHI] VK_EXT_device_fault: %u address info(s), %u vendor info(s), %llu bytes vendor binary",
                        FaultCounts.addressInfoCount, FaultCounts.vendorInfoCount, FaultCounts.vendorBinarySize);

                    if (FaultCounts.addressInfoCount > 0 || FaultCounts.vendorInfoCount > 0)
                    {
                        TArray<VkDeviceFaultAddressInfoEXT> AddressInfos(FaultCounts.addressInfoCount);
                        Memory::Memzero(AddressInfos.Data(), AddressInfos.Size() * sizeof(VkDeviceFaultAddressInfoEXT));
                        
                        TArray<VkDeviceFaultVendorInfoEXT> VendorInfos(FaultCounts.vendorInfoCount);
                        Memory::Memzero(VendorInfos.Data(), VendorInfos.Size() * sizeof(VkDeviceFaultVendorInfoEXT));

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

                    LOG_ERROR("----------------------------------------------------------");
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

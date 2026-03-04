#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanRHI.h"

#if VULKAN_ENABLE_BREADCRUMBS

FVulkanBreadcrumbs::FVulkanBreadcrumbs(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Backend(EBreadcrumbBackend::None)
    , GraphicsQueue(VK_NULL_HANDLE)
    , MemoryStorage(InDevice)
    , MappedData(nullptr)
    , NextIndex(0)
    , DrawCounter(0)
    , CurrentRegion()
    , MarkerNames()
{
}

FVulkanBreadcrumbs::~FVulkanBreadcrumbs()
{
    Release();
}

bool FVulkanBreadcrumbs::Initialize(VkQueue InGraphicsQueue)
{
    GraphicsQueue = InGraphicsQueue;

#if VK_AMD_buffer_marker
    if (GetDevice()->IsExtensionEnabled(VK_AMD_BUFFER_MARKER_EXTENSION_NAME))
    {
        Backend = EBreadcrumbBackend::AMDBufferMarker;

        constexpr VkDeviceSize          BufferSize       = MAX_MARKERS * sizeof(uint32);
        constexpr VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        constexpr VkBufferUsageFlags    BufferUsage      = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
        if (!MemoryManager.AllocateBufferMemory(MemoryProperties, BufferUsage, 0, BufferSize, sizeof(uint32), MemoryStorage))
        {
            VULKAN_ERROR("FVulkanBreadcrumbs: Failed to allocate breadcrumb buffer");
            Backend = EBreadcrumbBackend::None;
            return false;
        }

        MappedData = reinterpret_cast<uint32*>(MemoryStorage.GetMappedBaseAddress());
        if (!MappedData)
        {
            VULKAN_ERROR("FVulkanBreadcrumbs: Breadcrumb buffer is not host-mapped");
            MemoryStorage.ReleaseMemory();
            Backend = EBreadcrumbBackend::None;
            return false;
        }

        for (uint32 i = 0; i < MAX_MARKERS; i++)
        {
            MappedData[i] = SENTINEL;
        }

        VULKAN_INFO("GPU breadcrumb tracking initialized with VK_AMD_buffer_marker (%u slots)", MAX_MARKERS);
        return true;
    }
#endif

#if VK_NV_device_diagnostic_checkpoints
    if (GetDevice()->IsExtensionEnabled(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME))
    {
        Backend = EBreadcrumbBackend::NVCheckpoints;
        VULKAN_INFO("GPU breadcrumb tracking initialized with VK_NV_device_diagnostic_checkpoints (%u slots)", MAX_MARKERS);
        return true;
    }
#endif

    VULKAN_WARNING("FVulkanBreadcrumbs: No supported breadcrumb extension available (VK_AMD_buffer_marker or VK_NV_device_diagnostic_checkpoints)");
    return false;
}

void FVulkanBreadcrumbs::Release()
{
    if (Backend == EBreadcrumbBackend::AMDBufferMarker)
    {
        MemoryStorage.ReleaseMemory();
        MappedData = nullptr;
    }

    Backend = EBreadcrumbBackend::None;
}

void FVulkanBreadcrumbs::ResetMarkers(FVulkanCommandBuffer& CmdBuf)
{
    NextIndex   = 0;
    DrawCounter = 0;
    CurrentRegion.Clear();

    if (Backend == EBreadcrumbBackend::AMDBufferMarker && MappedData)
    {
        const VkDeviceSize BufferSize = MAX_MARKERS * sizeof(uint32);
        CmdBuf->FillBuffer(MemoryStorage.GetBackingBuffer(), MemoryStorage.GetBufferOffset(), BufferSize, SENTINEL);
    }
}

void FVulkanBreadcrumbs::WriteMarker(FVulkanCommandBuffer& CmdBuf, const FStringView& Name)
{
    CurrentRegion = Name.Data();
    DrawCounter   = 0;
    WriteBreadcrumb(CmdBuf, FString(Name.Data()));
}

void FVulkanBreadcrumbs::WriteDrawMarker(FVulkanCommandBuffer& CmdBuf, const FStringView& DrawType)
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
    WriteBreadcrumb(CmdBuf, Move(Name));
}

void FVulkanBreadcrumbs::WriteBreadcrumb(FVulkanCommandBuffer& CmdBuf, const FString& Name)
{
    if (Backend == EBreadcrumbBackend::None || NextIndex >= MAX_MARKERS)
    {
        return;
    }

    const uint32 Slot = NextIndex;
    MarkerNames[Slot] = Name;
    NextIndex++;

    if (Backend == EBreadcrumbBackend::AMDBufferMarker)
    {
#if VK_AMD_buffer_marker
        const uint32       Value  = Slot + 1;
        const VkDeviceSize Offset = MemoryStorage.GetBufferOffset() + static_cast<VkDeviceSize>(Slot) * sizeof(uint32);
        vkCmdWriteBufferMarkerAMD(CmdBuf.GetVkCommandBuffer(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, MemoryStorage.GetBackingBuffer(), Offset, Value);
#endif
    }
    else if (Backend == EBreadcrumbBackend::NVCheckpoints)
    {
#if VK_NV_device_diagnostic_checkpoints
        vkCmdSetCheckpointNV(CmdBuf.GetVkCommandBuffer(), &MarkerNames[Slot]);
#endif
    }
}

void FVulkanBreadcrumbs::DumpBreadcrumbTrail()
{
    LOG_ERROR("==========================================================");
    LOG_ERROR("[VulkanRHI] GPU BREADCRUMB TRAIL (VK_ERROR_DEVICE_LOST)");
    LOG_ERROR("==========================================================");

    if (NextIndex == 0)
    {
        LOG_ERROR("[VulkanRHI]   No breadcrumbs were recorded this frame.");
        LOG_ERROR("==========================================================");
        return;
    }

    if (Backend == EBreadcrumbBackend::AMDBufferMarker)
    {
#if VK_AMD_buffer_marker
        if (!MappedData)
        {
            LOG_ERROR("[VulkanRHI]   AMD buffer marker: mapped data is null.");
            LOG_ERROR("==========================================================");
            return;
        }

        const uint32 Count = (NextIndex < MAX_MARKERS) ? NextIndex : MAX_MARKERS;
        uint32 LastCompleted = UINT32_MAX;

        for (uint32 i = 0; i < Count; i++)
        {
            const uint32 Expected = i + 1;
            const bool   bDone    = (MappedData[i] == Expected);

            if (bDone)
            {
                LastCompleted = i;
            }

            LOG_ERROR("[VulkanRHI]   [%s] [%4u] %s", bDone ? "DONE       " : "NOT REACHED", i, *MarkerNames[i]);
        }

        LOG_ERROR("----------------------------------------------------------");

        if (LastCompleted == UINT32_MAX)
        {
            LOG_ERROR("[VulkanRHI]   GPU did not complete any recorded breadcrumb.");
        }
        else if (LastCompleted + 1 < Count)
        {
            LOG_ERROR("[VulkanRHI]   Last completed : [%u] %s", LastCompleted, *MarkerNames[LastCompleted]);
            LOG_ERROR("[VulkanRHI]   First not reached: [%u] %s", LastCompleted + 1, *MarkerNames[LastCompleted + 1]);
        }
        else
        {
            LOG_ERROR("[VulkanRHI]   All %u recorded breadcrumbs completed.", Count);
        }
#else
        LOG_ERROR("[VulkanRHI]   AMD buffer marker: extension not compiled in.");
#endif
    }
    else if (Backend == EBreadcrumbBackend::NVCheckpoints)
    {
#if VK_NV_device_diagnostic_checkpoints
        if (GraphicsQueue == VK_NULL_HANDLE)
        {
            LOG_ERROR("[VulkanRHI]   NV checkpoints: no graphics queue available.");
            LOG_ERROR("==========================================================");
            return;
        }

        uint32 CheckpointCount = 0;
        vkGetQueueCheckpointDataNV(GraphicsQueue, &CheckpointCount, nullptr);

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

        vkGetQueueCheckpointDataNV(GraphicsQueue, &CheckpointCount, Checkpoints.Data());

        const uint32 Count = (NextIndex < MAX_MARKERS) ? NextIndex : MAX_MARKERS;
        for (uint32 i = 0; i < Count; i++)
        {
            bool bReached = false;
            for (uint32 j = 0; j < CheckpointCount; j++)
            {
                if (Checkpoints[j].pCheckpointMarker == &MarkerNames[i])
                {
                    bReached = true;
                    break;
                }
            }

            LOG_ERROR("[VulkanRHI]   [%s] [%4u] %s", bReached ? "REACHED    " : "NOT REACHED", i, *MarkerNames[i]);
        }

        LOG_ERROR("----------------------------------------------------------");
        LOG_ERROR("[VulkanRHI]   %u checkpoint(s) returned by driver.", CheckpointCount);
#else
        LOG_ERROR("[VulkanRHI]   NV checkpoints: extension not compiled in.");
#endif
    }

    LOG_ERROR("==========================================================");
}

#endif // VULKAN_ENABLE_BREADCRUMBS

#if VULKAN_ENABLE_DEVICE_LOST_CHECK

bool VulkanCheckDeviceLost(VkResult Result)
{
    if (Result == VK_ERROR_DEVICE_LOST)
    {
#if VULKAN_ENABLE_BREADCRUMBS
        if (FVulkanRHI* RHI = FVulkanRHI::Get())
        {
            if (FVulkanBreadcrumbs* Breadcrumbs = RHI->GetBreadcrumbs())
            {
                Breadcrumbs->DumpBreadcrumbTrail();
            }
        }
#endif
    }

    return true;
}

#endif // VULKAN_ENABLE_DEVICE_LOST_CHECK

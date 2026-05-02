#include "VulkanRHI/VulkanQuery.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanDeviceLimits.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanQueue.h"

FVulkanQueryRHI::FVulkanQueryRHI(FVulkanDevice* InDevice, EQueryType InQueryType)
    : FRHIQuery(InQueryType)
    , FVulkanDeviceChild(InDevice)
    , CurrentQuery()
    , SyncFence()
    , QueryResult(static_cast<uint64*>(FMemory::Malloc(GetQueryResultElementCount(InQueryType) * sizeof(uint64))))
{
    FMemory::Memzero(QueryResult, GetQueryResultElementCount(InQueryType) * sizeof(uint64));
}

FVulkanQueryRHI::~FVulkanQueryRHI()
{
    FMemory::Free(QueryResult);
    QueryResult = nullptr;
}

bool FVulkanQuery::CopyResult(void* Dst, uint64 DstSize) const
{
    if (!QueryPool || QueryIndex == VULKAN_INVALID_QUERY_INDEX || !Dst || DstSize == 0)
    {
        return false;
    }

#if VULKAN_USE_CPU_QUERY_RESOLVE
    const uint64 TotalSize = DstSize + sizeof(uint64);
    uint8 TempBuffer[sizeof(FVulkanPipelineStatisticsQueryData)] = {};
    CHECK(TotalSize <= sizeof(TempBuffer));

    const VkQueryResultFlags Flags = VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT;
    VkResult Result = vkGetQueryPoolResults(
        QueryPool->GetDevice()->GetVkDevice(),
        QueryPool->GetVkQueryPool(),
        QueryIndex,
        1,
        TotalSize,
        TempBuffer,
        TotalSize,
        Flags);

    if (Result == VK_SUCCESS || Result == VK_NOT_READY)
    {
        const uint64 Availability = *reinterpret_cast<const uint64*>(TempBuffer + DstSize);
        if (Availability)
        {
            FMemory::Memcpy(Dst, TempBuffer, DstSize);
            return true;
        }
    }

    return false;
#else
    const uint64* MappedData = QueryPool->GetReadbackData();
    if (!MappedData)
    {
        return false;
    }

    const uint64 Stride = QueryPool->GetQuerySize();
    FMemory::Memcpy(Dst, reinterpret_cast<const uint8*>(MappedData) + QueryIndex * Stride, static_cast<size_t>(DstSize));
    return true;
#endif
}

FVulkanQueryPool::FVulkanQueryPool(FVulkanDevice* InDevice, VkQueryType InQueryType, int32 InNumQueries)
    : FVulkanDeviceChild(InDevice)
    , QueryType(InQueryType)
    , NumQueries(InNumQueries)
    , QueryPool(VK_NULL_HANDLE)
#if !VULKAN_USE_CPU_QUERY_RESOLVE
    , ReadbackStorage(InDevice)
    , ReadbackData(nullptr)
#endif
{
}

FVulkanQueryPool::~FVulkanQueryPool()
{
#if !VULKAN_USE_CPU_QUERY_RESOLVE
    ReadbackStorage.ReleaseMemory();
    ReadbackData = nullptr;
#endif

    if (VULKAN_CHECK_HANDLE(QueryPool))
    {
        vkDestroyQueryPool(GetDevice()->GetVkDevice(), QueryPool, nullptr);
        QueryPool = VK_NULL_HANDLE;

#if VULKAN_ENABLE_STATS
        STAT_SUBTRACT(STAT_Vulkan_QueryPoolCount, 1);
#endif
    }
}

bool FVulkanQueryPool::Initialize()
{
    VkQueryPoolCreateInfo QueryPoolCreateInfo = {};
    QueryPoolCreateInfo.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    QueryPoolCreateInfo.queryType  = QueryType;
    QueryPoolCreateInfo.queryCount = NumQueries;

    if (QueryType == VK_QUERY_TYPE_PIPELINE_STATISTICS)
    {
        QueryPoolCreateInfo.pipelineStatistics =
            VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
    }

    VkResult result = vkCreateQueryPool(GetDevice()->GetVkDevice(), &QueryPoolCreateInfo, nullptr, &QueryPool);
    if (VULKAN_FAILED(result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create QueryPool");
        return false;
    }

    vkResetQueryPool(GetDevice()->GetVkDevice(), QueryPool, 0, NumQueries);

#if !VULKAN_USE_CPU_QUERY_RESOLVE
    const uint64 ReadbackSize = NumQueries * GetQuerySize();
    if (!GetDevice()->GetMemoryManager().AllocateBufferMemory(
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        0,
        ReadbackSize,
        sizeof(uint64),
        ReadbackStorage))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate query readback buffer");
        return false;
    }

    ReadbackData = reinterpret_cast<uint64*>(ReadbackStorage.GetMappedBaseAddress());
    if (!ReadbackData)
    {
        VULKAN_ERROR_CRITICAL("Failed to map query readback buffer");
        return false;
    }
#endif

#if VULKAN_ENABLE_STATS
    STAT_ADD(STAT_Vulkan_QueryPoolCount, 1);
#endif

    return true;
}

void FVulkanQueryPool::ResetPool()
{
    vkResetQueryPool(GetDevice()->GetVkDevice(), QueryPool, 0, NumQueries);
}

void FVulkanQueryPool::SetDebugName(const FString& InName)
{
    if (VULKAN_CHECK_HANDLE(QueryPool))
    {
        VulkanSetObjectName(GetDevice()->GetVkDevice(), *InName, QueryPool, VK_OBJECT_TYPE_QUERY_POOL);
        DebugName = InName;
    }
}

FVulkanQueryAllocator::FVulkanQueryAllocator(FVulkanDevice* InDevice, VkQueryType InQueryType)
    : FVulkanDeviceChild(InDevice)
    , QueryType(InQueryType)
{
}

FVulkanQueryAllocator::~FVulkanQueryAllocator()
{
}

bool FVulkanQueryAllocator::Allocate(FVulkanQuery& OutQuery, uint64* ResultTarget, EVulkanQueryType InType)
{
    bool bNeedNewPool = Ranges.IsEmpty()
        || Ranges.LastElement().Count >= Ranges.LastElement().Pool->NumQueries;

    if (bNeedNewPool)
    {
        FVulkanQueryPool* Pool = GetDevice()->ObtainQueryPool(QueryType);
        if (!Pool)
            return false;

        Ranges.Add(FVulkanQueryRange(Pool, 0, 0));
    }

    FVulkanQueryRange& Range = Ranges.LastElement();
    OutQuery = FVulkanQuery(Range.Pool, Range.StartIndex + Range.Count, ResultTarget, InType);
    Range.Count++;
    return true;
}

void FVulkanQueryAllocator::Reset(TArray<FVulkanQueryRange>& OutRanges)
{
    for (FVulkanQueryRange& Range : Ranges)
    {
        OutRanges.Add(Move(Range));
    }
    Ranges.Clear();
}

FVulkanQueryPoolManager::FVulkanQueryPoolManager(FVulkanDevice* InDevice, VkQueryType InQueryType, int32 InQueriesPerPool)
    : FVulkanDeviceChild(InDevice)
    , QueryType(InQueryType)
    , QueriesPerPool(InQueriesPerPool)
{
}

FVulkanQueryPoolManager::~FVulkanQueryPoolManager()
{
    TScopedLock Lock(PoolsCS);
    for (FVulkanQueryPool* Pool : AllPools)
    {
        delete Pool;
    }
    AllPools.Clear();
}

FVulkanQueryPool* FVulkanQueryPoolManager::ObtainPool()
{
    TScopedLock Lock(PoolsCS);

    FVulkanQueryPool* Pool = nullptr;
    if (AvailablePools.Dequeue(Pool))
    {
        Pool->ResetPool();
        return Pool;
    }

    Pool = new FVulkanQueryPool(GetDevice(), QueryType, QueriesPerPool);
    if (!Pool->Initialize())
    {
        delete Pool;
        return nullptr;
    }

    const FString DebugName = FString::CreateFormatted("QueryPool [%d]", AllPools.Size());
    Pool->SetDebugName(DebugName);

    AllPools.Add(Pool);
    return Pool;
}

void FVulkanQueryPoolManager::RecyclePool(FVulkanQueryPool* Pool)
{
    TScopedLock Lock(PoolsCS);
    AvailablePools.Enqueue(Pool);
}

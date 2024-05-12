#include "VulkanQuery.h"
#include "VulkanDeviceLimits.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformInterlocked.h"

static TAutoConsoleVariable<int32> CVarVulkanQueryPoolSize(
    "VulkanRHI.QueryPoolSize",
    "Default size of QueryPools in Vulkan",
    VULKAN_DEFAULT_QUERY_COUNT,
    EConsoleVariableFlags::Default);

FVulkanQuery::FVulkanQuery(FVulkanDevice* InDevice, EQueryType InQueryType)
    : FRHIQuery(InQueryType)
    , FVulkanDeviceChild(InDevice)
    , QueryAllocation()
    , Result(0)
{
}

FVulkanQueryPool::FVulkanQueryPool(FVulkanDevice* InDevice, FVulkanQueryPoolManager* InQueryPoolManager, EQueryType InQueryType)
    : FVulkanDeviceChild(InDevice)
    , QueryPoolManager(InQueryPoolManager)
    , QueryPool(VK_NULL_HANDLE)
    , NumQueries(0)
    , CurrentQueryIndex(0)
    , QueryType(InQueryType)
{
}

FVulkanQueryPool::~FVulkanQueryPool()
{
    if (VULKAN_CHECK_HANDLE(QueryPool))
    {
        vkDestroyQueryPool(GetDevice()->GetVkDevice(), QueryPool, nullptr);
        QueryPool = VK_NULL_HANDLE;
    }
}

bool FVulkanQueryPool::Initialize()
{
    VkQueryPoolCreateInfo QueryPoolCreateInfo;
    FMemory::Memzero(&QueryPoolCreateInfo, sizeof(VkQueryPoolCreateInfo));

    QueryPoolCreateInfo.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    QueryPoolCreateInfo.queryType  = ConvertQueryType(QueryType);
    QueryPoolCreateInfo.queryCount = NumQueries = CVarVulkanQueryPoolSize.GetValue();

    VkResult result = vkCreateQueryPool(GetDevice()->GetVkDevice(), &QueryPoolCreateInfo, nullptr, &QueryPool);
    if (VULKAN_FAILED(result))
    {
        VULKAN_ERROR("Failed to create QueryPool");
        return false;
    }

    // Reset pool before use
    vkResetQueryPool(GetDevice()->GetVkDevice(), QueryPool, 0, NumQueries);

    // Allocate enough timing queries
    QueryAllocations.Resize(NumQueries);
    return true;
}

FVulkanQueryAllocation FVulkanQueryPool::Allocate(uint64* InResults)
{
    int32 Index = CurrentQueryIndex++;
    if (Index >= NumQueries)
    {
        return FVulkanQueryAllocation();
    }

    QueryAllocations[Index] = FVulkanQueryAllocation(this, Index, InResults);
    return QueryAllocations[Index];
}

void FVulkanQueryPool::Reset()
{
    // Reset query handle
    const uint32 NumUsedQueries = FMath::Min<int32>(CurrentQueryIndex, NumQueries);
    vkResetQueryPool(GetDevice()->GetVkDevice(), QueryPool, 0, NumUsedQueries);

    // Reset the query index
    CurrentQueryIndex = 0;
}

void FVulkanQueryPool::ResolveQueries()
{
    if (CurrentQueryIndex == 0)
    {
        return;
    }

    const uint32 NumUsedQueries = FMath::Min<int32>(CurrentQueryIndex, NumQueries);
    const VkQueryResultFlags Flags = VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT;

    if (QueryType == EQueryType::Timestamp)
    {
        TArray<FVulkanTimingQuery> QueryData;
        QueryData.Resize(NumUsedQueries);

        VkResult result = vkGetQueryPoolResults(GetDevice()->GetVkDevice(), QueryPool, 0, NumUsedQueries, QueryData.SizeInBytes(), QueryData.Data(), sizeof(FVulkanTimingQuery), Flags);
        if (!(result == VK_SUCCESS || result == VK_NOT_READY))
        {
            VULKAN_ERROR("Failed to retrieve QueryPool results");
            return;
        }

        for (int32 Index = 0; Index < NumUsedQueries; Index++)
        {
            FVulkanTimingQuery& TimingQuery = QueryData[Index];
            if (TimingQuery.Availability)
            {
                uint64* Results = QueryAllocations[Index].Results;
                *Results = static_cast<uint64>(static_cast<double>(TimingQuery.Timestamp) * static_cast<double>(FVulkanDeviceLimits::TimestampPeriod));
            }
        }
    }
    else if (QueryType == EQueryType::Occlusion)
    {
        TArray<FVulkanOcclusionQuery> QueryData;
        QueryData.Resize(NumUsedQueries);

        VkResult result = vkGetQueryPoolResults(GetDevice()->GetVkDevice(), QueryPool, 0, NumUsedQueries, QueryData.SizeInBytes(), QueryData.Data(), sizeof(FVulkanOcclusionQuery), Flags);
        if (!(result == VK_SUCCESS || result == VK_NOT_READY))
        {
            VULKAN_ERROR("Failed to retrieve QueryPool results");
            return;
        }

        for (int32 Index = 0; Index < NumUsedQueries; Index++)
        {
            FVulkanOcclusionQuery& OcclusionQuery = QueryData[Index];
            if (OcclusionQuery.Availability)
            {
                uint64* Results = QueryAllocations[Index].Results;
                *Results = OcclusionQuery.NumSamples;
            }
        }
    }
}

void FVulkanQueryPool::SetDebugName(const FString& InName)
{
    if (VULKAN_CHECK_HANDLE(QueryPool))
    {
        FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.GetCString(), QueryPool, VK_OBJECT_TYPE_QUERY_POOL);
        DebugName = InName;
    }
}

FVulkanQueryAllocator::FVulkanQueryAllocator(FVulkanDevice* InDevice, FVulkanCommandContext& InContext, EQueryType InQueryType)
    : FVulkanDeviceChild(InDevice)
    , Context(InContext)
    , QueryPool(nullptr)
    , QueryType(InQueryType)
{
    QueryPoolManager = InDevice->GetQueryPoolManager(QueryType);
    CHECK(QueryPoolManager != nullptr);
}

FVulkanQueryAllocator::~FVulkanQueryAllocator()
{
    // NOTE: The QueryPool should have been released when the context flushed it's CommandBuffer
    CHECK(QueryPool == nullptr);
}

FVulkanQueryAllocation FVulkanQueryAllocator::Allocate(uint64* InResults)
{
    if (!QueryPool)
    {
        QueryPool = QueryPoolManager->ObtainQueryPool();
    }

    FVulkanQueryAllocation QueryAllocation = QueryPool->Allocate(InResults);
    if (!QueryAllocation.IsValid())
    {
        Context.GetCommandPayload().AddQueryPool(QueryPool);
        QueryPool = QueryPoolManager->ObtainQueryPool();
        QueryAllocation = QueryPool->Allocate(InResults);
    }

    return QueryAllocation;
}

void FVulkanQueryAllocator::PrepareForNewCommanBuffer()
{
    if (QueryPool)
    {
        Context.GetCommandPayload().AddQueryPool(QueryPool);
        QueryPool = nullptr;
    }
}

FVulkanQueryPoolManager::FVulkanQueryPoolManager(FVulkanDevice* InDevice, EQueryType InQueryType)
    : FVulkanDeviceChild(InDevice)
    , QueryType(InQueryType)
    , AvailableQueryPools()
    , QueryPools()
    , QueryPoolsCS()
{
}

FVulkanQueryPoolManager::~FVulkanQueryPoolManager()
{
    TScopedLock Lock(QueryPoolsCS);

    for (FVulkanQueryPool* QueryPool : QueryPools)
    {
        delete QueryPool;
    }

    QueryPools.Clear();
    AvailableQueryPools.Clear();
}

FVulkanQueryPool* FVulkanQueryPoolManager::ObtainQueryPool()
{
    TScopedLock Lock(QueryPoolsCS);

    FVulkanQueryPool* QueryPool = nullptr;
    if (AvailableQueryPools.Dequeue(QueryPool))
    {
        QueryPool->Reset();
        return QueryPool;
    }

    QueryPool = new FVulkanQueryPool(GetDevice(), this, QueryType);
    if (!QueryPool->Initialize())
    {
        DEBUG_BREAK();
        delete QueryPool;
        return nullptr;
    }

    const FString DebugName = FString::CreateFormatted("QueryPool [%s %d]", ToString(QueryType), QueryPools.Size());
    QueryPool->SetDebugName(DebugName);

    QueryPools.Add(QueryPool);
    return QueryPool;
}

void FVulkanQueryPoolManager::RecycleQueryPool(FVulkanQueryPool* InQueryPool)
{
    TScopedLock Lock(QueryPoolsCS);
    AvailableQueryPools.Enqueue(InQueryPool);
}

#include "VulkanQuery.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformInterlocked.h"

static TAutoConsoleVariable<int32> CVarVulkanQueryPoolSize(
    "VulkanRHI.QueryPoolSize",
    "Default size of QueryPools in Vulkan",
    256);

// TODO: Store this with other globals in the Vulkan module
static float GTimestampPeriod = 0.0f;

FVulkanQuery::FVulkanQuery(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , QueryPool(nullptr)
    , ResolvedQueryPool(nullptr)
{
}

FVulkanQuery::~FVulkanQuery()
{
    if (ResolvedQueryPool)
    {
        GetDevice()->GetQueryPoolManager().RecycleQueryPool(ResolvedQueryPool);
    }
}

void FVulkanQuery::GetTimestampFromIndex(FTimingQuery& OutQuery, uint32 Index) const
{
    FVulkanQueryPool* CurrentPool = ResolvedQueryPool;
    if (CurrentPool)
    {
        const uint32 FinalIndex = Index * 2;
        if (FinalIndex < CurrentPool->QueryData.Size())
        {
            const FVulkanTimingQuery& FirstQuery = CurrentPool->QueryData[FinalIndex];
            if (FirstQuery.Availability)
            {
                OutQuery.Begin = static_cast<double>(FirstQuery.Timestamp) * GTimestampPeriod;
            }
            else
            {
                OutQuery.Begin = 0;
            }

            const FVulkanTimingQuery& SecondQuery = CurrentPool->QueryData[FinalIndex + 1];
            if (SecondQuery.Availability)
            {
                OutQuery.End = static_cast<double>(SecondQuery.Timestamp) * GTimestampPeriod;
            }
            else
            {
                OutQuery.End = OutQuery.Begin;
            }

            return;
        }
    }

    // NOTE: If we do not have any resolved queries yet, then return a null query
    OutQuery.Begin = 0;
    OutQuery.End   = 0;
}

uint64 FVulkanQuery::GetFrequency() const
{
    return 1000000000;
}

void FVulkanQuery::BeginQuery(FVulkanCommandBuffer& CommandBuffer, uint32 Index)
{
    if (!QueryPool)
    {
        QueryPool = GetDevice()->GetQueryPoolManager().ObtainQueryPool();
        CHECK(QueryPool->RHIQuery == nullptr);
        QueryPool->RHIQuery = this;
    }

    const uint32 FinalIndex = Index * 2;
    if (QueryPool->AllocateIndex(FinalIndex))
    {
        CommandBuffer->WriteTimestamp(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, QueryPool->GetVkQueryPool(), FinalIndex);
    }
    else
    {
        DEBUG_BREAK();
    }
}

void FVulkanQuery::EndQuery(FVulkanCommandBuffer& CommandBuffer, uint32 Index)
{
    if (!QueryPool)
    {
        QueryPool = GetDevice()->GetQueryPoolManager().ObtainQueryPool();
        CHECK(QueryPool->RHIQuery == nullptr);
        QueryPool->RHIQuery = this;
    }

    const uint32 FinalIndex = (Index * 2) + 1;
    if (QueryPool->AllocateIndex(FinalIndex))
    {
        CommandBuffer->WriteTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, QueryPool->GetVkQueryPool(), FinalIndex);
    }
    else
    {
        DEBUG_BREAK();
    }
}

FVulkanQueryPool::FVulkanQueryPool(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , QueryPool(VK_NULL_HANDLE)
    , UsedQueries()
    , QueryData()
    , RHIQuery(nullptr)
    , NumQueries(0)
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
    // TODO: Do this once at DeviceCreation
    if (GTimestampPeriod == 0.0f)
    {
        const VkPhysicalDeviceProperties& Properties = GetDevice()->GetPhysicalDevice()->GetProperties();
        GTimestampPeriod = Properties.limits.timestampPeriod;
        CHECK(GTimestampPeriod != 0.0f);
    }

    VkQueryPoolCreateInfo QueryPoolCreateInfo;
    FMemory::Memzero(&QueryPoolCreateInfo, sizeof(VkQueryPoolCreateInfo));

    QueryPoolCreateInfo.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    QueryPoolCreateInfo.queryType  = VK_QUERY_TYPE_TIMESTAMP;
    QueryPoolCreateInfo.queryCount = NumQueries = CVarVulkanQueryPoolSize.GetValue();

    VkResult result = vkCreateQueryPool(GetDevice()->GetVkDevice(), &QueryPoolCreateInfo, nullptr, &QueryPool);
    if (VULKAN_FAILED(result))
    {
        VULKAN_ERROR("Failed to create QueryPool");
        return false;
    }
    else
    {
        // Reset pool before use
        Reset();

        // Allocate enough timing queries
        QueryData.Resize(NumQueries);
        UsedQueries.Resize(NumQueries);
    }

    return true;
}

void FVulkanQueryPool::Reset()
{
    // Reset query handle
    vkResetQueryPool(GetDevice()->GetVkDevice(), QueryPool, 0, NumQueries);

    // Reset RHI Query object
    RHIQuery = nullptr;

    // Release all "used" indices
    UsedQueries.Reset();
}

void FVulkanQueryPool::ResolveQueries()
{
    if (UsedQueries.HasNoBitSet())
    {
        return;
    }

    // NOTE: This means that queries between 0 and MostSignificant could be unwritten, however that is solved since we have the availability bit set
    const uint32 NumUsedQueries = static_cast<uint32>(UsedQueries.MostSignificant()) + 1;

    const VkQueryResultFlags Flags = VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT;
    VkResult result = vkGetQueryPoolResults(GetDevice()->GetVkDevice(), QueryPool, 0, NumUsedQueries, QueryData.SizeInBytes(), QueryData.Data(), sizeof(FVulkanTimingQuery), Flags);
    if (result == VK_SUCCESS || result == VK_NOT_READY)
    {
        FVulkanQueryPool* OldQueryPool = reinterpret_cast<FVulkanQueryPool*>(FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&RHIQuery->ResolvedQueryPool), this));
        if (OldQueryPool)
        {
            GetDevice()->GetQueryPoolManager().RecycleQueryPool(OldQueryPool);
        }
    }
    else
    {
        VULKAN_ERROR("Failed to retrieve QueryPool results");
    }
}

bool FVulkanQueryPool::AllocateIndex(uint32 Index)
{
    if (Index >= static_cast<uint32>(UsedQueries.Size()))
        return false;

    if (UsedQueries[Index])
        return false;

    UsedQueries[Index] = true;
    return true;
}

FVulkanQueryPoolManager::FVulkanQueryPoolManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , QueryPools()
{
}

FVulkanQueryPoolManager::~FVulkanQueryPoolManager()
{
    ReleaseAll();
}

FVulkanQueryPool* FVulkanQueryPoolManager::ObtainQueryPool()
{
    {
        TScopedLock Lock(QueryPoolsCS);

        if (!QueryPools.IsEmpty())
        {
            FVulkanQueryPool* QueryPool = QueryPools.LastElement();
            QueryPool->Reset();
            QueryPools.Pop();
            return QueryPool;
        }
    }

    FVulkanQueryPool* QueryPool = new FVulkanQueryPool(GetDevice());
    if (!QueryPool->Initialize())
    {
        return nullptr;
    }

    return QueryPool;
}

void FVulkanQueryPoolManager::RecycleQueryPool(FVulkanQueryPool* InQueryPool)
{
    TScopedLock Lock(QueryPoolsCS);
    QueryPools.Add(InQueryPool);
}

void FVulkanQueryPoolManager::ReleaseAll()
{
    TScopedLock Lock(QueryPoolsCS);

    for (FVulkanQueryPool* QueryPool : QueryPools)
    {
        delete QueryPool;
    }

    QueryPools.Clear();
}
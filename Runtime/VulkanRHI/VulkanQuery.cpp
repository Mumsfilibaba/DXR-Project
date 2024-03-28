#include "VulkanQuery.h"
#include "Core/Misc/ConsoleManager.h"

static TAutoConsoleVariable<int32> CVarVulkanQueryPoolSize(
    "VulkanRHI.QueryPoolSize",
    "Default size of QueryPools in Vulkan",
    256);


FVulkanQuery::FVulkanQuery(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , QueryPool(VK_NULL_HANDLE)
    , NumQueries(0)
    , TimestampPeriod(0.0f)
{
}

FVulkanQuery::~FVulkanQuery()
{
    if (VULKAN_CHECK_HANDLE(QueryPool))
    {
        vkDestroyQueryPool(GetDevice()->GetVkDevice(), QueryPool, nullptr);
        QueryPool = VK_NULL_HANDLE;
    }
}

bool FVulkanQuery::Initialize()
{
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
        vkResetQueryPool(GetDevice()->GetVkDevice(), QueryPool, 0, NumQueries);
    }

    const VkPhysicalDeviceProperties& Properties = GetDevice()->GetPhysicalDevice()->GetProperties();
    TimestampPeriod = Properties.limits.timestampPeriod;
    return true;
}

void FVulkanQuery::GetTimestampFromIndex(FRHITimestamp& OutQuery, uint32 Index) const
{
    TScopedLock Lock(QueriesCS);

    if (Index < Queries.Size())
    {
        OutQuery = Queries[Index];
    }
    else
    {
        OutQuery.Begin = 0;
        OutQuery.End   = 0;
    }
}

uint64 FVulkanQuery::GetFrequency() const
{
    return 1000000;
}

void FVulkanQuery::BeginQuery(FVulkanCommandBuffer& CommandBuffer, uint32 Index)
{
    const uint32 FinalIndex = Index * 2;
    if (FinalIndex < NumQueries)
    {
        CommandBuffer->WriteTimestamp(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, QueryPool, FinalIndex);
    }
    else
    {
        DEBUG_BREAK();
    }
}

void FVulkanQuery::EndQuery(FVulkanCommandBuffer& CommandBuffer, uint32 Index)
{
    const uint32 FinalIndex = (Index * 2) + 1;
    if (FinalIndex < NumQueries)
    {
        CommandBuffer->WriteTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, QueryPool, FinalIndex);
    }
    else
    {
        DEBUG_BREAK();
    }
}

void FVulkanQuery::ResolveQueries()
{
    TScopedLock Lock(QueriesCS);

    // Each timestamp contains 2 queries (Both begin and end)
    const int32 NumTimestamps = NumQueries / 2;
    Queries.Resize(NumTimestamps);

    VkResult result = vkGetQueryPoolResults(GetDevice()->GetVkDevice(), QueryPool, 0, NumQueries, Queries.SizeInBytes(), Queries.Data(), sizeof(uint64), VK_QUERY_RESULT_64_BIT);
    if (VULKAN_FAILED(result))
    {
        VULKAN_ERROR("Failed to retrieve QueryPool results");
    }
    else
    {
        vkResetQueryPool(GetDevice()->GetVkDevice(), QueryPool, 0, NumQueries);
    }
}

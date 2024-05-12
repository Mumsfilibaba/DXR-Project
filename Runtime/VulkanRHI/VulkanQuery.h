#pragma once
#include "VulkanDeviceChild.h"
#include "RHI/RHIResources.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/BitArray.h"
#include "Core/Containers/Queue.h"

#define VULKAN_INVALID_QUERY_INDEX (-1)

class FVulkanQueryPool;
class FVulkanQueryPoolManager;
class FVulkanCommandBuffer;
class FVulkanCommandContext;

typedef TSharedRef<struct FVulkanQuery> FVulkanQueryRef;

struct FVulkanTimingQuery
{
    uint64 Timestamp;
    uint64 Availability;
};

struct FVulkanOcclusionQuery
{
    uint64 NumSamples;
    uint64 Availability;
};

struct FVulkanQueryAllocation
{
    FVulkanQueryAllocation()
        : QueryPool(nullptr)
        , IndexInQueryPool(VULKAN_INVALID_QUERY_INDEX)
        , Results(nullptr)
    {
    }

    FVulkanQueryAllocation(FVulkanQueryPool* InQueryPool, int32 InIndexInQueryPool, uint64* InResults)
        : QueryPool(InQueryPool)
        , IndexInQueryPool(InIndexInQueryPool)
        , Results(InResults)
    {
    }

    bool IsValid() const
    {
        return QueryPool != nullptr && IndexInQueryPool != VULKAN_INVALID_QUERY_INDEX;
    }

    FVulkanQueryPool* QueryPool;
    int32             IndexInQueryPool;
    uint64*           Results;
};

struct FVulkanQuery : public FRHIQuery, public FVulkanDeviceChild
{
    FVulkanQuery(FVulkanDevice* InDevice, EQueryType InQueryType);
    virtual ~FVulkanQuery() = default;

    // QueryAllocation should only used on RHI Thread
    FVulkanQueryAllocation QueryAllocation;
    uint64                 Result;
};

class FVulkanQueryPool : public FVulkanDeviceChild
{
public:
    FVulkanQueryPool(FVulkanDevice* InDevice, FVulkanQueryPoolManager* InQueryPoolManager, EQueryType InQueryType);
    ~FVulkanQueryPool();

    bool Initialize();
    FVulkanQueryAllocation Allocate(uint64* InResults);
    void Reset();
    void ResolveQueries();
    void SetDebugName(const FString& InName);
    const FString& GetDebugName() const { return DebugName; }

    VkQueryPool GetVkQueryPool() const
    {
        return QueryPool;
    }

    uint32 GetMaxQueries() const
    {
        return NumQueries;
    }

    FVulkanQueryPoolManager* GetQueryPoolManager() const
    {
        return QueryPoolManager;
    }

    EQueryType GetType() const 
    {
        return QueryType;
    }
    
private:
    FVulkanQueryPoolManager*       QueryPoolManager;
    VkQueryPool                    QueryPool;
    TArray<FVulkanQueryAllocation> QueryAllocations;
    int32                          CurrentQueryIndex;
    int32                          NumQueries;
    EQueryType                     QueryType;
    FString                        DebugName;
};

class FVulkanQueryAllocator : public FVulkanDeviceChild
{
public:
    FVulkanQueryAllocator(FVulkanDevice* InDevice, FVulkanCommandContext& InContext, EQueryType InQueryType);
    ~FVulkanQueryAllocator();

    FVulkanQueryAllocation Allocate(uint64* InResults);
    void PrepareForNewCommanBuffer();

private:
    FVulkanCommandContext&   Context; 
    FVulkanQueryPool*        QueryPool;
    EQueryType               QueryType;
    FVulkanQueryPoolManager* QueryPoolManager;
};

class FVulkanQueryPoolManager : public FVulkanDeviceChild
{
public:
    FVulkanQueryPoolManager(FVulkanDevice* InDevice, EQueryType InQueryType);
    ~FVulkanQueryPoolManager();

    FVulkanQueryPool* ObtainQueryPool();
    void RecycleQueryPool(FVulkanQueryPool* InQueryPool);

private:
    EQueryType                QueryType;
    TQueue<FVulkanQueryPool*> AvailableQueryPools;
    TArray<FVulkanQueryPool*> QueryPools;
    FCriticalSection          QueryPoolsCS;
};

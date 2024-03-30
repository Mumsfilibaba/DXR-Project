#pragma once
#include "VulkanDeviceChild.h"
#include "RHI/RHIResources.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/BitArray.h"

class FVulkanQueryPool;
class FVulkanCommandBuffer;

typedef TSharedRef<class FVulkanQuery> FVulkanQueryRef;

class FVulkanQuery : public FRHIQuery, public FVulkanDeviceChild
{
    friend class FVulkanQueryPool;

public:
    FVulkanQuery(FVulkanDevice* InDevice);
    virtual ~FVulkanQuery();

    virtual void   GetTimestampFromIndex(FTimingQuery& OutQuery, uint32 Index) const override final;
    virtual uint64 GetFrequency() const override final;

    void BeginQuery(FVulkanCommandBuffer& CommandBuffer, uint32 Index);
    void EndQuery(FVulkanCommandBuffer& CommandBuffer, uint32 Index);

    inline FVulkanQueryPool* DetachQueryPool()
    {
        FVulkanQueryPool* ReturnQueryPool = QueryPool;
        QueryPool = nullptr;
        return ReturnQueryPool;
    }

private:
    FVulkanQueryPool* volatile QueryPool;
    FVulkanQueryPool* volatile ResolvedQueryPool;
};

struct FVulkanTimingQuery
{
    uint64 Timestamp;
    uint64 Availability;
};

class FVulkanQueryPool : public FVulkanDeviceChild
{
    friend class FVulkanQuery;

public:
    FVulkanQueryPool(FVulkanDevice* InDevice);
    ~FVulkanQueryPool();
    
    bool Initialize();
    void Reset();
    void ResolveQueries();

    bool AllocateIndex(uint32 Index);

    VkQueryPool GetVkQueryPool() const
    {
        return QueryPool;
    }

    uint32 GetMaxQueries() const
    {
        return NumQueries;
    }

private:
    VkQueryPool                QueryPool;
    FBitArray                  UsedQueries;
    TArray<FVulkanTimingQuery> QueryData;
    FVulkanQuery*              RHIQuery;
    uint32                     NumQueries;
};

class FVulkanQueryPoolManager : public FVulkanDeviceChild
{
public:   
    FVulkanQueryPoolManager(FVulkanDevice* InDevice);
    ~FVulkanQueryPoolManager();

    FVulkanQueryPool* ObtainQueryPool();
    void RecycleQueryPool(FVulkanQueryPool* InQueryPool);

    void ReleaseAll();

private:
    TArray<FVulkanQueryPool*> QueryPools;
    FCriticalSection          QueryPoolsCS;
};

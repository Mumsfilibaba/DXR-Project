#pragma once
#include "VulkanDeviceChild.h"
#include "RHI/RHIResources.h"
#include "Core/Containers/SharedRef.h"

class FVulkanCommandBuffer;

typedef TSharedRef<class FVulkanQuery> FVulkanQueryRef;

class FVulkanQuery : public FRHIQuery, public FVulkanDeviceChild
{
public:
    FVulkanQuery(FVulkanDevice* InDevice);
    virtual ~FVulkanQuery();

    virtual void GetTimestampFromIndex(FRHITimestamp& OutQuery, uint32 Index) const override final;
    virtual uint64 GetFrequency() const override final;

    bool Initialize();

    void BeginQuery(FVulkanCommandBuffer& CommandBuffer, uint32 Index);
    void EndQuery(FVulkanCommandBuffer& CommandBuffer, uint32 Index);

    void ResolveQueries();

    VkQueryPool GetVkQueryPool() const
    {
        return QueryPool;
    }

    uint32 GetNumQueries() const
    {
        return NumQueries;
    }

private:
    VkQueryPool QueryPool;

    mutable TArray<FRHITimestamp> Queries;
    mutable FCriticalSection      QueriesCS;

    uint32 NumQueries;
    float  TimestampPeriod;
};

class FVulkanQueryPool : public FVulkanDeviceChild
{
public:
    FVulkanQueryPool(FVulkanDevice* InDevice);
    ~FVulkanQueryPool();
    
    
};

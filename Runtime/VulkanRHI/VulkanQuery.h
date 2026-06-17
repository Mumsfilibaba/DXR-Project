#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/ScopedLock.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanConfiguration.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanFence.h"
#if !VULKAN_USE_CPU_QUERY_RESOLVE
    #include "VulkanRHI/VulkanMemoryManager.h"
#endif

#define VULKAN_INVALID_QUERY_INDEX (-1)

class FVulkanQueryPool;
class FVulkanQueryPoolManager;
class FVulkanCommandBuffer;
class FVulkanCommandContext;

typedef TSharedRef<struct FVulkanQueryRHI> FVulkanQueryRHIRef;

enum class EVulkanQueryType : uint8
{
    CommandListBegin,
    CommandListEnd,
    Timestamp,
    Occlusion,
    PipelineStatistics,
};

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

struct FVulkanPipelineStatisticsQueryData
{
    uint64 IAVertices;
    uint64 IAPrimitives;
    uint64 VSInvocations;
    uint64 GSInvocations;
    uint64 GSPrimitives;
    uint64 CInvocations;
    uint64 CPrimitives;
    uint64 PSInvocations;
    uint64 HSInvocations;
    uint64 DSInvocations;
    uint64 CSInvocations;
    uint64 Availability;
};

struct FVulkanQuery
{
    FVulkanQuery() = default;

    FVulkanQuery(FVulkanQueryPool* InPool, int32 InIndex, uint64* InResultTarget, EVulkanQueryType InType)
        : QueryPool(InPool)
        , ResultTarget(InResultTarget)
        , Type(InType)
        , QueryIndex(InIndex)
    {
    }

    bool CopyResult(void* Dst, uint64 DstSize) const;

    bool IsValid() const
    {
        return QueryPool != nullptr && QueryIndex != VULKAN_INVALID_QUERY_INDEX;
    }

    operator bool() const
    {
        return IsValid();
    }

    FVulkanQueryPool* QueryPool    = nullptr;
    uint64*           ResultTarget = nullptr;
    EVulkanQueryType  Type         = EVulkanQueryType::Timestamp;
    int32             QueryIndex   = VULKAN_INVALID_QUERY_INDEX;
};

struct FVulkanQueryRHI : public FRHIQuery, public FVulkanDeviceChild
{
    FVulkanQueryRHI(FVulkanDevice* InDevice, EQueryType InQueryType);
    virtual ~FVulkanQueryRHI();

    FVulkanQuery             CurrentQuery;
    TSharedRef<FVulkanFence> SyncFence;
    uint64*                  QueryResult;
};

struct FVulkanQueryRange
{
    FVulkanQueryRange() = default;

    FVulkanQueryRange(FVulkanQueryPool* InPool, int32 InStartIndex, int32 InCount)
        : Pool(InPool)
        , StartIndex(InStartIndex)
        , Count(InCount)
    {
    }

    bool IsValid() const
    {
        return Pool != nullptr;
    }
    
    operator bool() const
    {
        return IsValid();
    }

    FVulkanQueryPool* Pool       = nullptr;
    int32             StartIndex = 0;
    int32             Count      = 0;
};

class FVulkanQueryPool : public FVulkanDeviceChild
{
public:
    FVulkanQueryPool(FVulkanDevice* InDevice, VkQueryType InQueryType, int32 InNumQueries);
    ~FVulkanQueryPool();

    bool Initialize();
    void ResetPool();
    void SetDebugName(const String& InName);

    VkQueryPool GetVkQueryPool() const
    {
        return QueryPool;
    }

    void GetDebugName(String& OutDebugName) const
    {
    #if VULKAN_STORE_DEBUG_NAMES
        OutDebugName = DebugName;
    #else
        OutDebugName.Clear();
    #endif
    }

    uint64 GetQuerySize() const
    {
        switch (QueryType)
        {
            case VK_QUERY_TYPE_TIMESTAMP:           return sizeof(uint64);
            case VK_QUERY_TYPE_OCCLUSION:           return sizeof(uint64);
            case VK_QUERY_TYPE_PIPELINE_STATISTICS: return sizeof(FVulkanPipelineStatisticsQueryData) - sizeof(uint64);
            default:                                return sizeof(uint64);
        }
    }

#if !VULKAN_USE_CPU_QUERY_RESOLVE
    VkBuffer GetReadbackBuffer() const
    {
        return ReadbackLocation.GetBackingBuffer();
    }

    VkDeviceSize GetReadbackBufferOffset() const
    {
        return ReadbackLocation.GetBufferOffset();
    }

    uint64* GetReadbackData() const
    {
        return ReadbackData;
    }
#endif

    const VkQueryType QueryType;
    const int32       NumQueries;

private:
    VkQueryPool           QueryPool;
#if VULKAN_STORE_DEBUG_NAMES
    String                DebugName;
#endif
#if !VULKAN_USE_CPU_QUERY_RESOLVE
    FVulkanMemoryLocation ReadbackLocation;
    uint64*               ReadbackData;
#endif
};

class FVulkanQueryAllocator : public FVulkanDeviceChild
{
public:
    FVulkanQueryAllocator(FVulkanDevice* InDevice, VkQueryType InQueryType);
    ~FVulkanQueryAllocator();

    bool Allocate(FVulkanQuery& OutQuery, uint64* ResultTarget, EVulkanQueryType InType);
    void Reset(TArray<FVulkanQueryRange>& OutRanges);

private:
    TArray<FVulkanQueryRange> Ranges;
    VkQueryType               QueryType;
};

class FVulkanQueryPoolManager : public FVulkanDeviceChild
{
public:
    FVulkanQueryPoolManager(FVulkanDevice* InDevice, VkQueryType InQueryType, int32 InQueriesPerPool);
    ~FVulkanQueryPoolManager();

    FVulkanQueryPool* ObtainPool();
    void RecyclePool(FVulkanQueryPool* Pool);

private:
    VkQueryType               QueryType;
    int32                     QueriesPerPool;
    TQueue<FVulkanQueryPool*> AvailablePools;
    TArray<FVulkanQueryPool*> AllPools;
    FCriticalSection          PoolsCS;
};

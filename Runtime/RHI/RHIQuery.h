#pragma once
#include "RHI/RHIResource.h"

enum class EQueryType
{
    Unknown = 0,
    Timestamp,
    Occlusion,
    PipelineStatistics,
};

enum class EQueryResultMode : uint8
{
    Available, // Non-blocking: process any completed GPU work and return the current result
    Wait,      // Blocking: wait for all pending GPU work to finish before returning the result
};

NODISCARD constexpr const CHAR* ToString(EQueryType QueryType)
{
    switch (QueryType)
    {
        case EQueryType::Timestamp:          return "Timestamp";
        case EQueryType::Occlusion:          return "Occlusion";
        case EQueryType::PipelineStatistics: return "PipelineStatistics";
        
        default: 
            return "Unknown EQueryType";
    }
}

struct FRHIPipelineStatistics
{
    bool HasAnyActivity() const
    {
        return (IAVertices != 0) || (IAPrimitives != 0) || (VSInvocations != 0) || (GSInvocations != 0) ||
            (GSPrimitives != 0)  || (CInvocations != 0) || (CPrimitives != 0)   || (PSInvocations != 0) ||
            (HSInvocations != 0) || (DSInvocations != 0) || (CSInvocations != 0) || (ASInvocations != 0) ||
            (MSInvocations != 0) || (MSPrimitives != 0);
    }

    uint64 IAVertices    = 0;
    uint64 IAPrimitives  = 0;
    uint64 VSInvocations = 0;
    uint64 GSInvocations = 0;
    uint64 GSPrimitives  = 0;
    uint64 CInvocations  = 0;
    uint64 CPrimitives   = 0;
    uint64 PSInvocations = 0;
    uint64 HSInvocations = 0;
    uint64 DSInvocations = 0;
    uint64 CSInvocations = 0;
    uint64 ASInvocations = 0;
    uint64 MSInvocations = 0;
    uint64 MSPrimitives  = 0;
};

NODISCARD constexpr uint32 GetQueryResultElementCount(EQueryType QueryType)
{
    switch (QueryType)
    {
        case EQueryType::PipelineStatistics: 
            return sizeof(FRHIPipelineStatistics) / sizeof(uint64);

        default: 
            return 1;
    }
}

class FRHIQuery : public FRHIResource
{
protected:
    FRHIQuery(EQueryType InQuery)
        : FRHIResource()
        , Query(InQuery)
    {
    }

    virtual ~FRHIQuery() = default;

public:
    EQueryType GetType() const
    {
        return Query;
    }

private:
    EQueryType Query;
};

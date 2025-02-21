#pragma once
#include "RHI/RHIResource.h"

enum class EQueryType
{
    Unknown = 0,
    Timestamp,
    Occlusion,
};

NODISCARD constexpr const CHAR* ToString(EQueryType QueryType)
{
    switch (QueryType)
    {
        case EQueryType::Timestamp: return "Timestamp";
        case EQueryType::Occlusion: return "Occlusion";
        default:                    return "Unknown EQueryType";
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
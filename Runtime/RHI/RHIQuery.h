#pragma once
#include "RHIResource.h"

struct FTimingQuery
{
    uint64 Begin = 0;
    uint64 End   = 0;
};

class FRHIQuery : public FRHIResource
{
protected:
    FRHIQuery() = default;
    virtual ~FRHIQuery() = default;

public:
    virtual void GetTimestampFromIndex(FTimingQuery& OutQuery, uint32 Index) const = 0;
    virtual uint64 GetFrequency() const = 0;
};
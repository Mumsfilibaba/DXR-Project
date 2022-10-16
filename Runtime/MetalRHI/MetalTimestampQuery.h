#pragma once
#include "RHI/RHITimestampQuery.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

class FMetalTimestampQuery 
    : public FRHITimestampQuery
{
public:
    FMetalTimestampQuery()  = default;
    ~FMetalTimestampQuery() = default;

    virtual void GetTimestampFromIndex(FRHITimestamp& OutQuery, uint32 Index) const override final
    {
        OutQuery = FRHITimestamp();
    }

    virtual uint64 GetFrequency() const override final
    {
        return 1;
    }
};

#pragma clang diagnostic pop

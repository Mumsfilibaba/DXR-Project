#pragma once
#include "RHI/RHITimestampQuery.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTimestampQuery

class CMetalTimestampQuery : public FRHITimestampQuery
{
public:
    CMetalTimestampQuery() = default;
    ~CMetalTimestampQuery() = default;

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

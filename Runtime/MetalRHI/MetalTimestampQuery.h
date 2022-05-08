#pragma once
#include "RHI/RHITimestampQuery.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalTimestampQuery

class CMetalTimestampQuery : public CRHITimestampQuery
{
public:
    CMetalTimestampQuery() = default;
    ~CMetalTimestampQuery() = default;

    virtual void GetTimestampFromIndex(SRHITimestamp& OutQuery, uint32 Index) const override final
    {
        OutQuery = SRHITimestamp();
    }

    virtual uint64 GetFrequency() const override final
    {
        return 1;
    }
};

#pragma clang diagnostic pop

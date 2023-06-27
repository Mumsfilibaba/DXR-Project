#pragma once
#include "RHI/RHITimestampQuery.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalTimestampQuery : public FRHITimestampQuery
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

ENABLE_UNREFERENCED_VARIABLE_WARNING

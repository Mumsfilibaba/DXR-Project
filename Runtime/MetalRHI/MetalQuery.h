#pragma once
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalQuery : public FRHIQuery
{
public:
    FMetalQuery()  = default;
    ~FMetalQuery() = default;

    virtual void GetTimestampFromIndex(FTimingQuery& OutQuery, uint32 Index) const override final
    {
        OutQuery = FTimingQuery();
    }

    virtual uint64 GetFrequency() const override final
    {
        return 1;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
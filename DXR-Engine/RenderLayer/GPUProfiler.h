#pragma once
#include "ResourceBase.h"

struct TimeQuery
{
    uint64 Begin;
    uint64 End;
};

class GPUProfiler : public Resource
{
public:
    virtual void   GetTimeQuery(TimeQuery& OutQuery, uint32 Index) const = 0;
    virtual uint64 GetFrequency() const = 0;
};
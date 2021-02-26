#pragma once
#include "ResourceBase.h"

struct TimeQuery
{
    UInt64 Begin;
    UInt64 End;
};

class GPUProfiler : public Resource
{
public:
    virtual void   GetTimeQuery(TimeQuery& OutQuery, UInt32 Index) const = 0;
    virtual UInt64 GetFrequency() const = 0;
};
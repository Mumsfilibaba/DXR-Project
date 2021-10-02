#pragma once
#include "RHIResourceBase.h"

struct STimeQuery
{
    uint64 Begin;
    uint64 End;
};

class CGPUProfiler : public CRHIResource
{
public:

    /* Retrieve a certain timestamp */
    virtual void GetTimeQuery( STimeQuery& OutQuery, uint32 Index ) const = 0;
    
    /* Get the frequency of the queue that the GPU profiler where used on */
    virtual uint64 GetFrequency() const = 0;
};
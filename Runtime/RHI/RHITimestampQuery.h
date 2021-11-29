#pragma once
#include "RHIResourceBase.h"

struct SRHITimestamp
{
    uint64 Begin;
    uint64 End;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CRHITimestampQuery : public CRHIResource
{
public:

    /* Retrieve a certain timestamp */
    virtual void GetTimestampFromIndex( SRHITimestamp& OutQuery, uint32 Index ) const = 0;

    /* Get the frequency of the queue that the query where used on */
    virtual uint64 GetFrequency() const = 0;
};
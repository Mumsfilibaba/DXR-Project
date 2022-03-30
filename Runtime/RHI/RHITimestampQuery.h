#pragma once
#include "RHIResourceBase.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHITimestamp

struct SRHITimestamp
{
    uint64 Begin;
    uint64 End;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITimestampQuery

class CRHITimestampQuery : public CRHIResource
{
public:
    virtual void GetTimestampFromIndex(SRHITimestamp& OutQuery, uint32 Index) const = 0;

    virtual uint64 GetFrequency() const = 0;
};
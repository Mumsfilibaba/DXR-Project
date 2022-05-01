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
protected:

    CRHITimestampQuery()  = default;
    ~CRHITimestampQuery() = default;

public:

    /**
     * @brief: Retrieve a certain timestamp 
     * 
     * @param OutQuery: Structure to store the timestamp in
     * @param Index: Index of the query to retrieve 
     */
    virtual void GetTimestampFromIndex(SRHITimestamp& OutQuery, uint32 Index) const = 0;

    /**
     * @brief: Get the frequency of the queue that the query where used on 
     * 
     * @return: Returns the frequency of the query
     */
    virtual uint64 GetFrequency() const = 0;
};
#pragma once
#include "RHIResourceBase.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITimestamp

struct FRHITimestamp
{
    uint64 Begin;
    uint64 End;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITimestampQuery

class FRHITimestampQuery : public FRHIResource
{
protected:

    FRHITimestampQuery()  = default;
    ~FRHITimestampQuery() = default;

public:

    /**
     * @brief: Retrieve a certain timestamp 
     * 
     * @param OutQuery: Structure to store the timestamp in
     * @param Index: Index of the query to retrieve 
     */
    virtual void GetTimestampFromIndex(FRHITimestamp& OutQuery, uint32 Index) const = 0;

    /**
     * @brief: Get the frequency of the queue that the query where used on 
     * 
     * @return: Returns the frequency of the query
     */
    virtual uint64 GetFrequency() const = 0;
};
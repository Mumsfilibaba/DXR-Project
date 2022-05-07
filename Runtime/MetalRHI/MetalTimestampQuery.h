#pragma once
#include "RHI/RHITimestampQuery.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

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

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif

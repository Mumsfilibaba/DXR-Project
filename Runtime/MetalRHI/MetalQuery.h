#pragma once
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FMetalQueryRHI : public FRHIQuery
{
    FMetalQueryRHI(EQueryType InQueryType)
        : FRHIQuery(InQueryType)
    {
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

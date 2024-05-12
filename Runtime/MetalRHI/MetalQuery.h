#pragma once
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FMetalQuery : public FRHIQuery
{
    FMetalQuery(EQueryType InQueryType)
        : FRHIQuery(InQueryType)
    {
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

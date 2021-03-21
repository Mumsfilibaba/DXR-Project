#pragma once
#include "Core/Types.h"

#include <cstdlib>

struct Mallocator
{
    void* Allocate(uint32 Size)
    {
        return malloc(Size);
    }

    void Free(void* Ptr)
    {
        free(Ptr);
    }
};
#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"

struct CORE_API CRC32
{
    static uint32 Generate(const void* Source, uint64 SourceSize);
};

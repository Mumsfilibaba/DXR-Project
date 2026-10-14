#include "Malloc.h"
#include "Core/Platform/PlatformMemory.h"

void* FMallocANSI::Malloc(uint64 InSize, uint32 InAlignment)
{
    if (!InSize)
    {
        return nullptr;
    }

    return FPlatformMemory::Malloc(InSize, InAlignment);
}

void* FMallocANSI::Realloc(void* Block, uint64 InSize, uint32 InAlignment)
{
    if (!Block)
    {
        return Malloc(InSize, InAlignment);
    }

    return FPlatformMemory::Realloc(Block, InSize, InAlignment);
}

void FMallocANSI::Free(void* Block)
{
    if (Block)
    {
        FPlatformMemory::Free(Block);
    }
}

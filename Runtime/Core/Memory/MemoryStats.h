#pragma once
#include "Core/Stats/Stats.h"

// -------------------------------------------------------------------------------------------
// Global Memory Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(CORE_API, STAT_Memory_AllocationCount);

// -------------------------------------------------------------------------------------------
// Memory Stack Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(CORE_API, STAT_Memory_StackBytes);
STAT_DECLARE_EXTERN(CORE_API, STAT_Memory_StackPageCount);
STAT_DECLARE_EXTERN(CORE_API, STAT_Memory_StackPooledBytes);
STAT_DECLARE_EXTERN(CORE_API, STAT_Memory_StackPooledPageCount);

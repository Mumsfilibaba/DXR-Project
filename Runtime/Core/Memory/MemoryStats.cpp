#include "Core/Memory/MemoryStats.h"

STAT_DEFINE_COUNTER(STAT_Memory_AllocationCount, "Live Allocations", "Memory");

STAT_DEFINE_MEMORY(STAT_Memory_StackBytes,            "Stack Bytes", "Memory");
STAT_DEFINE_COUNTER(STAT_Memory_StackPageCount,       "Stack Pages", "Memory");
STAT_DEFINE_MEMORY(STAT_Memory_StackPooledBytes,      "Stack Pooled Bytes", "Memory");
STAT_DEFINE_COUNTER(STAT_Memory_StackPooledPageCount, "Stack Pooled Pages", "Memory");

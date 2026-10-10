#include "Core/Memory/MemoryStats.h"

STAT_DEFINE_COUNTER(STAT_Memory_AllocationCount, TEXT("Live Allocations"), TEXT("Memory"));

STAT_DEFINE_MEMORY(STAT_Memory_StackBytes,            TEXT("Stack Bytes"), TEXT("Memory"));
STAT_DEFINE_COUNTER(STAT_Memory_StackPageCount,       TEXT("Stack Pages"), TEXT("Memory"));
STAT_DEFINE_MEMORY(STAT_Memory_StackPooledBytes,      TEXT("Stack Pooled Bytes"), TEXT("Memory"));
STAT_DEFINE_COUNTER(STAT_Memory_StackPooledPageCount, TEXT("Stack Pooled Pages"), TEXT("Memory"));

#include "D3D12RHI/D3D12Stats.h"

STAT_DEFINE_MEMORY(STAT_D3D12_BufferPoolAllocated,   "Buffer Pool Allocated",   "D3D12 Allocators");
STAT_DEFINE_MEMORY(STAT_D3D12_BufferPoolUsed,        "Buffer Pool Used",        "D3D12 Allocators");
STAT_DEFINE_MEMORY(STAT_D3D12_BufferPoolFragmented,  "Buffer Pool Fragmented",  "D3D12 Allocators");

STAT_DEFINE_MEMORY(STAT_D3D12_TexturePoolAllocated,  "Texture Pool Allocated",  "D3D12 Allocators");
STAT_DEFINE_MEMORY(STAT_D3D12_TexturePoolUsed,       "Texture Pool Used",       "D3D12 Allocators");
STAT_DEFINE_MEMORY(STAT_D3D12_TexturePoolFragmented, "Texture Pool Fragmented", "D3D12 Allocators");

STAT_DEFINE_MEMORY(STAT_D3D12_UploadHeapAllocated,   "Upload Heap Allocated",   "D3D12 Allocators");
STAT_DEFINE_MEMORY(STAT_D3D12_UploadHeapUsed,        "Upload Heap Used",        "D3D12 Allocators");
STAT_DEFINE_MEMORY(STAT_D3D12_UploadHeapFragmented,  "Upload Heap Fragmented",  "D3D12 Allocators");

STAT_DEFINE_MEMORY(STAT_D3D12_CommittedResourceMemory,  "Committed Resource Memory",  "D3D12 Allocators");
STAT_DEFINE_COUNTER(STAT_D3D12_CommittedResourceCount,  "Committed Resource Count",   "D3D12 Allocators");
STAT_DEFINE_MEMORY(STAT_D3D12_CommittedDefaultMemory,   "Committed Default Memory",   "D3D12 Allocators");
STAT_DEFINE_MEMORY(STAT_D3D12_CommittedUploadMemory,    "Committed Upload Memory",    "D3D12 Allocators");
STAT_DEFINE_MEMORY(STAT_D3D12_CommittedReadbackMemory,  "Committed Readback Memory",  "D3D12 Allocators");

// Defrag Activity Stats
STAT_DEFINE_COUNTER(STAT_D3D12_TextureDefragPending,        "Texture Defrag Pending",         "D3D12 Allocators");
STAT_DEFINE_COUNTER(STAT_D3D12_TextureDefragMovesCompleted, "Texture Defrag Moves Completed", "D3D12 Allocators");
STAT_DEFINE_MEMORY(STAT_D3D12_TextureDefragBytesMoved,      "Texture Defrag Bytes Moved",     "D3D12 Allocators");

STAT_DEFINE_COUNTER(STAT_D3D12_BufferDefragPending,         "Buffer Defrag Pending",          "D3D12 Allocators");
STAT_DEFINE_COUNTER(STAT_D3D12_BufferDefragMovesCompleted,  "Buffer Defrag Moves Completed",  "D3D12 Allocators");
STAT_DEFINE_MEMORY(STAT_D3D12_BufferDefragBytesMoved,       "Buffer Defrag Bytes Moved",      "D3D12 Allocators");

// Residency Stats
STAT_DEFINE_COUNTER(STAT_D3D12_ResidencyEvictionCount,      "Eviction Count",       "D3D12 Residency");
STAT_DEFINE_MEMORY(STAT_D3D12_ResidencyEvictedBytes,        "Evicted Bytes",        "D3D12 Residency");

STAT_DEFINE_COUNTER(STAT_D3D12_ResidencyMakeResidentCount,  "Make Resident Count",  "D3D12 Residency");
STAT_DEFINE_MEMORY(STAT_D3D12_ResidencyMakeResidentBytes,   "Make Resident Bytes",  "D3D12 Residency");

STAT_DEFINE_COUNTER(STAT_D3D12_ResidencyTrackedCount,       "Tracked Count",        "D3D12 Residency");
STAT_DEFINE_MEMORY(STAT_D3D12_ResidencyResidentBytes,       "Resident Bytes",       "D3D12 Residency");
STAT_DEFINE_MEMORY(STAT_D3D12_ResidencyBudget,              "Budget",               "D3D12 Residency");

// PSO Stats
STAT_DEFINE_COUNTER(STAT_D3D12_PSOCreateCount,                "PSOs Created",             "D3D12 PSO");
STAT_DEFINE_COUNTER(STAT_D3D12_NumGraphicsPipelineStates,     "Graphics PSOs Created",    "D3D12 PSO");
STAT_DEFINE_COUNTER(STAT_D3D12_NumComputePipelineStates,      "Compute PSOs Created",     "D3D12 PSO");
STAT_DEFINE_COUNTER(STAT_D3D12_NumRayTracingPipelineStates,   "RayTracing PSOs Created",  "D3D12 PSO");
STAT_DEFINE_COUNTER(STAT_D3D12_NumMeshletPipelineStates,      "Meshlet PSOs Created",     "D3D12 PSO");
STAT_DEFINE_MEMORY(STAT_D3D12_PSOCacheSize,                   "Cache Serialized Size",    "D3D12 PSO");

// Command Primitive Stats
STAT_DEFINE_COUNTER(STAT_D3D12_CommandListCount,      "Command Lists",      "D3D12 Commands");
STAT_DEFINE_COUNTER(STAT_D3D12_CommandAllocatorCount, "Command Allocators", "D3D12 Commands");

// Query Stats
STAT_DEFINE_COUNTER(STAT_D3D12_QueryHeapCount, "Query Heaps", "D3D12 Queries");

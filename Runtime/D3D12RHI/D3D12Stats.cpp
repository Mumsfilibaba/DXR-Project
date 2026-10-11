#include "D3D12RHI/D3D12Stats.h"

STAT_DEFINE_MEMORY(STAT_D3D12_BufferPoolAllocated,   TEXT("Buffer Pool Allocated"),   TEXT("D3D12 Allocators"));
STAT_DEFINE_MEMORY(STAT_D3D12_BufferPoolUsed,        TEXT("Buffer Pool Used"),        TEXT("D3D12 Allocators"));
STAT_DEFINE_MEMORY(STAT_D3D12_BufferPoolFragmented,  TEXT("Buffer Pool Fragmented"),  TEXT("D3D12 Allocators"));

STAT_DEFINE_MEMORY(STAT_D3D12_TexturePoolAllocated,  TEXT("Texture Pool Allocated"),  TEXT("D3D12 Allocators"));
STAT_DEFINE_MEMORY(STAT_D3D12_TexturePoolUsed,       TEXT("Texture Pool Used"),       TEXT("D3D12 Allocators"));
STAT_DEFINE_MEMORY(STAT_D3D12_TexturePoolFragmented, TEXT("Texture Pool Fragmented"), TEXT("D3D12 Allocators"));

STAT_DEFINE_MEMORY(STAT_D3D12_UploadHeapAllocated,   TEXT("Upload Heap Allocated"),   TEXT("D3D12 Allocators"));
STAT_DEFINE_MEMORY(STAT_D3D12_UploadHeapUsed,        TEXT("Upload Heap Used"),        TEXT("D3D12 Allocators"));
STAT_DEFINE_MEMORY(STAT_D3D12_UploadHeapFragmented,  TEXT("Upload Heap Fragmented"),  TEXT("D3D12 Allocators"));

STAT_DEFINE_MEMORY(STAT_D3D12_CommittedResourceMemory,  TEXT("Committed Resource Memory"),  TEXT("D3D12 Allocators"));
STAT_DEFINE_COUNTER(STAT_D3D12_CommittedResourceCount,  TEXT("Committed Resource Count"),   TEXT("D3D12 Allocators"));
STAT_DEFINE_MEMORY(STAT_D3D12_CommittedDefaultMemory,   TEXT("Committed Default Memory"),   TEXT("D3D12 Allocators"));
STAT_DEFINE_MEMORY(STAT_D3D12_CommittedUploadMemory,    TEXT("Committed Upload Memory"),    TEXT("D3D12 Allocators"));
STAT_DEFINE_MEMORY(STAT_D3D12_CommittedReadbackMemory,  TEXT("Committed Readback Memory"),  TEXT("D3D12 Allocators"));

// Defrag Activity Stats
STAT_DEFINE_COUNTER(STAT_D3D12_TextureDefragPending,        TEXT("Texture Defrag Pending"),         TEXT("D3D12 Allocators"));
STAT_DEFINE_COUNTER(STAT_D3D12_TextureDefragMovesCompleted, TEXT("Texture Defrag Moves Completed"), TEXT("D3D12 Allocators"));
STAT_DEFINE_MEMORY(STAT_D3D12_TextureDefragBytesMoved,      TEXT("Texture Defrag Bytes Moved"),     TEXT("D3D12 Allocators"));

STAT_DEFINE_COUNTER(STAT_D3D12_BufferDefragPending,         TEXT("Buffer Defrag Pending"),          TEXT("D3D12 Allocators"));
STAT_DEFINE_COUNTER(STAT_D3D12_BufferDefragMovesCompleted,  TEXT("Buffer Defrag Moves Completed"),  TEXT("D3D12 Allocators"));
STAT_DEFINE_MEMORY(STAT_D3D12_BufferDefragBytesMoved,       TEXT("Buffer Defrag Bytes Moved"),      TEXT("D3D12 Allocators"));

// Residency Stats
STAT_DEFINE_COUNTER(STAT_D3D12_ResidencyEvictionCount,      TEXT("Eviction Count"),       TEXT("D3D12 Residency"));
STAT_DEFINE_MEMORY(STAT_D3D12_ResidencyEvictedBytes,        TEXT("Evicted Bytes"),        TEXT("D3D12 Residency"));

STAT_DEFINE_COUNTER(STAT_D3D12_ResidencyMakeResidentCount,  TEXT("Make Resident Count"),  TEXT("D3D12 Residency"));
STAT_DEFINE_MEMORY(STAT_D3D12_ResidencyMakeResidentBytes,   TEXT("Make Resident Bytes"),  TEXT("D3D12 Residency"));

STAT_DEFINE_COUNTER(STAT_D3D12_ResidencyTrackedCount,       TEXT("Tracked Count"),        TEXT("D3D12 Residency"));
STAT_DEFINE_MEMORY(STAT_D3D12_ResidencyResidentBytes,       TEXT("Resident Bytes"),       TEXT("D3D12 Residency"));
STAT_DEFINE_MEMORY(STAT_D3D12_ResidencyBudget,              TEXT("Budget"),               TEXT("D3D12 Residency"));

// PSO Stats
STAT_DEFINE_COUNTER(STAT_D3D12_PSOCreateCount,                TEXT("PSOs Created"),             TEXT("D3D12 PSO"));
STAT_DEFINE_COUNTER(STAT_D3D12_NumGraphicsPipelineStates,     TEXT("Graphics PSOs Created"),    TEXT("D3D12 PSO"));
STAT_DEFINE_COUNTER(STAT_D3D12_NumComputePipelineStates,      TEXT("Compute PSOs Created"),     TEXT("D3D12 PSO"));
STAT_DEFINE_COUNTER(STAT_D3D12_NumRayTracingPipelineStates,   TEXT("RayTracing PSOs Created"),  TEXT("D3D12 PSO"));
STAT_DEFINE_COUNTER(STAT_D3D12_NumMeshletPipelineStates,      TEXT("Meshlet PSOs Created"),     TEXT("D3D12 PSO"));
STAT_DEFINE_MEMORY(STAT_D3D12_PSOCacheSize,                   TEXT("Cache Serialized Size"),    TEXT("D3D12 PSO"));

// Command Primitive Stats
STAT_DEFINE_COUNTER(STAT_D3D12_CommandListCount,      TEXT("Command Lists"),      TEXT("D3D12 Commands"));
STAT_DEFINE_COUNTER(STAT_D3D12_CommandAllocatorCount, TEXT("Command Allocators"), TEXT("D3D12 Commands"));

STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_Submits,                      TEXT("Submits"),       TEXT("D3D12 Submissions / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_SplitsCommandLimit,           TEXT("Limit Splits"),  TEXT("D3D12 Submissions / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_SplitsDescriptorHeapRollover, TEXT("Heap Splits"),   TEXT("D3D12 Submissions / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_SplitsFence,                  TEXT("Fence Splits"),  TEXT("D3D12 Submissions / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_SplitsOther,                  TEXT("Other Splits"),  TEXT("D3D12 Submissions / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_EmptyCommandListsSkipped,     TEXT("Skipped Empty"), TEXT("D3D12 Submissions / Frame"));

STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_BLASBuilds,                            TEXT("BLAS Builds"),      TEXT("D3D12 Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_BLASUpdates,                           TEXT("BLAS Updates"),     TEXT("D3D12 Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_TLASBuilds,                            TEXT("TLAS Builds"),      TEXT("D3D12 Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_TLASUpdates,                           TEXT("TLAS Updates"),     TEXT("D3D12 Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_AccelerationStructureCompactions,      TEXT("Compactions"),      TEXT("D3D12 Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_AccelerationStructureCopies,           TEXT("Copies"),           TEXT("D3D12 Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_AccelerationStructureSerializations,   TEXT("Serializations"),   TEXT("D3D12 Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_AccelerationStructureDeserializations, TEXT("Deserializations"), TEXT("D3D12 Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_D3D12_OpacityMicromapBuilds,                 TEXT("OMM Builds"),       TEXT("D3D12 Ray Tracing / Frame"));

// Query Stats
STAT_DEFINE_COUNTER(STAT_D3D12_QueryHeapCount, TEXT("Query Heaps"), TEXT("D3D12 Queries"));

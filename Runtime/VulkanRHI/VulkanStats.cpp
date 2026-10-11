#include "VulkanRHI/VulkanStats.h"

STAT_DEFINE_MEMORY(STAT_Vulkan_BufferPoolAllocated,   TEXT("Buffer Pool Allocated"),   TEXT("Vulkan Allocators"));
STAT_DEFINE_MEMORY(STAT_Vulkan_BufferPoolUsed,        TEXT("Buffer Pool Used"),        TEXT("Vulkan Allocators"));
STAT_DEFINE_MEMORY(STAT_Vulkan_BufferPoolFragmented,  TEXT("Buffer Pool Fragmented"),  TEXT("Vulkan Allocators"));

STAT_DEFINE_MEMORY(STAT_Vulkan_TexturePoolAllocated,  TEXT("Texture Pool Allocated"),  TEXT("Vulkan Allocators"));
STAT_DEFINE_MEMORY(STAT_Vulkan_TexturePoolUsed,       TEXT("Texture Pool Used"),       TEXT("Vulkan Allocators"));
STAT_DEFINE_MEMORY(STAT_Vulkan_TexturePoolFragmented, TEXT("Texture Pool Fragmented"), TEXT("Vulkan Allocators"));

STAT_DEFINE_MEMORY(STAT_Vulkan_UploadHeapAllocated,   TEXT("Upload Heap Allocated"),   TEXT("Vulkan Allocators"));
STAT_DEFINE_MEMORY(STAT_Vulkan_UploadHeapUsed,        TEXT("Upload Heap Used"),        TEXT("Vulkan Allocators"));
STAT_DEFINE_MEMORY(STAT_Vulkan_UploadHeapFragmented,  TEXT("Upload Heap Fragmented"),  TEXT("Vulkan Allocators"));

STAT_DEFINE_COUNTER(STAT_Vulkan_ActiveAllocations, TEXT("Active Allocations"), TEXT("Vulkan Allocators"));

// Defrag Activity Stats
STAT_DEFINE_COUNTER(STAT_Vulkan_TextureDefragPending,        TEXT("Texture Defrag Pending"),         TEXT("Vulkan Allocators"));
STAT_DEFINE_COUNTER(STAT_Vulkan_TextureDefragMovesCompleted, TEXT("Texture Defrag Moves Completed"), TEXT("Vulkan Allocators"));
STAT_DEFINE_MEMORY(STAT_Vulkan_TextureDefragBytesMoved,      TEXT("Texture Defrag Bytes Moved"),     TEXT("Vulkan Allocators"));

STAT_DEFINE_COUNTER(STAT_Vulkan_BufferDefragPending,         TEXT("Buffer Defrag Pending"),          TEXT("Vulkan Allocators"));
STAT_DEFINE_COUNTER(STAT_Vulkan_BufferDefragMovesCompleted,  TEXT("Buffer Defrag Moves Completed"),  TEXT("Vulkan Allocators"));
STAT_DEFINE_MEMORY(STAT_Vulkan_BufferDefragBytesMoved,       TEXT("Buffer Defrag Bytes Moved"),      TEXT("Vulkan Allocators"));

// PSO Stats
STAT_DEFINE_COUNTER(STAT_Vulkan_PSOCreateCount,                TEXT("PSOs Created"),            TEXT("Vulkan PSO"));
STAT_DEFINE_MEMORY(STAT_Vulkan_PSOCacheSize,                   TEXT("Cache Serialized Size"),   TEXT("Vulkan PSO"));
STAT_DEFINE_COUNTER(STAT_Vulkan_NumGraphicsPipelineStates,     TEXT("Graphics PSOs Created"),   TEXT("Vulkan PSO"));
STAT_DEFINE_COUNTER(STAT_Vulkan_NumComputePipelineStates,      TEXT("Compute PSOs Created"),    TEXT("Vulkan PSO"));
STAT_DEFINE_COUNTER(STAT_Vulkan_NumRayTracingPipelineStates,   TEXT("RayTracing PSOs Created"), TEXT("Vulkan PSO"));
STAT_DEFINE_COUNTER(STAT_Vulkan_NumMeshletPipelineStates,      TEXT("Meshlet PSOs Created"),    TEXT("Vulkan PSO"));

// Command Primitive Stats
STAT_DEFINE_COUNTER(STAT_Vulkan_CommandBufferCount, TEXT("Command Buffers"), TEXT("Vulkan Commands"));
STAT_DEFINE_COUNTER(STAT_Vulkan_CommandPoolCount,   TEXT("Command Pools"),   TEXT("Vulkan Commands"));

// Submission Stats (per frame)
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_Submits,                     TEXT("Submits"),        TEXT("Vulkan Submissions / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_SemaphoreOnlySubmits,        TEXT("Semaphore Only"), TEXT("Vulkan Submissions / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_SplitsCommandLimit,          TEXT("Limit Splits"),   TEXT("Vulkan Submissions / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_SplitsSwapChainAcquire,      TEXT("Acquire Splits"), TEXT("Vulkan Submissions / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_SplitsFence,                 TEXT("Fence Splits"),   TEXT("Vulkan Submissions / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_SplitsOther,                 TEXT("Other Splits"),   TEXT("Vulkan Submissions / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_EmptyCommandBuffersRecycled, TEXT("Recycled Empty"), TEXT("Vulkan Submissions / Frame"));

// Ray Tracing Stats (per frame)
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_BLASBuilds,                            TEXT("BLAS Builds"),      TEXT("Vulkan Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_BLASUpdates,                           TEXT("BLAS Updates"),     TEXT("Vulkan Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_TLASBuilds,                            TEXT("TLAS Builds"),      TEXT("Vulkan Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_TLASUpdates,                           TEXT("TLAS Updates"),     TEXT("Vulkan Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_AccelerationStructureCompactions,      TEXT("Compactions"),      TEXT("Vulkan Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_AccelerationStructureCopies,           TEXT("Copies"),           TEXT("Vulkan Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_AccelerationStructureSerializations,   TEXT("Serializations"),   TEXT("Vulkan Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_AccelerationStructureDeserializations, TEXT("Deserializations"), TEXT("Vulkan Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_OpacityMicromapBuilds,                 TEXT("OMM Builds"),       TEXT("Vulkan Ray Tracing / Frame"));
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_ClusterOperations,                     TEXT("Cluster Ops"),      TEXT("Vulkan Ray Tracing / Frame"));

// Query Stats
STAT_DEFINE_COUNTER(STAT_Vulkan_QueryPoolCount, TEXT("Query Pools"), TEXT("Vulkan Queries"));

#include "VulkanRHI/VulkanStats.h"

STAT_DEFINE_MEMORY(STAT_Vulkan_BufferPoolAllocated,   "Buffer Pool Allocated",   "Vulkan Allocators");
STAT_DEFINE_MEMORY(STAT_Vulkan_BufferPoolUsed,        "Buffer Pool Used",        "Vulkan Allocators");
STAT_DEFINE_MEMORY(STAT_Vulkan_BufferPoolFragmented,  "Buffer Pool Fragmented",  "Vulkan Allocators");

STAT_DEFINE_MEMORY(STAT_Vulkan_TexturePoolAllocated,  "Texture Pool Allocated",  "Vulkan Allocators");
STAT_DEFINE_MEMORY(STAT_Vulkan_TexturePoolUsed,       "Texture Pool Used",       "Vulkan Allocators");
STAT_DEFINE_MEMORY(STAT_Vulkan_TexturePoolFragmented, "Texture Pool Fragmented", "Vulkan Allocators");

STAT_DEFINE_MEMORY(STAT_Vulkan_UploadHeapAllocated,   "Upload Heap Allocated",   "Vulkan Allocators");
STAT_DEFINE_MEMORY(STAT_Vulkan_UploadHeapUsed,        "Upload Heap Used",        "Vulkan Allocators");
STAT_DEFINE_MEMORY(STAT_Vulkan_UploadHeapFragmented,  "Upload Heap Fragmented",  "Vulkan Allocators");

STAT_DEFINE_COUNTER(STAT_Vulkan_ActiveAllocations, "Active Allocations", "Vulkan Allocators");

// Defrag Activity Stats
STAT_DEFINE_COUNTER(STAT_Vulkan_TextureDefragPending,        "Texture Defrag Pending",         "Vulkan Allocators");
STAT_DEFINE_COUNTER(STAT_Vulkan_TextureDefragMovesCompleted, "Texture Defrag Moves Completed", "Vulkan Allocators");
STAT_DEFINE_MEMORY(STAT_Vulkan_TextureDefragBytesMoved,      "Texture Defrag Bytes Moved",     "Vulkan Allocators");

STAT_DEFINE_COUNTER(STAT_Vulkan_BufferDefragPending,         "Buffer Defrag Pending",          "Vulkan Allocators");
STAT_DEFINE_COUNTER(STAT_Vulkan_BufferDefragMovesCompleted,  "Buffer Defrag Moves Completed",  "Vulkan Allocators");
STAT_DEFINE_MEMORY(STAT_Vulkan_BufferDefragBytesMoved,       "Buffer Defrag Bytes Moved",      "Vulkan Allocators");

// PSO Stats
STAT_DEFINE_COUNTER(STAT_Vulkan_PSOCreateCount,                "PSOs Created",            "Vulkan PSO");
STAT_DEFINE_MEMORY(STAT_Vulkan_PSOCacheSize,                   "Cache Serialized Size",   "Vulkan PSO");
STAT_DEFINE_COUNTER(STAT_Vulkan_NumGraphicsPipelineStates,     "Graphics PSOs Created",   "Vulkan PSO");
STAT_DEFINE_COUNTER(STAT_Vulkan_NumComputePipelineStates,      "Compute PSOs Created",    "Vulkan PSO");
STAT_DEFINE_COUNTER(STAT_Vulkan_NumRayTracingPipelineStates,   "RayTracing PSOs Created", "Vulkan PSO");
STAT_DEFINE_COUNTER(STAT_Vulkan_NumMeshletPipelineStates,      "Meshlet PSOs Created",    "Vulkan PSO");

// Command Primitive Stats
STAT_DEFINE_COUNTER(STAT_Vulkan_CommandBufferCount, "Command Buffers", "Vulkan Commands");
STAT_DEFINE_COUNTER(STAT_Vulkan_CommandPoolCount,   "Command Pools",   "Vulkan Commands");

// Submission Stats (per frame)
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_Submits,                     "Submits",        "Vulkan Submissions / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_SemaphoreOnlySubmits,        "Semaphore Only", "Vulkan Submissions / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_SplitsCommandLimit,          "Limit Splits",   "Vulkan Submissions / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_SplitsSwapChainAcquire,      "Acquire Splits", "Vulkan Submissions / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_SplitsFence,                 "Fence Splits",   "Vulkan Submissions / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_SplitsOther,                 "Other Splits",   "Vulkan Submissions / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_EmptyCommandBuffersRecycled, "Recycled Empty", "Vulkan Submissions / Frame");

// Ray Tracing Stats (per frame)
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_BLASBuilds,                            "BLAS Builds",      "Vulkan Ray Tracing / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_BLASUpdates,                           "BLAS Updates",     "Vulkan Ray Tracing / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_TLASBuilds,                            "TLAS Builds",      "Vulkan Ray Tracing / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_TLASUpdates,                           "TLAS Updates",     "Vulkan Ray Tracing / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_AccelerationStructureCompactions,      "Compactions",      "Vulkan Ray Tracing / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_AccelerationStructureCopies,           "Copies",           "Vulkan Ray Tracing / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_AccelerationStructureSerializations,   "Serializations",   "Vulkan Ray Tracing / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_AccelerationStructureDeserializations, "Deserializations", "Vulkan Ray Tracing / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_OpacityMicromapBuilds,                 "OMM Builds",       "Vulkan Ray Tracing / Frame");
STAT_DEFINE_FRAME_COUNTER(STAT_Vulkan_ClusterOperations,                     "Cluster Ops",      "Vulkan Ray Tracing / Frame");

// Query Stats
STAT_DEFINE_COUNTER(STAT_Vulkan_QueryPoolCount, "Query Pools", "Vulkan Queries");

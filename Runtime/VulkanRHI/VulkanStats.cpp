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

// Query Stats
STAT_DEFINE_COUNTER(STAT_Vulkan_QueryPoolCount, "Query Pools", "Vulkan Queries");

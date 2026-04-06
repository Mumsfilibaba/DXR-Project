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

// PSO Stats
STAT_DEFINE_COUNTER(STAT_Vulkan_PSOCreateCount, "PSOs Created",          "Vulkan PSO");
STAT_DEFINE_MEMORY(STAT_Vulkan_PSOCacheSize,    "Cache Serialized Size", "Vulkan PSO");

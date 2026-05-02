#pragma once
#include "Core/Stats/Stats.h"

// -------------------------------------------------------------------------------------------
// Vulkan Allocator Pool Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_BufferPoolAllocated);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_BufferPoolUsed);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_BufferPoolFragmented);

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_TexturePoolAllocated);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_TexturePoolUsed);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_TexturePoolFragmented);

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_UploadHeapAllocated);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_UploadHeapUsed);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_UploadHeapFragmented);

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_ActiveAllocations);

// -------------------------------------------------------------------------------------------
// Vulkan PSO Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_PSOCreateCount);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_PSOCacheSize);

// -------------------------------------------------------------------------------------------
// Vulkan Command Primitive Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_CommandBufferCount);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_CommandPoolCount);

// -------------------------------------------------------------------------------------------
// Vulkan Query Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_QueryPoolCount);

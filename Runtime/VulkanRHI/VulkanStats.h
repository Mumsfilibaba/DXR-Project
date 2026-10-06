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
// Vulkan Defrag Activity Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_TextureDefragPending);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_TextureDefragMovesCompleted);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_TextureDefragBytesMoved);

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_BufferDefragPending);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_BufferDefragMovesCompleted);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_BufferDefragBytesMoved);

// -------------------------------------------------------------------------------------------
// Vulkan PSO Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_PSOCreateCount);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_PSOCacheSize);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_NumGraphicsPipelineStates);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_NumComputePipelineStates);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_NumRayTracingPipelineStates);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_NumMeshletPipelineStates);

// -------------------------------------------------------------------------------------------
// Vulkan Command Primitive Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_CommandBufferCount);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_CommandPoolCount);

// -------------------------------------------------------------------------------------------
// Vulkan Submission Stats (per frame)
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_Submits);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_SemaphoreOnlySubmits);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_SplitsCommandLimit);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_SplitsSwapChainAcquire);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_SplitsFence);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_SplitsOther);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_EmptyCommandBuffersRecycled);

// -------------------------------------------------------------------------------------------
// Vulkan Ray Tracing Stats (per frame)
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_BLASBuilds);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_BLASUpdates);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_TLASBuilds);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_TLASUpdates);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_AccelerationStructureCompactions);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_AccelerationStructureCopies);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_AccelerationStructureSerializations);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_AccelerationStructureDeserializations);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_OpacityMicromapBuilds);
STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_ClusterOperations);

// -------------------------------------------------------------------------------------------
// Vulkan Query Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(VULKANRHI_API, STAT_Vulkan_QueryPoolCount);

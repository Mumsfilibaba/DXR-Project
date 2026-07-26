#pragma once
#include "Core/Stats/Stats.h"

// -------------------------------------------------------------------------------------------
// Command Submission Stats (migrated from RHIStatistics)
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_DrawCalls);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_DispatchCalls);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_Commands);

// -------------------------------------------------------------------------------------------
// Texture Memory Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_TextureMemory);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_RenderTargetMemory);

// -------------------------------------------------------------------------------------------
// Buffer Memory Stats (per usage type)
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_VertexBufferMemory);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_IndexBufferMemory);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_ConstantBufferMemory);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_StructuredBufferMemory);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_MiscBufferMemory);

// -------------------------------------------------------------------------------------------
// Other Memory Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_AccelerationStructureMemory);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_UploadMemory);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_ReadbackMemory);

// -------------------------------------------------------------------------------------------
// Ray Tracing Acceleration Structure Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_BLASCount);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_TLASCount);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_AccelerationStructureBuilds);

// -------------------------------------------------------------------------------------------
// Budget Stats (polled once per frame)
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_LocalMemoryBudget);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_LocalMemoryUsage);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_NonLocalMemoryBudget);
STAT_DECLARE_EXTERN(RHI_API, STAT_RHI_NonLocalMemoryUsage);

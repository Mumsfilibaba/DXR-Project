#include "RHI/RHIStats.h"

// -------------------------------------------------------------------------------------------
// Command Submission Stats
// -------------------------------------------------------------------------------------------

STAT_DEFINE_COUNTER(STAT_RHI_DrawCalls,     TEXT("Draw Calls"),     TEXT("RHI"));
STAT_DEFINE_COUNTER(STAT_RHI_DispatchCalls, TEXT("Dispatch Calls"), TEXT("RHI"));
STAT_DEFINE_COUNTER(STAT_RHI_Commands,      TEXT("Commands"),       TEXT("RHI"));

// -------------------------------------------------------------------------------------------
// Texture Memory Stats
// -------------------------------------------------------------------------------------------

STAT_DEFINE_MEMORY(STAT_RHI_TextureMemory,      TEXT("Texture Memory"),       TEXT("RHI"));
STAT_DEFINE_MEMORY(STAT_RHI_RenderTargetMemory, TEXT("Render Target Memory"), TEXT("RHI"));

// -------------------------------------------------------------------------------------------
// Buffer Memory Stats
// -------------------------------------------------------------------------------------------

STAT_DEFINE_MEMORY(STAT_RHI_VertexBufferMemory,     TEXT("Vertex Buffer Memory"),     TEXT("RHI"));
STAT_DEFINE_MEMORY(STAT_RHI_IndexBufferMemory,      TEXT("Index Buffer Memory"),      TEXT("RHI"));
STAT_DEFINE_MEMORY(STAT_RHI_ConstantBufferMemory,   TEXT("Constant Buffer Memory"),   TEXT("RHI"));
STAT_DEFINE_MEMORY(STAT_RHI_StructuredBufferMemory, TEXT("Structured Buffer Memory"), TEXT("RHI"));
STAT_DEFINE_MEMORY(STAT_RHI_MiscBufferMemory,       TEXT("Misc Buffer Memory"),       TEXT("RHI"));

// -------------------------------------------------------------------------------------------
// Other Memory Stats
// -------------------------------------------------------------------------------------------

STAT_DEFINE_MEMORY(STAT_RHI_AccelerationStructureMemory, TEXT("Acceleration Structure Memory"), TEXT("RHI"));
STAT_DEFINE_MEMORY(STAT_RHI_AccelerationStructureCompactionSavings, TEXT("Saved by Acceleration Structure Compaction"), TEXT("RHI"));
STAT_DEFINE_MEMORY(STAT_RHI_UploadMemory,                TEXT("Upload Memory"),                 TEXT("RHI"));
STAT_DEFINE_MEMORY(STAT_RHI_ReadbackMemory,              TEXT("Readback Memory"),               TEXT("RHI"));

// -------------------------------------------------------------------------------------------
// Ray Tracing Acceleration Structure Stats
// -------------------------------------------------------------------------------------------

STAT_DEFINE_COUNTER(STAT_RHI_BLASCount,                  TEXT("BLAS Count"), TEXT("RHI"));
STAT_DEFINE_COUNTER(STAT_RHI_TLASCount,                  TEXT("TLAS Count"), TEXT("RHI"));
STAT_DEFINE_COUNTER(STAT_RHI_AccelerationStructureBuilds, TEXT("AS Builds"), TEXT("RHI"));

// -------------------------------------------------------------------------------------------
// Budget Stats
// -------------------------------------------------------------------------------------------

STAT_DEFINE_MEMORY(STAT_RHI_LocalMemoryBudget,     TEXT("Local Memory Budget"),      TEXT("RHI Budget"));
STAT_DEFINE_MEMORY(STAT_RHI_LocalMemoryUsage,      TEXT("Local Memory Usage"),       TEXT("RHI Budget"));
STAT_DEFINE_MEMORY(STAT_RHI_NonLocalMemoryBudget,  TEXT("Non-Local Memory Budget"),  TEXT("RHI Budget"));
STAT_DEFINE_MEMORY(STAT_RHI_NonLocalMemoryUsage,   TEXT("Non-Local Memory Usage"),   TEXT("RHI Budget"));

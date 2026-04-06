#include "RHI/RHIStats.h"

// -------------------------------------------------------------------------------------------
// Command Submission Stats
// -------------------------------------------------------------------------------------------

STAT_DEFINE_COUNTER(STAT_RHI_DrawCalls,     "Draw Calls",     "RHI");
STAT_DEFINE_COUNTER(STAT_RHI_DispatchCalls, "Dispatch Calls", "RHI");
STAT_DEFINE_COUNTER(STAT_RHI_Commands,      "Commands",       "RHI");

// -------------------------------------------------------------------------------------------
// Texture Memory Stats
// -------------------------------------------------------------------------------------------

STAT_DEFINE_MEMORY(STAT_RHI_TextureMemory,      "Texture Memory",       "RHI");
STAT_DEFINE_MEMORY(STAT_RHI_RenderTargetMemory, "Render Target Memory", "RHI");

// -------------------------------------------------------------------------------------------
// Buffer Memory Stats
// -------------------------------------------------------------------------------------------

STAT_DEFINE_MEMORY(STAT_RHI_VertexBufferMemory,     "Vertex Buffer Memory",     "RHI");
STAT_DEFINE_MEMORY(STAT_RHI_IndexBufferMemory,      "Index Buffer Memory",      "RHI");
STAT_DEFINE_MEMORY(STAT_RHI_ConstantBufferMemory,   "Constant Buffer Memory",   "RHI");
STAT_DEFINE_MEMORY(STAT_RHI_StructuredBufferMemory, "Structured Buffer Memory", "RHI");
STAT_DEFINE_MEMORY(STAT_RHI_MiscBufferMemory,       "Misc Buffer Memory",       "RHI");

// -------------------------------------------------------------------------------------------
// Other Memory Stats
// -------------------------------------------------------------------------------------------

STAT_DEFINE_MEMORY(STAT_RHI_AccelerationStructureMemory, "Acceleration Structure Memory", "RHI");
STAT_DEFINE_MEMORY(STAT_RHI_UploadMemory,                "Upload Memory",                 "RHI");
STAT_DEFINE_MEMORY(STAT_RHI_ReadbackMemory,              "Readback Memory",               "RHI");

// -------------------------------------------------------------------------------------------
// Budget Stats
// -------------------------------------------------------------------------------------------

STAT_DEFINE_MEMORY(STAT_RHI_LocalMemoryBudget,     "Local Memory Budget",      "RHI Budget");
STAT_DEFINE_MEMORY(STAT_RHI_LocalMemoryUsage,      "Local Memory Usage",       "RHI Budget");
STAT_DEFINE_MEMORY(STAT_RHI_NonLocalMemoryBudget,  "Non-Local Memory Budget",  "RHI Budget");
STAT_DEFINE_MEMORY(STAT_RHI_NonLocalMemoryUsage,   "Non-Local Memory Usage",   "RHI Budget");

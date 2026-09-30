#include "MetalRHI/MetalStats.h"

STAT_DEFINE_COUNTER(STAT_Metal_PSOCreateCount,            "PSOs Created",          "Metal PSO");
STAT_DEFINE_COUNTER(STAT_Metal_NumGraphicsPipelineStates, "Graphics PSOs Created", "Metal PSO");
STAT_DEFINE_COUNTER(STAT_Metal_NumComputePipelineStates,  "Compute PSOs Created",  "Metal PSO");
STAT_DEFINE_COUNTER(STAT_Metal_NumMeshletPipelineStates,  "Meshlet PSOs Created",  "Metal PSO");

STAT_DEFINE_COUNTER(STAT_Metal_CommandBufferCount,   "Command Buffers Submitted", "Metal Commands");
STAT_DEFINE_COUNTER(STAT_Metal_CommandBuffersAlive, "Command Buffers Alive",     "Metal Commands");
STAT_DEFINE_COUNTER(STAT_Metal_EncoderCount,         "Encoders Opened",           "Metal Commands");
STAT_DEFINE_COUNTER(STAT_Metal_EncodersOpen,         "Encoders Open",             "Metal Commands");

STAT_DEFINE_COUNTER(STAT_Metal_CounterSampleBufferCount, "Counter Sample Buffers",  "Metal Queries");
STAT_DEFINE_COUNTER(STAT_Metal_TimestampSlotsInFlight,   "Timestamp Slots In Flight", "Metal Queries");
STAT_DEFINE_COUNTER(STAT_Metal_OcclusionSlotsInFlight,   "Occlusion Slots In Flight", "Metal Queries");

STAT_DEFINE_MEMORY(STAT_Metal_UploadHeapAllocated,       "Upload Heap Allocated",       "Metal Allocators");
STAT_DEFINE_MEMORY(STAT_Metal_UploadHeapUsed,            "Upload Heap Used",            "Metal Allocators");
STAT_DEFINE_MEMORY(STAT_Metal_UploadHeapFragmented,      "Upload Heap Fragmented",      "Metal Allocators");
STAT_DEFINE_MEMORY(STAT_Metal_DynamicConstantsAllocated, "Dynamic Constants Allocated", "Metal Allocators");
STAT_DEFINE_MEMORY(STAT_Metal_DynamicConstantsUsed,      "Dynamic Constants Used",      "Metal Allocators");
STAT_DEFINE_COUNTER(STAT_Metal_DynamicConstantsPages,    "Dynamic Constants Pages",     "Metal Allocators");
STAT_DEFINE_MEMORY(STAT_Metal_StagingAllocated,          "Staging Allocated",           "Metal Allocators");
STAT_DEFINE_MEMORY(STAT_Metal_StagingUsed,               "Staging Used",                "Metal Allocators");
STAT_DEFINE_COUNTER(STAT_Metal_StagingPages,             "Staging Pages",               "Metal Allocators");
STAT_DEFINE_COUNTER(STAT_Metal_UploadPagesCreated,       "Upload Pages Created",        "Metal Allocators");
STAT_DEFINE_COUNTER(STAT_Metal_UploadPagesReleased,      "Upload Pages Released",       "Metal Allocators");

STAT_DEFINE_MEMORY(STAT_Metal_BufferHeapAllocated,   "Buffer Heap Allocated",   "Metal Heaps");
STAT_DEFINE_MEMORY(STAT_Metal_BufferHeapUsed,        "Buffer Heap Used",        "Metal Heaps");
STAT_DEFINE_MEMORY(STAT_Metal_BufferHeapFragmented,  "Buffer Heap Fragmented",  "Metal Heaps");
STAT_DEFINE_COUNTER(STAT_Metal_BufferHeaps,          "Buffer Heaps",            "Metal Heaps");
STAT_DEFINE_MEMORY(STAT_Metal_TextureHeapAllocated,  "Texture Heap Allocated",  "Metal Heaps");
STAT_DEFINE_MEMORY(STAT_Metal_TextureHeapUsed,       "Texture Heap Used",       "Metal Heaps");
STAT_DEFINE_MEMORY(STAT_Metal_TextureHeapFragmented, "Texture Heap Fragmented", "Metal Heaps");
STAT_DEFINE_COUNTER(STAT_Metal_TextureHeaps,         "Texture Heaps",           "Metal Heaps");

STAT_DEFINE_MEMORY(STAT_Metal_StandaloneBufferBytes,  "Standalone Buffer Bytes",  "Metal Standalone");
STAT_DEFINE_COUNTER(STAT_Metal_StandaloneBuffers,     "Standalone Buffers",       "Metal Standalone");
STAT_DEFINE_MEMORY(STAT_Metal_StandaloneTextureBytes, "Standalone Texture Bytes", "Metal Standalone");
STAT_DEFINE_COUNTER(STAT_Metal_StandaloneTextures,    "Standalone Textures",      "Metal Standalone");

STAT_DEFINE_COUNTER(STAT_Metal_DefragMoves,      "Defrag Moves",       "Metal Defrag");
STAT_DEFINE_COUNTER(STAT_Metal_DefragCancels,    "Defrag Cancels",     "Metal Defrag");
STAT_DEFINE_MEMORY(STAT_Metal_DefragBytesMoved,  "Defrag Bytes Moved", "Metal Defrag");
STAT_DEFINE_COUNTER(STAT_Metal_DefragPending,    "Defrag Pending",     "Metal Defrag");

STAT_DEFINE_MEMORY(STAT_Metal_ResidencyBudget, "Residency Budget", "Metal Residency");
STAT_DEFINE_MEMORY(STAT_Metal_ResidentBytes,   "Resident Bytes",   "Metal Residency");
STAT_DEFINE_MEMORY(STAT_Metal_EvictedBytes,    "Evicted Bytes",    "Metal Residency");
STAT_DEFINE_MEMORY(STAT_Metal_PinnedBytes,     "Pinned Bytes",     "Metal Residency");
STAT_DEFINE_COUNTER(STAT_Metal_Evictions,      "Evictions",        "Metal Residency");

STAT_DEFINE_COUNTER(STAT_Metal_BindlessResourceSlots,  "Bindless Resource Slots",  "Metal Bindless");
STAT_DEFINE_COUNTER(STAT_Metal_BindlessSamplerSlots,   "Bindless Sampler Slots",   "Metal Bindless");
STAT_DEFINE_MEMORY(STAT_Metal_BindlessTableBytes,      "Bindless Table Bytes",     "Metal Bindless");
STAT_DEFINE_COUNTER(STAT_Metal_BindlessPendingWrites,  "Bindless Pending Writes",  "Metal Bindless");
STAT_DEFINE_COUNTER(STAT_Metal_BindlessGrowthCount,    "Bindless Table Growths",   "Metal Bindless");

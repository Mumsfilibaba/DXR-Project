#include "MetalRHI/MetalStats.h"

STAT_DEFINE_COUNTER(STAT_Metal_PSOCreateCount,            TEXT("PSOs Created"),          TEXT("Metal PSO"));
STAT_DEFINE_COUNTER(STAT_Metal_NumGraphicsPipelineStates, TEXT("Graphics PSOs Created"), TEXT("Metal PSO"));
STAT_DEFINE_COUNTER(STAT_Metal_NumComputePipelineStates,  TEXT("Compute PSOs Created"),  TEXT("Metal PSO"));
STAT_DEFINE_COUNTER(STAT_Metal_NumMeshletPipelineStates,  TEXT("Meshlet PSOs Created"),  TEXT("Metal PSO"));
STAT_DEFINE_COUNTER(STAT_Metal_LibraryCacheHits,          TEXT("Library Cache Hits"),    TEXT("Metal PSO"));
STAT_DEFINE_COUNTER(STAT_Metal_LibraryCacheMisses,        TEXT("Library Cache Misses"),  TEXT("Metal PSO"));
STAT_DEFINE_COUNTER(STAT_Metal_PSOCacheHits,              TEXT("PSO Cache Hits"),        TEXT("Metal PSO"));
STAT_DEFINE_COUNTER(STAT_Metal_PSOCacheMisses,            TEXT("PSO Cache Misses"),      TEXT("Metal PSO"));
STAT_DEFINE_COUNTER(STAT_Metal_BinaryArchiveHits,         TEXT("Binary Archive Hits"),   TEXT("Metal PSO"));
STAT_DEFINE_COUNTER(STAT_Metal_BinaryArchiveMisses,       TEXT("Binary Archive Misses"), TEXT("Metal PSO"));
STAT_DEFINE_COUNTER(STAT_Metal_BinaryArchiveAdds,         TEXT("Binary Archive Adds"),   TEXT("Metal PSO"));
STAT_DEFINE_MEMORY(STAT_Metal_BinaryArchiveSize,          TEXT("Binary Archive Size"),   TEXT("Metal PSO"));

STAT_DEFINE_COUNTER(STAT_Metal_CommandBufferCount,  TEXT("Command Buffers Submitted"), TEXT("Metal Commands"));
STAT_DEFINE_COUNTER(STAT_Metal_CommandBuffersAlive, TEXT("Command Buffers Alive"),     TEXT("Metal Commands"));
STAT_DEFINE_COUNTER(STAT_Metal_EncoderCount,        TEXT("Encoders Opened"),           TEXT("Metal Commands"));
STAT_DEFINE_COUNTER(STAT_Metal_EncodersOpen,        TEXT("Encoders Open"),             TEXT("Metal Commands"));

STAT_DEFINE_COUNTER(STAT_Metal_CounterSampleBufferCount, TEXT("Counter Sample Buffers"),    TEXT("Metal Queries"));
STAT_DEFINE_COUNTER(STAT_Metal_TimestampSlotsInFlight,   TEXT("Timestamp Slots In Flight"), TEXT("Metal Queries"));
STAT_DEFINE_COUNTER(STAT_Metal_OcclusionSlotsInFlight,   TEXT("Occlusion Slots In Flight"), TEXT("Metal Queries"));

STAT_DEFINE_COUNTER(STAT_Metal_GPUTotalCycles,            TEXT("Total Cycles"),             TEXT("Metal GPU"));
STAT_DEFINE_COUNTER(STAT_Metal_GPUVertexCycles,           TEXT("Vertex Cycles"),            TEXT("Metal GPU"));
STAT_DEFINE_COUNTER(STAT_Metal_GPUTessellationCycles,     TEXT("Tessellation Cycles"),      TEXT("Metal GPU"));
STAT_DEFINE_COUNTER(STAT_Metal_GPUPostTessellationCycles, TEXT("Post-Tessellation Cycles"), TEXT("Metal GPU"));
STAT_DEFINE_COUNTER(STAT_Metal_GPUFragmentCycles,         TEXT("Fragment Cycles"),          TEXT("Metal GPU"));
STAT_DEFINE_COUNTER(STAT_Metal_GPURenderTargetCycles,     TEXT("Render Target Cycles"),     TEXT("Metal GPU"));

STAT_DEFINE_MEMORY(STAT_Metal_UploadHeapAllocated,       TEXT("Upload Heap Allocated"),       TEXT("Metal Allocators"));
STAT_DEFINE_MEMORY(STAT_Metal_UploadHeapUsed,            TEXT("Upload Heap Used"),            TEXT("Metal Allocators"));
STAT_DEFINE_MEMORY(STAT_Metal_UploadHeapFragmented,      TEXT("Upload Heap Fragmented"),      TEXT("Metal Allocators"));
STAT_DEFINE_MEMORY(STAT_Metal_DynamicConstantsAllocated, TEXT("Dynamic Constants Allocated"), TEXT("Metal Allocators"));
STAT_DEFINE_MEMORY(STAT_Metal_DynamicConstantsUsed,      TEXT("Dynamic Constants Used"),      TEXT("Metal Allocators"));
STAT_DEFINE_COUNTER(STAT_Metal_DynamicConstantsPages,    TEXT("Dynamic Constants Pages"),     TEXT("Metal Allocators"));
STAT_DEFINE_MEMORY(STAT_Metal_StagingAllocated,          TEXT("Staging Allocated"),           TEXT("Metal Allocators"));
STAT_DEFINE_MEMORY(STAT_Metal_StagingUsed,               TEXT("Staging Used"),                TEXT("Metal Allocators"));
STAT_DEFINE_COUNTER(STAT_Metal_StagingPages,             TEXT("Staging Pages"),               TEXT("Metal Allocators"));
STAT_DEFINE_COUNTER(STAT_Metal_UploadPagesCreated,       TEXT("Upload Pages Created"),        TEXT("Metal Allocators"));
STAT_DEFINE_COUNTER(STAT_Metal_UploadPagesReleased,      TEXT("Upload Pages Released"),       TEXT("Metal Allocators"));

STAT_DEFINE_MEMORY(STAT_Metal_BufferHeapAllocated,   TEXT("Buffer Heap Allocated"),   TEXT("Metal Heaps"));
STAT_DEFINE_MEMORY(STAT_Metal_BufferHeapUsed,        TEXT("Buffer Heap Used"),        TEXT("Metal Heaps"));
STAT_DEFINE_MEMORY(STAT_Metal_BufferHeapFragmented,  TEXT("Buffer Heap Fragmented"),  TEXT("Metal Heaps"));
STAT_DEFINE_COUNTER(STAT_Metal_BufferHeaps,          TEXT("Buffer Heaps"),            TEXT("Metal Heaps"));
STAT_DEFINE_MEMORY(STAT_Metal_TextureHeapAllocated,  TEXT("Texture Heap Allocated"),  TEXT("Metal Heaps"));
STAT_DEFINE_MEMORY(STAT_Metal_TextureHeapUsed,       TEXT("Texture Heap Used"),       TEXT("Metal Heaps"));
STAT_DEFINE_MEMORY(STAT_Metal_TextureHeapFragmented, TEXT("Texture Heap Fragmented"), TEXT("Metal Heaps"));
STAT_DEFINE_COUNTER(STAT_Metal_TextureHeaps,         TEXT("Texture Heaps"),           TEXT("Metal Heaps"));

STAT_DEFINE_MEMORY(STAT_Metal_StandaloneBufferBytes,                TEXT("Standalone Buffer Bytes"),                 TEXT("Metal Standalone"));
STAT_DEFINE_COUNTER(STAT_Metal_StandaloneBuffers,                   TEXT("Standalone Buffers"),                      TEXT("Metal Standalone"));
STAT_DEFINE_MEMORY(STAT_Metal_StandaloneTextureBytes,               TEXT("Standalone Texture Bytes"),                TEXT("Metal Standalone"));
STAT_DEFINE_COUNTER(STAT_Metal_StandaloneTextures,                  TEXT("Standalone Textures"),                     TEXT("Metal Standalone"));
STAT_DEFINE_MEMORY(STAT_Metal_StandaloneAccelerationStructureBytes, TEXT("Standalone Acceleration Structure Bytes"), TEXT("Metal Standalone"));
STAT_DEFINE_COUNTER(STAT_Metal_StandaloneAccelerationStructures,    TEXT("Standalone Acceleration Structures"),      TEXT("Metal Standalone"));

STAT_DEFINE_COUNTER(STAT_Metal_DefragMoves,      TEXT("Defrag Moves"),       TEXT("Metal Defrag"));
STAT_DEFINE_COUNTER(STAT_Metal_DefragCancels,    TEXT("Defrag Cancels"),     TEXT("Metal Defrag"));
STAT_DEFINE_MEMORY(STAT_Metal_DefragBytesMoved,  TEXT("Defrag Bytes Moved"), TEXT("Metal Defrag"));
STAT_DEFINE_COUNTER(STAT_Metal_DefragPending,    TEXT("Defrag Pending"),     TEXT("Metal Defrag"));

STAT_DEFINE_MEMORY(STAT_Metal_ResidencyBudget, TEXT("Residency Budget"), TEXT("Metal Residency"));
STAT_DEFINE_MEMORY(STAT_Metal_ResidentBytes,   TEXT("Resident Bytes"),   TEXT("Metal Residency"));
STAT_DEFINE_MEMORY(STAT_Metal_EvictedBytes,    TEXT("Evicted Bytes"),    TEXT("Metal Residency"));
STAT_DEFINE_MEMORY(STAT_Metal_PinnedBytes,     TEXT("Pinned Bytes"),     TEXT("Metal Residency"));
STAT_DEFINE_COUNTER(STAT_Metal_Evictions,      TEXT("Evictions"),        TEXT("Metal Residency"));

STAT_DEFINE_COUNTER(STAT_Metal_BindlessResourceSlots,  TEXT("Bindless Resource Slots"),  TEXT("Metal Bindless"));
STAT_DEFINE_COUNTER(STAT_Metal_BindlessSamplerSlots,   TEXT("Bindless Sampler Slots"),   TEXT("Metal Bindless"));
STAT_DEFINE_MEMORY(STAT_Metal_BindlessTableBytes,      TEXT("Bindless Table Bytes"),     TEXT("Metal Bindless"));
STAT_DEFINE_COUNTER(STAT_Metal_BindlessPendingWrites,  TEXT("Bindless Pending Writes"),  TEXT("Metal Bindless"));
STAT_DEFINE_COUNTER(STAT_Metal_BindlessGrowthCount,    TEXT("Bindless Table Growths"),   TEXT("Metal Bindless"));

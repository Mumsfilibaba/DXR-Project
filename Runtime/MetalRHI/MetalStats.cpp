#include "MetalRHI/MetalStats.h"

STAT_DEFINE_COUNTER(STAT_Metal_PSOCreateCount,            "PSOs Created",          "Metal PSO");
STAT_DEFINE_COUNTER(STAT_Metal_NumGraphicsPipelineStates, "Graphics PSOs Created", "Metal PSO");
STAT_DEFINE_COUNTER(STAT_Metal_NumComputePipelineStates,  "Compute PSOs Created",  "Metal PSO");
STAT_DEFINE_COUNTER(STAT_Metal_NumMeshletPipelineStates,  "Meshlet PSOs Created",  "Metal PSO");

STAT_DEFINE_COUNTER(STAT_Metal_CommandBufferCount, "Command Buffers", "Metal Commands");
STAT_DEFINE_COUNTER(STAT_Metal_EncoderCount,       "Encoders",        "Metal Commands");

STAT_DEFINE_COUNTER(STAT_Metal_CounterSampleBufferCount, "Counter Sample Buffers",  "Metal Queries");
STAT_DEFINE_COUNTER(STAT_Metal_TimestampSlotsInFlight,   "Timestamp Slots In Flight", "Metal Queries");
STAT_DEFINE_COUNTER(STAT_Metal_OcclusionSlotsInFlight,   "Occlusion Slots In Flight", "Metal Queries");

STAT_DEFINE_MEMORY(STAT_Metal_UploadHeapAllocated,  "Upload Heap Allocated",  "Metal Allocators");
STAT_DEFINE_MEMORY(STAT_Metal_UploadHeapUsed,       "Upload Heap Used",       "Metal Allocators");
STAT_DEFINE_MEMORY(STAT_Metal_UploadHeapFragmented, "Upload Heap Fragmented", "Metal Allocators");
STAT_DEFINE_MEMORY(STAT_Metal_DeviceHeapAllocated,  "Device Heap Allocated",  "Metal Allocators");
STAT_DEFINE_MEMORY(STAT_Metal_DeviceHeapUsed,       "Device Heap Used",       "Metal Allocators");
STAT_DEFINE_MEMORY(STAT_Metal_DeviceHeapFragmented, "Device Heap Fragmented", "Metal Allocators");

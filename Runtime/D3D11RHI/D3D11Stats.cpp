#include "D3D11RHI/D3D11Stats.h"

STAT_DEFINE_MEMORY(STAT_D3D11_ResourceMemory, TEXT("Resource Memory"), TEXT("D3D11 Resources"));
STAT_DEFINE_COUNTER(STAT_D3D11_ResourceCount, TEXT("Resource Count"),  TEXT("D3D11 Resources"));
STAT_DEFINE_MEMORY(STAT_D3D11_DefaultMemory,  TEXT("Default Memory"),  TEXT("D3D11 Resources"));
STAT_DEFINE_MEMORY(STAT_D3D11_DynamicMemory,  TEXT("Dynamic Memory"),  TEXT("D3D11 Resources"));
STAT_DEFINE_MEMORY(STAT_D3D11_StagingMemory,  TEXT("Staging Memory"),  TEXT("D3D11 Resources"));

// PSO Stats
STAT_DEFINE_COUNTER(STAT_D3D11_NumGraphicsPipelineStates, TEXT("Graphics PSOs Created"), TEXT("D3D11 PSO"));
STAT_DEFINE_COUNTER(STAT_D3D11_NumComputePipelineStates,  TEXT("Compute PSOs Created"),  TEXT("D3D11 PSO"));

// State Change Stats
STAT_DEFINE_COUNTER(STAT_D3D11_ShaderChanges,          TEXT("Shader Changes"),           TEXT("D3D11 State Changes"));
STAT_DEFINE_COUNTER(STAT_D3D11_InputAssemblerChanges,  TEXT("Input Assembler Changes"),  TEXT("D3D11 State Changes"));
STAT_DEFINE_COUNTER(STAT_D3D11_RasterizerChanges,      TEXT("Rasterizer Changes"),       TEXT("D3D11 State Changes"));
STAT_DEFINE_COUNTER(STAT_D3D11_OutputMergerChanges,    TEXT("Output Merger Changes"),    TEXT("D3D11 State Changes"));
STAT_DEFINE_COUNTER(STAT_D3D11_ConstantBufferChanges,  TEXT("Constant Buffer Changes"),  TEXT("D3D11 State Changes"));
STAT_DEFINE_COUNTER(STAT_D3D11_ShaderResourceChanges,  TEXT("Shader Resource Changes"),  TEXT("D3D11 State Changes"));
STAT_DEFINE_COUNTER(STAT_D3D11_SamplerChanges,         TEXT("Sampler Changes"),          TEXT("D3D11 State Changes"));
STAT_DEFINE_COUNTER(STAT_D3D11_UnorderedAccessChanges, TEXT("Unordered Access Changes"), TEXT("D3D11 State Changes"));
STAT_DEFINE_COUNTER(STAT_D3D11_ShaderConstantUploads,  TEXT("Shader Constant Uploads"),  TEXT("D3D11 State Changes"));

// Query Stats
STAT_DEFINE_COUNTER(STAT_D3D11_QueryCount, TEXT("Queries"), TEXT("D3D11 Queries"));

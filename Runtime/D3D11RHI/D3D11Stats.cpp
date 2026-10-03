#include "D3D11RHI/D3D11Stats.h"

STAT_DEFINE_MEMORY(STAT_D3D11_ResourceMemory, "Resource Memory", "D3D11 Resources");
STAT_DEFINE_COUNTER(STAT_D3D11_ResourceCount, "Resource Count",  "D3D11 Resources");
STAT_DEFINE_MEMORY(STAT_D3D11_DefaultMemory,  "Default Memory",  "D3D11 Resources");
STAT_DEFINE_MEMORY(STAT_D3D11_DynamicMemory,  "Dynamic Memory",  "D3D11 Resources");
STAT_DEFINE_MEMORY(STAT_D3D11_StagingMemory,  "Staging Memory",  "D3D11 Resources");

// PSO Stats
STAT_DEFINE_COUNTER(STAT_D3D11_NumGraphicsPipelineStates, "Graphics PSOs Created", "D3D11 PSO");
STAT_DEFINE_COUNTER(STAT_D3D11_NumComputePipelineStates,  "Compute PSOs Created",  "D3D11 PSO");

// State Change Stats
STAT_DEFINE_COUNTER(STAT_D3D11_ShaderChanges,          "Shader Changes",           "D3D11 State Changes");
STAT_DEFINE_COUNTER(STAT_D3D11_InputAssemblerChanges,  "Input Assembler Changes",  "D3D11 State Changes");
STAT_DEFINE_COUNTER(STAT_D3D11_RasterizerChanges,      "Rasterizer Changes",       "D3D11 State Changes");
STAT_DEFINE_COUNTER(STAT_D3D11_OutputMergerChanges,    "Output Merger Changes",    "D3D11 State Changes");
STAT_DEFINE_COUNTER(STAT_D3D11_ConstantBufferChanges,  "Constant Buffer Changes",  "D3D11 State Changes");
STAT_DEFINE_COUNTER(STAT_D3D11_ShaderResourceChanges,  "Shader Resource Changes",  "D3D11 State Changes");
STAT_DEFINE_COUNTER(STAT_D3D11_SamplerChanges,         "Sampler Changes",          "D3D11 State Changes");
STAT_DEFINE_COUNTER(STAT_D3D11_UnorderedAccessChanges, "Unordered Access Changes", "D3D11 State Changes");
STAT_DEFINE_COUNTER(STAT_D3D11_ShaderConstantUploads,  "Shader Constant Uploads",  "D3D11 State Changes");

// Query Stats
STAT_DEFINE_COUNTER(STAT_D3D11_QueryCount, "Queries", "D3D11 Queries");

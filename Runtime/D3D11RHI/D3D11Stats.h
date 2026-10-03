#pragma once
#include "Core/Stats/Stats.h"

// -------------------------------------------------------------------------------------------
// D3D11 Resource Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_ResourceMemory);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_ResourceCount);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_DefaultMemory);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_DynamicMemory);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_StagingMemory);

// -------------------------------------------------------------------------------------------
// D3D11 PSO Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_NumGraphicsPipelineStates);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_NumComputePipelineStates);

// -------------------------------------------------------------------------------------------
// D3D11 State Change Stats (calls into the device context during the last frame)
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_ShaderChanges);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_InputAssemblerChanges);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_RasterizerChanges);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_OutputMergerChanges);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_ConstantBufferChanges);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_ShaderResourceChanges);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_SamplerChanges);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_UnorderedAccessChanges);
STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_ShaderConstantUploads);

// -------------------------------------------------------------------------------------------
// D3D11 Query Stats
// -------------------------------------------------------------------------------------------

STAT_DECLARE_EXTERN(D3D11RHI_API, STAT_D3D11_QueryCount);

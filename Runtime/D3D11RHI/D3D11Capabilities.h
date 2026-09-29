#pragma once
#include "D3D11RHI/D3D11Core.h"

// -------------------------------------------------------------------------------------------
// D3D11 Feature Support
// -------------------------------------------------------------------------------------------

extern D3D11RHI_API D3D_FEATURE_LEVEL GD3D11FeatureLevel;

/** ID3D11Device5 / ID3D11DeviceContext4 are available, so ID3D11Fence can be used */
extern D3D11RHI_API bool GD3D11SupportsFences;

/** Feature level 11_1 allows UAVs in every shader stage and 64 UAV slots */
extern D3D11RHI_API bool GD3D11SupportsUAVsInAllStages;

/** ID3D11DeviceContext1 is available, so constant buffers can be updated partially */
extern D3D11RHI_API bool GD3D11SupportsPartialConstantBufferUpdates;

/** 8 on feature level 11_0, 64 on feature level 11_1 */
extern D3D11RHI_API uint32 GD3D11MaxUnorderedAccessViews;

// -------------------------------------------------------------------------------------------
// D3D11 Capability Logging
// -------------------------------------------------------------------------------------------

extern D3D11RHI_API void DumpD3D11Capabilities();

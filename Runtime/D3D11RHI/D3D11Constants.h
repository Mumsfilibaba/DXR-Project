#pragma once
#include "RHI/RHICore.h"
#include "D3D11RHI/D3D11Configuration.h"

#define D3D11_MAX_VERTEX_BUFFER_SLOTS            (D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)                // 32
#define D3D11_MAX_RENDER_TARGET_COUNT            (D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)                   // 8
#define D3D11_MAX_VIEWPORT_AND_SCISSORRECT_COUNT (D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) // 16
#define D3D11_MAX_SHADER_RESOURCE_VIEWS          (D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)             // 128
#define D3D11_MAX_SAMPLER_STATES                 (D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)                    // 16
#define D3D11_MAX_CONSTANT_BUFFERS               (D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)        // 14
#define D3D11_MAX_UNORDERED_ACCESS_VIEWS         (D3D11_1_UAV_SLOT_COUNT)                                   // 64 (8 on FL 11_0)

#define D3D11_NUM_BACK_BUFFERS (3)

// ------------------------------------------------------------------------------------------------
// D3D11 has no register spaces or root constants. The RHI shader-constant block lives in the last
// constant-buffer slot, which user constant buffers must never use.
// ------------------------------------------------------------------------------------------------

#define D3D11_SHADER_CONSTANTS_REGISTER        (D3D11_MAX_CONSTANT_BUFFERS - 1) // b13
#define D3D11_MAX_32BIT_SHADER_CONSTANTS_COUNT (RHI_MAX_SHADER_CONSTANTS)       // 32 dwords = 128 bytes

#pragma once
#include "RHI/RHICore.h"
#include "D3D11RHI/D3D11Configuration.h"

#define D3D11_MAX_VERTEX_BUFFER_SLOTS            (D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)                // 32
#define D3D11_MAX_RENDER_TARGET_COUNT            (D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)                   // 8
#define D3D11_MAX_STREAM_OUTPUT_BUFFER_COUNT     (D3D11_SO_BUFFER_SLOT_COUNT)                               // 4
#define D3D11_MAX_VIEWPORT_AND_SCISSORRECT_COUNT (D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) // 16
#define D3D11_MAX_SHADER_RESOURCE_VIEWS          (D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)             // 128
#define D3D11_MAX_SAMPLER_STATES                 (D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)                    // 16
#define D3D11_MAX_CONSTANT_BUFFERS               (D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)        // 14
#define D3D11_MAX_UNORDERED_ACCESS_VIEWS         (D3D11_1_UAV_SLOT_COUNT)                                   // 64
#define D3D11_MAX_UNORDERED_ACCESS_VIEWS_FL11_0  (D3D11_PS_CS_UAV_REGISTER_COUNT)                           // 8
#define D3D11_MAX_RESOURCE_SAMPLE_COUNT          (D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT)                       // 32

#define D3D11_MAX_TEXTURE1D_SIZE                 (D3D11_REQ_TEXTURE1D_U_DIMENSION)                          // 16384
#define D3D11_MAX_TEXTURE1D_ARRAY_SLICES         (D3D11_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION)                 // 2048
#define D3D11_MAX_TEXTURE2D_SIZE                 (D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)                     // 16384
#define D3D11_MAX_TEXTURE2D_ARRAY_SLICES         (D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION)                 // 2048
#define D3D11_MAX_TEXTURE3D_SIZE                 (D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION)                   // 2048
#define D3D11_MAX_TEXTURECUBE_SIZE               (D3D11_REQ_TEXTURECUBE_DIMENSION)                          // 16384

#define D3D11_MIN_RESOURCE_SIZE_IN_MEGABYTES     (D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM)   // 128
#define D3D11_MAX_RESOURCE_SIZE_IN_MEGABYTES     (D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM)   // 2048
#define D3D11_RESOURCE_SIZE_VIDEO_MEMORY_SCALE   (D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_B_TERM)   // 0.25
#define D3D11_MIN_STRUCTURED_BUFFER_STRIDE       (sizeof(uint32))                                           // 4
#define D3D11_MAX_STRUCTURED_BUFFER_STRIDE       (D3D11_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES)          // 2048
#define D3D11_RAW_BUFFER_ALIGNMENT               (sizeof(uint32))                                           // 4

#define D3D11_CONSTANT_BUFFER_ELEMENT_SIZE (D3D11_COMMONSHADER_CONSTANT_BUFFER_COMPONENTS * sizeof(uint32)) // 16
#define D3D11_READBACK_ROW_PITCH_ALIGNMENT (256)

#define D3D11_NUM_BACK_BUFFERS (3)

// ------------------------------------------------------------------------------------------------
// D3D11 has no register spaces or root constants. The RHI shader-constant block is the cbuffer
// named Constants_CB, which FXC places in a slot the shader does not use; reflection reports it.
// ------------------------------------------------------------------------------------------------

#define D3D11_SHADER_CONSTANTS_CBUFFER_NAME    "Constants_CB"
#define D3D11_MAX_32BIT_SHADER_CONSTANTS_COUNT (RHI_MAX_SHADER_CONSTANTS) // 32 dwords = 128 bytes

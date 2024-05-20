#ifndef CONSTANTS_HLSLI
#define CONSTANTS_HLSLI
#include "CoreDefines.hlsli"

#define PI (3.14159265359)
#define PI_2 (3.14159265359)
#define INV_PI (1.0 / PI)
#define GAMMA (2.2)

#define MIN_ROUGHNESS (0.0)
#define MAX_ROUGHNESS (1.0)
#define MIN_VALUE (0.0000001)
#define EPSILON (0.0001)
#define RAY_OFFSET (0.02)

#define FLT32_MAX (3.402823466e+38)
#define FLT32_MIN (1.175494351e-38)        

#define FLT32_EPSILON (1.192092896e-07)

#define NUM_SHADOW_CASCADES (4)
#define NUM_FRUSTUM_PLANES (6)

#define SHADING_RATE_1x1 (0x0)
#define SHADING_RATE_1x2 (0x1)
#define SHADING_RATE_2x1 (0x4)
#define SHADING_RATE_2x2 (0x5)
#define SHADING_RATE_2x4 (0x6)
#define SHADING_RATE_4x2 (0x9)
#define SHADING_RATE_4x4 (0xa)

// Space: | Usage:
// 0      | Standard
// 1      | Constants
// 2      | RT Local

#define D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS space1
#define D3D12_SHADER_REGISTER_SPACE_RT_LOCAL        space2

#endif
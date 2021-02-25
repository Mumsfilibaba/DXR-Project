#ifndef CONSTANTS_HLSLI
#define CONSTANTS_HLSLI

#define PI    (3.14159265359f)
#define GAMMA (2.2f)

#define MIN_ROUGHNESS (0.05f)
#define MAX_ROUGHNESS (1.0f)
#define MIN_VALUE     (0.0000001f)
#define EPSILON       (0.0001f)
#define RAY_OFFSET    (0.2f)

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
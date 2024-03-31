#include "Constants.hlsli"

#define NUM_THREADS (16)

// Shader Constants
SHADER_CONSTANT_BLOCK_BEGIN
    uint CubeMapSize; // Size of one side of the TextureCube
SHADER_CONSTANT_BLOCK_END

SamplerState LinearSampler : register(s0);

Texture2D<float4>             Source  : register(t0);
RWTexture2DArray<min16float4> OutCube : register(u0);

static const float2 INV_ATAN = float2(0.1591f, 0.3183f);

// Transform from dispatch ID to cubemap face direction
static const float3x3 ROTATE_UV[6] = 
{
    // +X
    float3x3( 0,  0,  1,
              0, -1,  0,
             -1,  0,  0),
    // -X
    float3x3( 0,  0, -1,
              0, -1,  0,
              1,  0,  0),
    // +Y
    float3x3( 1,  0,  0,
              0,  0,  1,
              0,  1,  0),
    // -Y
    float3x3( 1,  0,  0,
              0,  0, -1,
              0, -1,  0),
    // +Z
    float3x3( 1,  0,  0,
              0, -1,  0,
              0,  0,  1),
    // -Z
    float3x3(-1,  0,  0,
              0, -1,  0,
              0,  0, -1)
};

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint3 DispatchThreadID : SV_DispatchThreadID, uint GroupIndex : SV_GroupIndex)
{
    uint3 TexCoord = DispatchThreadID;

    // Map the UV coords of the cubemap face to a direction
    // [(0, 0), (1, 1)] => [(-0.5, -0.5), (0.5, 0.5)]
    float2 UVs = (TexCoord.xy / float(Constants.CubeMapSize));
    float3 Direction = float3(UVs - 0.5f, 0.5f);

    // Rotate to cubemap face
    Direction = normalize(mul(ROTATE_UV[TexCoord.z], Direction));

    // Convert the world space direction into U,V texture coordinates in the panoramic texture.
    // Source: http://gl.ict.usc.edu/Data/HighResProbes/
    float2 PanoramaTexCoords = float2(atan2(Direction.x, Direction.z), acos(Direction.y)) * INV_ATAN;
    OutCube[TexCoord] = (min16float4)Source.SampleLevel(LinearSampler, PanoramaTexCoords, 0);
}
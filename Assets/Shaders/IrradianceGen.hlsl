#include "PBRHelpers.hlsli"

#define NUM_THREADS (16)

TextureCube<float4> EnvironmentMap     : register(t0);
SamplerState        EnvironmentSampler : register(s0);

RWTexture2DArray<float4> IrradianceMap : register(u0);

// Transform from dispatch ID to cubemap face direction
static const float3x3 RotateUV[6] =
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
    
    // TODO: Put this in shaderconstants
    uint Width;
    uint Height;
    uint Elements;
    IrradianceMap.GetDimensions(Width, Height, Elements);
    
    float3 Normal = float3((TexCoord.xy / float(Width)) - 0.5, 0.5);
    Normal        = normalize(mul(RotateUV[TexCoord.z], Normal));
    
    float3 Up    = float3(0.0, 1.0, 0.0);
    float3 Right = cross(Up, Normal);
    Up           = cross(Normal, Right);

    float  SampleDelta = 0.025;
    float  NrSamples   = 0.0;
    float3 Irradiance  = float3(0.0, 0.0, 0.0);
    for (float Phi = 0.0; Phi < 2.0 * PI; Phi += SampleDelta)
    {
        for (float Theta = 0.0; Theta < 0.5 * PI; Theta += SampleDelta)
        {
            float3 TangentSample = float3(sin(Theta) * cos(Phi), sin(Theta) * sin(Phi), cos(Theta));
            float3 SampleVec     = TangentSample.x * Right + TangentSample.y * Up + TangentSample.z * Normal;

            Irradiance += EnvironmentMap.SampleLevel(EnvironmentSampler, SampleVec, 0).rgb * cos(Theta) * sin(Theta);
            NrSamples  += 1.0;
        }
    }
    
    Irradiance = PI * Irradiance * (1.0 / float(NrSamples));
    IrradianceMap[TexCoord] = float4(Irradiance, 1.0);
}
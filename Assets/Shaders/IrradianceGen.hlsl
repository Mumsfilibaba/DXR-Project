#include "PBRHelpers.hlsli"

#define BLOCK_SIZE 16

#define RootSig \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 1), " \
    "DescriptorTable(SRV(t0, numDescriptors = 1))," \
    "DescriptorTable(UAV(u0, numDescriptors = 1))," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_WRAP," \
        "addressV = TEXTURE_ADDRESS_WRAP," \
        "addressW = TEXTURE_ADDRESS_WRAP," \
        "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT)"

TextureCube<float4> EnvironmentMap     : register(t0, space0);
SamplerState        EnvironmentSampler : register(s0, space0);

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

[RootSignature(RootSig)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void Main(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint3 DispatchThreadID : SV_DispatchThreadID, uint GroupIndex : SV_GroupIndex)
{
    uint3 TexCoord = DispatchThreadID;
    
    // TODO: Put this in shaderconstants
    uint Width;
    uint Height;
    uint Elements;
    IrradianceMap.GetDimensions(Width, Height, Elements);
    
    float3 Normal = float3((TexCoord.xy / float(Width)) - 0.5f, 0.5f);
    Normal        = normalize(mul(RotateUV[TexCoord.z], Normal));
    
    float3 Up    = float3(0.0f, 1.0f, 0.0f);
    float3 Right = cross(Up, Normal);
    Up           = cross(Normal, Right);

    float  SampleDelta = 0.025f;
    float  NrSamples   = 0.0f;
    float3 Irradiance  = float3(0.0f, 0.0f, 0.0f);
    for (float Phi = 0.0f; Phi < 2.0f * PI; Phi += SampleDelta)
    {
        for (float Theta = 0.0f; Theta < 0.5f * PI; Theta += SampleDelta)
        {
            float3 TangentSample = float3(sin(Theta) * cos(Phi), sin(Theta) * sin(Phi), cos(Theta));
            float3 SampleVec     = TangentSample.x * Right + TangentSample.y * Up + TangentSample.z * Normal;

            Irradiance += EnvironmentMap.SampleLevel(EnvironmentSampler, SampleVec, 0).rgb * cos(Theta) * sin(Theta);
            NrSamples  += 1.0f;
        }
    }
    
    Irradiance = PI * Irradiance * (1.0f / float(NrSamples));
    IrradianceMap[TexCoord] = float4(Irradiance, 1.0f);
}
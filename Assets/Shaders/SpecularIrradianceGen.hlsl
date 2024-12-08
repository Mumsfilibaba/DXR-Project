#include "PBRHelpers.hlsli"
#include "Constants.hlsli"

#define NUM_THREADS (16)

SHADER_CONSTANT_BLOCK_BEGIN
    float CurrentRoughness;
    uint  SourceFaceResolution;
    uint  CurrentFaceResolution;
SHADER_CONSTANT_BLOCK_END

TextureCube<float4> EnvironmentMap     : register(t0);
SamplerState        EnvironmentSampler : register(s0);

RWTexture2DArray<float4> SpecularIrradianceMap : register(u0);

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
           
    float3 Normal = float3((TexCoord.xy / float(Constants.CurrentFaceResolution)) - 0.5f, 0.5f);
    Normal = normalize(mul(RotateUV[TexCoord.z], Normal));
    
    // Make the assumption that V equals R equals the normal (A.k.a viewangle is zero)
    float3 R = Normal;
    float3 V = R;

    float  FinalRoughness   = min(max(Constants.CurrentRoughness, MIN_ROUGHNESS), MAX_ROUGHNESS);
    float  TotalWeight      = 0.0f;
    float3 PrefilteredColor = float3(0.0f, 0.0f, 0.0f);
    
    const uint SAMPLE_COUNT = 512U;
    for (uint i = 0U; i < SAMPLE_COUNT; i++)
    {
        // Generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
        float2 Xi = Hammersley2(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, FinalRoughness, Normal);
        float3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(Normal, L), 0.0f);
        if (NdotL > 0.0f)
        {
            // Sample from the environment's mip level based on roughness/pdf
            float D     = DistributionGGX(Normal, H, FinalRoughness);
            float NdotH = max(dot(Normal, H), 0.0f);
            float HdotV = max(dot(H, V), 0.0f);
            float PDF   = D * NdotH / (4.0f * HdotV) + 0.0001f;

            float Resolution = float(Constants.SourceFaceResolution); // Resolution of source cubemap (per face)
            float SaTexel    = 4.0f * PI / (6.0f * Resolution * Resolution);
            float SaSample   = 1.0f / (float(SAMPLE_COUNT) * PDF + 0.0001f);

            const float Miplevel = FinalRoughness == 0.0f ? 0.0f : 0.5f * log2(SaSample / SaTexel);         
            PrefilteredColor += EnvironmentMap.SampleLevel(EnvironmentSampler, L, Miplevel).rgb * NdotL;
            TotalWeight += NdotL;
        }
    }

    PrefilteredColor = PrefilteredColor / TotalWeight;
    SpecularIrradianceMap[TexCoord] = float4(PrefilteredColor, 1.0f);
}
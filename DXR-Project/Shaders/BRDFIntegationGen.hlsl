#include "PBRCommon.hlsli"

#define RootSig \
    "RootFlags(0), " \
    "DescriptorTable(UAV(u0, numDescriptors = 1))" \

RWTexture2D<float2> IntegrationMap : register(u0, space0);

float RadicalInverse_VdC(uint Bits)
{
    Bits = (Bits << 16u) | (Bits >> 16u);
    Bits = ((Bits & 0x55555555u) << 1u) | ((Bits & 0xAAAAAAAAu) >> 1u);
    Bits = ((Bits & 0x33333333u) << 2u) | ((Bits & 0xCCCCCCCCu) >> 2u);
    Bits = ((Bits & 0x0F0F0F0Fu) << 4u) | ((Bits & 0xF0F0F0F0u) >> 4u);
    Bits = ((Bits & 0x00FF00FFu) << 8u) | ((Bits & 0xFF00FF00u) >> 8u);
    return float(Bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float Roughness)
{
    float A = Roughness * Roughness;
	
    float Phi      = 2.0f * PI * Xi.x;
    float CosTheta = sqrt((1.0f - Xi.y) / (1.0f + (A * A - 1.0f) * Xi.y));
    float SinTheta = sqrt(1.0f - CosTheta * CosTheta);
	
    // From spherical coordinates to cartesian coordinates
    float3 H;
    H.x = cos(Phi) * SinTheta;
    H.y = sin(Phi) * SinTheta;
    H.z = CosTheta;
	
    // From tangent-space vector to world-space sample vector
    float3 Up           = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 Tangent      = normalize(cross(Up, N));
    float3 Bitangent    = cross(N, Tangent);
	
    float3 SampleVec = Tangent * H.x + Bitangent * H.y + N * H.z;
    return normalize(SampleVec);
}

float2 IntegrateBRDF(float NdotV, float Roughness)
{ 
    float3 V;
    V.x = sqrt(1.0f - (NdotV * NdotV));
    V.y = 0.0f;
    V.z = NdotV;

    float A = 0.0f;
    float B = 0.0f;

    float3 N = float3(0.0f, 0.0f, 1.0f);

    const uint SAMPLE_COUNT = 1024u;
    for (uint Sample = 0u; Sample < SAMPLE_COUNT; Sample++)
    {
        float2 Xi   = Hammersley(Sample, SAMPLE_COUNT);
        float3 H    = ImportanceSampleGGX(Xi, N, Roughness);
        float3 L    = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0f);
        float NdotH = max(H.z, 0.0f);
        float VdotH = max(dot(V, H), 0.0f);

        if (NdotL > 0.0f)
        {
            float G     = GeometrySmith_IBL(N, V, L, Roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc    = pow(1.0f - VdotH, 5.0f);

            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    
    return float2(A, B) / SAMPLE_COUNT;
}


[RootSignature(RootSig)]
[numthreads(1, 1, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    float OutputWidth;
    float OutputHeight;
    IntegrationMap.GetDimensions(OutputWidth, OutputHeight);
    
    float2 TexCoord = float2(DispatchThreadID.xy) / float2(OutputWidth, OutputHeight);
    
    float NdotV     = max(TexCoord.x, MIN_VALUE);
    float Roughness = min(max(TexCoord.y, MIN_ROUGHNESS), MAX_ROUGHNESS);
    
    float2 IntegratedBDRF = IntegrateBRDF(NdotV, Roughness);
    IntegrationMap[DispatchThreadID.xy] = IntegratedBDRF;
}
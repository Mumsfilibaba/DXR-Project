#include "PBRHelpers.hlsli"
#include "Halton.hlsli"

#define RootSig \
    "RootFlags(0), " \
    "DescriptorTable(UAV(u0, numDescriptors = 1))," \

RWTexture2D<float2> IntegrationMap : register(u0, space0);

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
        float2 Xi = Hammersley2(Sample, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, Roughness, N);
        float3 L  = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = saturate(L.z);
        float NdotH = saturate(H.z);
        float VdotH = saturate(dot(V, H));

        if (NdotL > 0.0f)
        {
            float G     = GeometrySmithGGX_IBL(N, L, V, Roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc    = pow(1.0f - VdotH, 5.0f);

            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    
    return float2(A, B) / SAMPLE_COUNT;
}

[RootSignature(RootSig)]
[numthreads(16, 16, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    float OutputWidth;
    float OutputHeight;
    IntegrationMap.GetDimensions(OutputWidth, OutputHeight);
    
    float2 TexCoord = (float2(DispatchThreadID.xy) + Float2(0.5f)) / float2(OutputWidth, OutputHeight);
    
    float NdotV     = max(TexCoord.x, MIN_VALUE);
    float Roughness = min(max(1.0f - TexCoord.y, MIN_ROUGHNESS), MAX_ROUGHNESS);
    
    float2 IntegratedBDRF = IntegrateBRDF(NdotV, Roughness);
    IntegrationMap[DispatchThreadID.xy] = IntegratedBDRF;
}
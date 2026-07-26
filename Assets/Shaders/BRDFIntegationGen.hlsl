#include "PBRHelpers.hlsli"

#ifndef NUM_THREADS
    #define NUM_THREADS 16
#endif

TEXTURE_FORMAT_UNKNOWN RWTexture2D<float2> IntegrationMap : register(u0);

float2 IntegrateBRDF(float NdotV, float Roughness)
{ 
    float3 V;
    V.x = sqrt(1.0 - (NdotV * NdotV));
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    float3 N = float3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for (uint Sample = 0u; Sample < SAMPLE_COUNT; Sample++)
    {
        float2 Xi = Hammersley2(Sample, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, Roughness, N);
        float3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G     = GeometrySmithGGX_IBL(N, L, V, Roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc    = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    
    return float2(A, B) / SAMPLE_COUNT;
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    float OutputWidth;
    float OutputHeight;
    IntegrationMap.GetDimensions(OutputWidth, OutputHeight);
    
    float2 TexCoord  = (float2(DispatchThreadID.xy) + 0.5) / float2(OutputWidth, OutputHeight);
    float  NdotV     = max(TexCoord.x, MIN_VALUE);
    float  Roughness = min(max(1.0 - TexCoord.y, MIN_ROUGHNESS), MAX_ROUGHNESS);
    
    const float2 IntegratedBDRF = IntegrateBRDF(NdotV, Roughness);
    IntegrationMap[DispatchThreadID.xy] = IntegratedBDRF;
}
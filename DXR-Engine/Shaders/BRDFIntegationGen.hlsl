#include "PBRCommon.hlsli"

/*
* TODO: Support rootsignatures that does not need to support our default rootsignature,
* since this sort of removes the need for the custom rootsignature support
*/

#define RootSig \
	"RootFlags(0), " \
	"RootConstants(b0, num32BitConstants = 1), " \
	"DescriptorTable(CBV(b1, numDescriptors = 1))," \
	"DescriptorTable(SRV(t0, numDescriptors = 1))," \
	"DescriptorTable(UAV(u0, numDescriptors = 1))," \
	"DescriptorTable(Sampler(s1, numDescriptors = 1))," \

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
		float2 Xi	= Hammersley(Sample, SAMPLE_COUNT);
		float3 H	= ImportanceSampleGGX(Xi, N, Roughness);
		float3 L	= normalize(2.0f * dot(V, H) * H - V);

		float NdotL = max(L.z, 0.0f);
		float NdotH = max(H.z, 0.0f);
		float VdotH = max(dot(V, H), 0.0f);

		if (NdotL > 0.0f)
		{
			float G		= GeometrySmith_IBL(N, V, L, Roughness);
			float G_Vis	= (G * VdotH) / (NdotH * NdotV);
			float Fc	= pow(1.0f - VdotH, 5.0f);

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
	
	float2 TexCoord = (float2(DispatchThreadID.xy) + ToFloat2(0.5f)) / float2(OutputWidth, OutputHeight);
	
	float NdotV		= max(TexCoord.x, MIN_VALUE);
	float Roughness	= min(max(1.0 - TexCoord.y, MIN_ROUGHNESS), MAX_ROUGHNESS);
	
	float2 IntegratedBDRF = IntegrateBRDF(NdotV, Roughness);
	IntegrationMap[DispatchThreadID.xy] = IntegratedBDRF;
}
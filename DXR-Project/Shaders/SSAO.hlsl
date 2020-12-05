#include "PBRCommon.hlsli"

Texture2D<float3>	GBufferNormals	: register(t0, space0);
Texture2D<float>	GBufferDepth	: register(t1, space0);
Texture2D<float3>	Noise			: register(t2, space0);

StructuredBuffer<float3> Samples : register(t3, space0);

SamplerState GBufferSampler	: register(s0, space0);
SamplerState NoiseSampler	: register(s1, space0);

RWTexture2D<float> Output : register(u0, space0);

cbuffer Params : register(b0, space0)
{
	float4x4 ProjectionInv;
	float4x4 Projection;
	float4x4 ViewInvTranspose;
	float2	ScreenSize;
	float2	NoiseSize;
	float	Radius;
};

#define KERNEL_SIZE 64

[numthreads(16, 16, 1)]
void Main(ComputeShaderInput Input)
{
	const uint2 OutputTexCoords = Input.DispatchThreadID.xy;
	if (Input.DispatchThreadID.x > uint(ScreenSize.x) || Input.DispatchThreadID.y > uint(ScreenSize.y))
	{
		return;
	}
	
	const float2 TexCoords	= float2(OutputTexCoords) / ScreenSize;
	const float  Depth0		= GBufferDepth.SampleLevel(GBufferSampler, TexCoords, 0);
    float3 ViewPosition		= WorldPositionFromDepth(Depth0, TexCoords, ProjectionInv);
	
	float3 WorldNormal	= GBufferNormals.SampleLevel(GBufferSampler, TexCoords, 0).rgb;
	WorldNormal			= UnpackNormal(WorldNormal);
    float3 ViewNormal = normalize(mul(float4(WorldNormal, 0.0f), ViewInvTranspose).xyz);
    ViewNormal.z = -ViewNormal.z;
	
	const float2 NoiseScale = ScreenSize / NoiseSize;
	const float3 NoiseVec	= normalize(Noise.SampleLevel(NoiseSampler, TexCoords * NoiseScale, 0));
	const float3 Tangent	= normalize(NoiseVec - ViewNormal * dot(NoiseVec, ViewNormal));
	const float3 Bitangent	= cross(Tangent, ViewNormal);
	float3x3 TBN = float3x3(Tangent, Bitangent, ViewNormal);
	
	float Occlusion = 0.0f;
	for (int i = 0; i < KERNEL_SIZE; i++)
	{
		const float3 Sample = Samples[i];
		float3 SamplePos = mul(Sample, TBN);
		SamplePos = ViewPosition + (SamplePos * Radius);
			
		float4 Offset = mul(float4(SamplePos, 1.0f), Projection);
		Offset.xyz	= Offset.xyz / Offset.w;
		Offset.xy	= (Offset.xy * float2(0.5f, -0.5f)) + 0.5f;
		
		float Depth1	= GBufferDepth.SampleLevel(GBufferSampler, Offset.xy, 0);
        float3 DepthPos = WorldPositionFromDepth(Depth1, Offset.xy, ProjectionInv);
        Depth1 = DepthPos.z;
		
        const float RangeCheck = smoothstep(0.0f, 1.0f, Radius / abs(ViewPosition.z - Depth1));
        Occlusion += (Depth1 >= SamplePos.z ? 0.0f : 1.0f) * RangeCheck;
    }	
	
	Occlusion = 1.0f - (Occlusion / float(KERNEL_SIZE));
	Occlusion = Occlusion * Occlusion;
	Output[OutputTexCoords] = Occlusion;
}
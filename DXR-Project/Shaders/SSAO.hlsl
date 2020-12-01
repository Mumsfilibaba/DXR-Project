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
	float2	ScreenSize;
	float2	NoiseSize;
	float	Radius;
};

[numthreads(16, 16, 1)]
void Main(ComputeShaderInput Input)
{
	const uint2 OutputTexCoords = Input.DispatchThreadID.xy;
	if (Input.DispatchThreadID.x > uint(ScreenSize.x) || Input.DispatchThreadID.y > uint(ScreenSize.y))
	{
		return;
	}
	
	const float2 TexCoords = float2(OutputTexCoords) / ScreenSize;
	
	const float Depth		= GBufferDepth.SampleLevel(GBufferSampler, TexCoords, 0);
	const float3 Position	= ViewPositionFromDepth(Depth, TexCoords, ProjectionInv);
	const float3 Normal		= normalize(GBufferNormals.SampleLevel(GBufferSampler, TexCoords, 0));
	
	const float2 NoiseScale = ScreenSize / NoiseSize;
	float3 NoiseVec	= Noise.SampleLevel(NoiseSampler, TexCoords * NoiseScale, 0);
	NoiseVec = NoiseVec * 2.0f - 1.0f;
	
	const float3 Tangent	= normalize(NoiseVec - Normal * dot(NoiseVec, Normal));
	const float3 Bitangent	= cross(Normal, Tangent);
	float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
	
	float Occlusion = 0.0f;
	for (int i = 0; i < 32; i++)
	{
		float3 SamplePos = normalize(mul(Samples[i], TBN));
		SamplePos = Position + (SamplePos * Radius);
			
		float4 Offset = float4(SamplePos, 1.0f);
		Offset		= mul(Offset, Projection);
		Offset.xy	= Offset.xy / Offset.w;
		Offset.xy	= (Offset.xy * float2(0.5f, 0.5)) + 0.5f;
		
		float Depth0 = GBufferDepth.SampleLevel(GBufferSampler, Offset.xy, 0);
		Depth0 = ViewPositionFromDepth(Depth0, Offset.xy, ProjectionInv).z;
		
		const float RangeCheck = smoothstep(0.0f, 1.0f, Radius / abs(Depth0 - Position.z));
		Occlusion += (Depth0 < SamplePos.z ? 1.0f : 0.0f) * RangeCheck;
	}	
	
	Output[OutputTexCoords] = 1.0f - (Occlusion / 64.0f);
}
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
	float4x4 ViewProjectionInv;
	float4x4 Projection;
	float2	ScreenSize;
	float2	NoiseSize;
	float	Radius;
};

[numthreads(1, 1, 1)]
void Main(ComputeShaderInput Input)
{
	const uint2 OutputTexCoords = Input.DispatchThreadID.xy;
	const float2 TexCoords = float2(OutputTexCoords) / ScreenSize;
	
	const float Depth		= GBufferDepth.SampleLevel(GBufferSampler, TexCoords, 0);
	const float3 Position	= PositionFromDepth(Depth, TexCoords, ViewProjectionInv);
	const float3 Normal		= GBufferNormals.SampleLevel(GBufferSampler, TexCoords, 0);
	
	const float2 NoiseScale = ScreenSize / NoiseSize;
	float3 NoiseVec	= Noise.SampleLevel(NoiseSampler, TexCoords * NoiseScale, 0);
	NoiseVec = NoiseVec * 2.0f - 1.0f;
	
	const float3 Tangent	= normalize(NoiseVec - Normal * dot(NoiseVec, Normal));
	const float3 Bitangent	= cross(Normal, Tangent);
	float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
	
	const float Bias	= 0.025f;
	float Occlusion		= 0.0f;
	for (int i = 0; i < 64; i++)
	{
        float3 SamplePos = normalize(mul(Samples[i], TBN));
		SamplePos = Position + (SamplePos * Radius);
		
		float4 Offset = float4(SamplePos, 1.0f);
        Offset		= mul(Projection, Offset);
		Offset.xyz	= Offset.xyz / Offset.w;
		Offset.xy	= Offset.xy * 0.5f + 0.5f;
		
		float Depth0 = GBufferDepth.SampleLevel(GBufferSampler, Offset.xy, 0);
		Depth0 = PositionFromDepth(Depth0, Offset.xy, ViewProjectionInv).z;
		
		const float RangeCheck	= abs(Position.z - Depth0) < Radius ? 1.0f : 0.0f;
		Occlusion += ((Depth0 >= (SamplePos.z + Bias)) ? 1.0f : 0.0f) * RangeCheck;
	}	
	
	Output[OutputTexCoords] = 1.0f - (Occlusion / 64.0f);
}
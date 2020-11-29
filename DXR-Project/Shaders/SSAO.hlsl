#include "PBRCommon.hlsli"

Texture2D<float4> GBufferNormals	: register(t0, space0);
Texture2D<float4> GBufferDepth		: register(t1, space0);
Texture2D<float4> Noise				: register(t2, space0);

StructuredBuffer<float3> Samples : register(t4, space0);

SamplerState GBufferSampler	: register(s0, space0);
SamplerState NoiseSampler	: register(s1, space0);

RWTexture2D<float4> Output : register(u0, space0);

[numthreads(1, 1, 1)]
void Main(ComputeShaderInput Input)
{
	
}
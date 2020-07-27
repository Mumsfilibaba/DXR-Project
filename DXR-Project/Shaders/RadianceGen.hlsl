#include "PBRCommon.hlsli"

#define BLOCK_SIZE 1

#define RootSig \
	"RootFlags(0), " \
	"DescriptorTable(SRV(t0, numDescriptors = 1))," \
	"DescriptorTable(UAV(u0, numDescriptors = 1))," \
	"StaticSampler(s0," \
		"addressU = TEXTURE_ADDRESS_WRAP," \
		"addressV = TEXTURE_ADDRESS_WRAP," \
		"addressW = TEXTURE_ADDRESS_WRAP," \
		"filter = FILTER_MIN_MAG_LINEAR_MIP_POINT)"

TextureCube<float4> EnvironmentMap : register(t0, space0);
SamplerState EnvironmentSampler : register(s0, space0);

RWTexture2DArray<float4> IrradianceMap : register(u0);

// Transform from dispatch ID to cubemap face direction
static const float3x3 RotateUV[6] =
{
	// +X
    float3x3(	 0,  0, 1,
				 0, -1, 0,
				-1,  0, 0),
	// -X
	float3x3(	0,  0, -1,
				0, -1,  0,
				1,  0,  0),
	// +Y
	float3x3(	1, 0, 0,
				0, 0, 1,
				0, 1, 0),
	// -Y
	float3x3(	1,  0,  0,
				0,  0, -1,
				0, -1,  0),
	// +Z
	float3x3(	1,  0, 0,
				0, -1, 0,
				0,  0, 1),
	// -Z
	float3x3(	-1,  0,  0,
				 0, -1,  0,
				 0,  0, -1)
};

[RootSignature(RootSig)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void Main(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint3 DispatchThreadID : SV_DispatchThreadID, uint GroupIndex : SV_GroupIndex)
{
}
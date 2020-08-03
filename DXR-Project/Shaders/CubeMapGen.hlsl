#define BLOCK_SIZE 16

#define RootSig \
	"RootFlags(0), " \
	"RootConstants(b0, num32BitConstants = 1), " \
	"DescriptorTable(SRV(t0, numDescriptors = 1))," \
	"DescriptorTable(UAV(u0, numDescriptors = 1))," \
	"StaticSampler(s0," \
		"addressU = TEXTURE_ADDRESS_WRAP," \
		"addressV = TEXTURE_ADDRESS_WRAP," \
		"addressW = TEXTURE_ADDRESS_WRAP," \
		"filter = FILTER_MIN_MAG_LINEAR_MIP_POINT)"

// Properties
cbuffer CB0 : register(b0)
{
	uint CubeMapSize; // Size of one side of the TextureCube
}

SamplerState LinearSampler : register(s0);

// Textures
Texture2D<float4>			Source	: register(t0);
RWTexture2DArray<float4>	OutCube	: register(u0);

// Constants
static const float2 InvAtan = float2(0.1591f, 0.3183f);

// Transform from dispatch ID to cubemap face direction
static const float3x3 RotateUV[6] = 
{
	// +X
	float3x3(  0,  0,  1,
			   0, -1,  0,
			  -1,  0,  0 ),
	// -X
	float3x3(  0,  0, -1,
			   0, -1,  0,
			   1,  0,  0 ),
	// +Y
	float3x3(  1,  0,  0,
			   0,  0,  1,
			   0,  1,  0 ),
	// -Y
	float3x3(  1,  0,  0,
			   0,  0, -1,
			   0, -1,  0 ),
	// +Z
	float3x3(  1,  0,  0,
			   0, -1,  0,
			   0,  0,  1 ),
	// -Z
	float3x3( -1,  0,  0,
			   0, -1,  0,
			   0,  0, -1 )
};

[RootSignature(RootSig)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void Main(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint3 DispatchThreadID : SV_DispatchThreadID, uint GroupIndex : SV_GroupIndex)
{
	uint3 TexCoord = DispatchThreadID;

	// Map the UV coords of the cubemap face to a direction
	// [(0, 0), (1, 1)] => [(-0.5, -0.5), (0.5, 0.5)]
	float3 Direction = float3((TexCoord.xy / float(CubeMapSize)) - 0.5f, 0.5f);

	// Rotate to cubemap face
	Direction = normalize(mul(RotateUV[TexCoord.z], Direction));

	// Convert the world space direction into U,V texture coordinates in the panoramic texture.
	// Source: http://gl.ict.usc.edu/Data/HighResProbes/
	float2 PanoramaTexCoords = float2(atan2(-Direction.x, -Direction.z), acos(Direction.y)) * InvAtan;

	OutCube[TexCoord] = Source.SampleLevel(LinearSampler, PanoramaTexCoords, 0);
}
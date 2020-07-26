/*
* Based on GenerateMipsCS.hlsli by Microsoft
* https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/GenerateMipsCS.hlsli
*/

#define BLOCK_SIZE          8
#define CHANNEL_COMPONENTS  64
#define POWER_OF_TWO        1

#define RootSig \
	"RootFlags(0), " \
	"RootConstants(b0, num32BitConstants = 4), " \
	"DescriptorTable(SRV(t0, numDescriptors = 1))," \
	"DescriptorTable(UAV(u0, numDescriptors = 4))," \
	"StaticSampler(s0," \
		"addressU = TEXTURE_ADDRESS_CLAMP," \
		"addressV = TEXTURE_ADDRESS_CLAMP," \
		"addressW = TEXTURE_ADDRESS_CLAMP," \
		"filter = FILTER_MIN_MAG_MIP_LINEAR)"

// Input
Texture2D<float4> SourceMip : register(t0);

// Output
RWTexture2D<float4> OutMip1 : register(u0);
RWTexture2D<float4> OutMip2 : register(u1);
RWTexture2D<float4> OutMip3 : register(u2);
RWTexture2D<float4> OutMip4 : register(u3);

// Linear sampler
SamplerState LinearSampler : register(s0);

// Properties
cbuffer CB0 : register(b0)
{
	uint    SrcMipLevel;    // Texture level of source mip
	uint    NumMipLevels;   // Number of OutMips to write: [1, 4]
	float2  TexelSize;      // 1.0 / OutMip1.Dimensions
}

// The reason for separating channels is to reduce bank conflicts in the
// local data memory controller. A large stride will cause more threads
// to collide on the same memory bank.
groupshared float RedChannel[CHANNEL_COMPONENTS];
groupshared float GreenChannel[CHANNEL_COMPONENTS];
groupshared float BlueChannel[CHANNEL_COMPONENTS];
groupshared float AlphaChannel[CHANNEL_COMPONENTS];

void StoreColor(uint Index, float4 Color)
{
	RedChannel[Index]   = Color.r;
	GreenChannel[Index] = Color.g;
	BlueChannel[Index]  = Color.b;
	AlphaChannel[Index] = Color.a;
}

float4 LoadColor(uint Index)
{
	return float4(RedChannel[Index], GreenChannel[Index], BlueChannel[Index], AlphaChannel[Index]);
}

float3 ApplySRGBCurve(float3 x)
{
	// This is exactly the sRGB curve
	//return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;

	// This is cheaper but nearly equivalent
	return x < 0.0031308f ? 12.92f * x : 1.13005f * sqrt(abs(x - 0.00228f)) - 0.13448f * x + 0.005719f;
}

float4 PackColor(float4 Linear)
{
#ifdef CONVERT_TO_SRGB
	return float4(ApplySRGBCurve(Linear.rgb), Linear.a);
#else
	return Linear;
#endif
}

[RootSignature(RootSig)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint3 DispatchThreadID : SV_DispatchThreadID, uint GroupIndex : SV_GroupIndex)
{
	// One bilinear sample is insufficient when scaling down by more than 2x.
	// You will slightly undersample in the case where the source dimension
	// is odd.  This is why it's a really good idea to only generate mips on
	// power-of-two sized textures.  Trying to handle the undersampling case
	// will force this shader to be slower and more complicated as it will
	// have to take more source texture samples.
#if POWER_OF_TWO
	float2 TexCoord = TexelSize * (DispatchThreadID.xy + 0.5f);
	float4 Src1     = SourceMip.SampleLevel(LinearSampler, TexCoord, SrcMipLevel);
#else
	#error "Not supported yet"
#endif

	OutMip1[DispatchThreadID.xy] = PackColor(Src1);

	// A scalar (constant) branch can exit all threads coherently.
	if (NumMipLevels == 1)
	{
		return;
	}

	// Without lane swizzle operations, the only way to share data with other
	// threads is through LDS.
	StoreColor(GroupIndex, Src1);

	// This guarantees all LDS writes are complete and that all threads have
	// executed all instructions so far (and therefore have issued their LDS
	// write instructions.)
	GroupMemoryBarrierWithGroupSync();

	// With low three bits for X and high three bits for Y, this bit mask
	// (binary: 001001) checks that X and Y are even.
	if ((GroupIndex & 0x9) == 0)
	{
		float4 Src2 = LoadColor(GroupIndex + 0x01);
		float4 Src3 = LoadColor(GroupIndex + 0x08);
		float4 Src4 = LoadColor(GroupIndex + 0x09);
		Src1 = 0.25f * (Src1 + Src2 + Src3 + Src4);

		OutMip2[DispatchThreadID.xy / 2] = PackColor(Src1);
		StoreColor(GroupIndex, Src1);
	}

	if (NumMipLevels == 2)
	{
		return;
	}

	GroupMemoryBarrierWithGroupSync();

	// This bit mask (binary: 011011) checks that X and Y are multiples of four.
	if ((GroupIndex & 0x1B) == 0)
	{
		float4 Src2 = LoadColor(GroupIndex + 0x02);
		float4 Src3 = LoadColor(GroupIndex + 0x10);
		float4 Src4 = LoadColor(GroupIndex + 0x12);
		Src1 = 0.25f * (Src1 + Src2 + Src3 + Src4);

		OutMip3[DispatchThreadID.xy / 4] = PackColor(Src1);
		StoreColor(GroupIndex, Src1);
	}

	if (NumMipLevels == 3)
	{
		return;
	}

	GroupMemoryBarrierWithGroupSync();

	// This bit mask would be 111111 (X & Y multiples of 8), but only one
	// thread fits that criteria.
	if (GroupIndex == 0)
	{
		float4 Src2 = LoadColor(GroupIndex + 0x04);
		float4 Src3 = LoadColor(GroupIndex + 0x20);
		float4 Src4 = LoadColor(GroupIndex + 0x24);
		Src1 = 0.25f * (Src1 + Src2 + Src3 + Src4);

		OutMip4[DispatchThreadID.xy / BLOCK_SIZE] = PackColor(Src1);
	}
}

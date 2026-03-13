#ifndef BLOCK_COMPRESSION_SHARED_HLSLI
#define BLOCK_COMPRESSION_SHARED_HLSLI

#include "../CoreDefines.hlsli"

#ifndef NUM_THREADS
	#define NUM_THREADS (8)
#endif

#define BC_BLOCK_TEXELS (16)

SamplerState BlockSampler : register(s0);

SHADER_CONSTANT_BLOCK_BEGIN
	uint2  TextureSizeInBlocks;
	float2 TextureSizeRcp;
SHADER_CONSTANT_BLOCK_END

// ============================================================================
// Texel Gathering
// ============================================================================

struct FGatherUVs
{
	float2 UV0;
	float2 UV1;
	float2 UV2;
	float2 UV3;
};

FGatherUVs ComputeGatherUVs(uint2 BlockCoord)
{
	const float2 TexCoord = BlockCoord * Constants.TextureSizeRcp * 4.0 + Constants.TextureSizeRcp;

	FGatherUVs Result;
	Result.UV0 = TexCoord;
	Result.UV1 = TexCoord + float2(2.0 * Constants.TextureSizeRcp.x, 0.0);
	Result.UV2 = TexCoord + float2(0.0, 2.0 * Constants.TextureSizeRcp.y);
	Result.UV3 = TexCoord + float2(2.0 * Constants.TextureSizeRcp.x, 2.0 * Constants.TextureSizeRcp.y);
	return Result;
}

void GatherChannel(Texture2D<float4> Tex, FGatherUVs UVs, uint ChannelMask, out float Values[16])
{
	float4 B0, B1, B2, B3;

	if (ChannelMask == 0)
	{
		B0 = Tex.GatherRed(BlockSampler, UVs.UV0);
		B1 = Tex.GatherRed(BlockSampler, UVs.UV1);
		B2 = Tex.GatherRed(BlockSampler, UVs.UV2);
		B3 = Tex.GatherRed(BlockSampler, UVs.UV3);
	}
	else if (ChannelMask == 1)
	{
		B0 = Tex.GatherGreen(BlockSampler, UVs.UV0);
		B1 = Tex.GatherGreen(BlockSampler, UVs.UV1);
		B2 = Tex.GatherGreen(BlockSampler, UVs.UV2);
		B3 = Tex.GatherGreen(BlockSampler, UVs.UV3);
	}
	else if (ChannelMask == 2)
	{
		B0 = Tex.GatherBlue(BlockSampler, UVs.UV0);
		B1 = Tex.GatherBlue(BlockSampler, UVs.UV1);
		B2 = Tex.GatherBlue(BlockSampler, UVs.UV2);
		B3 = Tex.GatherBlue(BlockSampler, UVs.UV3);
	}
	else
	{
		B0 = Tex.GatherAlpha(BlockSampler, UVs.UV0);
		B1 = Tex.GatherAlpha(BlockSampler, UVs.UV1);
		B2 = Tex.GatherAlpha(BlockSampler, UVs.UV2);
		B3 = Tex.GatherAlpha(BlockSampler, UVs.UV3);
	}

	Values[0]  = B0.w; Values[1]  = B0.z;
	Values[2]  = B1.w; Values[3]  = B1.z;
	Values[4]  = B0.x; Values[5]  = B0.y;
	Values[6]  = B1.x; Values[7]  = B1.y;
	Values[8]  = B2.w; Values[9]  = B2.z;
	Values[10] = B3.w; Values[11] = B3.z;
	Values[12] = B2.x; Values[13] = B2.y;
	Values[14] = B3.x; Values[15] = B3.y;
}

void GatherRGB(Texture2D<float4> Tex, FGatherUVs UVs, out float3 Texels[16])
{
	float4 B0R = Tex.GatherRed(BlockSampler, UVs.UV0);
	float4 B1R = Tex.GatherRed(BlockSampler, UVs.UV1);
	float4 B2R = Tex.GatherRed(BlockSampler, UVs.UV2);
	float4 B3R = Tex.GatherRed(BlockSampler, UVs.UV3);

	float4 B0G = Tex.GatherGreen(BlockSampler, UVs.UV0);
	float4 B1G = Tex.GatherGreen(BlockSampler, UVs.UV1);
	float4 B2G = Tex.GatherGreen(BlockSampler, UVs.UV2);
	float4 B3G = Tex.GatherGreen(BlockSampler, UVs.UV3);

	float4 B0B = Tex.GatherBlue(BlockSampler, UVs.UV0);
	float4 B1B = Tex.GatherBlue(BlockSampler, UVs.UV1);
	float4 B2B = Tex.GatherBlue(BlockSampler, UVs.UV2);
	float4 B3B = Tex.GatherBlue(BlockSampler, UVs.UV3);

	Texels[0]  = float3(B0R.w, B0G.w, B0B.w);
	Texels[1]  = float3(B0R.z, B0G.z, B0B.z);
	Texels[2]  = float3(B1R.w, B1G.w, B1B.w);
	Texels[3]  = float3(B1R.z, B1G.z, B1B.z);
	Texels[4]  = float3(B0R.x, B0G.x, B0B.x);
	Texels[5]  = float3(B0R.y, B0G.y, B0B.y);
	Texels[6]  = float3(B1R.x, B1G.x, B1B.x);
	Texels[7]  = float3(B1R.y, B1G.y, B1B.y);
	Texels[8]  = float3(B2R.w, B2G.w, B2B.w);
	Texels[9]  = float3(B2R.z, B2G.z, B2B.z);
	Texels[10] = float3(B3R.w, B3G.w, B3B.w);
	Texels[11] = float3(B3R.z, B3G.z, B3B.z);
	Texels[12] = float3(B2R.x, B2G.x, B2B.x);
	Texels[13] = float3(B2R.y, B2G.y, B2B.y);
	Texels[14] = float3(B3R.x, B3G.x, B3B.x);
	Texels[15] = float3(B3R.y, B3G.y, B3B.y);
}

// ============================================================================
// BC4: Single-channel block encoding (64 bits)
//
// Layout: [endpoint0:8][endpoint1:8][16 x 3-bit indices : 48 bits]
// When endpoint0 > endpoint1: 8-value palette (6 interpolated + 2 endpoints)
// When endpoint0 <= endpoint1: 6-value palette (4 interpolated + 0.0 + 1.0)
// We always use the 8-value mode for best quality on continuous data.
// ============================================================================

uint2 EncodeBC4Block(float Texels[16])
{
	float BlockMin = Texels[0];
	float BlockMax = Texels[0];

	[unroll]
	for (uint i = 1; i < 16; i++)
	{
		BlockMin = min(BlockMin, Texels[i]);
		BlockMax = max(BlockMax, Texels[i]);
	}

	uint Endpoint0 = uint(round(BlockMax * 255.0));
	uint Endpoint1 = uint(round(BlockMin * 255.0));

	if (Endpoint0 == Endpoint1)
	{
		// All texels same value, indices all zero
		return uint2(Endpoint0 | (Endpoint1 << 8), 0);
	}

	// Ensure 8-value mode: endpoint0 > endpoint1
	if (Endpoint0 < Endpoint1)
	{
		uint Temp = Endpoint0;
		Endpoint0 = Endpoint1;
		Endpoint1 = Temp;

		float TempF = BlockMax;
		BlockMax = BlockMin;
		BlockMin = TempF;
	}

	// Compute palette (8 values)
	float Palette[8];
	Palette[0] = BlockMax;
	Palette[1] = BlockMin;
	Palette[2] = (6.0 * BlockMax + 1.0 * BlockMin) / 7.0;
	Palette[3] = (5.0 * BlockMax + 2.0 * BlockMin) / 7.0;
	Palette[4] = (4.0 * BlockMax + 3.0 * BlockMin) / 7.0;
	Palette[5] = (3.0 * BlockMax + 4.0 * BlockMin) / 7.0;
	Palette[6] = (2.0 * BlockMax + 5.0 * BlockMin) / 7.0;
	Palette[7] = (1.0 * BlockMax + 6.0 * BlockMin) / 7.0;

	// Find best index for each texel
	uint Indices[16];

	[unroll]
	for (uint i = 0; i < 16; i++)
	{
		float BestDist = abs(Texels[i] - Palette[0]);
		Indices[i] = 0;

		[unroll]
		for (uint j = 1; j < 8; j++)
		{
			float Dist = abs(Texels[i] - Palette[j]);
			if (Dist < BestDist)
			{
				BestDist = Dist;
				Indices[i] = j;
			}
		}
	}

	// Pack into uint2 (64 bits)
	// .x: endpoint0[7:0] | endpoint1[15:8] | indices[0..4] in bits 16-30, low bit of index[5] at bit 31
	// .y: remaining indices
	uint2 Block;
	Block.x = Endpoint0 | (Endpoint1 << 8);
	Block.x |= (Indices[0]  << 16);
	Block.x |= (Indices[1]  << 19);
	Block.x |= (Indices[2]  << 22);
	Block.x |= (Indices[3]  << 25);
	Block.x |= (Indices[4]  << 28);
	Block.x |= ((Indices[5] & 0x1) << 31);

	Block.y  = (Indices[5]  >> 1);
	Block.y |= (Indices[6]  << 2);
	Block.y |= (Indices[7]  << 5);
	Block.y |= (Indices[8]  << 8);
	Block.y |= (Indices[9]  << 11);
	Block.y |= (Indices[10] << 14);
	Block.y |= (Indices[11] << 17);
	Block.y |= (Indices[12] << 20);
	Block.y |= (Indices[13] << 23);
	Block.y |= (Indices[14] << 26);
	Block.y |= (Indices[15] << 29);

	return Block;
}

// ============================================================================
// BC1: RGB block encoding (64 bits)
//
// Layout: [color0:16 R5G6B5][color1:16 R5G6B5][16 x 2-bit indices : 32 bits]
// When color0 > color1: 4-color mode (2 endpoints + 2 interpolated)
// When color0 <= color1: 3-color + transparent mode
// We use 4-color mode (color0 > color1) for opaque compression.
// ============================================================================

uint PackR5G6B5(float3 Color)
{
	uint R = uint(round(saturate(Color.r) * 31.0));
	uint G = uint(round(saturate(Color.g) * 63.0));
	uint B = uint(round(saturate(Color.b) * 31.0));
	return (R << 11) | (G << 5) | B;
}

float3 UnpackR5G6B5(uint Packed)
{
	float R = float((Packed >> 11) & 0x1F) / 31.0;
	float G = float((Packed >> 5)  & 0x3F) / 63.0;
	float B = float(Packed & 0x1F) / 31.0;
	return float3(R, G, B);
}

uint2 EncodeBC1Block(float3 Texels[16])
{
	// Find bounding box
	float3 BlockMin = Texels[0];
	float3 BlockMax = Texels[0];

	[unroll]
	for (uint i = 1; i < 16; i++)
	{
		BlockMin = min(BlockMin, Texels[i]);
		BlockMax = max(BlockMax, Texels[i]);
	}

	// Inset bounding box by 1/16 for better quality
	float3 Inset = (BlockMax - BlockMin) / 16.0;
	BlockMin = saturate(BlockMin + Inset);
	BlockMax = saturate(BlockMax - Inset);

	// Quantize endpoints to R5G6B5
	uint Color0 = PackR5G6B5(BlockMax);
	uint Color1 = PackR5G6B5(BlockMin);

	// Ensure 4-color mode: color0 > color1
	if (Color0 < Color1)
	{
		uint Temp = Color0;
		Color0 = Color1;
		Color1 = Temp;
	}
	else if (Color0 == Color1)
	{
		// All same color, indices all zero
		return uint2(Color0 | (Color1 << 16), 0);
	}

	// Reconstruct actual endpoint colors after quantization
	float3 Ep0 = UnpackR5G6B5(Color0);
	float3 Ep1 = UnpackR5G6B5(Color1);

	// Build 4-color palette
	float3 Palette[4];
	Palette[0] = Ep0;
	Palette[1] = Ep1;
	Palette[2] = (2.0 / 3.0) * Ep0 + (1.0 / 3.0) * Ep1;
	Palette[3] = (1.0 / 3.0) * Ep0 + (2.0 / 3.0) * Ep1;

	// Find best 2-bit index for each texel
	uint IndexBlock = 0;

	[unroll]
	for (uint i = 0; i < 16; i++)
	{
		float3 Diff  = Texels[i] - Palette[0];
		float BestDist = dot(Diff, Diff);
		uint BestIdx = 0;

		[unroll]
		for (uint j = 1; j < 4; j++)
		{
			Diff = Texels[i] - Palette[j];
			float Dist = dot(Diff, Diff);
			if (Dist < BestDist)
			{
				BestDist = Dist;
				BestIdx = j;
			}
		}

		IndexBlock |= (BestIdx << (i * 2));
	}

	return uint2(Color0 | (Color1 << 16), IndexBlock);
}

// ============================================================================
// BC2: Explicit 4-bit alpha encoding (64 bits for alpha portion)
//
// Layout: 16 alpha values x 4 bits = 64 bits = uint2
// ============================================================================

uint2 EncodeBC2AlphaBlock(float Alphas[16])
{
	uint2 Block = uint2(0, 0);

	[unroll]
	for (uint i = 0; i < 8; i++)
	{
		uint A = uint(round(saturate(Alphas[i]) * 15.0));
		Block.x |= (A << (i * 4));
	}

	[unroll]
	for (uint i = 0; i < 8; i++)
	{
		uint A = uint(round(saturate(Alphas[i + 8]) * 15.0));
		Block.y |= (A << (i * 4));
	}

	return Block;
}

#endif

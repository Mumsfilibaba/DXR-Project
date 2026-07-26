#include "BlockCompressionShared.hlsli"

Texture2D<float4>                         SourceTexture : register(t0);
TEXTURE_FORMAT_UNKNOWN RWTexture2D<uint4> OutputTexture : register(u0);

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	uint2 BlockCoord = DispatchThreadID.xy;
	if (all(BlockCoord < Constants.TextureSizeInBlocks))
	{
		FGatherUVs UVs = ComputeGatherUVs(BlockCoord);

		float3 ColorTexels[16];
		GatherRGB(SourceTexture, UVs, ColorTexels);

		float AlphaTexels[16];
		GatherChannel(SourceTexture, UVs, 3, AlphaTexels);

		uint2 AlphaBlock = EncodeBC2AlphaBlock(AlphaTexels);
		uint2 ColorBlock = EncodeBC1Block(ColorTexels);

		// BC2 layout: [alpha:64][color:64]
		OutputTexture[BlockCoord] = uint4(AlphaBlock, ColorBlock);
	}
}

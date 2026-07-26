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

		float RedTexels[16];
		GatherChannel(SourceTexture, UVs, 0, RedTexels);

		float GreenTexels[16];
		GatherChannel(SourceTexture, UVs, 1, GreenTexels);

		uint2 RedBlock   = EncodeBC4Block(RedTexels);
		uint2 GreenBlock = EncodeBC4Block(GreenTexels);

		OutputTexture[BlockCoord] = uint4(RedBlock, GreenBlock);
	}
}

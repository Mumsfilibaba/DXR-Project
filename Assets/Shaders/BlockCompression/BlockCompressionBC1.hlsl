#include "BlockCompressionShared.hlsli"

Texture2D<float4> SourceTexture : register(t0);
TEXTURE_FORMAT_UNKNOWN RWTexture2D<uint2> OutputTexture : register(u0);

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	uint2 BlockCoord = DispatchThreadID.xy;
	if (all(BlockCoord < Constants.TextureSizeInBlocks))
	{
		FGatherUVs UVs = ComputeGatherUVs(BlockCoord);

		float3 Texels[16];
		GatherRGB(SourceTexture, UVs, Texels);

		OutputTexture[BlockCoord] = EncodeBC1Block(Texels);
	}
}

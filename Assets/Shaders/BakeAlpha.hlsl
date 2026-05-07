#include "Structs.hlsli"

SHADER_CONSTANT_BLOCK_BEGIN
	uint2 TextureSize;
SHADER_CONSTANT_BLOCK_END

Texture2D<float4> AlbedoTexture : register(t0);
Texture2D<float>  AlphaTexture  : register(t1);

TEXTURE_FORMAT_UNKNOWN RWTexture2D<float4> OutputAlbedo : register(u0);

[numthreads(8, 8, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	if (DispatchThreadID.x >= Constants.TextureSize.x || DispatchThreadID.y >= Constants.TextureSize.y)
	{
		return;
	}

	const int3 TexelCoord = int3(DispatchThreadID.xy, 0);

	float4 Albedo = AlbedoTexture.Load(TexelCoord);
	float  Alpha  = AlphaTexture.Load(TexelCoord);

	OutputAlbedo[DispatchThreadID.xy] = float4(Albedo.rgb, Alpha);
}

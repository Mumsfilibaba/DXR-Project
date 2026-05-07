#include "Structs.hlsli"

SHADER_CONSTANT_BLOCK_BEGIN
	uint2 TextureSize;
SHADER_CONSTANT_BLOCK_END

Texture2D<float> AOTexture        : register(t0);
Texture2D<float> RoughnessTexture : register(t1);
Texture2D<float> MetallicTexture  : register(t2);

TEXTURE_FORMAT_UNKNOWN RWTexture2D<float4> Output : register(u0);

[numthreads(8, 8, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	if (DispatchThreadID.x >= Constants.TextureSize.x || DispatchThreadID.y >= Constants.TextureSize.y)
	{
		return;
	}

	const int3 TexelCoord = int3(DispatchThreadID.xy, 0);

	const float AO        = AOTexture.Load(TexelCoord);
	const float Roughness = RoughnessTexture.Load(TexelCoord);
	const float Metallic  = MetallicTexture.Load(TexelCoord);

	Output[DispatchThreadID.xy] = float4(AO, Roughness, Metallic, 1.0);
}

Texture2D<float4> GBufferNormals : register(t0, space0);
Texture2D<float4> GBufferDepth   : register(t1, space0);

RWTexture2D<float4> Output : register(u0, space0);

[numthreads(1, 1, 1)]
void Main(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint3 DispatchThreadID : SV_DispatchThreadID, uint GroupIndex : SV_GroupIndex)
{
}
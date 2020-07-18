#include "PBRCommon.hlsli"

RWTexture2D<float2> IntegrationMap : register(u0, space0)

[numthreads(1, 1, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{

}
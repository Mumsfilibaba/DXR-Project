RayTracingAccelerationStructure g_RayTracingScene	: register(t0);
RWTexture2D<float4>				g_Output			: register(u0);

struct RayPayload
{
	float3 Color;
};

[shader("raygeneration")]
void RayGen()
{

}

[shader("miss")]
void Miss(inout RayPayload PayLoad)
{
	PayLoad.Color = float3(0.3921f, 0.5843f, 0.9394f);
}

[shader("closesthit")]
void ClosestHit(inout RayPayload PayLoad, in BuiltInTriangleIntersectionAttributes IntersectionAttributes)
{
	float3 BarycentricCoords = float3(1.0f - IntersectionAttributes.barycentrics.x - IntersectionAttributes.barycentrics.y, IntersectionAttributes.barycentrics.x, IntersectionAttributes.barycentrics.y);
	Payload.Color = float3(1.0f, 0.0f, 0.0f);
}
RaytracingAccelerationStructure g_RayTracingScene	: register(t0, space0);
RWTexture2D<float4>				g_Output			: register(u0, space0);

struct RayPayload
{
	float3 Color;
};

[shader("raygeneration")]
void RayGen()
{
	uint3 DispatchIndex			= DispatchRaysIndex();
	uint3 DispatchDimensions	= DispatchRaysDimensions();

	float2 Crd	= float2(DispatchIndex.xy);
	float2 Dims = float2(DispatchDimensions.xy);

	float2 Dir = ((Crd / Dims) * 2.0f - 1.0f);
	float AspectRatio = Dims.x / Dims.y;

	RayDesc Ray;
	Ray.Origin		= float3(0.0f, 0.0f, -2.0f);
	Ray.Direction	= normalize(float3(Dir.x * AspectRatio, -Dir.y, 1.0f));

	Ray.TMin = 0;
	Ray.TMax = 100000;

	RayPayload PayLoad;
	TraceRay(g_RayTracingScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, Ray, PayLoad);

	g_Output[DispatchIndex.xy] = float4(PayLoad.Color, 1.0f);
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
	PayLoad.Color = float3(1.0f, 0.0f, 0.0f);
}
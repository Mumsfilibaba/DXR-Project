// Scene and Output
struct Camera
{
	float4x4	ViewProjection;
	float4x4	ViewProjectionInverse;
	float3		Position;
};

RaytracingAccelerationStructure Scene : register(t0, space0);

Texture2D<float4>	Albedo		: register(t4, space0);
Texture2D<float4>	NormalMap	: register(t5, space0);
TextureCube<float4>	Skybox		: register(t1, space0);

SamplerState TextureSampler	: register(s0, space0);

RWTexture2D<float4> OutTexture 	: register(u0, space0);

ConstantBuffer<Camera> Camera : register(b0, space0);

// Geometry
struct Vertex
{
	float3 Position;
	float3 Normal;
	float3 Tangent;
	float2 TexCoord;
};

StructuredBuffer<Vertex>	Vertices	: register(t2, space0);
ByteAddressBuffer			InIndices	: register(t3, space0);

// Constants
static const float PI			= 3.14159265359f;
static const float MIN_VALUE	= 0.0000001f;

static const float3 LightPosition	= float3(0.0f, 10.0f, -10.0f);
static const float3 LightColor		= float3(300.0f, 300.0f, 300.0f);

// Helpers
float3 WorldHitPosition()
{
	return WorldRayOrigin() + (RayTCurrent() * WorldRayDirection());
}

float3 FresnelReflectanceSchlick(in float3 I, in float3 N, in float3 F0)
{
	float Cosi = saturate(dot(-I, N));
	return F0 + (1 - F0) * pow(1 - Cosi, 5);
}

float3 FresnelSchlick(float CosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - CosTheta, 5.0f);
}

float DistributionGGX(float3 N, float3 H, float Roughness)
{
    float A = Roughness * Roughness;
    float A2 = A * A;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float Nom = A2;
    float Denom = (NdotH2 * (A2 - 1.0f) + 1.0f);
    Denom = PI * Denom * Denom;

    return Nom / max(Denom, MIN_VALUE);
}

float GeometrySchlickGGX(float NdotV, float Roughness)
{
    float R = (Roughness + 1.0f);
    float K = (R * R) / 8.0f;

    return NdotV / ((NdotV * (1.0f - K)) + K);
}

float GeometrySmith(float3 N, float3 V, float3 L, float Roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);

    return GeometrySchlickGGX(NdotV, Roughness) * GeometrySchlickGGX(NdotL, Roughness);
}

// Diffuse lighting calculation.
float CalculateDiffuseCoefficient(in float3 HitPosition, in float3 IncidentLightRay, in float3 Normal)
{
	float fNDotL = saturate(dot(-IncidentLightRay, Normal));
	return fNDotL;
}

// Phong lighting specular component
float3 CalculateSpecularCoefficient(in float3 HitPosition, in float3 IncidentLightRay, in float3 Normal, in float SpecularPower)
{
	float3 ReflectedLightRay = normalize(reflect(IncidentLightRay, Normal));
	return pow(saturate(dot(ReflectedLightRay, normalize(-WorldRayDirection()))), SpecularPower);
}

float3 CalculatePhongLighting(in float3 Albedo, in float3 Normal, in float DiffuseCoef = 1.0f, in float SpecularCoef = 1.0f, in float SpecularPower = 50.0f)
{
	float3 HitPosition		= WorldHitPosition();
	float3 IncidentLightRay	= normalize(HitPosition - LightPosition);

	// Diffuse component.
	float3 LightDiffuseColor	= LightColor;
	float Kd					= CalculateDiffuseCoefficient(HitPosition, IncidentLightRay, Normal);
	float3 DiffuseColor			= DiffuseCoef * Kd * LightDiffuseColor * Albedo;

	// Specular component.
	float3 SpecularColor	= float3(1.0f, 1.0f, 1.0f);
	float3 Ks				= CalculateSpecularCoefficient(HitPosition, IncidentLightRay, Normal, SpecularPower);
	SpecularColor			= SpecularCoef * Ks * LightColor;

	// Ambient component.
	float3 AmbientColor		= float3(0.1f, 0.1f, 0.1f);
	AmbientColor			= Albedo * AmbientColor;

	return AmbientColor + DiffuseColor + SpecularColor;
}

// Shaders
struct RayPayload
{
	float3	Color;
	uint	CurrentRecursionDepth;
};

[shader("raygeneration")]
void RayGen()
{
	uint3 DispatchIndex			= DispatchRaysIndex();
	uint3 DispatchDimensions	= DispatchRaysDimensions();

	float2 Pixel		= float2(DispatchIndex.xy) + 0.5f;
	float2 ScreenPos	= (Pixel / float2(DispatchDimensions.xy)) * 2.0f - 1.0f;

	// Invert Y for DirectX-style coordinates.
	ScreenPos.y = -ScreenPos.y;

	// Unproject the pixel coordinate into a world positon.
	float4x4 ProjectionToWorld = Camera.ViewProjectionInverse;
	float4 World = mul(float4(ScreenPos, 0.0f, 1.0f), ProjectionToWorld);
	World.xyz /= World.w;

	// Send inital ray
	RayDesc Ray;
	Ray.Origin		= Camera.Position;
	Ray.Direction	= normalize(World.xyz - Ray.Origin);

	Ray.TMin = 0;
	Ray.TMax = 100000;

	RayPayload PayLoad;
	PayLoad.CurrentRecursionDepth = 1;

	TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, Ray, PayLoad);

	// HDR tonemapping
    float3 FinalColor = PayLoad.Color;
    FinalColor = FinalColor / (FinalColor + float3(1.0f, 1.0f, 1.0f));
    // Gamma correct
    const float Gamma = 1.0f / 2.2f;
    FinalColor = pow(FinalColor, float3(Gamma, Gamma, Gamma));
	
	// Output Image
    OutTexture[DispatchIndex.xy] = float4(FinalColor, 1.0f);
}

[shader("miss")]
void Miss(inout RayPayload PayLoad)
{
	PayLoad.Color = Skybox.SampleLevel(TextureSampler, WorldRayDirection(), 0).rgb; // float3(0.3921f, 0.5843f, 0.9394f);
}

[shader("closesthit")]
void ClosestHit(inout RayPayload PayLoad, in BuiltInTriangleIntersectionAttributes IntersectionAttributes)
{
	// Get the base index of the triangle's first 16 bit index.
	const uint IndexSizeInBytes		= 4;
	const uint IndicesPerTriangle	= 3;
	const uint TriangleIndexStride	= IndicesPerTriangle * IndexSizeInBytes;
	const uint BaseIndex			= PrimitiveIndex() * TriangleIndexStride;

	// Load up three indices for the triangle.
	uint3 Indices = InIndices.Load3(BaseIndex);

	// Retrieve corresponding vertex normals for the triangle vertices.
	float3 TriangleNormals[3] =
	{
		Vertices[Indices[0]].Normal,
		Vertices[Indices[1]].Normal,
		Vertices[Indices[2]].Normal
	};

	float3 BarycentricCoords = float3(1.0f - IntersectionAttributes.barycentrics.x - IntersectionAttributes.barycentrics.y, IntersectionAttributes.barycentrics.x, IntersectionAttributes.barycentrics.y);
	
	float3 Normal = (TriangleNormals[0] * BarycentricCoords.x) + (TriangleNormals[1] * BarycentricCoords.y) + (TriangleNormals[2] * BarycentricCoords.z);
	Normal = normalize(Normal);
	
	// Use instanceID to determine if we should use normalmaps
	float3 AlbedoColor;
	if (InstanceID() == 1)
	{
		float3 TriangleTangent[3] =
		{
			Vertices[Indices[0]].Tangent,
			Vertices[Indices[1]].Tangent,
			Vertices[Indices[2]].Tangent
		};

		float2 TriangleTexCoords[3] =
		{
			Vertices[Indices[0]].TexCoord,
			Vertices[Indices[1]].TexCoord,
			Vertices[Indices[2]].TexCoord
		};

		float2 TexCoords = (TriangleTexCoords[0] * BarycentricCoords.x) + (TriangleTexCoords[1] * BarycentricCoords.y) + (TriangleTexCoords[2] * BarycentricCoords.z);
		
		float3 Tangent= (TriangleTangent[0] * BarycentricCoords.x) + (TriangleTangent[1] * BarycentricCoords.y) + (TriangleTangent[2] * BarycentricCoords.z);
		Tangent = normalize(Tangent);

		float3 Bitangent = cross(Normal, Tangent);

		float3 MappedNormal = NormalMap.SampleLevel(TextureSampler, TexCoords, 0).rgb;
		MappedNormal = normalize((MappedNormal * 2.0f) - 1.0f);

		AlbedoColor = Albedo.SampleLevel(TextureSampler, TexCoords, 0).rgb;

		float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
		Normal = normalize(mul(TBN, (MappedNormal)));
	}
	else
	{
		AlbedoColor = float3(0.5f, 0.0f, 0.0f);
	}

	// Send a new ray for reflection
	const float3 HitPosition	= WorldHitPosition();
    const float3 LightDir		= normalize(LightPosition - HitPosition);
    const float3 ViewDir		= normalize(Camera.Position - HitPosition);
    const float3 HalfVec		= normalize(ViewDir + LightDir);
    const float Roughness		= 0.05f;
    const float Metallic		= 1.0f;
    const float AO				= 1.0f;
	
	float3 ReflectedColor = float3(0.0f, 0.0f, 0.0f);
	if (PayLoad.CurrentRecursionDepth < 4)
	{
		RayDesc Ray;
		Ray.Origin		= HitPosition;
		Ray.Direction	= reflect(WorldRayDirection(), Normal);

		Ray.TMin = 0;
		Ray.TMax = 100000;

		RayPayload ReflectancePayLoad;
		ReflectancePayLoad.CurrentRecursionDepth = PayLoad.CurrentRecursionDepth + 1;

		TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, Ray, ReflectancePayLoad);

		ReflectedColor = ReflectancePayLoad.Color;
	}
	
    float3 FresnelReflect = FresnelSchlick(saturate(dot(-WorldRayDirection(), Normal)), AlbedoColor);
    ReflectedColor = FresnelReflect * ReflectedColor;

	//float3 PhongColor = CalculatePhongLighting(AlbedoColor, Normal);

    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, AlbedoColor, Metallic);

    // Reflectance equation
    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    // Calculate per-light radiance
    float	Distance = length(LightPosition - HitPosition);
    float	Attenuation = 1.0f / (Distance * Distance);
    float3	Radiance	= LightColor * Attenuation;

    // Cook-Torrance BRDF
    float	NDF	= DistributionGGX(Normal, HalfVec, Roughness);
    float	G	= GeometrySmith(Normal, ViewDir, LightDir, Roughness);
    float3	F	= FresnelSchlick(saturate(dot(HalfVec, ViewDir)), F0);
           
    float3	Nominator	= NDF * G * F;
    float	Denominator = 4.0f * max(dot(Normal, ViewDir), 0.0f) * max(dot(Normal, LightDir), 0.0f);
    float3	Specular	= Nominator / max(Denominator, MIN_VALUE);
        
    // Ks is equal to Fresnel
    float3 Ks = F;
    // For energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    float3 Kd = float3(1.0f, 1.0f, 1.0f) - Ks;
    // Multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    Kd *= 1.0f - Metallic;

    // Scale light by NdotL
    float NdotL = max(dot(Normal, LightDir), 0.0f);

    // Add to outgoing radiance Lo
    Lo += (((Kd * AlbedoColor) / PI) + Specular) * Radiance * NdotL;
    
    float3 Ambient	= float3(0.03f, 0.03f, 0.03f) * AlbedoColor * AO;
    float3 Color	= Ambient + Lo;
	
	// Add rays together
    PayLoad.Color = Color + ReflectedColor;
}
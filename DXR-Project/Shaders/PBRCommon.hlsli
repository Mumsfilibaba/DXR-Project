
// Common Constants
static const float MIN_ROUGHNESS    = 0.05f;
static const float MAX_ROUGHNESS	= 1.0f;
static const float PI               = 3.14159265359f;
static const float MIN_VALUE        = 0.0000001f;
static const float RAY_OFFSET       = 0.2f;

static const float3 LightPosition   = float3(0.0f, 10.0f, -10.0f);
static const float3 LightColor      = float3(400.0f, 400.0f, 400.0f);

// Common Structs
struct Camera
{
	float4x4    ViewProjection;
	float3      Position;
	float		Padding;
	float4x4    ViewProjectionInverse;
};

struct PointLight
{
	float3	Color;
	float	Padding;
	float3	Position;
};

struct DirectionalLight
{
    float3		Color;
    float		ShadowBias;
    float3		Direction;
    float		Padding1;
	float4x4	LightMatrix;
};

struct Vertex
{
	float3 Position;
	float3 Normal;
	float3 Tangent;
	float2 TexCoord;
};

// Position Helper
float3 PositionFromDepth(float Depth, float2 TexCoord, float4x4 ViewProjectionInverse)
{
	float Z = Depth;
	float X = TexCoord.x * 2.0f - 1.0f;
	float Y = (1.0f - TexCoord.y) * 2.0f - 1.0f;

	float4 ProjectedPos     = float4(X, Y, Z, 1.0f);
	float4 WorldPosition    = mul(ProjectedPos, ViewProjectionInverse);
	
	return WorldPosition.xyz / WorldPosition.w;
}

// PBR Functions
float DistributionGGX(float3 N, float3 H, float Roughness)
{
	float A         = Roughness * Roughness;
	float A2        = A * A;
	float NdotH     = max(dot(N, H), 0.0f);
	float NdotH2    = NdotH * NdotH;

	float Nom   = A2;
	float Denom = (NdotH2 * (A2 - 1.0f) + 1.0f);
	Denom = PI * Denom * Denom;

	return Nom / max(Denom, MIN_VALUE);
}

float GeometrySchlickGGX(float NdotV, float Roughness)
{
	float R = (Roughness + 1.0f);
	float K = (R * R) / 8.0f;

	return NdotV / (NdotV * (1.0f - K) + K);
}

float GeometrySchlickGGX_IBL(float NdotV, float Roughness)
{
	float K = (Roughness * Roughness) / 2.0f;
	return NdotV / (NdotV * (1.0f - K) + K);
}

float GeometrySmith(float3 N, float3 V, float3 L, float Roughness)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);

	return GeometrySchlickGGX(NdotV, Roughness) * GeometrySchlickGGX(NdotL, Roughness);
}

float GeometrySmith_IBL(float3 N, float3 V, float3 L, float Roughness)
{
	float NdotV = max(dot(N, V), MIN_VALUE);
	float NdotL = max(dot(N, L), MIN_VALUE);

	return GeometrySchlickGGX_IBL(NdotV, Roughness) * GeometrySchlickGGX_IBL(NdotL, Roughness);
}

float3 FresnelSchlick(float CosTheta, float3 F0)
{
	return F0 + (1.0f - F0) * pow(1.0f - CosTheta, 5.0f);
}

float3 FresnelSchlickRoughness(float CosTheta, float3 F0, float Roughness)
{
	float R = 1.0f - Roughness;
	return F0 + (max(float3(R, R, R), F0) - F0) * pow(1.0f - CosTheta, 5.0f);
}

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint Bits)
{
	Bits = (Bits << 16u) | (Bits >> 16u);
	Bits = ((Bits & 0x55555555u) << 1u) | ((Bits & 0xAAAAAAAAu) >> 1u);
	Bits = ((Bits & 0x33333333u) << 2u) | ((Bits & 0xCCCCCCCCu) >> 2u);
	Bits = ((Bits & 0x0F0F0F0Fu) << 4u) | ((Bits & 0xF0F0F0F0u) >> 4u);
	Bits = ((Bits & 0x00FF00FFu) << 8u) | ((Bits & 0xFF00FF00u) >> 8u);
	return float(Bits) * 2.3283064365386963e-10; // 0x100000000
}

float2 Hammersley(uint I, uint N)
{
	return float2(float(I) / float(N), RadicalInverse_VdC(I));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float Roughness)
{
	float A = Roughness * Roughness;
	
	float Phi		= 2.0f * PI * Xi.x;
	float CosTheta	= sqrt((1.0f - Xi.y) / (1.0f + (A * A - 1.0f) * Xi.y));
	float SinTheta	= sqrt(1.0f - CosTheta * CosTheta);
	
	// From spherical coordinates to cartesian coordinates
	float3 H;
	H.x = cos(Phi) * SinTheta;
	H.y = sin(Phi) * SinTheta;
	H.z = CosTheta;
	
	// From tangent-space vector to world-space sample vector
	float3 Up			= abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
	float3 Tangent		= normalize(cross(Up, N));
	float3 Bitangent	= cross(N, Tangent);
	
	float3 SampleVec = Tangent * H.x + Bitangent * H.y + N * H.z;
	return normalize(SampleVec);
}

// HDR Helpers
float3 ApplyGammaCorrectionAndTonemapping(float3 InputColor)
{
	const float INTENSITY   = 0.75f;
	const float GAMMA       = 1.0f / 2.2f;
	
	// Gamma correct
	float3 FinalColor = InputColor;
	FinalColor = FinalColor / (FinalColor + float3(INTENSITY, INTENSITY, INTENSITY));
	FinalColor = pow(FinalColor, float3(GAMMA, GAMMA, GAMMA));
	return FinalColor;
}

float3 ApplyNormalMapping(float3 MappedNormal, float3 Normal, float3 Tangent, float3 Bitangent)
{
	float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
	return normalize(mul(MappedNormal, TBN));
}

float3 UnpackNormal(float3 SampledNormal)
{
	return normalize((SampledNormal * 2.0f) - 1.0f);
}

float3 PackNormal(float3 Normal)
{
	return (normalize(Normal) + 1.0f) * 0.5f;
}

// RayTracing Helpers
float3 WorldHitPosition()
{
	return WorldRayOrigin() + (RayTCurrent() * WorldRayDirection());
}

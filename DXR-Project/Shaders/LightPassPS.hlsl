// Resources
struct PSInput
{
	float2 TexCoord : TEXCOORD0;
};

Texture2D<float4> Albedo		: register(t0, space0);
Texture2D<float4> Normal		: register(t1, space0);
Texture2D<float4> Material		: register(t2, space0);
Texture2D<float4> DepthStencil	: register(t3, space0);

SamplerState GBufferSampler : register(s0, space0);

struct Camera
{
	float4x4	ViewProjection;
	float4x4	ViewProjectionInverse;
	float3		Position;
};

ConstantBuffer<Camera> Camera : register(b0, space0);

// Constants
static const float	PI				= 3.14159265359f;
static const float	MIN_VALUE		= 0.00000000001f;

static const float3	LightPosition	= float3(0.0f, 10.0f, -10.0f);
static const float3	LightColor		= float3(300.0f, 300.0f, 300.0f);

// Helpers
float3 PositionFromDepth(float2 TexCoord)
{
	float Z = DepthStencil.Sample(GBufferSampler, TexCoord).r;
	float X = TexCoord.x * 2.0f - 1.0f;
	float Y = (1 - TexCoord.y) * 2.0f - 1.0f;

	float4 ProjectedPos = float4(X, Y, Z, 1.0f);
	float4 WorldPosition = mul(ProjectedPos, Camera.ViewProjectionInverse);
	return WorldPosition.xyz / WorldPosition.w;
}

float DistributionGGX(float3 N, float3 H, float Roughness)
{
	float A			= Roughness * Roughness;
	float A2		= A * A;
	float NdotH		= max(dot(N, H), 0.0f);
	float NdotH2	= NdotH * NdotH;

	float Nom	= A2;
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

float3 FresnelSchlick(float CosTheta, float3 F0)
{
	return F0 + (1.0f - F0) * pow(1.0f - CosTheta, 5.0f);
}

// Main
float4 Main(PSInput Input) : SV_TARGET
{
	float2 TexCoord = Input.TexCoord;
	TexCoord.y = 1.0f - TexCoord.y;
	
    float Depth = DepthStencil.Sample(GBufferSampler, TexCoord).r;
    if (Depth >= 1.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
	
	float3 WorldPosition	= PositionFromDepth(TexCoord);
	float3 SampledAlbedo	= Albedo.Sample(GBufferSampler, TexCoord).rgb;
	float3 SampledNormal	= Normal.Sample(GBufferSampler, TexCoord).rgb;
    SampledNormal = ((SampledNormal * 2.0f) - 1.0f);
	
	float3 SampledMaterial	= Material.Sample(GBufferSampler, TexCoord).rgb;
	
	const float3	Norm		= normalize(SampledNormal);
	const float3	ViewDir		= normalize(Camera.Position - WorldPosition);
	const float		Roughness	= SampledMaterial.r;
	const float		Metallic	= SampledMaterial.g;
	const float		AO			= SampledMaterial.b;

	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	F0 = lerp(F0, SampledAlbedo, Metallic);

    // Reflectance equation
	float3 Lo = float3(0.0f, 0.0f, 0.0f);

    // Calculate per-light radiance
	float3	LightDir	= normalize(LightPosition - WorldPosition);
	float3	HalfVec		= normalize(ViewDir + LightDir);
	float	Distance	= length(LightPosition - WorldPosition);
	float	Attenuation = 1.0f / (Distance * Distance);
	float3	Radiance	= LightColor * Attenuation;

    // Cook-Torrance BRDF
	float	NDF	= DistributionGGX(Norm, HalfVec, Roughness);
	float	G	= GeometrySmith(Norm, ViewDir, LightDir, Roughness);
	float3	F	= FresnelSchlick(saturate(dot(HalfVec, ViewDir)), F0);
           
	float3	Nominator	= NDF * G * F;
	float	Denominator = 4.0f * max(dot(Norm, ViewDir), 0.0f) * max(dot(Norm, LightDir), 0.0f);
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
	float NdotL = max(dot(Norm, LightDir), 0.0f);

    // Add to outgoing radiance Lo
	Lo += (((Kd * SampledAlbedo) / PI) + Specular) * Radiance * NdotL;
    
	float3 Ambient	= float3(0.03f, 0.03f, 0.03f) * SampledAlbedo * AO;
	float3 Color	= Ambient + Lo;

    // HDR tonemapping
	Color = Color / (Color + float3(1.0f, 1.0f, 1.0f));
    // Gamma correct
	const float Gamma = 1.0f / 2.2f;
	Color = pow(Color, float3(Gamma, Gamma, Gamma));

	return float4(Color, 1.0f);
}
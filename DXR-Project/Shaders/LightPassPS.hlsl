#include "PBRCommon.hlsli"

// Resources
struct PSInput
{
	float2 TexCoord : TEXCOORD0;
};

Texture2D<float4> Albedo			: register(t0, space0);
Texture2D<float4> Normal			: register(t1, space0);
Texture2D<float4> Material			: register(t2, space0);
Texture2D<float4> DepthStencil		: register(t3, space0);
Texture2D<float4> DXRReflection		: register(t4, space0);
Texture2D<float2> IntegrationLUT	: register(t5, space0);

SamplerState GBufferSampler : register(s0, space0);
SamplerState LUTSampler		: register(s1, space0);

ConstantBuffer<Camera> Camera : register(b0, space0);

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

// Main
float4 Main(PSInput Input) : SV_TARGET
{
	float2 TexCoord = Input.TexCoord;
	TexCoord.y = 1.0f - TexCoord.y;
	
	float3 WorldPosition		= PositionFromDepth(TexCoord);
	float3 SampledAlbedo		= Albedo.Sample(GBufferSampler, TexCoord).rgb;
    float3 SampledReflection	= DXRReflection.Sample(LUTSampler, TexCoord).rgb;
	float3 SampledMaterial		= Material.Sample(GBufferSampler, TexCoord).rgb;
	float3 SampledNormal		= Normal.Sample(GBufferSampler, TexCoord).rgb;
    SampledNormal = ((SampledNormal * 2.0f) - 1.0f);
	
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
	float3 Kd = 1.0f - Ks;
	Kd *= 1.0f - Metallic;

    // Scale light by NdotL
	float NdotL = max(dot(Norm, LightDir), 0.0f);

    // Add to outgoing radiance Lo
	Lo += (((Kd * SampledAlbedo) / PI) + Specular) * Radiance * NdotL;
    
    float3 F_IBL	= FresnelSchlickRoughness(max(dot(Norm, ViewDir), 0.0), F0, Roughness);
    float3 Ks_IBL	= F;
    float3 Kd_IBL	= 1.0f - Ks_IBL;
    Kd_IBL *= 1.0 - Metallic;
	
	float2 IntegrationBRDF	= IntegrationLUT.Sample(LUTSampler, float2(max(dot(Norm, ViewDir), 0.0), Roughness)).rg;
    float3 IBL_Specular		= SampledReflection * ((F_IBL * IntegrationBRDF.x) + IntegrationBRDF.y);
	
    float3 Ambient	= IBL_Specular * AO;
    float3 Color	= Ambient + Lo;

    // HDR tonemapping
    const float INTENSITY	= 0.5f;
    const float GAMMA		= 1.0f / 2.2f;
	
    Color = Color / (Color + float3(INTENSITY, INTENSITY, INTENSITY));
    // Gamma correct
    Color = pow(Color, float3(GAMMA, GAMMA, GAMMA));

	return float4(Color, 1.0f);
}
#include "PBRCommon.hlsli"

// Resources
struct PSInput
{
	float2 TexCoord : TEXCOORD0;
};

Texture2D<float4>	Albedo			: register(t0, space0);
Texture2D<float4>	Normal			: register(t1, space0);
Texture2D<float4>	Material		: register(t2, space0);
Texture2D<float4>	DepthStencil	: register(t3, space0);
Texture2D<float4>	DXRReflection	: register(t4, space0);
TextureCube<float4>	IrradianceMap	: register(t5, space0);
Texture2D<float4>	IntegrationLUT	: register(t6, space0);

SamplerState GBufferSampler : register(s0, space0);
SamplerState LUTSampler		: register(s1, space0);

ConstantBuffer<Camera> Camera : register(b0, space0);

// Main
float4 Main(PSInput Input) : SV_TARGET
{
	float2 TexCoord = Input.TexCoord;
	TexCoord.y		= 1.0f - TexCoord.y;
	
    float Depth = DepthStencil.Sample(GBufferSampler, TexCoord).r;
    if (Depth >= 1.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
	float3 WorldPosition		= PositionFromDepth(Depth, TexCoord, Camera.ViewProjectionInverse);
	float3 SampledAlbedo		= Albedo.Sample(GBufferSampler, TexCoord).rgb;
    float3 SampledReflection	= DXRReflection.Sample(LUTSampler, TexCoord).rgb;
	float3 SampledMaterial		= Material.Sample(GBufferSampler, TexCoord).rgb;
	float3 SampledNormal		= Normal.Sample(GBufferSampler, TexCoord).rgb;
	
    const float3	Norm		= UnpackNormal(SampledNormal);
	const float3	ViewDir		= normalize(Camera.Position - WorldPosition);
    const float		Roughness	= SampledMaterial.r;
	const float		Metallic	= SampledMaterial.g;
    const float		SampledAO	= SampledMaterial.b;
	
	float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, SampledAlbedo, Metallic);

	// Reflectance equation
	float	DotNV	= max(dot(Norm, ViewDir), 0.0f);
	float3	Lo		= float3(0.0f, 0.0f, 0.0f);
	{
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
		float	Denominator	= 4.0f * DotNV * max(dot(Norm, LightDir), 0.0f);
		float3	Specular	= Nominator / max(Denominator, MIN_VALUE);
        
		// Ks is equal to Fresnel
		float3 Ks = F;
		float3 Kd = 1.0f - Ks;
		Kd *= 1.0f - Metallic;

		// Scale light by NdotL
		float NdotL = max(dot(Norm, LightDir), 0.0f);

		// Add to outgoing radiance Lo
		Lo += (Kd * SampledAlbedo / PI + Specular) * Radiance * NdotL;
    }
    
    float3 F_IBL	= FresnelSchlick(DotNV, F0); // FresnelSchlickRoughness(DotNV, F0, Roughness);
    float3 Ks_IBL	= F_IBL;
    float3 Kd_IBL	= 1.0f - Ks_IBL;
    Kd_IBL *= 1.0 - Metallic;
	
    float3 Irradiance	= IrradianceMap.Sample(GBufferSampler, Norm).rgb;
    float3 IBL_Diffuse	= Irradiance * SampledAlbedo * Kd_IBL;
	
    //float2	IntegrationBRDF	= IntegrationLUT.Sample(LUTSampler, float2(DotNV, Roughness)).rg;
    //float3	IBL_Specular	= SampledReflection * (F_IBL * IntegrationBRDF.x + IntegrationBRDF.y);
	
    float3 Ambient	= (IBL_Diffuse) * SampledAO;
    float3 Color	= Ambient + Lo;
	
    return float4(ApplyGammaCorrectionAndTonemapping(Color), 1.0f);
}
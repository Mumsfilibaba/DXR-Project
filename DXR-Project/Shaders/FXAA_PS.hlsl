#include "PBRCommon.hlsli"

cbuffer CB0 : register(b0, space0)
{
	float2 TextureSize;
}

Texture2D FinalImage : register(t0, space0);
SamplerState Sampler : register(s0, space0);

static const float3	LUMA			= float3(0.299f, 0.587f, 0.114f);
static const float	FXAA_REDUCE_MUL	= 1.0f / 8.0f;

float4 Main(float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	// Correct the texcoords
	TexCoord.y = 1.0f - TexCoord.y;
	
	// Perform edge detection
	const float2 InvTextureSize = float2(1.0f, 1.0f) / TextureSize;
    const float3 Middle = FinalImage.Sample(Sampler, TexCoord).rgb;
    float LumaM		= dot(Middle, LUMA);
	float LumaTL	= dot(FinalImage.Sample(Sampler, TexCoord + float2(-InvTextureSize.x,  InvTextureSize.y)).rgb, LUMA);
	float LumaTR	= dot(FinalImage.Sample(Sampler, TexCoord + float2( InvTextureSize.x,  InvTextureSize.y)).rgb, LUMA);
	float LumaBL	= dot(FinalImage.Sample(Sampler, TexCoord + float2(-InvTextureSize.x, -InvTextureSize.y)).rgb, LUMA);
	float LumaBR	= dot(FinalImage.Sample(Sampler, TexCoord + float2( InvTextureSize.x, -InvTextureSize.y)).rgb, LUMA);
	
	float2 Direction;
	Direction.x = -((LumaTL + LumaTR) - (LumaBL + LumaBR));
	Direction.y =  ((LumaTL + LumaBL) - (LumaTR + LumaBR));
	
	float DirectionReduce = max((LumaTL + LumaTR + LumaBL + LumaBR) * FXAA_REDUCE_MUL * 0.25f, 0.0001f);
	float InvDirectionMul = 1.0f / (min(abs(Direction.x), abs(Direction.y)) + DirectionReduce);
	Direction = Direction * InvDirectionMul;
    Direction = Direction * InvTextureSize;
	
    float3 Color0 = FinalImage.Sample(Sampler, TexCoord + (Direction * ((0.0f / 3.0f) - 0.5f))).rgb;
    float3 Color1 = FinalImage.Sample(Sampler, TexCoord + (Direction * ((1.0f / 3.0f) - 0.5f))).rgb;
    float3 Color2 = FinalImage.Sample(Sampler, TexCoord + (Direction * ((2.0f / 3.0f) - 0.5f))).rgb;
    float3 Color3 = FinalImage.Sample(Sampler, TexCoord + (Direction * ((3.0f / 3.0f) - 0.5f))).rgb;
	
	// float Luminocity = (LumaM + LumaTL + LumaTR + LumaBL + LumaBR) * 0.2f;
    float3 Color = (Middle + Color0 + Color1 + Color2 + Color3) * 0.2f;
	return float4(ApplyGammaCorrectionAndTonemapping(Color), 1.0f);
}
// PerObject
cbuffer TransformBuffer : register(b0, space0)
{
	float4x4 Transform;
};

// PerFrame DescriptorTable
cbuffer LightBuffer : register(b1, space0)
{
	float4x4	LightProjection;
	float3		LightPosition;
	float		LightFarPlane;
}

// VertexShader
struct VSInput
{
	float3 Position : POSITION0;
	float3 Normal	: NORMAL0;
	float3 Tangent	: TANGENT0;
	float2 TexCoord : TEXCOORD0;
};

// Normal ShadowMap Generation
float4 Main(VSInput Input) : SV_POSITION
{
	float4 WorldPosition = mul(float4(Input.Position, 1.0f), Transform);
	return mul(WorldPosition, LightProjection);
}

// Linear Shadow Generation
struct VSOutput
{
	float3 WorldPosition	: POSITION0;
	float4 Position			: SV_POSITION;
};

VSOutput VSMain(VSInput Input)
{
	VSOutput Output = (VSOutput)0;
	
	float4 WorldPosition	= mul(float4(Input.Position, 1.0f), Transform);
	Output.WorldPosition	= WorldPosition.xyz;
	Output.Position			= mul(WorldPosition, LightProjection);
	
	return Output;
}

float PSMain(float3 WorldPosition : POSITION0) : SV_Depth
{
	float LightDistance = length(WorldPosition.xyz - LightPosition);
	LightDistance = LightDistance / LightFarPlane;
	return LightDistance;
}
// Scene and Output
cbuffer TransformBuffer : register(b0)
{
	float4x4	Transform;
	float		Roughness;
	float		Metallic;
	float		AO;
};

struct Camera
{
	float4x4	ViewProjection;
	float4x4	ViewProjectionInverse;
	float3		Position;
};

ConstantBuffer<Camera> Camera : register(b1, space0);

static const float MIN_ROUGHNESS = 0.01f;

// VertexShader
struct VSInput
{
	float3 Position : POSITION0;
	float3 Normal	: NORMAL0;
	float3 Tangent	: TANGENT0;
	float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
	float3 Normal	: NORMAL0;
	float3 Tangent	: TANGENT0;
	float2 TexCoord : TEXCOORD0;
	float4 Position : SV_Position;
};

VSOutput VSMain(VSInput Input)
{
    float3x3 Mat = float3x3(Transform[0].xyz, Transform[1].xyz, Transform[2].xyz);
	
	VSOutput Output;
    Output.Normal	= mul(Input.Normal, Mat);
    Output.Tangent	= mul(Input.Tangent, Mat);
	Output.TexCoord = Input.TexCoord;
	Output.Position = mul(mul(float4(Input.Position, 1.0f), Transform), Camera.ViewProjection);
	return Output;
}

// PixelShader
struct PSInput
{
	float3 Normal	: NORMAL0;
	float3 Tangent	: TANGENT0;
	float2 TexCoord : TEXCOORD0;
};

struct PSOutput
{
	float4 Albedo	: SV_Target0;
	float4 Normal	: SV_Target1;
	float4 Material : SV_Target2;
};

PSOutput PSMain(PSInput Input)
{
	const float3 ObjectColor = float3(1.0f, 0.0f, 0.0f);
	
    float3 Normal = normalize(Input.Normal);
    Normal = (Normal + 1.0f) * 0.5f;

    const float FinalRoughness = max(Roughness, MIN_ROUGHNESS);
	
	PSOutput Output;
	Output.Albedo	= float4(ObjectColor, 1.0f);
    Output.Normal	= float4(Normal, 1.0f);
    Output.Material = float4(FinalRoughness, Metallic, AO, 1.0f);
	return Output;
}
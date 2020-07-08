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
	float4 Position : SV_Position;
	float3 Normal	: NORMAL0;
	float3 Tangent	: TANGENT0;
	float2 TexCoord : TEXCOORD0;
};

VSOutput VSMain(VSInput Input)
{
	VSOutput Output;
	Output.Position = float4(Input.Position, 1.0f);
	Output.Normal	= Input.Normal;
	Output.Tangent	= Input.Tangent;
	Output.TexCoord = Input.TexCoord;
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
	PSOutput Output;
	Output.Albedo	= float4(1.0f, 1.0f, 1.0f, 1.0f);
	Output.Normal	= float4(0.5f, 0.5f, 1.0f, 1.0f);
	Output.Material	= float4(0.5f, 0.5f, 0.5f, 1.0f);
	return Output;
}
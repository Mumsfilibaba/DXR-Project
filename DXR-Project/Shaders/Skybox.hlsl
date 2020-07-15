// Resources
cbuffer CameraBuffer : register(b0)
{
    float4x4 ViewProjection;
};

TextureCube<float4> Skybox : register(t0, space0);

SamplerState SkyboxSampler : register(s0, space0);

// VertexShader
struct VSInput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float3 TexCoord : TEXCOORD0;
    float4 Position : SV_POSITION0;
};

VSOutput VSMain(VSInput Input)
{
    VSOutput Output;
    Output.TexCoord = Input.Position;
    Output.Position = mul(float4(Input.Position, 1.0f), ViewProjection);
    Output.Position = Output.Position.xyww;
    return Output;
}

// PixelShader
float4 PSMain(float3 TexCoord : TEXCOORD0) : SV_TARGET0
{
    return Skybox.Sample(SkyboxSampler, TexCoord);
}
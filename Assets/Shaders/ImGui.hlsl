struct FShaderConstants
{
    float4x4 ProjectionMatrix;
};

#if SHADER_LANG == SHADER_LANG_SPIRV
    [[vk::push_constant]]
    FShaderConstants Constants;
#else
    ConstantBuffer<FShaderConstants> Constants : register(b0, space1);
#endif

struct FVSInput
{
    float2 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR0;
};

struct FVSOutput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output;
    Output.Position = mul(Constants.ProjectionMatrix, float4(Input.Position.xy, 0.0f, 1.0f));
    Output.Color    = Input.Color;
    Output.TexCoord = Input.TexCoord;
    return Output;
}

struct FPSInput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

SamplerState Sampler0 : register(s0);
Texture2D    Texture0 : register(t0);

float4 PSMain(FPSInput Input) : SV_Target
{
    float4 OutColor = Input.Color * Texture0.Sample(Sampler0, Input.TexCoord);
    return OutColor;
}

#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"

// PerFrame
ConstantBuffer<FCamera> CameraBuffer : register(b0);

// PerObject Samplers
SamplerState MaterialSampler : register(s0);

ConstantBuffer<FTransform> TransformBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS);
ConstantBuffer<FMaterial> MaterialBuffer  : register(b1);

Texture2D<float4> AlbedoMap    : register(t0);
Texture2D<float4> NormalTex    : register(t1);
Texture2D<float4> RoughnessTex : register(t2);
Texture2D<float4> HeightTex    : register(t3);
Texture2D<float4> MetallicTex  : register(t4);
Texture2D<float4> AOTex        : register(t5);
Texture2D<float>  AlphaMaskTex : register(t6);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// VertexShader

struct FVSInput
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

struct FVSOutput
{
    float3 Normal          : NORMAL0;
    float3 ViewNormal      : NORMAL1;
    float3 Tangent         : TANGENT0;
    float3 Bitangent       : BITANGENT0;
    float2 TexCoord	       : TEXCOORD0;
    float3 TangentViewPos  : TANGENTVIEWPOS0;
    float3 TangentPosition : TANGENTPOSITION0;
    float4 Position        : SV_Position;
};

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output;
    
    const float4x4 TransformInv = TransformBuffer.TransformInv;
    float3 Normal = normalize(mul(float4(Input.Normal, 0.0f), TransformInv).xyz);
    Output.Normal = Normal;
    
    float3 ViewNormal = mul(float4(Normal, 0.0f), CameraBuffer.View).xyz;
    Output.ViewNormal = ViewNormal;
    
    float3 Tangent = normalize(mul(float4(Input.Tangent, 0.0f), TransformInv).xyz);
    Tangent        = normalize(Tangent - dot(Tangent, Normal) * Normal);
    Output.Tangent = Tangent;
    
    float3 Bitangent = normalize(cross(Tangent, Normal));
    Output.Bitangent = Bitangent;

    Output.TexCoord = Input.TexCoord;

    float4 WorldPosition = mul(float4(Input.Position, 1.0f), TransformBuffer.Transform);
    Output.Position      = mul(WorldPosition, CameraBuffer.ViewProjection);
    
    float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
    Output.TangentViewPos  = mul(TBN, CameraBuffer.Position);
    Output.TangentPosition = mul(TBN, WorldPosition.xyz);

    return Output;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// PixelShader

struct FPSInput
{
    float3 Normal          : NORMAL0;
    float3 ViewNormal      : NORMAL1;
    float3 Tangent         : TANGENT0;
    float3 Bitangent       : BITANGENT0;
    float2 TexCoord        : TEXCOORD0;
    float3 TangentViewPos  : TANGENTVIEWPOS0;
    float3 TangentPosition : TANGENTPOSITION0;
};

struct FPSOutput
{
    float4 Albedo     : SV_Target0;
    float4 Normal     : SV_Target1;
    float4 Material   : SV_Target2;
    float4 ViewNormal : SV_Target3;
};

static const float HEIGHT_SCALE = 0.03f;

float SampleHeightMap(float2 TexCoords)
{
    return 1.0f - HeightTex.Sample(MaterialSampler, TexCoords).r;
}

float2 ParallaxMapping(float2 TexCoords, float3 ViewDir)
{
    const float MinLayers = 32;
    const float MaxLayers = 64;

    float NumLayers  = lerp(MaxLayers, MinLayers, abs(dot(float3(0.0f, 0.0f, 1.0f), ViewDir)));
    float LayerDepth = 1.0f / NumLayers;
    
    float2 P              = ViewDir.xy / ViewDir.z * HEIGHT_SCALE;
    float2 DeltaTexCoords = P / NumLayers;

    float2 CurrentTexCoords     = TexCoords;
    float  CurrentDepthMapValue = SampleHeightMap(CurrentTexCoords);
    
    float CurrentLayerDepth	= 0.0f;
    while (CurrentLayerDepth < CurrentDepthMapValue)
    {
        CurrentTexCoords     -= DeltaTexCoords;
        CurrentDepthMapValue =  SampleHeightMap(CurrentTexCoords);
        CurrentLayerDepth    += LayerDepth;
    }

    float2 PrevTexCoords = CurrentTexCoords + DeltaTexCoords;

    float AfterDepth  = CurrentDepthMapValue - CurrentLayerDepth;
    float BeforeDepth = SampleHeightMap(PrevTexCoords) - CurrentLayerDepth + LayerDepth;

    float  Weight         = AfterDepth / (AfterDepth - BeforeDepth);
    float2 FinalTexCoords = PrevTexCoords * Weight + CurrentTexCoords * (1.0f - Weight);

    return FinalTexCoords;
}

FPSOutput PSMain(FPSInput Input)
{
    float2 TexCoords = Input.TexCoord;
    TexCoords.y = 1.0f - TexCoords.y;
    
    if (MaterialBuffer.EnableHeight != 0)
    {
        float3 ViewDir = normalize(Input.TangentViewPos - Input.TangentPosition);
        TexCoords      = ParallaxMapping(TexCoords, ViewDir);
        if (TexCoords.x > 1.0f || TexCoords.y > 1.0f || TexCoords.x < 0.0f || TexCoords.y < 0.0f)
        {
            discard;
        }
    }
    
    if (MaterialBuffer.EnableMask)
    {
        float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords);
        if (AlphaMask < 0.5f)
        {
            discard;
        }
    }

    float3 SampledAlbedo = ApplyGamma(AlbedoMap.Sample(MaterialSampler, TexCoords).rgb) * MaterialBuffer.Albedo;
    
    float3 SampledNormal = NormalTex.Sample(MaterialSampler, TexCoords).rgb;
    SampledNormal        = UnpackNormal(SampledNormal);
    SampledNormal.y      = -SampledNormal.y;
    
    float3 Tangent      = normalize(Input.Tangent);
    float3 Bitangent    = normalize(Input.Bitangent);
    float3 Normal       = normalize(Input.Normal);
    float3 MappedNormal = ApplyNormalMapping(SampledNormal, Normal, Tangent, Bitangent);
    MappedNormal = PackNormal(MappedNormal);

    const float SampledAO        = AOTex.Sample(MaterialSampler, TexCoords).r * MaterialBuffer.AO;
    const float SampledMetallic  = MetallicTex.Sample(MaterialSampler, TexCoords).r * MaterialBuffer.Metallic;
    const float SampledRoughness = RoughnessTex.Sample(MaterialSampler, TexCoords).r * MaterialBuffer.Roughness;
    const float FinalRoughness   = min(max(SampledRoughness, MIN_ROUGHNESS), MAX_ROUGHNESS);
    
    FPSOutput Output;
    Output.Albedo     = float4(SampledAlbedo, 1.0f);
    Output.Normal     = float4(MappedNormal, 1.0f);
    Output.Material   = float4(FinalRoughness, SampledMetallic, SampledAO, 1.0f);
    Output.ViewNormal = float4(PackNormal(Input.ViewNormal), 1.0f);

    return Output;
}
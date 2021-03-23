#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"

// PerFrame
ConstantBuffer<Camera> CameraBuffer : register(b0, space0);

// PerObject Samplers
SamplerState MaterialSampler : register(s0, space0);

ConstantBuffer<Transform> TransformBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS);
ConstantBuffer<Material>  MaterialBuffer  : register(b1, space0);

Texture2D<float4> AlbedoMap    : register(t0, space0);
Texture2D<float4> NormalMap    : register(t1, space0);
Texture2D<float4> RoughnessMap : register(t2, space0);
Texture2D<float4> HeightMap    : register(t3, space0);
Texture2D<float4> MetallicMap  : register(t4, space0);
Texture2D<float4> AOMap        : register(t5, space0);

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
    float3 Normal           : NORMAL0;
    float3 Tangent          : TANGENT0;
    float3 Bitangent        : BITANGENT0;
    float2 TexCoord         : TEXCOORD0;
    float3 TangentViewPos   : TANGENTVIEWPOS0;
    float3 TangentPosition  : TANGENTPOSITION0;
    float4 ClipPosition     : POSITION0;
    float4 PrevClipPosition : POSITION1;
    float4 Position         : SV_Position;
};

VSOutput VSMain(VSInput Input)
{
    VSOutput Output;
    
    const float4x4 TransformInv = TransformBuffer.TransformInv;
    float3 Normal = normalize(mul(float4(Input.Normal, 0.0f), TransformInv).xyz);
    Output.Normal = Normal;
    
    float3 Tangent = normalize(mul(float4(Input.Tangent, 0.0f), TransformInv).xyz);
    Tangent        = normalize(Tangent - dot(Tangent, Normal) * Normal);
    Output.Tangent = Tangent;
    
    float3 Bitangent = normalize(cross(Tangent, Normal));
    Output.Bitangent = Bitangent;
    Output.TexCoord  = Input.TexCoord;

    float4 WorldPosition    = mul(float4(Input.Position, 1.0f), TransformBuffer.Transform);
    Output.ClipPosition     = mul(WorldPosition, CameraBuffer.ViewProjection);
    Output.Position         = Output.ClipPosition;
    Output.PrevClipPosition = mul(WorldPosition, CameraBuffer.PrevViewProjection);
    
    float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
    Output.TangentViewPos  = mul(TBN, CameraBuffer.Position);
    Output.TangentPosition = mul(TBN, WorldPosition.xyz);
    return Output;
}

// PixelShader
struct PSInput
{
    float3 Normal           : NORMAL0;
    float3 Tangent          : TANGENT0;
    float3 Bitangent        : BITANGENT0;
    float2 TexCoord         : TEXCOORD0;
    float3 TangentViewPos   : TANGENTVIEWPOS0;
    float3 TangentPosition  : TANGENTPOSITION0;
    float4 ClipPosition     : POSITION0;
    float4 PrevClipPosition : POSITION1;
};

struct PSOutput
{
    float4 Albedo     : SV_Target0;
    float4 Normal     : SV_Target1;
    float4 Material   : SV_Target2;
    float4 GeomNormal : SV_Target3;
    float4 Velocity   : SV_Target4;
};

static const float HEIGHT_SCALE = 0.03f;

float SampleHeightMap(float2 TexCoords)
{
    return 1.0f - HeightMap.Sample(MaterialSampler, TexCoords).r;
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
    
    float CurrentLayerDepth = 0.0f;
    while (CurrentLayerDepth < CurrentDepthMapValue)
    {
        CurrentTexCoords     -= DeltaTexCoords;
        CurrentDepthMapValue = SampleHeightMap(CurrentTexCoords);
        CurrentLayerDepth    += LayerDepth;
    }

    float2 PrevTexCoords = CurrentTexCoords + DeltaTexCoords;

    float AfterDepth  = CurrentDepthMapValue - CurrentLayerDepth;
    float BeforeDepth = SampleHeightMap(PrevTexCoords) - CurrentLayerDepth + LayerDepth;

    float  Weight         = AfterDepth / (AfterDepth - BeforeDepth);
    float2 FinalTexCoords = PrevTexCoords * Weight + CurrentTexCoords * (1.0f - Weight);

    return FinalTexCoords;
}

PSOutput PSMain(PSInput Input)
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

    float3 SampledAlbedo = ApplyGamma(AlbedoMap.Sample(MaterialSampler, TexCoords).rgb) * MaterialBuffer.Albedo;
    
    float3 SampledNormal = NormalMap.Sample(MaterialSampler, TexCoords).rgb;
    SampledNormal        = UnpackNormal(SampledNormal);
    SampledNormal.y      = -SampledNormal.y;
    
    float3 Tangent      = normalize(Input.Tangent);
    float3 Bitangent    = normalize(Input.Bitangent);
    float3 Normal       = normalize(Input.Normal);
    float3 MappedNormal = ApplyNormalMapping(SampledNormal, Normal, Tangent, Bitangent);
    MappedNormal = PackNormal(MappedNormal);

    float SampledAO        = AOMap.Sample(MaterialSampler, TexCoords).r * MaterialBuffer.AO;
    float SampledMetallic  = MetallicMap.Sample(MaterialSampler, TexCoords).r * MaterialBuffer.Metallic;
    float SampledRoughness = RoughnessMap.Sample(MaterialSampler, TexCoords).r * MaterialBuffer.Roughness;
    float FinalRoughness   = min(max(SampledRoughness, MIN_ROUGHNESS), MAX_ROUGHNESS);

    float2 ScreenSize   = float2(CameraBuffer.Width, CameraBuffer.Height);
    float2 ClipPosition = Input.ClipPosition.xy / Input.ClipPosition.w;
    ClipPosition = (ClipPosition * float2(0.5f, -0.5)) + 0.5f;
    ClipPosition = ClipPosition * ScreenSize; // Convert to pixel coords since jitter is in pixels
    
    float2 PrevClipPosition = Input.PrevClipPosition.xy / Input.PrevClipPosition.w;
    PrevClipPosition = (PrevClipPosition * float2(0.5f, -0.5)) + 0.5f;
    PrevClipPosition = PrevClipPosition * ScreenSize;
    
    float2 JitterDiff = (CameraBuffer.Jitter - CameraBuffer.PrevJitter) * 0.5f;
    float2 Velocity   = ClipPosition - PrevClipPosition;
    Velocity = Velocity - JitterDiff;
    Velocity = Velocity / ScreenSize; // Then convert back to clip space
    
    PSOutput Output;
    Output.Albedo     = float4(SampledAlbedo, 1.0f);
    Output.Normal     = float4(MappedNormal, 0.0f);
    Output.Material   = float4(FinalRoughness, SampledMetallic, SampledAO, 1.0f);
    Output.GeomNormal = float4(PackNormal(Normal), 0.0f);
    Output.Velocity   = float4(Velocity, length(fwidth(Normal)), 0.0f);
    return Output;
}
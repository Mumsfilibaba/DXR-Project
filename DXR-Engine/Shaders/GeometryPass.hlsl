#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"

// PerFrame
ConstantBuffer<Camera> CameraBuffer : register(b0);

// PerObject Samplers
SamplerState MaterialSampler : register(s0);

ConstantBuffer<Transform> TransformBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS);
ConstantBuffer<Material>  MaterialBuffer  : register(b1);

Texture2D<float4> DiffuseMap  : register(t0);
Texture2D<float4> NormalMap   : register(t1);
Texture2D<float4> SpecularMap : register(t2);
Texture2D<float4> EmissiveMap : register(t3);
Texture2D<float4> HeightMap   : register(t4);

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
    float4 Emissive   : SV_Target5;
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
    
    float2 P = ViewDir.xy / ViewDir.z * HEIGHT_SCALE;
    float2 DeltaTexCoords = P / NumLayers;

    float2 CurrentTexCoords     = TexCoords;
    float  CurrentDepthMapValue = SampleHeightMap(CurrentTexCoords);
    
    float CurrentLayerDepth = 0.0f;
    while (CurrentLayerDepth < CurrentDepthMapValue)
    {
        CurrentTexCoords  -= DeltaTexCoords;
        
        CurrentDepthMapValue = SampleHeightMap(CurrentTexCoords);
        
        CurrentLayerDepth += LayerDepth;
    }

    float2 PrevTexCoords = CurrentTexCoords + DeltaTexCoords;

    float AfterDepth  = CurrentDepthMapValue - CurrentLayerDepth;
    float BeforeDepth = SampleHeightMap(PrevTexCoords) - CurrentLayerDepth + LayerDepth;

    float  Weight = AfterDepth / (AfterDepth - BeforeDepth);
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

    float4 AlbedoAlpha = DiffuseMap.Sample(MaterialSampler, TexCoords);
    if (AlbedoAlpha.a < 0.5f)
    {
        discard;
    }
    
    float4 Emissive = Float4(0.0f);
    if (MaterialBuffer.EnableEmissive != 0)
    {
        Emissive = EmissiveMap.Sample(MaterialSampler, TexCoords);
    }
    
    float3 SampledAlbedo = ApplyGamma(AlbedoAlpha.rgb) * MaterialBuffer.Albedo;
    
    float3 SampledNormal = NormalMap.Sample(MaterialSampler, TexCoords).rgb;
    SampledNormal        = UnpackNormal(SampledNormal);
    
    float3 Tangent      = normalize(Input.Tangent);
    float3 Bitangent    = normalize(Input.Bitangent);
    float3 Normal       = normalize(Input.Normal);
    float3 MappedNormal = ApplyNormalMapping(SampledNormal, Normal, Tangent, Bitangent);
    MappedNormal = PackNormal(MappedNormal);

    float4 Specular = SpecularMap.Sample(MaterialSampler, TexCoords);
    float SampledAO        = Specular.r * MaterialBuffer.AO;
    float SampledRoughness = Specular.g * MaterialBuffer.Roughness;
    float SampledMetallic  = Specular.b * MaterialBuffer.Metallic;
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
    Output.Emissive   = float4(Emissive.rgb, 1.0f);
    return Output;
}
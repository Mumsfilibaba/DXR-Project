#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Tonemapping.hlsli"

#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING (0)
#endif

#ifndef ENABLE_NORMAL_MAPPING
    #define ENABLE_NORMAL_MAPPING (0)
#endif

#ifndef ENABLE_PACKED_MATERIAL_TEXTURE
    #define ENABLE_PACKED_MATERIAL_TEXTURE (0)
#endif

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK (0)
#endif

// PerFrame
ConstantBuffer<FCamera> CameraBuffer : register(b0);

// PerObject Samplers
SamplerState MaterialSampler : register(s0);

SHADER_CONSTANT_BLOCK_BEGIN
    FTransform Transform;
SHADER_CONSTANT_BLOCK_END

ConstantBuffer<FMaterial> MaterialBuffer : register(b1);

#if ENABLE_PACKED_MATERIAL_TEXTURE
    Texture2D<float4> AlbedoAlphaMap : register(t0);
#if ENABLE_NORMAL_MAPPING
    Texture2D<float3> NormalTex : register(t1);
#endif
    Texture2D<float3> AO_Roughness_Metal_Tex : register(t2);
#if ENABLE_PARALLAX_MAPPING
    Texture2D<float> HeightTex : register(t3);
#endif
#else
    Texture2D<float3> AlbedoMap : register(t0);
#if ENABLE_NORMAL_MAPPING
    Texture2D<float3> NormalTex : register(t1);
#endif
    Texture2D<float>  RoughnessTex : register(t2);
    Texture2D<float>  MetallicTex  : register(t3);
    Texture2D<float>  AOTex        : register(t4);
#if ENABLE_ALPHA_MASK
    Texture2D<float> AlphaMaskTex : register(t5);
#endif
#if ENABLE_PARALLAX_MAPPING
    Texture2D<float> HeightTex : register(t6);
#endif
#endif

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
    float3 Normal    : NORMAL0;
    float3 Tangent   : TANGENT0;
    float3 Bitangent : BITANGENT0;
    float2 TexCoord	 : TEXCOORD0;

#if ENABLE_PARALLAX_MAPPING 
    float3 TangentViewPos   : TANGENTVIEWPOS0;
    float3 TangentPosition  : TANGENTPOSITION0;
#endif

    float4 ClipPosition     : POSITION0;
    float4 PrevClipPosition : POSITION1;
    float4 Position         : SV_Position;
};

FVSOutput VSMain(FVSInput Input)
{
    // Position
    const float4x4 Transform     = Constants.Transform.Transform; 
    const float4   WorldPosition = mul(float4(Input.Position, 1.0f), Transform);

    // Normal
    const float4x4 TransformInv = Constants.Transform.TransformInv;  
    float3 Normal = normalize(mul(float4(Input.Normal, 0.0f), TransformInv).xyz);

    // Check if the triangle is back-facing (based on the direction of the normal)
    // float IsBackFacing = sign(dot(Normal, WorldPosition.xyz - Transform[3].xyz));
    // Normal = normalize(Normal * IsBackFacing);

    float3 Tangent = normalize(mul(float4(Input.Tangent, 0.0f), TransformInv).xyz);
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);

    float3 Bitangent = normalize(cross(Tangent, Normal));

    FVSOutput Output;
    Output.Normal    = Normal;
    Output.Tangent   = Tangent;
    Output.Bitangent = Bitangent;
    Output.Position  = mul(WorldPosition, CameraBuffer.ViewProjection);

    // TODO: Handle moving objects (aka PrevTransform)
    Output.ClipPosition     = Output.Position;
    Output.PrevClipPosition = mul(WorldPosition, CameraBuffer.PrevViewProjection);

    Output.TexCoord = Input.TexCoord;

#if ENABLE_PARALLAX_MAPPING
    const float3x3 TangentSpace = float3x3(Tangent, Bitangent, Normal);
    Output.TangentViewPos  = mul(TangentSpace, CameraBuffer.Position);
    Output.TangentPosition = mul(TangentSpace, WorldPosition.xyz);
#endif

    return Output;
}

// PixelShader

struct FPSInput
{
    float3 Normal    : NORMAL0;
    float3 Tangent   : TANGENT0;
    float3 Bitangent : BITANGENT0;
    float2 TexCoord  : TEXCOORD0;

#if ENABLE_PARALLAX_MAPPING
    float3 TangentViewPos  : TANGENTVIEWPOS0;
    float3 TangentPosition : TANGENTPOSITION0;
#endif

    float4 ClipPosition     : POSITION0;
    float4 PrevClipPosition : POSITION1;
    float4 Position         : SV_Position;
};

struct FPSOutput
{
    float4 Albedo   : SV_Target0;
    float4 Normal   : SV_Target1;
    float4 Material : SV_Target2;
    float2 Velocity : SV_Target3;
};

#if ENABLE_PARALLAX_MAPPING
// TODO: We do not want any constants like this, it should be a constantbuffer or something similar
static const float HEIGHT_SCALE = 0.03f;

float SampleHeightMap(float2 TexCoords)
{
    return 1.0f - HeightTex.Sample(MaterialSampler, TexCoords);
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
        CurrentDepthMapValue  = SampleHeightMap(CurrentTexCoords);
        CurrentLayerDepth    += LayerDepth;
    }

    float2 PrevTexCoords = CurrentTexCoords + DeltaTexCoords;

    float AfterDepth  = CurrentDepthMapValue - CurrentLayerDepth;
    float BeforeDepth = SampleHeightMap(PrevTexCoords) - CurrentLayerDepth + LayerDepth;

    float  Weight         = AfterDepth / (AfterDepth - BeforeDepth);
    float2 FinalTexCoords = PrevTexCoords * Weight + CurrentTexCoords * (1.0f - Weight);
    return FinalTexCoords;
}
#endif

FPSOutput PSMain(FPSInput Input)
{
    float2 TexCoords = Input.TexCoord;
    TexCoords.y = 1.0f - TexCoords.y;

#if ENABLE_PARALLAX_MAPPING
    float3 ViewDir = normalize(Input.TangentViewPos - Input.TangentPosition);
    TexCoords      = ParallaxMapping(TexCoords, ViewDir);
    if (TexCoords.x > 1.0 || TexCoords.y > 1.0 || TexCoords.x < 0.0 || TexCoords.y < 0.0)
    {
        discard;
    }
#endif

#if ENABLE_PACKED_MATERIAL_TEXTURE
    const float4 AlbedoAlphaMask = AlbedoAlphaMap.Sample(MaterialSampler, TexCoords);
#endif

#if ENABLE_ALPHA_MASK
    #if ENABLE_PACKED_MATERIAL_TEXTURE
        [[branch]]
        if (AlbedoAlphaMask.a < 0.5)
        {
            discard;
        }
    #else
        const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords);
        
        [[branch]]
        if (AlphaMask < 0.5)
        {
            discard;
        }
    #endif
#endif

#if ENABLE_PACKED_MATERIAL_TEXTURE
    float3 SampledAlbedo = ApplyGamma(AlbedoAlphaMask.rgb) * MaterialBuffer.Albedo;
#else
    float3 SampledAlbedo = ApplyGamma(AlbedoMap.Sample(MaterialSampler, TexCoords)) * MaterialBuffer.Albedo;
#endif

#if ENABLE_NORMAL_MAPPING
    float3 SampledNormal = NormalTex.Sample(MaterialSampler, TexCoords);
    SampledNormal = UnpackNormalBC5(SampledNormal);

    float3 Tangent   = normalize(Input.Tangent);
    float3 Bitangent = normalize(Input.Bitangent);
    float3 Normal    = normalize(Input.Normal);

    float3 OutputNormal = ApplyNormalMapping(SampledNormal, Normal, Tangent, Bitangent);
    OutputNormal = PackNormal(OutputNormal);
#else
    float3 OutputNormal = normalize(Input.Normal);
    OutputNormal = PackNormal(OutputNormal);
#endif

#if ENABLE_PACKED_MATERIAL_TEXTURE
    const float3 AO_Roughness_Metal = AO_Roughness_Metal_Tex.Sample(MaterialSampler, TexCoords);
    const float  SampledAO          = AO_Roughness_Metal.r * MaterialBuffer.AO;
    const float  SampledRoughness   = AO_Roughness_Metal.g * MaterialBuffer.Roughness;
    const float  SampledMetallic    = AO_Roughness_Metal.b * MaterialBuffer.Metallic;
#else
    const float SampledAO        = AOTex.Sample(MaterialSampler, TexCoords)        * MaterialBuffer.AO;
    const float SampledMetallic  = MetallicTex.Sample(MaterialSampler, TexCoords)  * MaterialBuffer.Metallic;
    const float SampledRoughness = RoughnessTex.Sample(MaterialSampler, TexCoords) * MaterialBuffer.Roughness;
#endif

    const float FinalRoughness = min(max(SampledRoughness, MIN_ROUGHNESS), MAX_ROUGHNESS);

    FPSOutput Output;
    Output.Albedo   = float4(SampledAlbedo, 1.0f);
    Output.Normal   = float4(OutputNormal, 1.0f);
    Output.Material = float4(FinalRoughness, SampledMetallic, SampledAO, 1.0f);

    const float3 PositionNDC     = (Input.ClipPosition.xyz / Input.ClipPosition.w);
    const float3 PrevPositionNDC = (Input.PrevClipPosition.xyz / Input.PrevClipPosition.w);
    Output.Velocity = (PositionNDC.xy - CameraBuffer.Jitter) - (PrevPositionNDC.xy - CameraBuffer.PrevJitter);
    return Output;
}
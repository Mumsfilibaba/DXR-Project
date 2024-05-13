#include "Structs.hlsli"
#include "Constants.hlsli"

#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING (0)
#endif

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK (0)
#endif

#ifndef ENABLE_PACKED_MATERIAL_TEXTURE
    #define ENABLE_PACKED_MATERIAL_TEXTURE (0)
#endif

// PerObject Constants
SHADER_CONSTANT_BLOCK_BEGIN
    FTransform Transform;
SHADER_CONSTANT_BLOCK_END

// Per Frame
ConstantBuffer<FCamera> CameraBuffer : register(b0);

// Per Object
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    ConstantBuffer<FMaterial> MaterialBuffer : register(b1);
    SamplerState MaterialSampler : register(s0);
#if ENABLE_ALPHA_MASK
    #if ENABLE_PACKED_MATERIAL_TEXTURE
        Texture2D<float4> AlbedoAlphaTex : register(t0);
    #else
        Texture2D<float> AlphaMaskTex : register(t0);
    #endif
#endif
#if ENABLE_PARALLAX_MAPPING
    Texture2D<float> HeightTex : register(t1);
#endif
#endif

// VertexShader

struct FVSInput
{
    float3 Position : POSITION0;
#if ENABLE_PARALLAX_MAPPING
    float3 Normal  : NORMAL0;
    float3 Tangent : TANGENT0;
#endif
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif
};

struct FVSOutput
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif
#if ENABLE_PARALLAX_MAPPING 
    float3 TangentViewPos  : TANGENTVIEWPOS0;
    float3 TangentPosition : TANGENTPOSITION0;
#endif
    float4 Position : SV_Position;
};

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output = (FVSOutput)0;

    const float4 WorldPosition = mul(float4(Input.Position, 1.0f), Constants.Transform.Transform);
    Output.Position = mul(WorldPosition, CameraBuffer.ViewProjection);

#if ENABLE_PARALLAX_MAPPING
    // Normal
    const float4x4 TransformInv = Constants.Transform.TransformInv;  
    float3 Normal  = normalize(mul(float4(Input.Normal, 0.0f), TransformInv).xyz);
    float3 Tangent = normalize(mul(float4(Input.Tangent, 0.0f), TransformInv).xyz);
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    float3 Bitangent = normalize(cross(Tangent, Normal));

    const float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
    Output.TangentViewPos  = mul(TBN, CameraBuffer.Position);
    Output.TangentPosition = mul(TBN, WorldPosition.xyz);
#endif

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    Output.TexCoord = Input.TexCoord;
#endif
    return Output;
}

// PixelShader

struct FPSInput
{
    float2 TexCoord : TEXCOORD0;
#if ENABLE_PARALLAX_MAPPING 
    float3 TangentViewPos  : TANGENTVIEWPOS0;
    float3 TangentPosition : TANGENTPOSITION0;
#endif
};

#if ENABLE_PARALLAX_MAPPING

// TODO: We probably do not want any constants like this, it should be a constantbuffer or something similar
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

void PSMain(FPSInput Input)
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
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

#if ENABLE_ALPHA_MASK
    #if ENABLE_PACKED_MATERIAL_TEXTURE
        const float AlphaMask = AlbedoAlphaTex.Sample(MaterialSampler, TexCoords).a;
        [[branch]]
        if (AlphaMask < 0.5)
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
#endif
}
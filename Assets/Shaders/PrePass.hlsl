#include "Structs.hlsli"
#include "Constants.hlsli"
#include "ParallaxMapping.hlsli"

#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING (0)
#endif

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK (0)
#endif

#ifndef USE_UNJITTERED_CAMERA
    #define USE_UNJITTERED_CAMERA (0)
#endif

#ifndef BINDLESS_PRE_PASS
    #define BINDLESS_PRE_PASS (0)
#endif

// Per Frame
ConstantBuffer<FCamera> CameraBuffer : register(b0);

// Per Object
ConstantBuffer<FTransform> TransformBuffer : register(b1);

// Per Object
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    ConstantBuffer<FMaterial> MaterialBuffer : register(b1);
#if BINDLESS_PRE_PASS
    #define MATERIAL_BINDLESS_REGISTER b2
    #include "MaterialBindless.hlsli"
#else
    SamplerState MaterialSampler : register(s0);
#if ENABLE_ALPHA_MASK
    Texture2D<float4> AlbedoAlphaTex : register(t0);
#endif // ENABLE_ALPHA_MASK
#if ENABLE_PARALLAX_MAPPING
    Texture2D<float> HeightTex : register(t1);
#endif // ENABLE_PARALLAX_MAPPING
#endif // BINDLESS_PRE_PASS
#endif // ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING

// VertexShader

struct FVSInput
{
    float3 Position : POSITION0;
#if ENABLE_PARALLAX_MAPPING
    float3 Normal  : NORMAL0;
    float3 Tangent : TANGENT0;
#endif // ENABLE_PARALLAX_MAPPING
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif // ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
};

struct FVSOutput
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif // ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
#if ENABLE_PARALLAX_MAPPING 
    float3 TangentViewPos  : TANGENTVIEWPOS0;
    float3 TangentPosition : TANGENTPOSITION0;
#endif // ENABLE_PARALLAX_MAPPING
    float4 Position : SV_Position;
};

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output;

    // Position
    const float3 PositionWS3 = TransformPositionWS(TransformBuffer, Input.Position); 
    const float4 PositionWS  = float4(PositionWS3, 1.0);
#if USE_UNJITTERED_CAMERA
    Output.Position = mul(PositionWS, CameraBuffer.ViewProjectionUnjittered);
#else
    Output.Position = mul(PositionWS, CameraBuffer.ViewProjection);
#endif // USE_UNJITTERED_CAMERA

    // Normal
#if ENABLE_PARALLAX_MAPPING
    float3 Normal  = normalize(TransformDirectionInvT(TransformBuffer, Input.Normal));
    float3 Tangent = normalize(TransformDirectionInvT(TransformBuffer, Input.Tangent));
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    float3 Bitangent = normalize(cross(Tangent, Normal));

    const float3x3 TangentSpace = float3x3(Tangent, Bitangent, Normal);
    Output.TangentViewPos  = mul(TangentSpace, CameraBuffer.PositionWS);
    Output.TangentPosition = mul(TangentSpace, PositionWS3);
#endif // ENABLE_PARALLAX_MAPPING

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    Output.TexCoord = Input.TexCoord;
#endif // ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING

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

void PSMain(FPSInput Input)
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoords = Input.TexCoord;

#if ENABLE_PARALLAX_MAPPING
    TexCoords.y = 1.0 - TexCoords.y;

    const float2 TexCoordsDx = ddx(TexCoords);
    const float2 TexCoordsDy = ddy(TexCoords);

    float3 ViewDir = normalize(Input.TangentViewPos - Input.TangentPosition);

    uint bParallaxDiscard = 0;
#if BINDLESS_PRE_PASS
    TexCoords = ParallaxMapUV(GetHeightBindless(), GetMaterialSamplerBindless(), TexCoords, ViewDir, TexCoordsDx, TexCoordsDy, MaterialBuffer.ParallaxHeightScale, MaterialBuffer.ParallaxMinLayers, MaterialBuffer.ParallaxMaxLayers, bParallaxDiscard);
#else
    TexCoords = ParallaxMapUV(HeightTex, MaterialSampler, TexCoords, ViewDir, TexCoordsDx, TexCoordsDy, MaterialBuffer.ParallaxHeightScale, MaterialBuffer.ParallaxMinLayers, MaterialBuffer.ParallaxMaxLayers, bParallaxDiscard);
#endif // BINDLESS_PRE_PASS
    if (bParallaxDiscard != 0)
    {
        discard;
    }
#endif // ENABLE_PARALLAX_MAPPING

#if ENABLE_ALPHA_MASK
#if BINDLESS_PRE_PASS
    const float AlphaMask = GetAlbedoBindless().Sample(GetMaterialSamplerBindless(), TexCoords).a;
#else
    const float AlphaMask = AlbedoAlphaTex.Sample(MaterialSampler, TexCoords).a;
#endif // BINDLESS_PRE_PASS
    [[branch]]
    if (AlphaMask < 0.5)
    {
        discard;
    }
#endif // ENABLE_ALPHA_MASK
#endif // ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
}

#include "Structs.hlsli"
#include "TransformHelpers.hlsli"
#include "Constants.hlsli"
#include "ParallaxMapping.hlsli"
#include "TangentSpace.hlsli"

#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING (0)
#endif

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK (0)
#endif

#ifndef ENABLE_DOUBLE_SIDED
    #define ENABLE_DOUBLE_SIDED (0)
#endif

#ifndef USE_UNJITTERED_CAMERA
    #define USE_UNJITTERED_CAMERA (0)
#endif

#ifndef BINDLESS_PRE_PASS
    #define BINDLESS_PRE_PASS (0)
#endif

ConstantBuffer<FCamera>    CameraBuffer    : register(b0);
ConstantBuffer<FPerObject> PerObjectBuffer : register(b1);

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    #if ENABLE_PARALLAX_MAPPING || (BINDLESS_PRE_PASS && ENABLE_ALPHA_MASK)
        #define MATERIAL_ARRAY_REGISTER t2
        #include "MaterialArray.hlsli"
    #endif

    #if BINDLESS_PRE_PASS
        #include "MaterialBindless.hlsli"
    #else
        SamplerState MaterialSampler : register(s0);
        
        #if ENABLE_ALPHA_MASK
            Texture2D<float4> AlbedoAlphaTex : register(t0);
        #endif
        #if ENABLE_PARALLAX_MAPPING
            Texture2D<float> HeightTex : register(t1);
        #endif
    #endif
#endif

// ------------------------------------------------------------------------------------------------
// VertexShader
// ------------------------------------------------------------------------------------------------

struct FVSInput
{
    float3 Position : POSITION0;

#if ENABLE_PARALLAX_MAPPING
    float3 Normal  : NORMAL0;
    float4 Tangent : TANGENT0;
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
    float3 Normal     : NORMAL0;
    float4 Tangent    : TANGENT0;
    float3 PositionWS : POSITION0;
#endif

    float4 Position : SV_Position;
};

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output;

    // Position
    const float3 PositionWS3 = TransformPositionWS(PerObjectBuffer, Input.Position); 
    const float4 PositionWS  = float4(PositionWS3, 1.0);

#if USE_UNJITTERED_CAMERA
    Output.Position = mul(PositionWS, CameraBuffer.ViewProjectionUnjittered);
#else
    Output.Position = mul(PositionWS, CameraBuffer.ViewProjection);
#endif

#if ENABLE_PARALLAX_MAPPING
    Output.Normal     = TransformDirectionInvT(PerObjectBuffer, Input.Normal);
    Output.Tangent    = float4(TransformDirectionWS(PerObjectBuffer, Input.Tangent.xyz), Input.Tangent.w * PerObjectBuffer.DeterminantSign);
    Output.PositionWS = PositionWS3;
#endif

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    Output.TexCoord = Input.TexCoord;
#endif

    return Output;
}

// ------------------------------------------------------------------------------------------------
// PixelShader
// ------------------------------------------------------------------------------------------------

struct FPSInput
{
    float2 TexCoord : TEXCOORD0;
    
#if ENABLE_PARALLAX_MAPPING 
    float3 Normal     : NORMAL0;
    float4 Tangent    : TANGENT0;
    float3 PositionWS : POSITION0;

    #if ENABLE_DOUBLE_SIDED
        bool bIsFrontFace : SV_IsFrontFace;
    #endif
#endif
};

void PSMain(FPSInput Input)
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoords = Input.TexCoord;

#if ENABLE_PARALLAX_MAPPING || (BINDLESS_PRE_PASS && ENABLE_ALPHA_MASK)
    const FMaterial MaterialData = Materials[PerObjectBuffer.MaterialIndex];
#endif

#if ENABLE_PARALLAX_MAPPING
    float3 SurfaceNormal = Input.Normal;
    float  TangentSign   = Input.Tangent.w;

    #if ENABLE_DOUBLE_SIDED
        if (!Input.bIsFrontFace)
        {
            SurfaceNormal = -SurfaceNormal;
            TangentSign   = -TangentSign;
        }
    #endif

    const float2   TexCoordsDx    = ddx(TexCoords);
    const float2   TexCoordsDy    = ddy(TexCoords);
    const float3x3 WorldToTangent = CreateWorldToTangent(SurfaceNormal, Input.Tangent.xyz, TangentSign);
    const float3   ViewDir        = normalize(mul(WorldToTangent, CameraBuffer.PositionWS - Input.PositionWS));

    bool bParallaxDiscard = false;
#if BINDLESS_PRE_PASS
    TexCoords = ParallaxMapUV(GetHeightBindless(MaterialData), GetMaterialSamplerBindless(MaterialData), TexCoords, ViewDir, TexCoordsDx, TexCoordsDy, MaterialData.ParallaxHeightScale, MaterialData.ParallaxMinLayers, MaterialData.ParallaxMaxLayers, bParallaxDiscard);
#else
    TexCoords = ParallaxMapUV(HeightTex, MaterialSampler, TexCoords, ViewDir, TexCoordsDx, TexCoordsDy, MaterialData.ParallaxHeightScale, MaterialData.ParallaxMinLayers, MaterialData.ParallaxMaxLayers, bParallaxDiscard);
#endif
    #if ENABLE_PARALLAX_CLIPPING
        if (bParallaxDiscard)
        {
            discard;
        }
    #endif
#endif

#if ENABLE_ALPHA_MASK
    #if BINDLESS_PRE_PASS
        const float AlphaMask = GetAlbedoBindless(MaterialData).Sample(GetMaterialSamplerBindless(MaterialData), TexCoords).a;
    #else
        const float AlphaMask = AlbedoAlphaTex.Sample(MaterialSampler, TexCoords).a;
    #endif
        [[branch]]
        if (AlphaMask < 0.5)
        {
            discard;
        }
    #endif
#endif
}

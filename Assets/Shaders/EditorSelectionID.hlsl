#include "Structs.hlsli"
#include "Constants.hlsli"
#include "ParallaxMapping.hlsli"

#ifndef USE_UNJITTERED_CAMERA
    #define USE_UNJITTERED_CAMERA (1)
#endif

#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING (0)
#endif

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK (0)
#endif

// Per Frame
ConstantBuffer<FCamera> CameraBuffer : register(b0);

// Per Object
ConstantBuffer<FTransform> TransformBuffer : register(b1);

// Per Object
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    ConstantBuffer<FMaterial> MaterialBuffer : register(b2);
    SamplerState MaterialSampler : register(s0);
#if ENABLE_ALPHA_MASK
    Texture2D<float4> AlbedoAlphaTex : register(t0);
#endif
#if ENABLE_PARALLAX_MAPPING
    Texture2D<float> HeightTex : register(t1);
#endif
#endif

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
    FVSOutput Output;

    const float3 PositionWS3 = TransformPositionWS(TransformBuffer, Input.Position); 
    const float4 PositionWS  = float4(PositionWS3, 1.0); 
#if USE_UNJITTERED_CAMERA
    Output.Position = mul(PositionWS, CameraBuffer.ViewProjectionUnjittered);
#else
    Output.Position = mul(PositionWS, CameraBuffer.ViewProjection);
#endif
  
#if ENABLE_PARALLAX_MAPPING 
    float3 Normal  = normalize(TransformDirectionInvT(TransformBuffer, Input.Normal));
    float3 Tangent = normalize(TransformDirectionInvT(TransformBuffer, Input.Tangent));
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    float3 Bitangent = normalize(cross(Tangent, Normal));

    const float3x3 TangentSpace = float3x3(Tangent, Bitangent, Normal);
    Output.TangentViewPos  = mul(TangentSpace, CameraBuffer.PositionWS);
    Output.TangentPosition = mul(TangentSpace, PositionWS3);
#endif

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    Output.TexCoord = Input.TexCoord;
#endif

    return Output;
}

struct FPSInput 
{ 
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING 
    float2 TexCoord : TEXCOORD0; 
#endif 
#if ENABLE_PARALLAX_MAPPING 
    float3 TangentViewPos  : TANGENTVIEWPOS0; 
    float3 TangentPosition : TANGENTPOSITION0; 
#endif 
}; 

uint PSMain(FPSInput Input) : SV_Target0 
{ 
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoords = Input.TexCoord;

#if ENABLE_PARALLAX_MAPPING
    TexCoords.y = 1.0 - TexCoords.y;

    const float2 TexCoordsDx = ddx(TexCoords);
    const float2 TexCoordsDy = ddy(TexCoords);

    float3 ViewDir = normalize(Input.TangentViewPos - Input.TangentPosition);

    uint bParallaxDiscard = 0;
    TexCoords = ParallaxMapUV(HeightTex, MaterialSampler, TexCoords, ViewDir, TexCoordsDx, TexCoordsDy, MaterialBuffer.ParallaxHeightScale, MaterialBuffer.ParallaxMinLayers, MaterialBuffer.ParallaxMaxLayers, bParallaxDiscard);
    if (bParallaxDiscard != 0)
    {
        discard;
    }
    
#endif

#if ENABLE_ALPHA_MASK
    const float AlphaMask = AlbedoAlphaTex.Sample(MaterialSampler, TexCoords).a;
    [[branch]]
    if (AlphaMask < 0.5)
    {
        discard;
    }
#endif
#endif 
 
    return TransformBuffer.ObjectID; 
} 

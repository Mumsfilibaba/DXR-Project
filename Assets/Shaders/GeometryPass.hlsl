#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "ColorSpaceTransforms.hlsli"
#include "FastMath.hlsli"
#include "ParallaxMapping.hlsli"

#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING (0)
#endif

#ifndef ENABLE_NORMAL_MAPPING
    #define ENABLE_NORMAL_MAPPING (0)
#endif

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK (0)
#endif

#ifndef ENABLE_DOUBLE_SIDED
    #define ENABLE_DOUBLE_SIDED (1)
#endif

// PerFrame
ConstantBuffer<FCamera> CameraBuffer : register(b0);

// PerObject Samplers
SamplerState MaterialSampler : register(s0);

ConstantBuffer<FTransform> TransformBuffer : register(b1);
ConstantBuffer<FMaterial>  MaterialBuffer  : register(b1);

// Unified texture layout: AlbedoMap (RGBA), NormalMap, MaterialMap (R=AO, G=Roughness, B=Metallic), HeightMap
Texture2D<float4> AlbedoMap   : register(t0);
#if ENABLE_NORMAL_MAPPING
Texture2D<float3> NormalTex   : register(t1);
#endif
Texture2D<float3> MaterialMap : register(t2);
#if ENABLE_PARALLAX_MAPPING
Texture2D<float>  HeightTex   : register(t3);
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
    float3 Normal           : NORMAL0;
    float3 Tangent          : TANGENT0;
    float3 Bitangent        : BITANGENT0;
    float2 TexCoord	        : TEXCOORD0;
#if ENABLE_PARALLAX_MAPPING 
    float3 TangentViewPos   : TANGENTVIEWPOS0;
    float3 TangentPosition  : TANGENTPOSITION0;
#endif
    float3 PositionWS       : POSITION0;
    float4 ClipPosition     : POSITION1;
    float4 PrevClipPosition : POSITION2;
    float4 Position         : SV_Position;
};

FVSOutput VSMain(FVSInput Input)
{
    // Position
    const float3 PositionWS3 = TransformPositionWS(TransformBuffer, Input.Position);
    const float4 PositionWS  = float4(PositionWS3, 1.0);

    // Normal
    float3 Normal = normalize(TransformDirectionInvT(TransformBuffer, Input.Normal));

    // Tangent 
    float3 Tangent = normalize(TransformDirectionInvT(TransformBuffer, Input.Tangent));
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    
    // Bitangent 
    float3 Bitangent = normalize(cross(Tangent, Normal));

    FVSOutput Output;
    Output.Normal           = Normal;
    Output.Tangent          = Tangent;
    Output.Bitangent        = Bitangent;
    Output.Position         = mul(PositionWS, CameraBuffer.ViewProjection);
    Output.PositionWS       = PositionWS3;

    // TODO: Handle moving objects (aka PrevTransform)
    Output.ClipPosition     = Output.Position;
    Output.PrevClipPosition = mul(PositionWS, CameraBuffer.PrevViewProjection);
    Output.TexCoord         = Input.TexCoord;

#if ENABLE_PARALLAX_MAPPING
    const float3x3 TangentSpace = float3x3(Tangent, Bitangent, Normal);
    Output.TangentViewPos  = mul(TangentSpace, CameraBuffer.PositionWS);
    Output.TangentPosition = mul(TangentSpace, PositionWS.xyz);
#endif

    return Output;
}

// PixelShader

struct FPSInput
{
    float3 Normal           : NORMAL0;
    float3 Tangent          : TANGENT0;
    float3 Bitangent        : BITANGENT0;
    float2 TexCoord         : TEXCOORD0;
#if ENABLE_PARALLAX_MAPPING
    float3 TangentViewPos   : TANGENTVIEWPOS0;
    float3 TangentPosition  : TANGENTPOSITION0;
#endif
    float3 PositionWS       : POSITION0;
    float4 ClipPosition     : POSITION1;
    float4 PrevClipPosition : POSITION2;
    float4 Position         : SV_Position;
};

struct FPSOutput
{
    float4 Albedo   : SV_Target0;
    float4 Normal   : SV_Target1;
    float4 Material : SV_Target2;
    float2 Velocity : SV_Target3;
};

FPSOutput PSMain(FPSInput Input)
{
    float2 TexCoords = Input.TexCoord;

    // Handle parallax mapping
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

    // Sample albedo (alpha in .a channel)
    const float4 AlbedoSample = AlbedoMap.Sample(MaterialSampler, TexCoords);

#if ENABLE_ALPHA_MASK
    [[branch]]
    if (AlbedoSample.a < 0.5)
    {
        discard;
    }
#endif

    float3 Albedo = SRGBToLinear(AlbedoSample.rgb) * MaterialBuffer.Albedo;

    // Sample normal
#if ENABLE_NORMAL_MAPPING
    float3 SampledNormal = NormalTex.Sample(MaterialSampler, TexCoords);
    SampledNormal = UnpackNormalBC5(SampledNormal);

    // Ensure Tangent frame is orthogonal
    float3 Tangent   = normalize(Input.Tangent);
    float3 Bitangent = normalize(Input.Bitangent);
    float3 Normal    = normalize(Input.Normal);

    Normal = ApplyNormalMapping(SampledNormal, Normal, Tangent, Bitangent);
#else
    float3 Normal = normalize(Input.Normal);
#endif

#if ENABLE_DOUBLE_SIDED
    {
        // Check if the triangle is back-facing (based on the direction of the normal)
        float3 ViewDir = normalize(CameraBuffer.PositionWS - Input.PositionWS);
        
        float Facing = dot(Normal, ViewDir);
        // Facing = Facing >= 0.0 ? 1.0 : -1.0;

        // If facing is negative, the triangle is back-facing.
        Normal = normalize(Normal * Facing);
    }
#endif

    // Pack the normal and prepare for output
    Normal = PackNormal(Normal);

    // Sample material params from packed materialparam texture (R=AO, G=Roughness, B=Metallic)
    const float3 MaterialParams = MaterialMap.Sample(MaterialSampler, TexCoords);
    const float  Occlusion      = MaterialParams.r * MaterialBuffer.AO;
    float        Roughness      = MaterialParams.g * MaterialBuffer.Roughness;
    const float  Metallic       = MaterialParams.b * MaterialBuffer.Metallic;

    // Specular anti-aliasing
    {
        static const float Strength         = 1.0;
        static const float MaxRoughnessGain = 0.02;

        float  Roughness2         = Roughness * Roughness;
        float3 DnDu               = ddx(Normal);
        float3 DnDv               = ddy(Normal);
        float  Variance           = (dot(DnDu, DnDu) + dot(DnDv, DnDv));
        float  KernelRoughness2   = min(Variance * Strength, MaxRoughnessGain);
        float  FilteredRoughness2 = saturate(Roughness2 + KernelRoughness2);
        
        Roughness = FastSqrt(FilteredRoughness2);
    }

    // Ensure we do not go above or below a certain roughness threshold
    Roughness = min(max(Roughness, MIN_ROUGHNESS), MAX_ROUGHNESS);

    // Velocity
    float3 PositionNDC     = (Input.ClipPosition.xyz / Input.ClipPosition.w);
    float3 PrevPositionNDC = (Input.PrevClipPosition.xyz / Input.PrevClipPosition.w);
    float2 Velocity        = (PositionNDC.xy - CameraBuffer.Jitter) - (PrevPositionNDC.xy - CameraBuffer.PrevJitter);

    // Output
    FPSOutput Output;
    Output.Albedo   = float4(Albedo, 1.0);
    Output.Normal   = float4(Normal, 1.0);
    Output.Material = float4(Roughness, Metallic, Occlusion, 1.0);
    Output.Velocity = Velocity;

    return Output;
}

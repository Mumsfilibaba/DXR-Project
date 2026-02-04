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

#ifndef ENABLE_PACKED_MATERIAL_TEXTURE
    #define ENABLE_PACKED_MATERIAL_TEXTURE (0)
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
    Texture2D<float> RoughnessTex : register(t2);
    Texture2D<float> MetallicTex : register(t3);
    Texture2D<float> AOTex : register(t4);
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

    float3 PositionWS       : POSITION0;
    float4 ClipPosition     : POSITION1;
    float4 PrevClipPosition : POSITION2;
    float4 Position         : SV_Position;
};

FVSOutput VSMain(FVSInput Input)
{
    // Position
    const float3 PositionWS3 = TransformPositionWS(Constants.Transform, Input.Position);
    const float4 PositionWS  = float4(PositionWS3, 1.0);

    // Normal
    float3 Normal = normalize(TransformDirectionInvT(Constants.Transform, Input.Normal));

    // Tangent 
    float3 Tangent = normalize(TransformDirectionInvT(Constants.Transform, Input.Tangent));
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
    float3 Normal    : NORMAL0;
    float3 Tangent   : TANGENT0;
    float3 Bitangent : BITANGENT0;
    float2 TexCoord  : TEXCOORD0;

#if ENABLE_PARALLAX_MAPPING
    float3 TangentViewPos  : TANGENTVIEWPOS0;
    float3 TangentPosition : TANGENTPOSITION0;
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

    // If we are using a packed albedo texture, sample it here 
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

    // Sample albedo
#if ENABLE_PACKED_MATERIAL_TEXTURE
    float3 Albedo = SRGBToLinear(AlbedoAlphaMask.rgb);
#else
    float3 Albedo = SRGBToLinear(AlbedoMap.Sample(MaterialSampler, TexCoords));
#endif
    Albedo *= MaterialBuffer.Albedo;

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

    // Sample material params
#if ENABLE_PACKED_MATERIAL_TEXTURE
    const float3 AO_Roughness_Metal = AO_Roughness_Metal_Tex.Sample(MaterialSampler, TexCoords);
    float Occlusion = AO_Roughness_Metal.r;
    float Roughness = AO_Roughness_Metal.g;
    float Metallic  = AO_Roughness_Metal.b;
#else
    float Occlusion = AOTex.Sample(MaterialSampler, TexCoords);
    float Metallic  = MetallicTex.Sample(MaterialSampler, TexCoords);
    float Roughness = RoughnessTex.Sample(MaterialSampler, TexCoords);
#endif

    Occlusion *= MaterialBuffer.AO;
    Roughness *= MaterialBuffer.Roughness;
    Metallic  *= MaterialBuffer.Metallic;

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

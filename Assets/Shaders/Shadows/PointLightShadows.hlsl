#include "Structs.hlsli"
#include "TransformHelpers.hlsli"
#include "Constants.hlsli"
#include "ParallaxMapping.hlsli"
#include "TangentSpace.hlsli"

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK 0
#endif
#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING 0
#endif

#define NUM_CUBE_FACES 6

// Mirrors ECubeMapRenderPassType.
#define POINTLIGHT_PASS_UNKNOWN     0
#define POINTLIGHT_PASS_MULTI       1
#define POINTLIGHT_PASS_SINGLE      2
#define POINTLIGHT_PASS_GEOMETRY    3

#ifndef POINTLIGHT_PASS_KIND
    #define POINTLIGHT_PASS_KIND POINTLIGHT_PASS_MULTI
#endif

#define ENABLE_POINTLIGHT_VS_INSTANCING (POINTLIGHT_PASS_KIND == POINTLIGHT_PASS_SINGLE)
#define ENABLE_POINTLIGHT_GS_INSTANCING (POINTLIGHT_PASS_KIND == POINTLIGHT_PASS_GEOMETRY)

#if !ENABLE_POINTLIGHT_VS_INSTANCING && !ENABLE_POINTLIGHT_GS_INSTANCING
    #define ENABLE_POINTLIGHT_MULTI_PASS 1
#endif

// Per-object
ConstantBuffer<FPerObject> PerObjectBuffer : register(b1);

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    #define MATERIAL_SAMPLE_NORMAL     (0)
    #define MATERIAL_SRV_REGISTER_BASE 0
    #define MATERIAL_ARRAY_REGISTER    t7
    #include "MaterialSampling.hlsli"
#endif

struct FVSInput
{
    float3 Position : POSITION0;

#if HAS_VERTEX_TANGENT_BASIS
    float4 Normal  : NORMAL0;
    float4 Tangent : TANGENT0;
#endif

#if HAS_VERTEX_TEXCOORD0
    float2 TexCoord : TEXCOORD0;
#endif

#if ENABLE_POINTLIGHT_VS_INSTANCING
    uint InstanceID : SV_InstanceID;
#endif
};

// ------------------------------------------------------------------------------------------------
// VertexShader
// ------------------------------------------------------------------------------------------------

#if ENABLE_POINTLIGHT_VS_INSTANCING || ENABLE_POINTLIGHT_GS_INSTANCING
struct FSinglePassPointLightBuffer
{
    float4x4 LightProjections[NUM_CUBE_FACES];
    float3   LightPosition;
    float    LightFarPlane;
};

ConstantBuffer<FSinglePassPointLightBuffer> PointLightBuffer : register(b0);
#else
struct FPointLightBuffer
{
    float4x4 LightProjection;
    float3   LightPosition;
    float    LightFarPlane;
};

ConstantBuffer<FPointLightBuffer> PointLightBuffer : register(b0);
#endif // ENABLE_POINTLIGHT_VS_INSTANCING || ENABLE_POINTLIGHT_GS_INSTANCING

struct FVSPointOutput
{
    float3 WorldPosition : POSITION0;
    
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif

#if ENABLE_PARALLAX_MAPPING
    float3 Normal  : NORMAL0;
    float4 Tangent : TANGENT0;
#endif

#if !ENABLE_POINTLIGHT_GS_INSTANCING
    float4 Position : SV_Position;
#endif

#if ENABLE_POINTLIGHT_VS_INSTANCING || ENABLE_VIEW_INSTANCING_WORK_AROUND
    uint RenderTargetArrayIndex : SV_RenderTargetArrayIndex;
#endif
};

FVSPointOutput Point_VSMain(FVSInput Input)
{
    FVSPointOutput Output = (FVSPointOutput)0;

    const float3 WorldPositionWS = TransformPositionWS(PerObjectBuffer, Input.Position);
    const float4 WorldPosition   = float4(WorldPositionWS, 1.0f);
    Output.WorldPosition = WorldPosition.xyz;

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    Output.TexCoord = Input.TexCoord;
#endif

#if ENABLE_PARALLAX_MAPPING
    Output.Normal  = TransformDirectionInvT(PerObjectBuffer, Input.Normal.xyz);
    Output.Tangent = float4(TransformDirectionWS(PerObjectBuffer, Input.Tangent.xyz), Input.Tangent.w * PerObjectBuffer.DeterminantSign);
#endif

// Vertex-Shader instancing
#if ENABLE_POINTLIGHT_VS_INSTANCING
    const uint FaceIndex = clamp(Input.InstanceID, 0, 5);
    Output.Position = mul(WorldPosition, PointLightBuffer.LightProjections[FaceIndex]);
    Output.RenderTargetArrayIndex = FaceIndex;
#endif

// Multi-pass (One pass per face)
#if ENABLE_POINTLIGHT_MULTI_PASS
    Output.Position = mul(WorldPosition, PointLightBuffer.LightProjection);
#endif

    return Output;
}

// ------------------------------------------------------------------------------------------------
// Geometry-Shader
// ------------------------------------------------------------------------------------------------

// NOTE: For some reason it seems like this part of the shader is always compiled, so disable it to avoid compilation errors
#if ENABLE_POINTLIGHT_GS_INSTANCING
struct FGSPointOutput
{
    float3 WorldPosition : POSITION0;

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif

#if ENABLE_PARALLAX_MAPPING
    float3 Normal  : NORMAL0;
    float4 Tangent : TANGENT0;
#endif

    float4 Position : SV_Position;
    uint   RenderTargetViewIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(NUM_CUBE_FACES * 3)]
void Point_GSMain(triangle FVSPointOutput Input[3], inout TriangleStream<FGSPointOutput> OutStream)
{
    [unroll]
    for (int FaceIndex = 0; FaceIndex < NUM_CUBE_FACES; FaceIndex++)
    {
        FGSPointOutput Output;
        Output.RenderTargetViewIndex = FaceIndex;

        const float4x4 LightViewProjection = PointLightBuffer.LightProjections[FaceIndex];

        [unroll]
        for(int Vertex = 0; Vertex < 3; Vertex++)
        {
        #if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
            Output.TexCoord = Input[Vertex].TexCoord;
        #endif

        #if ENABLE_PARALLAX_MAPPING
            Output.Normal  = Input[Vertex].Normal;
            Output.Tangent = Input[Vertex].Tangent;
        #endif

            Output.WorldPosition = Input[Vertex].WorldPosition;
            Output.Position      = mul(float4(Input[Vertex].WorldPosition, 1.0f), LightViewProjection);

            OutStream.Append(Output);
        }

        OutStream.RestartStrip();
    }
}
#endif // ENABLE_POINTLIGHT_GS_INSTANCING

// ------------------------------------------------------------------------------------------------
// PixelShader
// ------------------------------------------------------------------------------------------------

struct FPSPointInput
{
    float3 WorldPosition : POSITION0;

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif

#if ENABLE_PARALLAX_MAPPING
    float3 Normal  : NORMAL0;
    float4 Tangent : TANGENT0;
#endif
};

float Point_PSMain(FPSPointInput Input) : SV_DepthLessEqual
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoords = Input.TexCoord;

    const FMaterial MaterialData = Materials[PerObjectBuffer.MaterialIndex];

    #if ENABLE_PARALLAX_MAPPING
    {
        const float2   TexCoordsDx    = ddx(TexCoords);
        const float2   TexCoordsDy    = ddy(TexCoords);
        const float3x3 WorldToTangent = CreateWorldToTangent(Input.Normal, Input.Tangent.xyz, Input.Tangent.w);
        const float3   LightDir       = normalize(mul(WorldToTangent, PointLightBuffer.LightPosition - Input.WorldPosition));

        bool bParallaxDiscard = false;
        TexCoords = ApplyMaterialParallax(MaterialData, TexCoords, LightDir, TexCoordsDx, TexCoordsDy, bParallaxDiscard);
        #if ENABLE_PARALLAX_CLIPPING
            if (bParallaxDiscard)
            {
                discard;
            }
        #endif
    }
    #endif

    #if ENABLE_ALPHA_MASK 
        [[branch]]
        if (SampleMaterialOpacity(MaterialData, TexCoords) < 0.5)
        {
            discard;
        }
    #endif
#endif

    const float LightDistance = length(Input.WorldPosition.xyz - PointLightBuffer.LightPosition) / PointLightBuffer.LightFarPlane;
    return LightDistance;
}

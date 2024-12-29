#include "Constants.hlsli"
#include "Structs.hlsli"

#ifndef ENABLE_PACKED_MATERIAL_TEXTURE
    #define ENABLE_PACKED_MATERIAL_TEXTURE (0)
#endif
#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK (0)
#endif
#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING (0)
#endif

// Cascaded-Shadow-Maps defines
#ifndef MAX_CASCADES
    #define MAX_CASCADES (4)
#endif
#ifndef ENABLE_CASCADE_VS_INSTANCING
    #define ENABLE_CASCADE_VS_INSTANCING (0)
#endif
#ifndef ENABLE_CASCADE_GS_INSTANCING
    #define ENABLE_CASCADE_GS_INSTANCING (0)
#endif
#ifndef ENABLE_CASCADE_VIEW_INSTANCING
    #define ENABLE_CASCADE_VIEW_INSTANCING (0)
#endif
#if !ENABLE_CASCADE_VS_INSTANCING && !ENABLE_CASCADE_GS_INSTANCING && !ENABLE_CASCADE_VIEW_INSTANCING
    #define ENABLE_CASCADE_MULTI_PASS (1)
#endif

// PointLight defines
#define NUM_CUBE_FACES (6)
#define NUM_CUBE_FACES_HALF (3)

#ifndef ENABLE_POINTLIGHT_VS_INSTANCING
    #define ENABLE_POINTLIGHT_VS_INSTANCING (0)
#endif
#ifndef ENABLE_POINTLIGHT_GS_INSTANCING
    #define ENABLE_POINTLIGHT_GS_INSTANCING (0)
#endif
#if !ENABLE_POINTLIGHT_VS_INSTANCING && !ENABLE_POINTLIGHT_GS_INSTANCING
    #define ENABLE_POINTLIGHT_MULTI_PASS (1)
#endif

// NOTE: This is a workaround for NVIDIA using D3D12, for some reason we have to write to SV_RenderTargetArrayIndex
// and have the RenderTargetArrayIndex inside the PSO to be set to BaseLayer which then gets offset by using SV_RenderTargetArrayIndex
#if ENABLE_CASCADE_VIEW_INSTANCING && SHADER_LANG == SHADER_LANG_HLSL
    #define ENABLE_VIEW_INSTANCING_WORK_AROUND (1)
#endif

struct FPerCascade
{
    int CascadeIndex;
    int Padding0;
    int Padding1;
    int Padding2;
};

// Per-object
SHADER_CONSTANT_BLOCK_BEGIN
    float4x4 WorldModelMatrix;
SHADER_CONSTANT_BLOCK_END

#if SHADER_LANG == SHADER_LANG_MSL
    ConstantBuffer<FPerCascade> PerCascadeBuffer : register(b2);
#else
    ConstantBuffer<FPerCascade> PerCascadeBuffer : register(b0);
#endif

StructuredBuffer<FCascadeMatrices> CascadeMatrixBuffer : register(t0);

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    SamplerState MaterialSampler : register(s0);

    ConstantBuffer<FMaterial> MaterialBuffer : register(b1);

    #if ENABLE_ALPHA_MASK
        #if ENABLE_PACKED_MATERIAL_TEXTURE
            Texture2D<float4> AlphaMaskTex : register(t0);
        #else
            Texture2D<float> AlphaMaskTex : register(t0);
        #endif
    #endif

    #if ENABLE_PARALLAX_MAPPING
        Texture2D<float> HeightMap : register(t1);
    #endif
#endif

struct FVSInput
{
    float3 Position : POSITION0;

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif

// For view-instancing
#if ENABLE_CASCADE_VIEW_INSTANCING
    uint ViewID : SV_ViewID;
#endif

// For vertex-shader instancing
#if ENABLE_CASCADE_VS_INSTANCING || ENABLE_POINTLIGHT_VS_INSTANCING
    uint InstanceID : SV_InstanceID;
#endif
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Cascade Shadow Generation

// VertexShader

struct FVSCascadeOutput
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif

// For geometry-shader instancing we output the worldposition to GS otherwise we want to output final position directly
#if ENABLE_CASCADE_GS_INSTANCING
    float4 WorldPosition : POSITION0;
#else
    float4 Position : SV_Position;
#endif

// For vertex-shader instancing, we write directly what layer we want to write to
#if ENABLE_CASCADE_VS_INSTANCING || ENABLE_VIEW_INSTANCING_WORK_AROUND
    uint RenderTargetArrayIndex : SV_RenderTargetArrayIndex;
#endif
};

FVSCascadeOutput Cascade_VSMain(FVSInput Input)
{
    FVSCascadeOutput Output;

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING 
    Output.TexCoord = Input.TexCoord;
#endif

    const float4 WorldPosition = mul(float4(Input.Position, 1.0f), Constants.WorldModelMatrix);

// Geometry shader instancing
#if ENABLE_CASCADE_GS_INSTANCING
    Output.WorldPosition = WorldPosition;
#else

// View-instancing
#if ENABLE_CASCADE_VIEW_INSTANCING
    const int CascadeIndex = min(Input.ViewID, MAX_CASCADES - 1);
// Vertex-shader instancing
#elif ENABLE_CASCADE_VS_INSTANCING
    const int CascadeIndex = min(Input.InstanceID, MAX_CASCADES - 1);
// Regular multi-pass
#elif ENABLE_CASCADE_MULTI_PASS
    const int CascadeIndex = min(PerCascadeBuffer.CascadeIndex, MAX_CASCADES - 1);
#endif

// Work-around using HLSL (Otherwise it does not work on NVIDIA hardware)
#if ENABLE_CASCADE_VS_INSTANCING || ENABLE_VIEW_INSTANCING_WORK_AROUND
    Output.RenderTargetArrayIndex = CascadeIndex;
#endif

    // Unless we use a geometry-shader we transform the world-position here
    const float4x4 LightViewProjection = CascadeMatrixBuffer[CascadeIndex].ViewProj;
    Output.Position = mul(WorldPosition, LightViewProjection);
#endif // ENABLE_CASCADE_GS_INSTANCING

    return Output;
}

// GeometryShader

// NOTE: For some reason it seems like this part of the shader is always compiled, so disable it to avoid compilation errors
#if ENABLE_CASCADE_GS_INSTANCING
struct FGSCascadeOutput
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif

    float4 Position : SV_Position;

    // Index into what ArraySlice we want to write into
    uint RenderTargetViewIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(MAX_CASCADES * 3)]
void Cascade_GSMain(triangle FVSCascadeOutput Input[3], inout TriangleStream<FGSCascadeOutput> OutStream)
{
    [unroll]
    for (int Cascade = 0; Cascade < MAX_CASCADES; Cascade++)
    {
        FGSCascadeOutput Output;
        Output.RenderTargetViewIndex = Cascade;

        const float4x4 LightViewProjection = CascadeMatrixBuffer[Cascade].ViewProj;

        [unroll]
        for(int Vertex = 0; Vertex < 3; Vertex++)
        {
        #if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
            Output.TexCoord = Input[Vertex].TexCoord;
        #endif

            Output.Position = mul(Input[Vertex].WorldPosition, LightViewProjection);
            OutStream.Append(Output);
        }

        OutStream.RestartStrip();
    }
}
#endif // ENABLE_CASCADE_GS_INSTANCING

// PixelShader

struct FPSCascadeInput
{
    float2 TexCoord : TEXCOORD0;
};

void Cascade_PSMain(FPSCascadeInput Input)
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoords = Input.TexCoord;
    TexCoords.y = 1.0f - TexCoords.y;

    // TODO: Perform Parallax mapping

#if ENABLE_ALPHA_MASK
    #if ENABLE_PACKED_MATERIAL_TEXTURE
        const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords).a;
    #else
        const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords).r;        
    #endif

    [[branch]]
    if (AlphaMask < 0.5f)
    {
        discard;
    }
#endif // ENABLE_ALPHA_MASK
#endif // ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Point Light Shadow Generation

// VertexShader

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

    const float4 WorldPosition = mul(float4(Input.Position, 1.0f), Constants.WorldModelMatrix);
    Output.WorldPosition = WorldPosition.xyz;

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    Output.TexCoord = Input.TexCoord;
#endif

// Vertex-shader instancing
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

// GeometryShader

// NOTE: For some reason it seems like this part of the shader is always compiled, so disable it to avoid compilation errors
#if ENABLE_POINTLIGHT_GS_INSTANCING
struct FGSPointOutput
{
    float3 WorldPosition : POSITION0;

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif

    float4 Position : SV_Position;

    // Index into what ArraySlice we want to write into
    uint RenderTargetViewIndex : SV_RenderTargetArrayIndex;
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

            Output.WorldPosition = Input[Vertex].WorldPosition;
            Output.Position = mul(float4(Input[Vertex].WorldPosition, 1.0f), LightViewProjection);

            OutStream.Append(Output);
        }

        OutStream.RestartStrip();
    }
}
#endif // ENABLE_POINTLIGHT_GS_INSTANCING

// PixelShader

struct FPSPointInput
{
    float3 WorldPosition : POSITION0;

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif
};

float Point_PSMain(FPSPointInput Input) : SV_DepthLessEqual
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoords = Input.TexCoord;
    TexCoords.y = 1.0f - TexCoords.y;

    // TODO: Do parallax-mapping

#if ENABLE_ALPHA_MASK 
    #if ENABLE_PACKED_MATERIAL_TEXTURE
        const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords).a;
    #else
        const float AlphaMask = AlphaMaskTex.Sample(MaterialSampler, TexCoords);
    #endif

    [[branch]]
    if (AlphaMask < 0.5f)
    {
        discard;
    }
#endif // ENABLE_ALPHA_MASK
#endif // ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING

    const float LightDistance = length(Input.WorldPosition.xyz - PointLightBuffer.LightPosition) / PointLightBuffer.LightFarPlane;
    return LightDistance;
}

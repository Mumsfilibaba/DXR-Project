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
#ifndef ENABLE_CASCADE_GS_INSTANCING
    #define ENABLE_CASCADE_GS_INSTANCING (0)
#endif
#ifndef ENABLE_CASCADE_VIEW_INSTANCING
    #define ENABLE_CASCADE_VIEW_INSTANCING (0)
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
#ifndef ENABLE_POINTLIGHT_VIEW_INSTANCING
    #define ENABLE_POINTLIGHT_VIEW_INSTANCING (0)
#endif
#if !ENABLE_POINTLIGHT_VS_INSTANCING && !ENABLE_POINTLIGHT_GS_INSTANCING && !ENABLE_POINTLIGHT_VIEW_INSTANCING
    #define ENABLE_POINTLIGHT_MULTI_PASS (1)
#endif

// Workaround on NVIDIA
#if SHADER_LANG == SHADER_LANG_HLSL
    #define ENABLE_RENDER_TARGET_VIEW_INDEX (1)
#endif

struct FPerCascade
{
    int CascadeIndex;
    int Padding0;
    int Padding1;
    int Padding2;
};

// PerObject
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
    ConstantBuffer<FMaterial> MaterialBuffer : register(b1);
    SamplerState MaterialSampler : register(s0);

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

///////////////////////////////////////////////////////////////////////////////////////////////////
// Cascade Shadow Generation

// VertexShader

struct FVSInput
{
    float3 Position : POSITION0;

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif

#if ENABLE_CASCADE_VIEW_INSTANCING || ENABLE_POINTLIGHT_VIEW_INSTANCING
    uint ViewID : SV_ViewID;
#endif

#if ENABLE_POINTLIGHT_VS_INSTANCING || ENABLE_POINTLIGHT_VIEW_INSTANCING
    uint InstanceID : SV_InstanceID;
#endif
};

struct FVSCascadeOutput
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif

#if ENABLE_CASCADE_GS_INSTANCING
    float4 Position : POSITION0;
#else
    float4 Position : SV_Position;
#endif

// NOTE: This is a workaround for NVIDIA using D3D12, for some reason we have to write to SV_RenderTargetArrayIndex
// and have the RenderTargetArrayIndex inside the PSO to be set to BaseLayer which then gets offset by using SV_RenderTargetArrayIndex
#if ENABLE_CASCADE_VIEW_INSTANCING && ENABLE_RENDER_TARGET_VIEW_INDEX
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

#if ENABLE_CASCADE_GS_INSTANCING
    Output.Position = WorldPosition;
#else
#if ENABLE_CASCADE_VIEW_INSTANCING
    const int CascadeIndex = min(Input.ViewID, MAX_CASCADES - 1);
#if ENABLE_RENDER_TARGET_VIEW_INDEX
    Output.RenderTargetArrayIndex = CascadeIndex;
#endif
#else
    const int CascadeIndex = min(PerCascadeBuffer.CascadeIndex, MAX_CASCADES - 1);
#endif

    const float4x4 LightViewProjection = CascadeMatrixBuffer[CascadeIndex].ViewProj;
    Output.Position = mul(WorldPosition, LightViewProjection);
#endif

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

            Output.Position = mul(Input[Vertex].Position, LightViewProjection);
            OutStream.Append(Output);
        }

        OutStream.RestartStrip();
    }
}
#endif

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
#endif
#endif
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
#elif ENABLE_POINTLIGHT_VIEW_INSTANCING
struct FTwoPassPointLightBuffer
{
    float4x4 LightProjections[NUM_CUBE_FACES_HALF];
    float3   LightPosition;
    float    LightFarPlane;
};

ConstantBuffer<FTwoPassPointLightBuffer> PointLightBuffer : register(b0);
#else
struct FPointLightBuffer
{
    float4x4 LightProjection;
    float3   LightPosition;
    float    LightFarPlane;
};

ConstantBuffer<FPointLightBuffer> PointLightBuffer : register(b0);
#endif

struct FVSPointOutput
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif

    float3 WorldPosition : POSITION0;

#if !ENABLE_POINTLIGHT_GS_INSTANCING
    float4 Position : SV_Position;
#endif

#if ENABLE_POINTLIGHT_VS_INSTANCING
    uint RenderTargetArrayIndex : SV_RenderTargetArrayIndex;
#endif

// NOTE: This is a workaround for NVIDIA using D3D12, for some reason we have to write to SV_RenderTargetArrayIndex
// and have the RenderTargetArrayIndex inside the PSO to be set to BaseLayer which then gets offset by using SV_RenderTargetArrayIndex
#if ENABLE_POINTLIGHT_VIEW_INSTANCING && ENABLE_RENDER_TARGET_VIEW_INDEX
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

// View-instancing
#if ENABLE_POINTLIGHT_VIEW_INSTANCING
    Output.Position = mul(WorldPosition, PointLightBuffer.LightProjections[Input.ViewID]);

    // Work-around using HLSL (Otherwise it does not work on NVIDIA hardware)
#if ENABLE_RENDER_TARGET_VIEW_INDEX
    Output.RenderTargetArrayIndex = Input.ViewID;
#endif
#endif

// VS instancing
#if ENABLE_POINTLIGHT_VS_INSTANCING
    const uint FaceIndex = clamp(Input.InstanceID, 0, 5);
    Output.Position = mul(WorldPosition, PointLightBuffer.LightProjections[FaceIndex]);
    Output.RenderTargetArrayIndex = Input.InstanceID;
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
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif

    float3 WorldPosition : POSITION0;
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
#endif

// PixelShader

struct FPSPointInput
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif

    float3 WorldPosition : POSITION0;
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
#endif
#endif

    const float LightDistance = length(Input.WorldPosition.xyz - PointLightBuffer.LightPosition) / PointLightBuffer.LightFarPlane;
    return LightDistance;
}

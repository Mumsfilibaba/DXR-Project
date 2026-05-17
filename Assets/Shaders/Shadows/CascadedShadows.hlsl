#include "../Structs.hlsli"
#include "../Constants.hlsli"
#include "CascadeStructs.hlsli"

#ifndef ENABLE_ALPHA_MASK
    #define ENABLE_ALPHA_MASK 0
#endif
#ifndef ENABLE_PARALLAX_MAPPING
    #define ENABLE_PARALLAX_MAPPING 0
#endif

#ifndef BINDLESS_SHADOWS
    #define BINDLESS_SHADOWS (0)
#endif

#ifndef MAX_CASCADES
    #define MAX_CASCADES 4
#endif
#ifndef ENABLE_CASCADE_VS_INSTANCING
    #define ENABLE_CASCADE_VS_INSTANCING 0
#endif
#ifndef ENABLE_CASCADE_GS_INSTANCING
    #define ENABLE_CASCADE_GS_INSTANCING 0
#endif
#ifndef ENABLE_CASCADE_VIEW_INSTANCING
    #define ENABLE_CASCADE_VIEW_INSTANCING 0
#endif
#if !ENABLE_CASCADE_VS_INSTANCING && !ENABLE_CASCADE_GS_INSTANCING && !ENABLE_CASCADE_VIEW_INSTANCING
    #define ENABLE_CASCADE_MULTI_PASS 1
#endif

// NOTE: This is a workaround for NVIDIA using D3D12, for some reason we have to write to SV_RenderTargetArrayIndex
// and have the RenderTargetArrayIndex inside the PSO to be set to BaseLayer which then gets offset by using SV_RenderTargetArrayIndex
#if ENABLE_CASCADE_VIEW_INSTANCING && SHADER_LANG == SHADER_LANG_HLSL
    #define ENABLE_VIEW_INSTANCING_WORK_AROUND 1
#endif

struct FPerCascade
{
    int CascadeIndex;
    int Padding0;
    int Padding1;
    int Padding2;
};

// Per-object
ConstantBuffer<FTransform> TransformBuffer : register(b1);

#if SHADER_LANG == SHADER_LANG_MSL
    ConstantBuffer<FPerCascade> PerCascadeBuffer : register(b2);
#else
    ConstantBuffer<FPerCascade> PerCascadeBuffer : register(b0);
#endif

StructuredBuffer<FCascadeMatrices> CascadeMatrixBuffer : register(t0);

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    // MaterialBuffer
    ConstantBuffer<FMaterial> MaterialBuffer : register(b2);

    #if BINDLESS_SHADOWS
        #define MATERIAL_BINDLESS_REGISTER b3
        #include "../MaterialBindless.hlsli"
    #else
        // Sampler
        SamplerState MaterialSampler : register(s0);

        // Material Textures
        #if ENABLE_ALPHA_MASK
            Texture2D<float4> AlbedoAlphaTex : register(t0);
        #endif // ENABLE_ALPHA_MASK

        #if ENABLE_PARALLAX_MAPPING
            Texture2D<float> HeightMap : register(t1);
        #endif // ENABLE_PARALLAX_MAPPING
    #endif // BINDLESS_SHADOWS
#endif // ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING

struct FVSInput
{
    float3 Position : POSITION0;

#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoord : TEXCOORD0;
#endif // ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
// For view-instancing
#if ENABLE_CASCADE_VIEW_INSTANCING
    uint ViewID : SV_ViewID;
#endif // ENABLE_CASCADE_VIEW_INSTANCING
// For vertex-shader instancing
#if ENABLE_CASCADE_VS_INSTANCING
    uint InstanceID : SV_InstanceID;
#endif // ENABLE_CASCADE_VS_INSTANCING
};

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

    const float3 WorldPositionWS = TransformPositionWS(TransformBuffer, Input.Position);
    const float4 WorldPosition   = float4(WorldPositionWS, 1.0f);

// Geometry shader instancing
#if ENABLE_CASCADE_GS_INSTANCING
    Output.WorldPosition = WorldPosition;
#else

// View-instancing
#if ENABLE_CASCADE_VIEW_INSTANCING
    const int CascadeIndex = clamp(Input.ViewID, 0, MAX_CASCADES - 1);
// Vertex-shader instancing
#elif ENABLE_CASCADE_VS_INSTANCING
    const int CascadeIndex = clamp(Input.InstanceID, 0, MAX_CASCADES - 1);
// Regular multi-pass
#elif ENABLE_CASCADE_MULTI_PASS
    const int CascadeIndex = clamp(PerCascadeBuffer.CascadeIndex, 0, MAX_CASCADES - 1);
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

struct FPSCascadeInput
{
    float2 TexCoord : TEXCOORD0;
};

void Cascade_PSMain(FPSCascadeInput Input)
{
#if ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
    float2 TexCoords = Input.TexCoord;

    // TODO: Perform Parallax mapping

#if ENABLE_ALPHA_MASK
#if BINDLESS_SHADOWS
    const float AlphaMask = GetAlbedoBindless().Sample(GetMaterialSamplerBindless(), TexCoords).a;
#else
    const float AlphaMask = AlbedoAlphaTex.Sample(MaterialSampler, TexCoords).a;
#endif // BINDLESS_SHADOWS

    [[branch]]
    if (AlphaMask < 0.5f)
    {
        discard;
    }
#endif // ENABLE_ALPHA_MASK
#endif // ENABLE_ALPHA_MASK || ENABLE_PARALLAX_MAPPING
}

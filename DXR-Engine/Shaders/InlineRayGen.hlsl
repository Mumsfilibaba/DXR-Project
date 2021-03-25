#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"

struct VSOutput
{
    float2 TexCoord : TEXCOORD0;
    float4 Position : SV_POSITION;
};

RaytracingAccelerationStructure Scene : register(t0);

TextureCube<float4> Skybox : register(t1);

Texture2D<float4> GBufferAlbedoTex   : register(t2);
Texture2D<float4> GBufferNormalTex   : register(t3);
Texture2D<float>  GBufferDepthTex    : register(t4);
Texture2D<float4> GBufferMaterialTex : register(t5);

StructuredBuffer<Vertex> Vertices[400] : register(t6);
ByteAddressBuffer        Indices[400]  : register(t406);

ConstantBuffer<Camera> CameraBuffer : register(b0);

SamplerState SkyboxSampler : register(s0);

RWTexture2D<float4> ColorDepth : register(u0);
RWTexture2D<float4> RayPDF     : register(u1);

void PSMain(VSOutput Input)
{
    // Half Resolution
    uint2 TexCoord     = (uint2)floor(Input.Position.xy);
    uint2 FullTexCoord = TexCoord * 2;
    
    float  GBufferDepth  = GBufferDepthTex[FullTexCoord];
    float4 GBufferNormal = GBufferNormalTex[FullTexCoord];

    float3 WorldPosition = PositionFromDepth(GBufferDepth, Input.TexCoord, CameraBuffer.ViewProjectionInverse);
    
    float3 N = UnpackNormal(GBufferNormal.rgb);
    float3 V = normalize(CameraBuffer.Position - WorldPosition);
    float3 L = reflect(-V, N);
    
    if (length(GBufferNormal) == 0.0f)
    {
        ColorDepth[TexCoord] = Float4(0.0f);
        RayPDF[TexCoord]     = Float4(0.0f);
        return;
    }
    
    RayDesc Ray;
    Ray.Origin    = WorldPosition + (N * RAY_OFFSET);
    Ray.Direction = L;
    Ray.TMin      = 0.0f;
    Ray.TMax      = 1000.0f;
    
    RayQuery<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> Query;
    Query.TraceRayInline(Scene, 0, 0xff, Ray);
    Query.Proceed();

    float3 Result = Float3(0.0f);
    if (Query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        Result = Float3(1.0);
    }
    else
    {
        Result = Skybox.SampleLevel(SkyboxSampler, L, 0).rgb;
    }

    ColorDepth[TexCoord] = float4(Result, GBufferDepth);
    RayPDF[TexCoord]     = float4(L, 1.0f);
}
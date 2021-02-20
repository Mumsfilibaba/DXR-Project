#include "Constants.hlsli"

//#define RootSig \
//    "RootFlags(0), " \
//    "DescriptorTable(CBV(b0, numDescriptors = 16))," \
//    "DescriptorTable(SRV(t0, numDescriptors = 16))," \
//    "DescriptorTable(UAV(u0, numDescriptors = 16))," \
//    "DescriptorTable(Sampler(s0, numDescriptors = 16))," \

struct Material
{
    float3 Albedo;
    float  Roughness;
    float  Metallic;
    float  AO;
    int    EnableHeight;
};

// Space1 for constantbuffers are used for root constants
cbuffer Constants : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    uint SamplerIndex;
};

ConstantBuffer<Material> Materials[1] : register(b1);

RWTexture2D<float4> Texture2DTableUAV[1] : register(u0);

Texture2D Texture2DTable[] : register(t0);

SamplerState Samplers[1] : register(s0);

//[RootSignature(RootSig)]
[numthreads(16, 16, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint Index = DispatchThreadID.z;
    float2 TexCoord = float2(DispatchThreadID.xy);
    
    SamplerState Sampler = Samplers[SamplerIndex];
    
    float4 Color  = Texture2DTable[Index].SampleLevel(Sampler, TexCoord, 0.0f);
    
    Color.rgb = Color.rgb * Materials[Index].Albedo.rgb;
    Texture2DTableUAV[Index][DispatchThreadID.xy] = Color;
}
#ifndef MATERIAL_BINDLESS_HLSLI
#define MATERIAL_BINDLESS_HLSLI

#include "DescriptorTypes.hlsli"

struct FMaterialBindlessIndices
{
    uint AlbedoHandle;
    uint NormalHandle;
    uint MaterialHandle;
    uint HeightHandle;
    uint SamplerHandle;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

#ifndef MATERIAL_BINDLESS_REGISTER
    #define MATERIAL_BINDLESS_REGISTER b2
#endif

ConstantBuffer<FMaterialBindlessIndices> MaterialIndicesBuffer : register(MATERIAL_BINDLESS_REGISTER);

SamplerState GetMaterialSamplerBindless()
{
    const FDescriptorHandle Sampler = FDescriptorHandle::FromPacked(MaterialIndicesBuffer.SamplerHandle);
    return SamplerDescriptorHeap[Sampler.GetIndex()];
}

Texture2D<float4> GetAlbedoBindless()
{
    const FDescriptorHandle Albedo = FDescriptorHandle::FromPacked(MaterialIndicesBuffer.AlbedoHandle);
    return ResourceDescriptorHeap[Albedo.GetIndex()];
}

Texture2D<float3> GetNormalBindless()
{
    const FDescriptorHandle Normal = FDescriptorHandle::FromPacked(MaterialIndicesBuffer.NormalHandle);
    return ResourceDescriptorHeap[Normal.GetIndex()];
}

Texture2D<float3> GetMaterialBindless()
{
    const FDescriptorHandle Material = FDescriptorHandle::FromPacked(MaterialIndicesBuffer.MaterialHandle);
    return ResourceDescriptorHeap[Material.GetIndex()];
}

Texture2D<float> GetHeightBindless()
{
    const FDescriptorHandle Height = FDescriptorHandle::FromPacked(MaterialIndicesBuffer.HeightHandle);
    return ResourceDescriptorHeap[Height.GetIndex()];
}

#endif

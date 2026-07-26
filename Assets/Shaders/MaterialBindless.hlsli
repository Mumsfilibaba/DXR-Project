#ifndef MATERIAL_BINDLESS_HLSLI
#define MATERIAL_BINDLESS_HLSLI

#include "BindlessHelpers.hlsli"
#include "Structs.hlsli"

bool IsAlbedoBindlessValid(FMaterial MaterialData)
{
    return FDescriptorHandle::FromPacked(MaterialData.AlbedoHandle).IsValid();
}

bool IsNormalBindlessValid(FMaterial MaterialData)
{
    return FDescriptorHandle::FromPacked(MaterialData.NormalHandle).IsValid();
}

bool IsMaterialBindlessValid(FMaterial MaterialData)
{
    return FDescriptorHandle::FromPacked(MaterialData.MaterialHandle).IsValid();
}

bool IsHeightBindlessValid(FMaterial MaterialData)
{
    return FDescriptorHandle::FromPacked(MaterialData.HeightHandle).IsValid();
}

bool IsMaterialSamplerBindlessValid(FMaterial MaterialData)
{
    return FDescriptorHandle::FromPacked(MaterialData.SamplerHandle).IsValid();
}

SamplerState GetMaterialSamplerBindless(FMaterial MaterialData)
{
    return GetSamplerFromPackedDescriptorIndex(MaterialData.SamplerHandle);
}

Texture2D<float4> GetAlbedoBindless(FMaterial MaterialData)
{
    return GetResourceFromPackedDescriptorIndex(MaterialData.AlbedoHandle);
}

Texture2D<float3> GetNormalBindless(FMaterial MaterialData)
{
    return GetResourceFromPackedDescriptorIndex(MaterialData.NormalHandle);
}

Texture2D<float3> GetMaterialBindless(FMaterial MaterialData)
{
    return GetResourceFromPackedDescriptorIndex(MaterialData.MaterialHandle);
}

Texture2D<float> GetHeightBindless(FMaterial MaterialData)
{
    return GetResourceFromPackedDescriptorIndex(MaterialData.HeightHandle);
}

#endif

#pragma once
#include "RHI/RHIResources.h"
#include "Engine/Resources/Material.h"

struct FMaterialBindlessIndicesHLSL
{
    FRHIDescriptorHandle AlbedoHandle;
    FRHIDescriptorHandle NormalHandle;
    FRHIDescriptorHandle MaterialHandle;
    FRHIDescriptorHandle HeightHandle;
    FRHIDescriptorHandle SamplerHandle;
    uint32               Padding[3] = { 0, 0, 0 };
};

static_assert(sizeof(FMaterialBindlessIndicesHLSL) == 32, "Must stay 16-byte aligned for the constant buffer");
static_assert(sizeof(FRHIDescriptorHandle) == sizeof(uint32), "FRHIDescriptorHandle must be 4 bytes for the HLSL bitfield layout");

inline void FillMaterialBindlessIndices(const FMaterial& InMaterial, FMaterialBindlessIndicesHLSL& OutIndices)
{
    if (InMaterial.AlbedoMap)
    {
        OutIndices.AlbedoHandle = InMaterial.AlbedoMap->GetShaderResourceView()->GetBindlessHandle();
    }

    if (InMaterial.HasNormalMap() && InMaterial.NormalMap)
    {
        OutIndices.NormalHandle = InMaterial.NormalMap->GetShaderResourceView()->GetBindlessHandle();
    }

    if (InMaterial.MaterialMap)
    {
        OutIndices.MaterialHandle = InMaterial.MaterialMap->GetShaderResourceView()->GetBindlessHandle();
    }

    if (InMaterial.HasHeightMap() && InMaterial.HeightMap)
    {
        OutIndices.HeightHandle = InMaterial.HeightMap->GetShaderResourceView()->GetBindlessHandle();
    }

    if (FRHISamplerState* MaterialSampler = InMaterial.GetMaterialSampler())
    {
        OutIndices.SamplerHandle = MaterialSampler->GetBindlessHandle();
    }
}

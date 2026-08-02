#pragma once
#include "RHI/RHI.h"
#include "RHI/RHIResources.h"
#include "Engine/Engine.h"
#include "Engine/Resources/Material.h"
#include "Core/Misc/Asserts.h"
#include "Core/Misc/OutputDeviceLogger.h"

static_assert(sizeof(FRHIDescriptorHandle) == sizeof(uint32), "FRHIDescriptorHandle must be 4 bytes for the HLSL bitfield layout");

inline FRHIDescriptorHandle ResolveBindlessSRV(const FMaterial& InMaterial, FRHITexture* InTexture, const char* InSlotName)
{
    if (!InTexture)
    {
        return FRHIDescriptorHandle();
    }

    FRHIShaderResourceView* ShaderResourceView = InTexture->GetShaderResourceView();
    if (!ShaderResourceView)
    {
        LOG_ERROR("[Bindless] Material '%s' %s map has no ShaderResourceView", *InMaterial.GetName(), InSlotName);
        CHECK(false);
        return FRHIDescriptorHandle();
    }

    const FRHIDescriptorHandle Handle = ShaderResourceView->GetBindlessHandle();
    if (!Handle.IsValid())
    {
        LOG_ERROR("[Bindless] Material '%s' %s map SRV has no valid bindless descriptor", *InMaterial.GetName(), InSlotName);
        CHECK(false);
    }

    return Handle;
}

inline void FillMaterialHandles(const FMaterial& InMaterial, FMaterialHLSL& OutData)
{
    FRHIShaderResourceView* AlbedoSRV   = SafeGetDefaultSRV(InMaterial.AlbedoMap);
    FRHIShaderResourceView* NormalSRV   = SafeGetDefaultSRV(InMaterial.NormalMap);
    FRHIShaderResourceView* MaterialSRV = SafeGetDefaultSRV(InMaterial.MaterialMap);

    const bool bHasRealNormalMap = (NormalSRV != nullptr);
    OutData.NormalMapFlags = bHasRealNormalMap ? 1u : 0u;

    if (!RHI::bSupportsBindless)
    {
        return;
    }

    if (FEngine* Engine = FEngine::Get())
    {
        if (!AlbedoSRV && Engine->BaseTexture)
        {
            AlbedoSRV = Engine->BaseTexture->GetShaderResourceView();
        }

        if (!NormalSRV && Engine->BaseNormal)
        {
            NormalSRV = Engine->BaseNormal->GetShaderResourceView();
        }

        if (!MaterialSRV && Engine->BaseTexture)
        {
            MaterialSRV = Engine->BaseTexture->GetShaderResourceView();
        }
    }

    if (AlbedoSRV)
    {
        OutData.AlbedoHandle = AlbedoSRV->GetBindlessHandle();
    }

    if (NormalSRV)
    {
        OutData.NormalHandle = NormalSRV->GetBindlessHandle();
    }

    if (MaterialSRV)
    {
        OutData.MaterialHandle = MaterialSRV->GetBindlessHandle();
    }

    if (InMaterial.HasHeightMap() && InMaterial.HeightMap)
    {
        OutData.HeightHandle = ResolveBindlessSRV(InMaterial, InMaterial.HeightMap.Get(), "Height");
    }

    if (FRHISamplerState* MaterialSampler = InMaterial.GetMaterialSampler())
    {
        OutData.SamplerHandle = MaterialSampler->GetBindlessHandle();
    }
}

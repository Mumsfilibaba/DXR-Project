#pragma once
#include "RHI/RHI.h"
#include "RHI/RHIResources.h"
#include "Engine/Engine.h"
#include "Engine/Resources/Material.h"
#include "Core/Misc/Asserts.h"
#include "Core/Misc/OutputDeviceLogger.h"

static_assert(sizeof(FRHIDescriptorHandle) == sizeof(uint32), "FRHIDescriptorHandle must be 4 bytes for the HLSL bitfield layout");

inline FRHITexture* ResolveMaterialSlotTexture(const FMaterial& InMaterial, EMaterialTextureSlot::Type Slot)
{
    if (FRHITexture* Texture = InMaterial.GetTexture(Slot).Get())
    {
        return Texture;
    }

    if (Slot == EMaterialTextureSlot::Height)
    {
        return nullptr;
    }

    if (FEngine* Engine = FEngine::Get())
    {
        return (Slot == EMaterialTextureSlot::Normal) ? Engine->BaseNormal.Get() : Engine->BaseTexture.Get();
    }

    return nullptr;
}

inline void FillMaterialHandles(const FMaterial& InMaterial, FMaterialHLSL& OutData)
{
    const FRHITextureRef& NormalTexture = InMaterial.GetTexture(EMaterialTextureSlot::Normal);

    OutData.NormalMapFlags = NormalTexture ? ENormalMapFlags::Enabled : ENormalMapFlags::None;
    if (InMaterial.IsNormalMapPositiveY())
    {
        OutData.NormalMapFlags |= ENormalMapFlags::PositiveY;
    }

    if (NormalTexture && IsTwoChannelFormat(NormalTexture->GetDesc().Format))
    {
        OutData.NormalMapFlags |= ENormalMapFlags::TwoChannel;
    }

    if (!RHI::bSupportsBindless)
    {
        return;
    }

    for (uint32 Slot = 0; Slot < EMaterialTextureSlot::Count; ++Slot)
    {
        FRHITexture* Texture = ResolveMaterialSlotTexture(InMaterial, EMaterialTextureSlot::Type(Slot));
        if (!Texture)
        {
            continue;
        }

        FRHIShaderResourceView* ShaderResourceView = Texture->GetShaderResourceView();
        if (!ShaderResourceView)
        {
            LOG_ERROR("[Bindless] Material '%s' slot %u has no ShaderResourceView", *InMaterial.GetName(), Slot);
            CHECK(false);
            continue;
        }

        OutData.SlotHandles[Slot] = ShaderResourceView->GetBindlessHandle();
    }

    if (FRHISamplerState* MaterialSampler = InMaterial.GetMaterialSampler())
    {
        OutData.SamplerHandle = MaterialSampler->GetBindlessHandle();
    }
}

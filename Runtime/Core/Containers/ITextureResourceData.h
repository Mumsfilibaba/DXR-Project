#pragma once
#include "Core/Core.h"

struct ITextureResourceData
{
    virtual ~ITextureResourceData() = default;

    virtual void* GetMipData(uint32 MipLevel = 0) const = 0;

    virtual int64 GetMipRowPitch(uint32 MipLevel = 0)   const = 0;
    virtual int64 GetMipSlicePitch(uint32 MipLevel = 0) const = 0;
};
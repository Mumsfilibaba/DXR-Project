#pragma once
#include "RHI/RHIResources.h"

// This is based on the max texture-size 32678
#define MAX_TEXTURE_MIPS (15)

class RENDERERCORE_API FTextureResourceData : public IRHITextureData
{
public:
    FTextureResourceData();
    ~FTextureResourceData();

    void InitMipData(const void* InTextureData, int64 InTextureDataRowPitch, int64 InTextureDataSlicePitch, uint32 MipLevel = 0);

    virtual void* GetMipData(uint32 MipLevel = 0) const override final
    {
        CHECK(MipLevel < MAX_TEXTURE_MIPS);
        return TextureData[MipLevel];
    }

    virtual int64 GetMipRowPitch(uint32 MipLevel = 0) const override final
    {
        CHECK(MipLevel < MAX_TEXTURE_MIPS);
        return TextureDataRowPitch[MipLevel];
    }

    virtual int64 GetMipSlicePitch(uint32 MipLevel = 0) const override final
    {
        CHECK(MipLevel < MAX_TEXTURE_MIPS);
        return TextureDataSlicePitch[MipLevel];
    }

private:
    void MemzeroData();

    void* TextureData[MAX_TEXTURE_MIPS];
    int64 TextureDataRowPitch[MAX_TEXTURE_MIPS];
    int64 TextureDataSlicePitch[MAX_TEXTURE_MIPS];
};
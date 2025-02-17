#include "TextureResourceData.h"

FTextureResourceData::FTextureResourceData()
    : TextureData()
    , TextureDataRowPitch()
    , TextureDataSlicePitch()
{
    FMemory::Memzero(TextureData, sizeof(TextureData));
}

FTextureResourceData::~FTextureResourceData()
{
    for (void* Data : TextureData)
    {
        if (Data)
        {
            FMemory::Free(Data);
        }
        else
        {
            break;
        }
    }
}

void FTextureResourceData::InitMipData(const void* InTextureData, int64 InTextureDataRowPitch, int64 InTextureDataSlicePitch, uint32 MipLevel)
{
    CHECK(MipLevel < MAX_TEXTURE_MIPS);

    TextureData[MipLevel] = FMemory::Malloc(InTextureDataSlicePitch);
    FMemory::Memcpy(TextureData[MipLevel], InTextureData, InTextureDataSlicePitch);

    TextureDataRowPitch[MipLevel]   = InTextureDataRowPitch;
    TextureDataSlicePitch[MipLevel] = InTextureDataSlicePitch;
}
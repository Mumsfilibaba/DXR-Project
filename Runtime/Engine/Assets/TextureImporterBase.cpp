#include "TextureImporterBase.h"
#include "TextureResource.h"

#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Platform/PlatformMisc.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static EFormat GetByteFormat(int32 Channels)
{
    if (Channels == 4)
    {
        return EFormat::R8G8B8A8_Unorm;
    }
    else if (Channels == 2)
    {
        return EFormat::R8G8_Unorm;
    }
    else if (Channels == 1)
    {
        return EFormat::R8_Unorm;
    }
    else
    {
        return EFormat::Unknown;
    }
}

static EFormat GetExtendedFormat(int32 Channels)
{
    if (Channels == 4)
    {
        return EFormat::R16G16B16A16_Unorm;
    }
    else if (Channels == 2)
    {
        return EFormat::R16G16_Unorm;
    }
    else if (Channels == 1)
    {
        return EFormat::R16_Unorm;
    }
    else
    {
        return EFormat::Unknown;
    }
}

static EFormat GetFloatFormat(int32 Channels)
{
    if (Channels == 4)
    {
        return EFormat::R32G32B32A32_Float;
    }
    else if (Channels == 3)
    {
        return EFormat::R32G32B32_Float;
    }
    else if (Channels == 2)
    {
        return EFormat::R32G32_Float;
    }
    else if (Channels == 1)
    {
        return EFormat::R32_Float;
    }
    else
    {
        return EFormat::Unknown;
    }
}


FTexture* FTextureImporterBase::ImportFromFile(const FStringView& FileName)
{
    FFileHandleRef File = FPlatformFile::OpenForRead(FString(FileName));
    if (!File)
    {
        LOG_ERROR("[FTextureImporterBase]: Failed to open '%s'", FileName.GetCString());
        return nullptr;
    }

    // Read in the whole file
    const int64 FileSize = File->Size();
    
    TArray<uint8> FileData;
    FileData.Resize(int32(FileSize));
    File->Read(FileData.GetData(), FileData.GetSize());

    if (FileData.IsEmpty())
    {
        return nullptr;
    }

    // Retrieve info about the file
    int32 Width        = 0;
    int32 Height       = 0;
    int32 ChannelCount = 0;
    stbi_info_from_memory(FileData.GetData(), FileData.GetSize(), &Width, &Height, &ChannelCount);

    const bool bIsFloat    = stbi_is_hdr_from_memory(FileData.GetData(), FileData.GetSize());
    const bool bIsExtented = stbi_is_16_bit_from_memory(FileData.GetData(), FileData.GetSize());

    EFormat Format = EFormat::Unknown;

    // Load based on format
    TUniquePtr<uint8[]> Pixels;
    if (bIsExtented)
    {
        // We do not support 3 channel formats, force RGBA
        const auto NumChannels = (ChannelCount == 3) ? 4 : ChannelCount;

        Pixels = TUniquePtr<uint8[]>(reinterpret_cast<uint8*>(stbi_load_16_from_memory(
            FileData.GetData(),
            FileData.GetSize(),
            &Width,
            &Height,
            &ChannelCount,
            NumChannels)));

        Format = GetExtendedFormat(NumChannels);
    }
    else if (bIsFloat)
    {
        Pixels = TUniquePtr<uint8[]>(reinterpret_cast<uint8*>(stbi_loadf_from_memory(
            FileData.GetData(),
            FileData.GetSize(),
            &Width,
            &Height,
            &ChannelCount,
            0)));

        Format = GetFloatFormat(ChannelCount);
    }
    else
    {
        // We do not support 3 channel formats, force RGBA
        const auto NumChannels = (ChannelCount == 3) ? 4 : ChannelCount;

        Pixels = TUniquePtr<uint8[]>(stbi_load_from_memory(
            FileData.GetData(),
            FileData.GetSize(),
            &Width,
            &Height,
            &ChannelCount,
            NumChannels));

        Format = GetByteFormat(NumChannels);
    }

    CHECK(Format != EFormat::Unknown);

    // CHECK if succeeded
    if (!Pixels)
    {
        LOG_ERROR("[FTextureImporterBase]: Failed to load image '%s'", FileName.GetCString());
        return nullptr;
    }

    const int64 RowPitch   = Width * GetByteStrideFromFormat(Format);
    const int64 SlicePitch = RowPitch * Height;

    FTexture2D* NewTexture = dbg_new FTexture2D(Format, Width, Height, 1);
    NewTexture->CreateData();

    FTextureResourceData* TextureData = NewTexture->GetTextureResourceData();
    TextureData->InitMipData(Pixels.Get(), RowPitch, SlicePitch);

    return NewTexture;
}

bool FTextureImporterBase::MatchExtenstion(const FStringView& FileName)
{
    return 
        FileName.EndsWithNoCase(".jpeg") ||
        FileName.EndsWithNoCase(".jpg") ||
        FileName.EndsWithNoCase(".png") ||
        FileName.EndsWithNoCase(".bmp") ||
        FileName.EndsWithNoCase(".psd") ||
        FileName.EndsWithNoCase(".gif") ||
        FileName.EndsWithNoCase(".tga") ||
        FileName.EndsWithNoCase(".hdr");
}

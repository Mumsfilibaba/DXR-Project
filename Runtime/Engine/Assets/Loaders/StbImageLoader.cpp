#include "StbImageLoader.h"

#include "Core/Threading/AsyncTaskManager.h"
#include "Core/Logging/Log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CStbImageLoader

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

TSharedPtr<SImage2D> CStbImageLoader::LoadFile(const String& Filename)
{
    TSharedPtr<SImage2D> Image = MakeShared<SImage2D>(Filename, uint16(0), uint16(0), EFormat::Unknown);

    // Async lambda
    const auto LoadImageAsync = [Image, Filename]()
    {
        FILE* File = fopen(Filename.CStr(), "rb");
        if (!File)
        {
            LOG_ERROR("[CStbImageLoader]: Failed to open '%s'", Filename.CStr());
            return;
        }

        // Retrieve info about the file
        int32 Width = 0;
        int32 Height = 0;
        int32 ChannelCount = 0;
        stbi_info_from_file(File, &Width, &Height, &ChannelCount);

        const bool bIsFloat = stbi_is_hdr_from_file(File);
        const bool bIsExtented = stbi_is_16_bit_from_file(File);

        EFormat Format = EFormat::Unknown;

        // Load based on format
        TUniquePtr<uint8[]> Pixels;
        if (bIsExtented)
        {
            if (ChannelCount == 3)
            {
                Pixels = TUniquePtr<uint8[]>(reinterpret_cast<uint8*>(stbi_load_from_file_16(File, &Width, &Height, &ChannelCount, 4)));
            }
            else
            {
                Pixels = TUniquePtr<uint8[]>(reinterpret_cast<uint8*>(stbi_load_from_file_16(File, &Width, &Height, &ChannelCount, 0)));
            }

            Format = GetExtendedFormat(ChannelCount);
        }
        else if (bIsFloat)
        {
            Pixels = TUniquePtr<uint8[]>(reinterpret_cast<uint8*>(stbi_loadf_from_file(File, &Width, &Height, &ChannelCount, 0)));
            Format = GetFloatFormat(ChannelCount);
        }
        else
        {
            if (ChannelCount == 3)
            {
                Pixels = TUniquePtr<uint8[]>(stbi_load_from_file(File, &Width, &Height, &ChannelCount, 4));
            }
            else
            {
                Pixels = TUniquePtr<uint8[]>(stbi_load_from_file(File, &Width, &Height, &ChannelCount, 0));
            }

            Format = GetByteFormat(ChannelCount);
        }

        // Check if succeeded
        if (!Pixels)
        {
            LOG_ERROR("[CStbImageLoader]: Failed to load image '%s'", Filename.CStr());
            return;
        }
        else
        {
            LOG_INFO("[CStbImageLoader]: Loaded image '%s'", Filename.CStr());
        }

        Image->Image     = Move(Pixels);
        Image->Format    = Format;
        Image->Width     = (uint16)Width;
        Image->Height    = (uint16)Height;
        Image->bIsLoaded = true;
    };

    SAsyncTask NewTask;
    NewTask.Delegate.BindLambda(LoadImageAsync);

    CAsyncTaskManager::Get().Dispatch(NewTask);

    return Image;
}

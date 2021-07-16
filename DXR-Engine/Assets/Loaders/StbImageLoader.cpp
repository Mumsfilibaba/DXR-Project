#include "StbImageLoader.h"

#include <stb_image.h>

static EFormat GetByteFormat( int32 Channels )
{
    if ( Channels == 4)
    {
        return EFormat::R8G8B8A8_Unorm;
    }
    else if ( Channels == 2 )
    {
        return EFormat::R8G8_Unorm;
    }
    else if ( Channels == 1 )
    {
        return EFormat::R8_Unorm;
    }
    else
    {
        return EFormat::Unknown;
    }
}

static EFormat GetExtendedFormat( int32 Channels )
{
    if ( Channels == 4)
    {
        return EFormat::R16G16B16A16_Unorm;
    }
    else if (Channels == 2)
    {
        return EFormat::R16G16_Unorm;
    }
    else if ( Channels == 1 )
    {
        return EFormat::R16_Unorm;
    }
    else
    {
        return EFormat::Unknown;
    }
}

static EFormat GetFloatFormat( int32 Channels )
{
    if ( Channels == 4 )
    {
        return EFormat::R32G32B32A32_Float;
    }
    else if ( Channels == 3 )
    {
        return EFormat::R32G32B32_Float;
    }
    else if ( Channels == 2 )
    {
        return EFormat::R32G32_Float;
    }
    else if ( Channels == 1 )
    {
        return EFormat::R32_Float;
    }
    else
    {
        return EFormat::Unknown;
    }
}

TSharedPtr<SImage2D> CStbImageLoader::LoadFile( const String& Filename )
{
    FILE* File = fopen( Filename.c_str(), "rb" );
    if ( !File )
    {
        LOG_ERROR( "[CStbImageLoader]: Failed to open '" + Filename + "'" );
        return nullptr;
    }

    // Retrive info about the file
    int32 Width = 0;
    int32 Height = 0;
    int32 ChannelCount = 0;
    stbi_info_from_file( File, &Width, &Height, &ChannelCount );

    const bool IsFloat = stbi_is_hdr_from_file( File );
    const bool IsExtented = stbi_is_16_bit_from_file( File );

    EFormat Format = EFormat::Unknown;
    
    // Load based on format
    TUniquePtr<uint8[]> Pixels;
    if ( IsExtented )
    {
        if ( ChannelCount == 3 )
        {
            Pixels = TUniquePtr<uint8[]>( reinterpret_cast<uint8*>(stbi_load_from_file_16( File, &Width, &Height, &ChannelCount, 4 )) );
        }
        else
        {
            Pixels = TUniquePtr<uint8[]>( reinterpret_cast<uint8*>(stbi_load_from_file_16( File, &Width, &Height, &ChannelCount, 0 )) );
        }

        Format = GetExtendedFormat( ChannelCount );
    }
    else if ( IsFloat )
    {
        Pixels = TUniquePtr<uint8[]>( reinterpret_cast<uint8*>(stbi_loadf_from_file( File, &Width, &Height, &ChannelCount, 0 )) );
        Format = GetFloatFormat( ChannelCount );
    }
    else
    {
        if ( ChannelCount == 3 )
        {
            Pixels = TUniquePtr<uint8[]>( stbi_load_from_file( File, &Width, &Height, &ChannelCount, 4 ) );
        }
        else
        {
            Pixels = TUniquePtr<uint8[]>( stbi_load_from_file( File, &Width, &Height, &ChannelCount, 0 ) );
        }

        Format = GetByteFormat( ChannelCount );
    }

    // Check if succeeded
    if ( !Pixels )
    {
        LOG_ERROR( "[CStbImageLoader]: Failed to load image '" + Filename + "'" );
        return nullptr;
    }
    else
    {
        LOG_INFO( "[CStbImageLoader]: Loaded image '" + Filename + "'" );
    }

    TSharedPtr<SImage2D> Image = MakeShared<SImage2D>( Filename, Width, Height, Format );
    Image->Image = TSharedPtr<uint8[]>( ::Move( Pixels ) );
    return Image;
}

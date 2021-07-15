#include "StbImageLoader.h"

#include <stb_image.h>

static EFormat GetFormat( int32 Width, int32 Height, int32 Channels )
{
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

    const bool Is16Bit = stbi_is_16_bit_from_file( File );
    const bool IsFloat = stbi_is_hdr_from_file( File );

    // Load based on format
    TUniquePtr<uint8[]> Pixels;
    if ( !Is16Bit && !IsFloat )
    {
        Pixels = TUniquePtr<uint8[]>( stbi_load_from_file( File, &Width, &Height, &ChannelCount, 0 ) );
    }
    else if ( Is16Bit )
    {
        Pixels = TUniquePtr<uint8[]>( reinterpret_cast<uint8*>(stbi_load_from_file_16( File, &Width, &Height, &ChannelCount, 0 )) );
    }
    else if ( IsFloat )
    {
        Pixels = TUniquePtr<uint8[]>( reinterpret_cast<uint8*>(stbi_loadf_from_file( File, &Width, &Height, &ChannelCount, 0 )) );
    }
    else
    {
        LOG_ERROR( "[CStbImageLoader]: Format not supported" );
        return nullptr;
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

    const EFormat Format = GetFormat( Width, Height, ChannelCount );

    TSharedPtr<SImage2D> Image = MakeShared<SImage2D>( Filename, Width, Height, Format );
    Image->Image = TSharedPtr<uint8[]>( ::Move( Pixels ) );
    return Image;
}

#include "TextureImporterDDS.h"
#include "TextureResource.h"

#include "Core/Misc/OutputDeviceLogger.h"

#include "RHI/RHITypes.h"

// Temporarily disable 4456: variable hides a already existing variable
#pragma warning(push)
#pragma warning(disable : 4456)

#define TINYDDSLOADER_IMPLEMENTATION
#include <tinyddsloader.h>

#pragma warning(pop)

CONSTEXPR EFormat ConvertFormat(tinyddsloader::DDSFile::DXGIFormat Format)
{
    switch (Format)
    {
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_Typeless: return EFormat::R32G32B32A32_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_Float:    return EFormat::R32G32B32A32_Float;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_UInt:     return EFormat::R32G32B32A32_Uint;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_SInt:     return EFormat::R32G32B32A32_Sint;

        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_Typeless:    return EFormat::R32G32B32_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_Float:       return EFormat::R32G32B32_Float;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_UInt:        return EFormat::R32G32B32_Uint;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_SInt:        return EFormat::R32G32B32_Sint;

        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_Typeless: return EFormat::R16G16B16A16_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_Float:    return EFormat::R16G16B16A16_Float;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_UNorm:    return EFormat::R16G16B16A16_Unorm;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_UInt:     return EFormat::R16G16B16A16_Uint;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_SNorm:    return EFormat::R16G16B16A16_Snorm;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_SInt:     return EFormat::R16G16B16A16_Sint;

        case tinyddsloader::DDSFile::DXGIFormat::R32G32_Typeless:       return EFormat::R32G32_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32_Float:          return EFormat::R32G32_Float;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32_UInt:           return EFormat::R32G32_Uint;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32_SInt:           return EFormat::R32G32_Sint;

        case tinyddsloader::DDSFile::DXGIFormat::R10G10B10A2_Typeless:  return EFormat::R10G10B10A2_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::R10G10B10A2_UNorm:     return EFormat::R10G10B10A2_Unorm;
        case tinyddsloader::DDSFile::DXGIFormat::R10G10B10A2_UInt:      return EFormat::R10G10B10A2_Uint;

        case tinyddsloader::DDSFile::DXGIFormat::R11G11B10_Float:       return EFormat::R11G11B10_Float;

        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_Typeless:     return EFormat::R8G8B8A8_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm:        return EFormat::R8G8B8A8_Unorm;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm_SRGB:   return EFormat::R8G8B8A8_Unorm_SRGB;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UInt:         return EFormat::R8G8B8A8_Uint;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_SNorm:        return EFormat::R8G8B8A8_Snorm;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_SInt:         return EFormat::R8G8B8A8_Sint;

        case tinyddsloader::DDSFile::DXGIFormat::R16G16_Typeless:       return EFormat::R16G16_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_Float:          return EFormat::R16G16_Float;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_UNorm:          return EFormat::R16G16_Unorm;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_UInt:           return EFormat::R16G16_Uint;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_SNorm:          return EFormat::R16G16_Snorm;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_SInt:           return EFormat::R16G16_Sint;

        case tinyddsloader::DDSFile::DXGIFormat::R32_Typeless:          return EFormat::R32_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::D32_Float:             return EFormat::D32_Float;
        case tinyddsloader::DDSFile::DXGIFormat::R32_Float:             return EFormat::R32_Float;
        case tinyddsloader::DDSFile::DXGIFormat::R32_UInt:              return EFormat::R32_Uint;
        case tinyddsloader::DDSFile::DXGIFormat::R32_SInt:              return EFormat::R32_Sint;

        case tinyddsloader::DDSFile::DXGIFormat::R24G8_Typeless:        return EFormat::R24G8_Typeless;

        case tinyddsloader::DDSFile::DXGIFormat::D24_UNorm_S8_UInt:     return EFormat::D24_Unorm_S8_Uint;
        case tinyddsloader::DDSFile::DXGIFormat::R24_UNorm_X8_Typeless: return EFormat::R24_Unorm_X8_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::X24_Typeless_G8_UInt:  return EFormat::X24_Typeless_G8_Uint;

        case tinyddsloader::DDSFile::DXGIFormat::R8G8_Typeless:         return EFormat::R8G8_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_UNorm:            return EFormat::R8G8_Unorm;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_UInt:             return EFormat::R8G8_Uint;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_SNorm:            return EFormat::R8G8_Snorm;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_SInt:             return EFormat::R8G8_Sint;

        case tinyddsloader::DDSFile::DXGIFormat::R16_Typeless:          return EFormat::R16_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::R16_Float:             return EFormat::R16_Float;
        case tinyddsloader::DDSFile::DXGIFormat::D16_UNorm:             return EFormat::D16_Unorm;
        case tinyddsloader::DDSFile::DXGIFormat::R16_UNorm:             return EFormat::R16_Unorm;
        case tinyddsloader::DDSFile::DXGIFormat::R16_UInt:              return EFormat::R16_Uint;
        case tinyddsloader::DDSFile::DXGIFormat::R16_SNorm:             return EFormat::R16_Snorm;
        case tinyddsloader::DDSFile::DXGIFormat::R16_SInt:              return EFormat::R16_Sint;

        case tinyddsloader::DDSFile::DXGIFormat::R8_Typeless:           return EFormat::R8_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::R8_UNorm:              return EFormat::R8_Unorm;
        case tinyddsloader::DDSFile::DXGIFormat::R8_UInt:               return EFormat::R8_Uint;
        case tinyddsloader::DDSFile::DXGIFormat::R8_SNorm:              return EFormat::R8_Snorm;
        case tinyddsloader::DDSFile::DXGIFormat::R8_SInt:               return EFormat::R8_Sint;

        case tinyddsloader::DDSFile::DXGIFormat::BC1_Typeless:          return EFormat::BC1_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm:             return EFormat::BC1_UNorm;
        case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm_SRGB:        return EFormat::BC1_UNorm_SRGB;
        case tinyddsloader::DDSFile::DXGIFormat::BC2_Typeless:          return EFormat::BC2_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::BC2_UNorm:             return EFormat::BC2_UNorm;
        case tinyddsloader::DDSFile::DXGIFormat::BC2_UNorm_SRGB:        return EFormat::BC2_UNorm_SRGB;
        case tinyddsloader::DDSFile::DXGIFormat::BC3_Typeless:          return EFormat::BC3_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm:             return EFormat::BC3_UNorm;
        case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm_SRGB:        return EFormat::BC3_UNorm_SRGB;
        case tinyddsloader::DDSFile::DXGIFormat::BC4_Typeless:          return EFormat::BC4_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::BC4_UNorm:             return EFormat::BC4_UNorm;
        case tinyddsloader::DDSFile::DXGIFormat::BC4_SNorm:             return EFormat::BC4_SNorm;
        case tinyddsloader::DDSFile::DXGIFormat::BC5_Typeless:          return EFormat::BC5_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::BC5_UNorm:             return EFormat::BC5_UNorm;
        case tinyddsloader::DDSFile::DXGIFormat::BC5_SNorm:             return EFormat::BC5_SNorm;
        case tinyddsloader::DDSFile::DXGIFormat::BC6H_Typeless:         return EFormat::BC6H_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::BC6H_UF16:             return EFormat::BC6H_UF16;
        case tinyddsloader::DDSFile::DXGIFormat::BC6H_SF16:             return EFormat::BC6H_SF16;
        case tinyddsloader::DDSFile::DXGIFormat::BC7_Typeless:          return EFormat::BC7_Typeless;
        case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm:             return EFormat::BC7_UNorm;
        case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm_SRGB:        return EFormat::BC7_UNorm_SRGB;

        default: return EFormat::Unknown;
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTextureImporterDDS

FTexture* FTextureImporterDDS::ImportFromFile(const FStringView& FileName)
{
    tinyddsloader::DDSFile File;

    auto Result = File.Load(FileName.GetCString());
    if (Result != tinyddsloader::Success)
    {
        LOG_ERROR("[FTextureImporterDDS]: Failed to open '%s'", FileName.GetCString());
        return nullptr;
    }

    // TODO: Support other types
    CHECK(File.GetTextureDimension() == tinyddsloader::DDSFile::TextureDimension::Texture2D);

    const EFormat Format = ConvertFormat(File.GetFormat());
    
    FTexture2D* NewTexture = dbg_new FTexture2D(Format, File.GetWidth(), File.GetHeight(), File.GetMipCount());
    NewTexture->CreateData();

    FTextureResourceData* TextureData = NewTexture->GetTextureResourceData();
    for (uint32 Index = 0; Index < File.GetMipCount(); ++Index)
    {
        const tinyddsloader::DDSFile::ImageData* Data = File.GetImageData(Index);
        TextureData->InitMipData(Data->m_mem, Data->m_memPitch, Data->m_memSlicePitch, Index);
    }

    return NewTexture;
}

bool FTextureImporterDDS::MatchExtenstion(const FStringView& FileName)
{
    return FileName.EndsWithNoCase(".dds");
}

#include "AssetCore/AssetFile.h"
#include "AssetCore/TextureAsset.h"
#include "Core/Memory/Memory.h"

struct FTexturePayloadHeader
{
    uint32 Magic;
    uint32 Version;
    uint8  Format;
    uint8  Usage;
    uint8  bSRGB;
    uint8  NumMips;
    uint32 Reserved;
};

struct FTexturePayloadMip
{
    uint32 Width;
    uint32 Height;
    uint64 Offset;
    uint64 Size;
};

static_assert(sizeof(FTexturePayloadHeader) == 16, "FTexturePayloadHeader is stored in files and must keep its layout");
static_assert(sizeof(FTexturePayloadMip) == 24, "FTexturePayloadMip is stored in files and must keep its layout");

static constexpr uint32 TexturePayloadMagic   = MakeAssetChunkId("DXTX");
static constexpr uint32 TexturePayloadVersion = 1;
static constexpr uint64 MipAlignment          = 16;

// More than a 32768 pixel texture has
static constexpr int32 MaxMips = 16;

static constexpr ETextureUsage Usages[] =
{
    ETextureUsage::Color,
    ETextureUsage::Normal,
    ETextureUsage::Mask,
    ETextureUsage::Single,
    ETextureUsage::HDR,
};

static uint64 AlignMipOffset(uint64 Offset)
{
    return (Offset + MipAlignment - 1) & ~(MipAlignment - 1);
}

const CHAR_T* TextureAssets::GetUsageName(ETextureUsage Usage)
{
    switch (Usage)
    {
        case ETextureUsage::Color:  return TEXT("Color");
        case ETextureUsage::Normal: return TEXT("Normal");
        case ETextureUsage::Mask:   return TEXT("Mask");
        case ETextureUsage::Single: return TEXT("Single");
        case ETextureUsage::HDR:    return TEXT("HDR");
        default:                    return TEXT("Unknown");
    }
}

bool TextureAssets::FindUsageByName(const FString& Name, ETextureUsage& OutUsage)
{
    for (ETextureUsage Usage : Usages)
    {
        if (Name.Equals(GetUsageName(Usage), EStringCaseType::NoCase))
        {
            OutUsage = Usage;
            return true;
        }
    }

    return false;
}

const CHAR_T* TextureAssets::GetFormatName(ETextureFormat Format)
{
    switch (Format)
    {
        case ETextureFormat::R8_UNorm:           return TEXT("R8_UNorm");
        case ETextureFormat::R8G8B8A8_UNorm:     return TEXT("R8G8B8A8_UNorm");
        case ETextureFormat::R16G16B16A16_Float: return TEXT("R16G16B16A16_Float");
        case ETextureFormat::BC4_UNorm:          return TEXT("BC4_UNorm");
        case ETextureFormat::BC5_UNorm:          return TEXT("BC5_UNorm");
        case ETextureFormat::BC6H_UF16:          return TEXT("BC6H_UF16");
        case ETextureFormat::BC7_UNorm:          return TEXT("BC7_UNorm");
        default:                                 return TEXT("Unknown");
    }
}

bool TextureAssets::IsBlockCompressed(ETextureFormat Format)
{
    return Format == ETextureFormat::BC4_UNorm || Format == ETextureFormat::BC5_UNorm || Format == ETextureFormat::BC6H_UF16 || Format == ETextureFormat::BC7_UNorm;
}

int32 TextureAssets::GetElementSize(ETextureFormat Format)
{
    switch (Format)
    {
        case ETextureFormat::R8_UNorm:           return 1;
        case ETextureFormat::R8G8B8A8_UNorm:     return 4;
        case ETextureFormat::R16G16B16A16_Float: return 8;
        case ETextureFormat::BC4_UNorm:          return 8;
        case ETextureFormat::BC5_UNorm:          return 16;
        case ETextureFormat::BC6H_UF16:          return 16;
        case ETextureFormat::BC7_UNorm:          return 16;
        default:                                 return 0;
    }
}

int64 TextureAssets::GetMipSize(ETextureFormat Format, int32 Width, int32 Height)
{
    return GetRowPitch(Format, Width) * GetNumRows(Format, Height);
}

int64 TextureAssets::GetRowPitch(ETextureFormat Format, int32 Width)
{
    const int64 Elements = IsBlockCompressed(Format) ? (Width + BlockSize - 1) / BlockSize : Width;
    return Elements * GetElementSize(Format);
}

int32 TextureAssets::GetNumRows(ETextureFormat Format, int32 Height)
{
    return IsBlockCompressed(Format) ? (Height + BlockSize - 1) / BlockSize : Height;
}

void TextureAssets::Write(const FTextureAssetData& Texture, TArray<uint8>& OutPayload)
{
    const int32 NumMips = Texture.Mips.Size();
    CHECK(NumMips > 0 && NumMips <= MaxMips);

    FTexturePayloadHeader Header;
    Header.Magic    = TexturePayloadMagic;
    Header.Version  = TexturePayloadVersion;
    Header.Format   = static_cast<uint8>(Texture.Format);
    Header.Usage    = static_cast<uint8>(Texture.Usage);
    Header.bSRGB    = Texture.bSRGB ? 1 : 0;
    Header.NumMips  = static_cast<uint8>(NumMips);
    Header.Reserved = 0;

    TArray<FTexturePayloadMip> Entries;
    uint64 Offset = AlignMipOffset(sizeof(FTexturePayloadHeader) + NumMips * sizeof(FTexturePayloadMip));

    for (const FTextureMip& Mip : Texture.Mips)
    {
        FTexturePayloadMip& Entry = Entries.Emplace();
        Entry.Width  = static_cast<uint32>(Mip.Width);
        Entry.Height = static_cast<uint32>(Mip.Height);
        Entry.Offset = Offset;
        Entry.Size   = static_cast<uint64>(Mip.Data.Size());
        Offset = AlignMipOffset(Offset + Entry.Size);
    }

    OutPayload.Reset(static_cast<int32>(Offset), 0);

    Memory::Memcpy(OutPayload.Data(), &Header, sizeof(Header));
    Memory::Memcpy(OutPayload.Data() + sizeof(Header), Entries.Data(), NumMips * sizeof(FTexturePayloadMip));

    for (int32 Index = 0; Index < NumMips; ++Index)
    {
        Memory::Memcpy(OutPayload.Data() + Entries[Index].Offset, Texture.Mips[Index].Data.Data(), Entries[Index].Size);
    }
}

bool TextureAssets::Read(const TArray<uint8>& Payload, FTextureAssetData& OutTexture, FString& OutError)
{
    const uint64 PayloadSize = static_cast<uint64>(Payload.Size());

    FTexturePayloadHeader Header;
    if (PayloadSize < sizeof(Header))
    {
        OutError = FString(TEXT("The texture data is shorter than its header"));
        return false;
    }

    Memory::Memcpy(&Header, Payload.Data(), sizeof(Header));
    if (Header.Magic != TexturePayloadMagic)
    {
        OutError = FString(TEXT("The texture data doesn't start with 'DXTX'"));
        return false;
    }

    if (Header.Version != TexturePayloadVersion)
    {
        OutError = FString::Printf(TEXT("The texture data is version %u, and only version %u can be read"), Header.Version, TexturePayloadVersion);
        return false;
    }

    const ETextureFormat Format = static_cast<ETextureFormat>(Header.Format);
    if (GetElementSize(Format) == 0)
    {
        OutError = FString::Printf(TEXT("The texture data has an unknown format (%u)"), Header.Format);
        return false;
    }

    if (Header.Usage > static_cast<uint8>(ETextureUsage::HDR))
    {
        OutError = FString::Printf(TEXT("The texture data has an unknown usage (%u)"), Header.Usage);
        return false;
    }

    if (Header.NumMips == 0 || Header.NumMips > MaxMips || PayloadSize < sizeof(Header) + Header.NumMips * sizeof(FTexturePayloadMip))
    {
        OutError = FString::Printf(TEXT("The texture data has %u mips, or is shorter than their table"), Header.NumMips);
        return false;
    }

    FTextureAssetData Texture;
    Texture.Format = Format;
    Texture.Usage  = static_cast<ETextureUsage>(Header.Usage);
    Texture.bSRGB  = Header.bSRGB != 0;

    for (uint32 Index = 0; Index < Header.NumMips; ++Index)
    {
        FTexturePayloadMip Entry;
        Memory::Memcpy(&Entry, Payload.Data() + sizeof(Header) + Index * sizeof(FTexturePayloadMip), sizeof(Entry));

        const uint64 ExpectedSize = static_cast<uint64>(GetMipSize(Format, static_cast<int32>(Entry.Width), static_cast<int32>(Entry.Height)));
        if (Entry.Width == 0 || Entry.Height == 0 || Entry.Width > 32768 || Entry.Height > 32768 || Entry.Size != ExpectedSize)
        {
            OutError = FString::Printf(TEXT("Mip %u is %ux%u with %llu bytes, which isn't the %llu bytes a %s mip of that size takes"),
                Index, Entry.Width, Entry.Height, Entry.Size, ExpectedSize, GetFormatName(Format));
            return false;
        }

        if (Entry.Offset > PayloadSize || Entry.Size > PayloadSize - Entry.Offset)
        {
            OutError = FString::Printf(TEXT("Mip %u lies past the end of the texture data"), Index);
            return false;
        }

        FTextureMip& Mip = Texture.Mips.Emplace();
        Mip.Width  = static_cast<int32>(Entry.Width);
        Mip.Height = static_cast<int32>(Entry.Height);
        Mip.Data.ResizeUninitialized(static_cast<int32>(Entry.Size));
        Memory::Memcpy(Mip.Data.Data(), Payload.Data() + Entry.Offset, Entry.Size);
    }

    OutTexture = ::Move(Texture);
    return true;
}

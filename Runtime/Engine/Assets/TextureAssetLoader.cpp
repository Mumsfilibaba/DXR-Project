#include "AssetCore/AssetFile.h"
#include "Engine/Assets/TextureAssetLoader.h"

EFormat TextureAssetLoader::GetRHIFormat(ETextureFormat Format, bool bSRGB)
{
    switch (Format)
    {
        case ETextureFormat::R8_UNorm:           return EFormat::R8_Unorm;
        case ETextureFormat::R8G8B8A8_UNorm:     return bSRGB ? EFormat::R8G8B8A8_Unorm_SRGB : EFormat::R8G8B8A8_Unorm;
        case ETextureFormat::R16G16B16A16_Float: return EFormat::R16G16B16A16_Float;
        case ETextureFormat::BC4_UNorm:          return EFormat::BC4_UNorm;
        case ETextureFormat::BC5_UNorm:          return EFormat::BC5_UNorm;
        case ETextureFormat::BC6H_UF16:          return EFormat::BC6H_UF16;
        case ETextureFormat::BC7_UNorm:          return bSRGB ? EFormat::BC7_UNorm_SRGB : EFormat::BC7_UNorm;
        default:                                 return EFormat::Unknown;
    }
}

TSharedPtr<FTexture> TextureAssetLoader::Load(const FAssetData& Asset, const FString& DiskPath, FString& OutError)
{
    FAssetFileReader  Reader;
    TArray<uint8>     Payload;
    FTextureAssetData Texture;

    if (!Reader.Open(DiskPath, OutError) || !Reader.ReadChunk(AssetChunk::Data, Payload, OutError))
    {
        return nullptr;
    }

    if (!TextureAssets::Read(Payload, Texture, OutError))
    {
        OutError = FString::Printf(TEXT("The texture data of '%s' can't be read: %s"), *Asset.Path, *OutError);
        return nullptr;
    }

    Payload.Clear(true);

    const FTextureMip& TopMip = Texture.Mips[0];
    if (Texture.Mips.Size() > MAX_TEXTURE_MIPS)
    {
        OutError = FString::Printf(TEXT("'%s' has %d mips, more than the %d a texture can have"), *Asset.Path, Texture.Mips.Size(), MAX_TEXTURE_MIPS);
        return nullptr;
    }

    if (TextureAssets::IsBlockCompressed(Texture.Format) && ((TopMip.Width % TextureAssets::BlockSize) != 0 || (TopMip.Height % TextureAssets::BlockSize) != 0))
    {
        OutError = FString::Printf(TEXT("'%s' is %dx%d, which a block-compressed texture can't be"), *Asset.Path, TopMip.Width, TopMip.Height);
        return nullptr;
    }

    TSharedPtr<FTexture2D> Result = MakeSharedPtr<FTexture2D>(GetRHIFormat(Texture.Format, Texture.bSRGB), TopMip.Width, TopMip.Height, Texture.Mips.Size());
    Result->CreateData();

    for (int32 Index = 0; Index < Texture.Mips.Size(); ++Index)
    {
        const FTextureMip& Mip = Texture.Mips[Index];
        Result->GetTextureResourceData()->InitMipData(Mip.Data.Data(), TextureAssets::GetRowPitch(Texture.Format, Mip.Width), Mip.Data.Size(), Index);
    }

    if (!Result->CreateRHITexture(false))
    {
        OutError = FString::Printf(TEXT("The RHI texture for '%s' couldn't be created"), *Asset.Path);
        return nullptr;
    }

    Result->ReleaseData();
    Result->SetDebugName(Asset.Name);
    Result->SetFilename(DiskPath);
    return Result;
}

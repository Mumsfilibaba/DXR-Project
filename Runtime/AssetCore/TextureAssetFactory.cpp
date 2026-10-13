#include "AssetCore/TextureAssetFactory.h"
#include "Core/Containers/StringConversion.h"
#include "Core/Filesystem/File.h"
#include "Core/Image/PngWriter.h"
#include "Core/Math/Float.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Tasks/ParallelFor.h"

// Static, so the copy Engine compiles for its legacy importers doesn't clash with this one
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_MALLOC Memory::Malloc
#define STBI_REALLOC Memory::Realloc
#define STBI_FREE Memory::Free
#include <stb_image.h>

static constexpr const CHAR_T* NormalSuffixes[] =
{
    TEXT("normal"), TEXT("normals"), TEXT("norm"), TEXT("nrm"), TEXT("nor"), TEXT("ddn"), TEXT("n"),
};

static constexpr const CHAR_T* MaskSuffixes[] =
{
    TEXT("orm"), TEXT("arm"), TEXT("rma"), TEXT("mra"), TEXT("masks"), TEXT("packed"),
};

static constexpr const CHAR_T* SingleSuffixes[] =
{
    TEXT("roughness"), TEXT("rough"), TEXT("metallic"), TEXT("metalness"), TEXT("metal"), TEXT("ao"), TEXT("occlusion"),
    TEXT("height"), TEXT("displacement"), TEXT("disp"), TEXT("bump"), TEXT("gloss"), TEXT("glossiness"), TEXT("opacity"),
};

static constexpr int32 MaxTextureSize = 32768;

// The largest value a half float holds, which BC6H_UF16 can't exceed either
static constexpr float MaxHalfValue = 65504.0f;

// One level of the mip chain while it is built: RGBA bytes, or RGBA floats for HDR
struct FImageLevel
{
    int32         Width  = 0;
    int32         Height = 0;
    TArray<uint8> Pixels;
    TArray<float> Colors;
};

struct FSRGBToLinearTable
{
    FSRGBToLinearTable()
    {
        for (int32 Index = 0; Index < 256; ++Index)
        {
            const float Value = static_cast<float>(Index) / 255.0f;
            Values[Index] = (Value <= 0.04045f) ? (Value / 12.92f) : Math::Pow((Value + 0.055f) / 1.055f, 2.4f);
        }
    }

    float Values[256];
};

static float SRGBToLinear(uint8 Value)
{
    static const FSRGBToLinearTable Table;
    return Table.Values[Value];
}

static float LinearToSRGB(float Value)
{
    Value = Math::Clamp(Value, 0.0f, 1.0f);
    return (Value <= 0.0031308f) ? (Value * 12.92f) : (1.055f * Math::Pow(Value, 1.0f / 2.4f) - 0.055f);
}

static uint8 ToByte(float Value)
{
    return static_cast<uint8>(Math::Clamp(Value, 0.0f, 1.0f) * 255.0f + 0.5f);
}

template<int32 NumSuffixes>
static bool HasSuffix(const FString& Suffix, const CHAR_T* const (&Suffixes)[NumSuffixes])
{
    for (const CHAR_T* Candidate : Suffixes)
    {
        if (Suffix.Equals(Candidate))
        {
            return true;
        }
    }

    return false;
}

static ETextureFormat GetUncompressedFormat(ETextureUsage Usage)
{
    switch (Usage)
    {
        case ETextureUsage::Single: return ETextureFormat::R8_UNorm;
        case ETextureUsage::HDR:    return ETextureFormat::R16G16B16A16_Float;
        default:                    return ETextureFormat::R8G8B8A8_UNorm;
    }
}

static ETextureFormat GetCompressedFormat(ETextureUsage Usage)
{
    switch (Usage)
    {
        case ETextureUsage::Normal: return ETextureFormat::BC5_UNorm;
        case ETextureUsage::Single: return ETextureFormat::BC4_UNorm;
        case ETextureUsage::HDR:    return ETextureFormat::BC6H_UF16;
        default:                    return ETextureFormat::BC7_UNorm;
    }
}

static bool ReadBool(const FJsonValue& Object, const CHAR_T* Name, bool& OutValue, FString& OutError)
{
    const FJsonValue* Value = Object.IsObject() ? Object.Find(Name) : nullptr;
    if (Value && !Value->TryGetBool(OutValue))
    {
        OutError = FString::Printf(TEXT("%s isn't true or false"), Name);
        return false;
    }

    return true;
}

static bool DecodeImage(const TArray<uint8>& Bytes, bool bFloat, FImageLevel& OutLevel, FString& OutError)
{
    int32 Width    = 0;
    int32 Height   = 0;
    int32 Channels = 0;

    if (bFloat)
    {
        float* Colors = stbi_loadf_from_memory(Bytes.Data(), Bytes.Size(), &Width, &Height, &Channels, 4);
        if (Colors)
        {
            OutLevel.Colors.Reset(Colors, Width * Height * 4);
            stbi_image_free(Colors);
        }
    }
    else
    {
        stbi_uc* Pixels = stbi_load_from_memory(Bytes.Data(), Bytes.Size(), &Width, &Height, &Channels, 4);
        if (Pixels)
        {
            OutLevel.Pixels.Reset(Pixels, Width * Height * 4);
            stbi_image_free(Pixels);
        }
    }

    if (OutLevel.Pixels.IsEmpty() && OutLevel.Colors.IsEmpty())
    {
        OutError = StringCast<CHAR_T>(stbi_failure_reason()).ToString();
        return false;
    }

    OutLevel.Width  = Width;
    OutLevel.Height = Height;
    return true;
}

static void DownsamplePixels(const FImageLevel& Source, ETextureUsage Usage, bool bSRGB, FImageLevel& OutLevel)
{
    OutLevel.Width  = Math::Max(Source.Width / 2, 1);
    OutLevel.Height = Math::Max(Source.Height / 2, 1);
    OutLevel.Pixels.ResizeUninitialized(OutLevel.Width * OutLevel.Height * 4);

    for (int32 Y = 0; Y < OutLevel.Height; ++Y)
    {
        const int32 SourceRows[2] = { Math::Min(Y * 2, Source.Height - 1), Math::Min(Y * 2 + 1, Source.Height - 1) };

        for (int32 X = 0; X < OutLevel.Width; ++X)
        {
            const int32 SourceColumns[2] = { Math::Min(X * 2, Source.Width - 1), Math::Min(X * 2 + 1, Source.Width - 1) };

            float Sum[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            for (int32 Row : SourceRows)
            {
                for (int32 Column : SourceColumns)
                {
                    const uint8* Texel = &Source.Pixels[(Row * Source.Width + Column) * 4];
                    for (int32 Channel = 0; Channel < 3; ++Channel)
                    {
                        if (Usage == ETextureUsage::Normal)
                        {
                            Sum[Channel] += static_cast<float>(Texel[Channel]) / 127.5f - 1.0f;
                        }
                        else if (bSRGB)
                        {
                            Sum[Channel] += SRGBToLinear(Texel[Channel]);
                        }
                        else
                        {
                            Sum[Channel] += static_cast<float>(Texel[Channel]) / 255.0f;
                        }
                    }

                    Sum[3] += static_cast<float>(Texel[3]) / 255.0f;
                }
            }

            uint8* Destination = &OutLevel.Pixels[(Y * OutLevel.Width + X) * 4];
            if (Usage == ETextureUsage::Normal)
            {
                const float Length = Math::Sqrt(Sum[0] * Sum[0] + Sum[1] * Sum[1] + Sum[2] * Sum[2]);
                const float Scale  = (Length > 0.0f) ? (1.0f / Length) : 0.0f;
                for (int32 Channel = 0; Channel < 3; ++Channel)
                {
                    Destination[Channel] = ToByte(Sum[Channel] * Scale * 0.5f + 0.5f);
                }
            }
            else
            {
                for (int32 Channel = 0; Channel < 3; ++Channel)
                {
                    const float Average = Sum[Channel] * 0.25f;
                    Destination[Channel] = ToByte(bSRGB ? LinearToSRGB(Average) : Average);
                }
            }

            Destination[3] = ToByte(Sum[3] * 0.25f);
        }
    }
}

static void DownsampleColors(const FImageLevel& Source, FImageLevel& OutLevel)
{
    OutLevel.Width  = Math::Max(Source.Width / 2, 1);
    OutLevel.Height = Math::Max(Source.Height / 2, 1);
    OutLevel.Colors.ResizeUninitialized(OutLevel.Width * OutLevel.Height * 4);

    for (int32 Y = 0; Y < OutLevel.Height; ++Y)
    {
        const int32 SourceRows[2] = { Math::Min(Y * 2, Source.Height - 1), Math::Min(Y * 2 + 1, Source.Height - 1) };

        for (int32 X = 0; X < OutLevel.Width; ++X)
        {
            const int32 SourceColumns[2] = { Math::Min(X * 2, Source.Width - 1), Math::Min(X * 2 + 1, Source.Width - 1) };

            float* Destination = &OutLevel.Colors[(Y * OutLevel.Width + X) * 4];
            for (int32 Channel = 0; Channel < 4; ++Channel)
            {
                float Sum = 0.0f;
                for (int32 Row : SourceRows)
                {
                    for (int32 Column : SourceColumns)
                    {
                        Sum += Source.Colors[(Row * Source.Width + Column) * 4 + Channel];
                    }
                }

                Destination[Channel] = Sum * 0.25f;
            }
        }
    }
}

static void ConvertLevel(const FImageLevel& Level, ETextureFormat Format, FTextureMip& OutMip)
{
    const int32 NumTexels = Level.Width * Level.Height;

    OutMip.Width  = Level.Width;
    OutMip.Height = Level.Height;
    OutMip.Data.ResizeUninitialized(static_cast<int32>(TextureAssets::GetMipSize(Format, Level.Width, Level.Height)));

    if (Format == ETextureFormat::R8_UNorm)
    {
        for (int32 Index = 0; Index < NumTexels; ++Index)
        {
            OutMip.Data[Index] = Level.Pixels[Index * 4];
        }
    }
    else if (Format == ETextureFormat::R16G16B16A16_Float)
    {
        uint16* Halves = reinterpret_cast<uint16*>(OutMip.Data.Data());
        for (int32 Index = 0; Index < NumTexels * 4; ++Index)
        {
            Halves[Index] = FFloat16(Math::Clamp(Level.Colors[Index], 0.0f, MaxHalfValue)).Encoded;
        }
    }
    else
    {
        Memory::Memcpy(OutMip.Data.Data(), Level.Pixels.Data(), NumTexels * 4);
    }
}

static bool EncodeThumbnail(const FImageLevel& Level, ETextureUsage Usage, TArray<uint8>& OutPng)
{
    const int32 NumTexels = Level.Width * Level.Height;

    TArray<uint8> Pixels;
    Pixels.ResizeUninitialized(NumTexels * 4);

    for (int32 Index = 0; Index < NumTexels; ++Index)
    {
        uint8* Destination = &Pixels[Index * 4];
        if (Usage == ETextureUsage::HDR)
        {
            for (int32 Channel = 0; Channel < 3; ++Channel)
            {
                const float Value = Level.Colors[Index * 4 + Channel];
                Destination[Channel] = ToByte(LinearToSRGB(Value / (1.0f + Value)));
            }

            Destination[3] = 255;
            continue;
        }

        const uint8* Source = &Level.Pixels[Index * 4];
        for (int32 Channel = 0; Channel < 3; ++Channel)
        {
            Destination[Channel] = (Usage == ETextureUsage::Single) ? Source[0] : Source[Channel];
        }

        Destination[3] = (Usage == ETextureUsage::Color) ? Source[3] : 255;
    }

    FPngWriter Writer;
    return Writer.Encode(FImageView(Pixels.Data(), Level.Width, Level.Height), OutPng);
}

static bool HasMipSizes(const TArray<FTextureMip>& Mips, const TArray<FTextureMip>& SourceMips, ETextureFormat Format)
{
    if (Mips.Size() != SourceMips.Size())
    {
        return false;
    }

    for (int32 Index = 0; Index < Mips.Size(); ++Index)
    {
        const FTextureMip& Mip = Mips[Index];
        if (Mip.Width != SourceMips[Index].Width || Mip.Height != SourceMips[Index].Height || Mip.Data.Size() != TextureAssets::GetMipSize(Format, Mip.Width, Mip.Height))
        {
            return false;
        }
    }

    return true;
}

static bool BuildTexture(FTextureBuildJob& Job, bool bCanCompress, FTextureAssetData& OutTexture, bool& bOutCompress)
{
    TArray<uint8>           Bytes;
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Job.SourcePath);

    if (!FileHandle || !File::ReadFile(FileHandle.Get(), Bytes) || Bytes.IsEmpty())
    {
        Job.Error = FString::Printf(TEXT("Couldn't read '%s'"), *Job.SourcePath);
        return false;
    }

    int32 Width    = 0;
    int32 Height   = 0;
    int32 Channels = 0;

    if (!stbi_info_from_memory(Bytes.Data(), Bytes.Size(), &Width, &Height, &Channels))
    {
        Job.Error = FString::Printf(TEXT("'%s' isn't an image that can be read: %s"), *Job.SourcePath, *StringCast<CHAR_T>(stbi_failure_reason()).ToString());
        return false;
    }

    if (Width > MaxTextureSize || Height > MaxTextureSize)
    {
        Job.Error = FString::Printf(TEXT("'%s' is %dx%d, larger than the %d pixels a texture can be"), *Job.SourcePath, Width, Height, MaxTextureSize);
        return false;
    }

    const bool          bHDR         = stbi_is_hdr_from_memory(Bytes.Data(), Bytes.Size()) != 0;
    const ETextureUsage DefaultUsage = FTextureAssetFactory::GuessUsage(Job.SourcePath, Channels, bHDR);

    FTextureImportSettings Settings;
    if (!FTextureAssetFactory::ReadSettings(Job.Settings, DefaultUsage, Settings, Job.Error))
    {
        Job.Error = FString::Printf(TEXT("The settings for '%s' aren't valid: %s"), *Job.SourcePath, *Job.Error);
        return false;
    }

    TArray<FImageLevel> Levels;
    if (!DecodeImage(Bytes, Settings.Usage == ETextureUsage::HDR, Levels.Emplace(), Job.Error))
    {
        Job.Error = FString::Printf(TEXT("'%s' couldn't be decoded: %s"), *Job.SourcePath, *Job.Error);
        return false;
    }

    Bytes.Clear(true);

    while (Levels.Last().Width > 1 || Levels.Last().Height > 1)
    {
        FImageLevel Level;
        if (Settings.Usage == ETextureUsage::HDR)
        {
            DownsampleColors(Levels.Last(), Level);
        }
        else
        {
            DownsamplePixels(Levels.Last(), Settings.Usage, Settings.bSRGB, Level);
        }

        Levels.Add(::Move(Level));
    }

    int32 FirstLevel = 0;
    while (Settings.MaxResolution > 0 && Math::Max(Levels[FirstLevel].Width, Levels[FirstLevel].Height) > Settings.MaxResolution)
    {
        ++FirstLevel;
    }

    int32 ThumbnailLevel = FirstLevel;
    while (Math::Max(Levels[ThumbnailLevel].Width, Levels[ThumbnailLevel].Height) > FTextureAssetFactory::ThumbnailSize)
    {
        ++ThumbnailLevel;
    }

    if (!EncodeThumbnail(Levels[ThumbnailLevel], Settings.Usage, Job.Thumbnail))
    {
        Job.Error = FString::Printf(TEXT("The thumbnail of '%s' couldn't be encoded"), *Job.SourcePath);
        return false;
    }

    const int32 LastLevel = Settings.bGenerateMips ? Levels.Size() - 1 : FirstLevel;

    OutTexture.Format = GetUncompressedFormat(Settings.Usage);
    OutTexture.Usage  = Settings.Usage;
    OutTexture.bSRGB  = Settings.bSRGB;
    OutTexture.Mips.Clear();

    for (int32 Index = FirstLevel; Index <= LastLevel; ++Index)
    {
        ConvertLevel(Levels[Index], OutTexture.Format, OutTexture.Mips.Emplace());
    }

    const FTextureMip& TopMip = OutTexture.Mips[0];
    bOutCompress = Settings.bCompress && bCanCompress && (TopMip.Width % TextureAssets::BlockSize) == 0 && (TopMip.Height % TextureAssets::BlockSize) == 0;

    Job.UsedSettings = FTextureAssetFactory::WriteSettings(Settings);
    return true;
}

ETextureUsage FTextureAssetFactory::GuessUsage(const FString& SourcePath, int32 NumChannels, bool bHDR)
{
    if (bHDR)
    {
        return ETextureUsage::HDR;
    }

    const FString Name      = File::ExtractFilenameWithoutExtension(SourcePath);
    const int32   Separator = Name.FindLastCharWithPredicate([](CHAR_T Char) { return Char == '_' || Char == '-' || Char == ' ' || Char == '.'; });
    const FString Suffix    = ((Separator == FString::InvalidIndex) ? Name : Name.SubString(Separator + 1, Name.Length() - Separator - 1)).ToLower();

    if (HasSuffix(Suffix, NormalSuffixes))
    {
        return ETextureUsage::Normal;
    }

    if (HasSuffix(Suffix, MaskSuffixes))
    {
        return ETextureUsage::Mask;
    }

    if (HasSuffix(Suffix, SingleSuffixes) || NumChannels == 1)
    {
        return ETextureUsage::Single;
    }

    return ETextureUsage::Color;
}

bool FTextureAssetFactory::ReadSettings(const FJsonValue& Value, ETextureUsage DefaultUsage, FTextureImportSettings& OutSettings, FString& OutError)
{
    if (!Value.IsNull() && !Value.IsObject())
    {
        OutError = FString(TEXT("they aren't a JSON object"));
        return false;
    }

    FTextureImportSettings Settings;
    Settings.Usage = DefaultUsage;

    if (const FJsonValue* Usage = Value.IsObject() ? Value.Find(TEXT("Usage")) : nullptr)
    {
        FString UsageName;
        if (!Usage->TryGetString(UsageName) || !TextureAssets::FindUsageByName(UsageName, Settings.Usage))
        {
            OutError = FString(TEXT("Usage isn't one of Color, Normal, Mask, Single or HDR"));
            return false;
        }
    }

    Settings.bSRGB = Settings.Usage == ETextureUsage::Color;
    if (!ReadBool(Value, TEXT("SRGB"), Settings.bSRGB, OutError) ||
        !ReadBool(Value, TEXT("Compress"), Settings.bCompress, OutError) ||
        !ReadBool(Value, TEXT("GenerateMips"), Settings.bGenerateMips, OutError))
    {
        return false;
    }

    if (Settings.Usage != ETextureUsage::Color && Settings.Usage != ETextureUsage::Mask)
    {
        Settings.bSRGB = false;
    }

    if (const FJsonValue* MaxResolution = Value.IsObject() ? Value.Find(TEXT("MaxResolution")) : nullptr)
    {
        int64 Resolution = 0;
        if (!MaxResolution->TryGetInt64(Resolution) || Resolution < 0 || Resolution > MaxTextureSize)
        {
            OutError = FString::Printf(TEXT("MaxResolution isn't a size from 1 to %d, or 0 for the source's size"), MaxTextureSize);
            return false;
        }

        Settings.MaxResolution = static_cast<int32>(Resolution);
    }

    OutSettings = Settings;
    return true;
}

FJsonValue FTextureAssetFactory::WriteSettings(const FTextureImportSettings& Settings)
{
    FJsonValue Value = FJsonValue::MakeObject();
    Value.AddMember(TEXT("Usage"), FJsonValue(TextureAssets::GetUsageName(Settings.Usage)));
    Value.AddMember(TEXT("SRGB"), FJsonValue(Settings.bSRGB));
    Value.AddMember(TEXT("Compress"), FJsonValue(Settings.bCompress));
    Value.AddMember(TEXT("MaxResolution"), FJsonValue(Settings.MaxResolution));
    Value.AddMember(TEXT("GenerateMips"), FJsonValue(Settings.bGenerateMips));
    return Value;
}

FTextureAssetFactory::FTextureAssetFactory(const TSharedPtr<ITextureBlockCompressor>& InCompressor)
    : Compressor(InCompressor)
{
}

FTextureAssetFactory::~FTextureAssetFactory() = default;

const CHAR_T* FTextureAssetFactory::GetName() const
{
    return TEXT("Texture");
}

TArray<FString> FTextureAssetFactory::GetExtensions() const
{
    return
    {
        FString(TEXT(".png")),
        FString(TEXT(".jpg")),
        FString(TEXT(".jpeg")),
        FString(TEXT(".tga")),
        FString(TEXT(".bmp")),
        FString(TEXT(".psd")),
        FString(TEXT(".gif")),
        FString(TEXT(".hdr")),
    };
}

TArray<EAssetType> FTextureAssetFactory::GetAssetTypes() const
{
    return { EAssetType::Texture };
}

bool FTextureAssetFactory::Import(FAssetImportContext& Context, TArray<FImportedAsset>& OutAssets, FJsonValue& OutSettings, FString& OutError)
{
    TArray<FTextureBuildJob> Jobs;

    FTextureBuildJob& Job = Jobs.Emplace();
    Job.SourcePath = Context.GetSourcePath();
    Job.Settings   = Context.GetSettings();

    if (!BuildTextures(Jobs))
    {
        OutError = ::Move(Jobs[0].Error);
        return false;
    }

    FImportedAsset& Asset = OutAssets.Emplace();
    Asset.Name      = Context.GetImportName();
    Asset.Type      = EAssetType::Texture;
    Asset.Guid      = Context.GetGuid(Asset.Name, Asset.Type);
    Asset.Data      = ::Move(Jobs[0].Data);
    Asset.Thumbnail = ::Move(Jobs[0].Thumbnail);

    OutSettings = ::Move(Jobs[0].UsedSettings);
    return true;
}

bool FTextureAssetFactory::BuildTextures(TArray<FTextureBuildJob>& Jobs) const
{
    TArray<FTextureAssetData> Textures;
    Textures.Resize(Jobs.Size());

    TArray<bool> Compress;
    Compress.Reset(Jobs.Size(), false);

    TArray<bool> Succeeded;
    Succeeded.Reset(Jobs.Size(), false);

    const bool bCanCompress = Compressor.IsValid();
    Tasks::ParallelFor(Jobs.Size(), [&](int32 Index)
    {
        Jobs[Index].Error.Clear();
        Succeeded[Index] = BuildTexture(Jobs[Index], bCanCompress, Textures[Index], Compress[Index]);
    });

    bool bAllSucceeded = true;
    for (int32 Index = 0; Index < Jobs.Size(); ++Index)
    {
        FTextureBuildJob&  Job     = Jobs[Index];
        FTextureAssetData& Texture = Textures[Index];

        if (Succeeded[Index] && Compress[Index])
        {
            const ETextureFormat Format = GetCompressedFormat(Texture.Usage);

            TArray<FTextureMip> Blocks;
            if (!Compressor->Compress(Texture.Format, Texture.Mips, Format, Blocks, Job.Error))
            {
                Job.Error        = FString::Printf(TEXT("'%s' couldn't be compressed to %s: %s"), *Job.SourcePath, TextureAssets::GetFormatName(Format), *Job.Error);
                Succeeded[Index] = false;
            }
            else if (!HasMipSizes(Blocks, Texture.Mips, Format))
            {
                Job.Error        = FString::Printf(TEXT("'%s' couldn't be compressed to %s: the compressor returned mips of the wrong size"), *Job.SourcePath, TextureAssets::GetFormatName(Format));
                Succeeded[Index] = false;
            }
            else
            {
                Texture.Format = Format;
                Texture.Mips   = ::Move(Blocks);
            }
        }

        if (!Succeeded[Index])
        {
            Job.Thumbnail.Clear();
            Job.UsedSettings.Reset();
            bAllSucceeded = false;
            continue;
        }

        TextureAssets::Write(Texture, Job.Data);
        Texture.Mips.Clear(true);
    }

    return bAllSucceeded;
}

#include "TextureAssetTests.h"

#include <AssetCore/AssetFile.h>
#include <AssetCore/AssetImporter.h>
#include <AssetCore/AssetMetadata.h>
#include <AssetCore/TextureAssetFactory.h>
#include <Core/Filesystem/File.h>
#include <Core/Image/PngWriter.h>
#include <Core/Math/Float.h>
#include <Core/Platform/CriticalSection.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Threading/ScopedLock.h>

#include "TestCommon/TestMacros.h"

struct FCompressCall
{
    ETextureFormat SourceFormat;
    ETextureFormat Format;
    int32          NumMips;
};

class FTestBlockCompressor final : public ITextureBlockCompressor
{
public:
    virtual bool Compress(ETextureFormat SourceFormat, const TArray<FTextureMip>& Mips, ETextureFormat Format, TArray<FTextureMip>& OutMips, FString& OutError) override
    {
        {
            SCOPED_LOCK(CallsCS);
            Calls.Add({ SourceFormat, Format, Mips.Size() });
        }

        if (bFail)
        {
            OutError = TEXT("the test compressor says to fail");
            return false;
        }

        for (const FTextureMip& Mip : Mips)
        {
            FTextureMip& Blocks = OutMips.Emplace();
            Blocks.Width  = Mip.Width;
            Blocks.Height = Mip.Height;
            Blocks.Data.Reset(static_cast<int32>(TextureAssets::GetMipSize(Format, Mip.Width, Mip.Height)), static_cast<uint8>(Format));
        }

        return true;
    }

    TArray<FCompressCall> Calls;
    FCriticalSection      CallsCS;
    bool                  bFail = false;
};

static bool WriteBytes(const FString& Filename, const TArray<uint8>& Bytes)
{
    File::CreateDirectoryTree(File::GetDirectoryOf(Filename));
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    return FileHandle && FileHandle->Write(Bytes.Data(), static_cast<uint32>(Bytes.Size())) == Bytes.Size();
}

static bool WritePng(const FString& Filename, int32 Width, int32 Height, const TArray<uint8>& Pixels)
{
    FPngWriter Writer;
    return Writer.WriteToFile(Filename, FImageView(Pixels.Data(), Width, Height));
}

static bool WriteGreyTga(const FString& Filename, int32 Width, int32 Height, const TArray<uint8>& Values)
{
    TArray<uint8> Bytes = { 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    Bytes.Add(static_cast<uint8>(Width & 0xFF));
    Bytes.Add(static_cast<uint8>(Width >> 8));
    Bytes.Add(static_cast<uint8>(Height & 0xFF));
    Bytes.Add(static_cast<uint8>(Height >> 8));
    Bytes.Add(8);
    Bytes.Add(0x20);
    Bytes.Append(Values);
    return WriteBytes(Filename, Bytes);
}

static bool WriteHdr(const FString& Filename, int32 Width, int32 Height)
{
    const FString Header = FString::Printf(TEXT("#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y %d +X %d\n"), Height, Width);

    TArray<uint8> Bytes;
    for (int32 Index = 0; Index < Header.Length(); ++Index)
    {
        Bytes.Add(static_cast<uint8>(Header[Index]));
    }

    for (int32 Index = 0; Index < Width * Height; ++Index)
    {
        Bytes.Append({ 128, 64, 32, 130 });
    }

    return WriteBytes(Filename, Bytes);
}

static TArray<uint8> MakeGradient(int32 Width, int32 Height)
{
    TArray<uint8> Pixels;
    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            Pixels.Append({ static_cast<uint8>(X), static_cast<uint8>(Y), static_cast<uint8>(X + Y), 255 });
        }
    }

    return Pixels;
}

static bool ReadTextureAsset(const FString& DiskPath, FTextureAssetData& OutTexture, FAssetMetadata& OutMetadata, TArray<uint8>& OutThumbnail)
{
    FAssetFileReader Reader;
    TArray<uint8>    Data;
    FString          Error;

    return Reader.Open(DiskPath, Error)
        && Reader.ReadChunk(AssetChunk::Data, Data, Error)
        && Reader.ReadChunk(AssetChunk::Thumbnail, OutThumbnail, Error)
        && TextureAssets::Read(Data, OutTexture, Error)
        && AssetMetadata::ReadFromFile(DiskPath, OutMetadata, Error);
}

static bool BuildOne(const FTextureAssetFactory& Factory, const FString& SourcePath, const FJsonValue& Settings, FTextureAssetData& OutTexture, FString* OutError = nullptr)
{
    TArray<FTextureBuildJob> Jobs;
    Jobs.Emplace().SourcePath = SourcePath;
    Jobs[0].Settings = Settings;

    FString Error;
    const bool bResult = Factory.BuildTextures(Jobs) && TextureAssets::Read(Jobs[0].Data, OutTexture, Error);
    if (OutError)
    {
        *OutError = Jobs[0].Error;
    }

    return bResult;
}

static FJsonValue MakeSettings(const CHAR_T* Name, const FJsonValue& Value)
{
    FJsonValue Settings = FJsonValue::MakeObject();
    Settings.AddMember(Name, FJsonValue(Value));
    return Settings;
}

static FString GetString(const FJsonValue& Object, const CHAR_T* Name)
{
    FString           Text;
    const FJsonValue* Value = Object.Find(Name);
    return (Value && Value->TryGetString(Text)) ? Text : FString();
}

static int32 ReadBigEndian32(const TArray<uint8>& Bytes, int32 Offset)
{
    return (Bytes[Offset] << 24) | (Bytes[Offset + 1] << 16) | (Bytes[Offset + 2] << 8) | Bytes[Offset + 3];
}

static uint16 ReadHalf(const FTextureMip& Mip, int32 Index)
{
    return reinterpret_cast<const uint16*>(Mip.Data.Data())[Index];
}

bool TextureAsset_Test()
{
    TEST_BEGIN();

    const FString Root       = File::MakeAbsolute(TEXT("TextureAssetTests.Scratch"));
    const FString ProjectDir = Root + TEXT("/Game");
    const FString Sources    = ProjectDir + TEXT("/Sources");
    File::DeleteDirectoryTree(Root);

    FContentMounts Mounts;
    TEST_EXPECT(Mounts.Mount(FContentMounts::ProjectRoot, ProjectDir + TEXT("/Content")));
    TEST_EXPECT(File::CreateDirectoryTree(ProjectDir + TEXT("/Content/Textures")));

    TEST_EXPECT(WritePng(Sources + TEXT("/Brick.png"), 512, 256, MakeGradient(512, 256)));
    TEST_EXPECT(WritePng(Sources + TEXT("/Checker.png"), 2, 2, { 0, 0, 0, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255 }));
    TEST_EXPECT(WritePng(Sources + TEXT("/Checker_ORM.png"), 2, 2, { 0, 0, 0, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255 }));
    TEST_EXPECT(WritePng(Sources + TEXT("/Wall_normal.png"), 2, 1, { 255, 128, 128, 255, 128, 255, 128, 255 }));
    TEST_EXPECT(WritePng(Sources + TEXT("/Odd.png"), 6, 6, MakeGradient(6, 6)));
    TEST_EXPECT(WriteGreyTga(Sources + TEXT("/Grey.tga"), 4, 4, { 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240 }));
    TEST_EXPECT(WriteHdr(Sources + TEXT("/Sky.hdr"), 4, 4));
    TEST_EXPECT(WriteBytes(Sources + TEXT("/Broken.png"), { 'n', 'o', 't', ' ', 'a', ' ', 'p', 'n', 'g' }));

    TEST_SECTION(TEXT("The usage is guessed from the name, the channels and whether the image is HDR"));
    {
        TEST_EXPECT(FTextureAssetFactory::GuessUsage(TEXT("C:/Art/Brick_N.png"), 4, false) == ETextureUsage::Normal);
        TEST_EXPECT(FTextureAssetFactory::GuessUsage(TEXT("C:/Art/sponza_thorn_normal.png"), 3, false) == ETextureUsage::Normal);
        TEST_EXPECT(FTextureAssetFactory::GuessUsage(TEXT("C:/Art/Normal.png"), 3, false) == ETextureUsage::Normal);
        TEST_EXPECT(FTextureAssetFactory::GuessUsage(TEXT("C:/Art/Rock-ORM.png"), 3, false) == ETextureUsage::Mask);
        TEST_EXPECT(FTextureAssetFactory::GuessUsage(TEXT("C:/Art/Floor_Roughness.jpg"), 3, false) == ETextureUsage::Single);
        TEST_EXPECT(FTextureAssetFactory::GuessUsage(TEXT("C:/Art/lion.png"), 1, false) == ETextureUsage::Single);
        TEST_EXPECT(FTextureAssetFactory::GuessUsage(TEXT("C:/Art/lion.png"), 4, false) == ETextureUsage::Color);
        TEST_EXPECT(FTextureAssetFactory::GuessUsage(TEXT("C:/Art/sponza_floor_a_diff.png"), 3, false) == ETextureUsage::Color);
        TEST_EXPECT(FTextureAssetFactory::GuessUsage(TEXT("C:/Art/Brick_normal.hdr"), 3, true) == ETextureUsage::HDR);
    }

    TEST_SECTION(TEXT("An image imports with mips, its format, sRGB, a thumbnail and the settings it used"));
    {
        FAssetImporter Importer(Mounts);
        Importer.RegisterFactory(MakeSharedPtr<FTextureAssetFactory>());

        const FAssetImportResult Result = Importer.ImportFile(Sources + TEXT("/Brick.png"), TEXT("/Project/Textures"));
        TEST_EXPECT(Result.Error == EAssetImportError::None && Result.WrittenAssets.Size() == 1);

        FTextureAssetData Texture;
        FAssetMetadata    Metadata;
        TArray<uint8>     Thumbnail;
        TEST_EXPECT(ReadTextureAsset(ProjectDir + TEXT("/Content/Textures/Brick.dxrasset"), Texture, Metadata, Thumbnail));
        TEST_EXPECT(Texture.Format == ETextureFormat::R8G8B8A8_UNorm && Texture.Usage == ETextureUsage::Color && Texture.bSRGB);
        TEST_EXPECT(Texture.Mips.Size() == 10);
        TEST_EXPECT(Texture.Mips.Size() == 10 && Texture.Mips[0].Width == 512 && Texture.Mips[0].Height == 256);
        TEST_EXPECT(Texture.Mips.Size() == 10 && Texture.Mips[8].Width == 2 && Texture.Mips[8].Height == 1);
        TEST_EXPECT(Texture.Mips.Size() == 10 && Texture.Mips[9].Width == 1 && Texture.Mips[9].Height == 1);
        TEST_EXPECT(Texture.Mips.Size() == 10 && Texture.Mips[0].Data == MakeGradient(512, 256));

        TEST_EXPECT(Metadata.Factory.Equals(TEXT("Texture")));
        TEST_EXPECT(GetString(Metadata.ImportSettings, TEXT("Usage")).Equals(TEXT("Color")));

        bool  bSRGB = false;
        bool  bCompress = false;
        bool  bGenerateMips = false;
        int64 MaxResolution = -1;
        TEST_EXPECT(Metadata.ImportSettings.Find(TEXT("SRGB")) && Metadata.ImportSettings.Find(TEXT("SRGB"))->TryGetBool(bSRGB) && bSRGB);
        TEST_EXPECT(Metadata.ImportSettings.Find(TEXT("Compress")) && Metadata.ImportSettings.Find(TEXT("Compress"))->TryGetBool(bCompress) && bCompress);
        TEST_EXPECT(Metadata.ImportSettings.Find(TEXT("GenerateMips")) && Metadata.ImportSettings.Find(TEXT("GenerateMips"))->TryGetBool(bGenerateMips) && bGenerateMips);
        TEST_EXPECT(Metadata.ImportSettings.Find(TEXT("MaxResolution")) && Metadata.ImportSettings.Find(TEXT("MaxResolution"))->TryGetInt64(MaxResolution) && MaxResolution == 0);

        TEST_EXPECT(Thumbnail.Size() > 24 && Thumbnail[0] == 0x89 && Thumbnail[1] == 'P' && Thumbnail[2] == 'N' && Thumbnail[3] == 'G');
        TEST_EXPECT(Thumbnail.Size() > 24 && ReadBigEndian32(Thumbnail, 16) == 256 && ReadBigEndian32(Thumbnail, 20) == 128);

        TEST_EXPECT(Importer.Reimport(TEXT("/Project/Textures/Brick.dxrasset")).Error == EAssetImportError::None);

        FTextureAssetData Reimported;
        TEST_EXPECT(ReadTextureAsset(ProjectDir + TEXT("/Content/Textures/Brick.dxrasset"), Reimported, Metadata, Thumbnail));
        TEST_EXPECT(Reimported.Mips.Size() == Texture.Mips.Size() && Reimported.Mips.LastIndex() >= 0 && Reimported.Mips.Last().Data == Texture.Mips.Last().Data);
    }

    TEST_SECTION(TEXT("Mips average sRGB colour in linear space, and data linearly"));
    {
        FTextureAssetFactory Factory;
        FTextureAssetData    Texture;

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Checker.png"), FJsonValue(), Texture));
        TEST_EXPECT(Texture.bSRGB && Texture.Mips.Size() == 2);
        TEST_EXPECT(Texture.Mips.Size() == 2 && Texture.Mips[1].Data[0] == 188 && Texture.Mips[1].Data[1] == 188 && Texture.Mips[1].Data[3] == 255);

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Checker_ORM.png"), FJsonValue(), Texture));
        TEST_EXPECT(Texture.Usage == ETextureUsage::Mask && !Texture.bSRGB && Texture.Mips.Size() == 2);
        TEST_EXPECT(Texture.Mips.Size() == 2 && Texture.Mips[1].Data[0] == 128 && Texture.Mips[1].Data[3] == 255);

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Checker.png"), MakeSettings(TEXT("SRGB"), FJsonValue(false)), Texture));
        TEST_EXPECT(!Texture.bSRGB && Texture.Mips.Size() == 2 && Texture.Mips[1].Data[0] == 128);

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Checker_ORM.png"), MakeSettings(TEXT("SRGB"), FJsonValue(true)), Texture));
        TEST_EXPECT(Texture.bSRGB && Texture.Mips.Size() == 2 && Texture.Mips[1].Data[0] == 188);
    }

    TEST_SECTION(TEXT("Normal map mips are renormalized"));
    {
        FTextureAssetFactory Factory;
        FTextureAssetData    Texture;

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Wall_normal.png"), FJsonValue(), Texture));
        TEST_EXPECT(Texture.Usage == ETextureUsage::Normal && !Texture.bSRGB && Texture.Format == ETextureFormat::R8G8B8A8_UNorm);
        TEST_EXPECT(Texture.Mips.Size() == 2 && Texture.Mips[1].Data[0] == 218 && Texture.Mips[1].Data[1] == 218 && Texture.Mips[1].Data[2] == 128);

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Wall_normal.png"), MakeSettings(TEXT("SRGB"), FJsonValue(true)), Texture));
        TEST_EXPECT(!Texture.bSRGB);
    }

    TEST_SECTION(TEXT("One channel stores R8, and HDR stores half floats"));
    {
        FTextureAssetFactory Factory;
        FTextureAssetData    Texture;

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Grey.tga"), FJsonValue(), Texture));
        TEST_EXPECT(Texture.Usage == ETextureUsage::Single && Texture.Format == ETextureFormat::R8_UNorm && Texture.Mips.Size() == 3);
        TEST_EXPECT(Texture.Mips.Size() == 3 && Texture.Mips[0].Data.Size() == 16 && Texture.Mips[0].Data[5] == 80 && Texture.Mips[0].Data[15] == 240);
        TEST_EXPECT(Texture.Mips.Size() == 3 && Texture.Mips[2].Data.Size() == 1);

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Sky.hdr"), FJsonValue(), Texture));
        TEST_EXPECT(Texture.Usage == ETextureUsage::HDR && Texture.Format == ETextureFormat::R16G16B16A16_Float && !Texture.bSRGB);
        TEST_EXPECT(Texture.Mips.Size() == 3 && Texture.Mips[0].Data.Size() == 4 * 4 * 8);
        TEST_EXPECT(Texture.Mips.Size() == 3 && ReadHalf(Texture.Mips[0], 0) == FFloat16(2.0f).Encoded && ReadHalf(Texture.Mips[0], 1) == FFloat16(1.0f).Encoded);
        TEST_EXPECT(Texture.Mips.Size() == 3 && ReadHalf(Texture.Mips[2], 2) == FFloat16(0.5f).Encoded && ReadHalf(Texture.Mips[2], 3) == FFloat16(1.0f).Encoded);

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Grey.tga"), MakeSettings(TEXT("Usage"), FJsonValue(TEXT("Color"))), Texture));
        TEST_EXPECT(Texture.Usage == ETextureUsage::Color && Texture.Format == ETextureFormat::R8G8B8A8_UNorm && Texture.bSRGB);
    }

    TEST_SECTION(TEXT("A maximum resolution halves the image until it fits, and mips can be left out"));
    {
        FTextureAssetFactory Factory;
        FTextureAssetData    Texture;

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Brick.png"), MakeSettings(TEXT("MaxResolution"), FJsonValue(int64(128))), Texture));
        TEST_EXPECT(Texture.Mips.Size() == 8 && Texture.Mips[0].Width == 128 && Texture.Mips[0].Height == 64);

        FJsonValue Settings = MakeSettings(TEXT("MaxResolution"), FJsonValue(int64(200)));
        Settings.AddMember(TEXT("GenerateMips"), FJsonValue(false));
        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Brick.png"), Settings, Texture));
        TEST_EXPECT(Texture.Mips.Size() == 1 && Texture.Mips[0].Width == 128 && Texture.Mips[0].Height == 64);

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Brick.png"), MakeSettings(TEXT("MaxResolution"), FJsonValue(int64(4096))), Texture));
        TEST_EXPECT(Texture.Mips.Size() == 10 && Texture.Mips[0].Width == 512);
    }

    TEST_SECTION(TEXT("Compression picks BC7, BC5, BC4 or BC6H from the usage"));
    {
        TSharedPtr<FTestBlockCompressor> Compressor = MakeSharedPtr<FTestBlockCompressor>();
        FTextureAssetFactory             Factory(Compressor);
        FTextureAssetData                Texture;

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Brick.png"), FJsonValue(), Texture));
        TEST_EXPECT(Texture.Format == ETextureFormat::BC7_UNorm && Texture.bSRGB && Texture.Mips.Size() == 10);
        TEST_EXPECT(Texture.Mips.Size() == 10 && Texture.Mips[0].Data.Size() == 128 * 64 * 16 && Texture.Mips[0].Data[0] == static_cast<uint8>(ETextureFormat::BC7_UNorm));
        TEST_EXPECT(Texture.Mips.Size() == 10 && Texture.Mips[9].Width == 1 && Texture.Mips[9].Data.Size() == 16);
        TEST_EXPECT(Compressor->Calls.Size() == 1 && Compressor->Calls[0].SourceFormat == ETextureFormat::R8G8B8A8_UNorm && Compressor->Calls[0].NumMips == 10);

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Checker_ORM.png"), FJsonValue(), Texture) == true);
        TEST_EXPECT(Texture.Format == ETextureFormat::R8G8B8A8_UNorm);

        FJsonValue Mask = MakeSettings(TEXT("Usage"), FJsonValue(TEXT("Mask")));
        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Brick.png"), Mask, Texture) && Texture.Format == ETextureFormat::BC7_UNorm && !Texture.bSRGB);

        FJsonValue Normal = MakeSettings(TEXT("Usage"), FJsonValue(TEXT("Normal")));
        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Brick.png"), Normal, Texture) && Texture.Format == ETextureFormat::BC5_UNorm);

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Grey.tga"), FJsonValue(), Texture) && Texture.Format == ETextureFormat::BC4_UNorm);
        TEST_EXPECT(Texture.Mips.Size() == 3 && Texture.Mips[0].Data.Size() == 8 && Texture.Mips[0].Data[0] == static_cast<uint8>(ETextureFormat::BC4_UNorm));
        TEST_EXPECT(Compressor->Calls.Last().SourceFormat == ETextureFormat::R8_UNorm);

        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Sky.hdr"), FJsonValue(), Texture) && Texture.Format == ETextureFormat::BC6H_UF16);
        TEST_EXPECT(Compressor->Calls.Last().SourceFormat == ETextureFormat::R16G16B16A16_Float);

        const int32 NumCalls = Compressor->Calls.Size();
        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Odd.png"), FJsonValue(), Texture) && Texture.Format == ETextureFormat::R8G8B8A8_UNorm);
        TEST_EXPECT(BuildOne(Factory, Sources + TEXT("/Brick.png"), MakeSettings(TEXT("Compress"), FJsonValue(false)), Texture) && Texture.Format == ETextureFormat::R8G8B8A8_UNorm);
        TEST_EXPECT(Compressor->Calls.Size() == NumCalls);

        Compressor->bFail = true;

        FString Error;
        TEST_EXPECT(!BuildOne(Factory, Sources + TEXT("/Brick.png"), FJsonValue(), Texture, &Error));
        TEST_EXPECT(Error.Contains(TEXT("Brick.png")) && Error.Contains(TEXT("BC7_UNorm")) && Error.Contains(TEXT("the test compressor says to fail")));
    }

    TEST_SECTION(TEXT("Settings that aren't valid fail the import, naming the source"));
    {
        FTextureAssetFactory Factory;
        FTextureAssetData    Texture;
        FString              Error;

        TEST_EXPECT(!BuildOne(Factory, Sources + TEXT("/Brick.png"), MakeSettings(TEXT("Usage"), FJsonValue(TEXT("Banana"))), Texture, &Error));
        TEST_EXPECT(Error.Contains(TEXT("Brick.png")) && Error.Contains(TEXT("Usage")));

        TEST_EXPECT(!BuildOne(Factory, Sources + TEXT("/Brick.png"), MakeSettings(TEXT("MaxResolution"), FJsonValue(int64(-1))), Texture, &Error));
        TEST_EXPECT(Error.Contains(TEXT("MaxResolution")));

        TEST_EXPECT(!BuildOne(Factory, Sources + TEXT("/Brick.png"), MakeSettings(TEXT("SRGB"), FJsonValue(TEXT("yes"))), Texture, &Error));
        TEST_EXPECT(Error.Contains(TEXT("SRGB")));

        TEST_EXPECT(!BuildOne(Factory, Sources + TEXT("/Broken.png"), FJsonValue(), Texture, &Error));
        TEST_EXPECT(Error.Contains(TEXT("Broken.png")) && Error.Contains(TEXT("isn't an image")));

        FAssetImporter Importer(Mounts);
        Importer.RegisterFactory(MakeSharedPtr<FTextureAssetFactory>());

        FAssetImportOptions Options;
        Options.Settings = MakeSettings(TEXT("Usage"), FJsonValue(TEXT("Banana")));
        const FAssetImportResult Result = Importer.ImportFile(Sources + TEXT("/Checker.png"), TEXT("/Project/Textures"), Options);
        TEST_EXPECT(Result.Error == EAssetImportError::ImportFailed && Result.Message.Contains(TEXT("Usage")));
        TEST_EXPECT(!FPlatformFile::IsFile(*(ProjectDir + TEXT("/Content/Textures/Checker.dxrasset"))));
    }

    TEST_SECTION(TEXT("Several textures build in parallel, with the same results as one at a time"));
    {
        TSharedPtr<FTestBlockCompressor> Compressor = MakeSharedPtr<FTestBlockCompressor>();
        FTextureAssetFactory             Factory(Compressor);

        const TArray<FString> Names =
        {
            TEXT("Brick.png"), TEXT("Checker.png"), TEXT("Missing.png"), TEXT("Wall_normal.png"), TEXT("Grey.tga"),
            TEXT("Sky.hdr"), TEXT("Broken.png"), TEXT("Checker_ORM.png"), TEXT("Odd.png"),
        };

        TArray<FTextureBuildJob> Jobs;
        for (const FString& Name : Names)
        {
            Jobs.Emplace().SourcePath = Sources + TEXT("/") + Name;
        }

        TEST_EXPECT(!Factory.BuildTextures(Jobs));

        for (int32 Index = 0; Index < Jobs.Size(); ++Index)
        {
            TArray<FTextureBuildJob> Single;
            Single.Emplace().SourcePath = Jobs[Index].SourcePath;
            Factory.BuildTextures(Single);

            const bool bShouldFail = Names[Index].Equals(TEXT("Missing.png")) || Names[Index].Equals(TEXT("Broken.png"));
            TEST_EXPECT(Jobs[Index].Error.IsEmpty() != bShouldFail);
            TEST_EXPECT(Jobs[Index].Data == Single[0].Data && Jobs[Index].Thumbnail == Single[0].Thumbnail);
            TEST_EXPECT(Jobs[Index].Data.IsEmpty() == bShouldFail);
        }

        TEST_EXPECT(Jobs[2].Error.Contains(TEXT("Couldn't read")) && Jobs[2].Error.Contains(TEXT("Missing.png")));
    }

    TEST_SECTION(TEXT("A texture payload is read back only when every mip has the size its format gives"));
    {
        FTextureAssetData Texture;
        Texture.Format = ETextureFormat::BC7_UNorm;
        Texture.Usage  = ETextureUsage::Mask;
        Texture.bSRGB  = true;

        const int32 Widths[]  = { 8, 4, 2, 1 };
        const int32 Heights[] = { 4, 2, 1, 1 };
        for (int32 Index = 0; Index < 4; ++Index)
        {
            FTextureMip& Mip = Texture.Mips.Emplace();
            Mip.Width  = Widths[Index];
            Mip.Height = Heights[Index];
            Mip.Data.Reset(static_cast<int32>(TextureAssets::GetMipSize(Texture.Format, Mip.Width, Mip.Height)), static_cast<uint8>(Index + 1));
        }

        TEST_EXPECT(TextureAssets::GetMipSize(ETextureFormat::BC7_UNorm, 8, 4) == 32 && TextureAssets::GetMipSize(ETextureFormat::BC4_UNorm, 1, 1) == 8);
        TEST_EXPECT(TextureAssets::GetRowPitch(ETextureFormat::BC5_UNorm, 10) == 48 && TextureAssets::GetNumRows(ETextureFormat::BC5_UNorm, 10) == 3);

        TArray<uint8> Payload;
        TextureAssets::Write(Texture, Payload);

        FTextureAssetData Read;
        FString           Error;
        TEST_EXPECT(TextureAssets::Read(Payload, Read, Error));
        TEST_EXPECT(Read.Format == Texture.Format && Read.Usage == Texture.Usage && Read.bSRGB && Read.Mips.Size() == 4);
        TEST_EXPECT(Read.Mips.Size() == 4 && Read.Mips[1].Width == 4 && Read.Mips[1].Height == 2 && Read.Mips[1].Data == Texture.Mips[1].Data);

        TArray<uint8> Truncated = Payload;
        Truncated.Resize(Payload.Size() - 8);
        TEST_EXPECT(!TextureAssets::Read(Truncated, Read, Error) && Error.Contains(TEXT("Mip 3")));

        TArray<uint8> WrongSize = Payload;
        WrongSize[16] = 9;
        TEST_EXPECT(!TextureAssets::Read(WrongSize, Read, Error) && Error.Contains(TEXT("Mip 0")));

        TArray<uint8> WrongMagic = Payload;
        WrongMagic[0] = 'X';
        TEST_EXPECT(!TextureAssets::Read(WrongMagic, Read, Error) && Error.Contains(TEXT("DXTX")));
    }

    File::DeleteDirectoryTree(Root);

    TEST_END();
}

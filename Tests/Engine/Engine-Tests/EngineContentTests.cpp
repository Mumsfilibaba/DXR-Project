#include "EngineContentTests.h"

#include <AssetCore/AssetFile.h>
#include <AssetCore/TextureAsset.h>
#include <Core/Filesystem/File.h>
#include <Core/Platform/PlatformFile.h>
#include <Engine/Assets/EngineContent.h>
#include <Engine/Assets/MaterialAsset.h>

#include "TestCommon/TestMacros.h"

static bool ReadBytes(const FString& Filename, TArray<uint8>& OutBytes)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
    return FileHandle && File::ReadFile(FileHandle.Get(), OutBytes);
}

static bool ReadAsset(const FString& Filename, FGuid& OutGuid, EAssetType& OutType, TArray<uint8>& OutData)
{
    FAssetFileReader Reader;
    FString          Error;

    if (!Reader.Open(Filename, Error) || !Reader.ReadChunk(AssetChunk::Data, OutData, Error))
    {
        return false;
    }

    OutGuid = Reader.GetGuid();
    OutType = Reader.GetType();
    return true;
}

static bool ReadTexture(const FString& Filename, const FGuid& Guid, ETextureUsage Usage, uint8 Red, uint8 Green, uint8 Blue)
{
    FGuid             ReadGuid;
    EAssetType        Type = EAssetType::Unknown;
    TArray<uint8>     Data;
    FTextureAssetData Texture;
    FString           Error;

    if (!ReadAsset(Filename, ReadGuid, Type, Data) || !TextureAssets::Read(Data, Texture, Error))
    {
        return false;
    }

    return ReadGuid == Guid && Type == EAssetType::Texture && Texture.Usage == Usage && !Texture.bSRGB && Texture.Format == ETextureFormat::R8G8B8A8_UNorm
        && Texture.Mips.Size() == 1 && Texture.Mips[0].Width == 1 && Texture.Mips[0].Height == 1
        && Texture.Mips[0].Data[0] == Red && Texture.Mips[0].Data[1] == Green && Texture.Mips[0].Data[2] == Blue && Texture.Mips[0].Data[3] == 255;
}

bool EngineContent_Test()
{
    TEST_BEGIN();

    const FString Root = File::MakeAbsolute(TEXT("EngineContentTests.Scratch"));
    File::DeleteDirectoryTree(Root);

    TEST_SECTION(TEXT("Every engine asset has its own GUID"));
    {
        const TArray<FGuid> Guids = { EngineContent::BaseTexture, EngineContent::BaseNormal, EngineContent::BaseMaterial, EngineContent::DefaultSky };

        for (int32 Index = 0; Index < Guids.Size(); ++Index)
        {
            TEST_EXPECT(Guids[Index].IsValid());
            for (int32 Other = Index + 1; Other < Guids.Size(); ++Other)
            {
                TEST_EXPECT(Guids[Index] != Guids[Other]);
            }
        }
    }

    TEST_SECTION(TEXT("The content made in code is written with its GUIDs, and the material references the base textures"));
    {
        FString Error;
        TEST_EXPECT(EngineContent::WriteGeneratedContent(Root, Error));
        TEST_EXPECT(Error.IsEmpty());

        TEST_EXPECT(ReadTexture(Root + TEXT("/Textures/BaseTexture.dxrasset"), EngineContent::BaseTexture, ETextureUsage::Color, 255, 255, 255));
        TEST_EXPECT(ReadTexture(Root + TEXT("/Textures/BaseNormal.dxrasset"), EngineContent::BaseNormal, ETextureUsage::Normal, 127, 127, 255));

        FMaterialAssetData Material;
        TEST_EXPECT(MaterialAssets::LoadFromFile(Root + TEXT("/Materials/BaseMaterial.dxrmaterial"), Material, Error));
        TEST_EXPECT(Material.Guid == EngineContent::BaseMaterial);
        TEST_EXPECT(Material.Textures[EMaterialTextureSlot::BaseColor].GetGuid() == EngineContent::BaseTexture);
        TEST_EXPECT(Material.Textures[EMaterialTextureSlot::MaskA].GetGuid() == EngineContent::BaseTexture);
        TEST_EXPECT(Material.Textures[EMaterialTextureSlot::Normal].GetGuid() == EngineContent::BaseNormal);
        TEST_EXPECT(Material.Info.Roughness == 1.0f && Material.Info.Metallic == 0.0f && Material.Info.AmbientOcclusion == 1.0f);
    }

    TEST_SECTION(TEXT("Writing the content again gives the same files"));
    {
        TArray<uint8> First;
        TArray<uint8> Second;
        FString       Error;

        TEST_EXPECT(ReadBytes(Root + TEXT("/Textures/BaseNormal.dxrasset"), First));
        TEST_EXPECT(EngineContent::WriteGeneratedContent(Root, Error));
        TEST_EXPECT(ReadBytes(Root + TEXT("/Textures/BaseNormal.dxrasset"), Second));
        TEST_EXPECT(First.Size() > 0 && First == Second);
    }

    File::DeleteDirectoryTree(Root);

    TEST_END();
}

#include "EnvironmentAssetTests.h"

#include <AssetCore/EnvironmentAsset.h>

#include "TestCommon/TestMacros.h"

static FEnvironmentCube MakeCube(ETextureFormat Format, int32 Size, int32 NumMips, uint8 Fill)
{
    FEnvironmentCube Cube;
    Cube.Format = Format;

    for (int32 Mip = 0; Mip < NumMips; ++Mip)
    {
        const int32 MipSize = Size >> Mip;

        FTextureMip& CubeMip = Cube.Mips.Emplace();
        CubeMip.Width  = MipSize;
        CubeMip.Height = MipSize;
        CubeMip.Data.Reset(static_cast<int32>(TextureAssets::GetMipSize(Format, MipSize, MipSize)) * EnvironmentAssets::NumCubeFaces, static_cast<uint8>(Fill + Mip));
    }

    return Cube;
}

bool EnvironmentAsset_Test()
{
    TEST_BEGIN();

    FEnvironmentAssetData Environment;
    Environment.FilterVersion = 3;
    Environment.Sky           = MakeCube(ETextureFormat::BC6H_UF16, 64, 7, 10);
    Environment.Specular      = MakeCube(ETextureFormat::BC6H_UF16, 32, 5, 20);
    Environment.Diffuse       = MakeCube(ETextureFormat::R16G16B16A16_Float, 8, 1, 30);

    TArray<uint8> Payload;
    EnvironmentAssets::Write(Environment, Payload);

    TEST_SECTION(TEXT("An environment's three cubes and its filter version survive a round trip"));
    {
        FEnvironmentAssetData Read;
        FString               Error;
        TEST_EXPECT(EnvironmentAssets::Read(Payload, Read, Error));
        TEST_EXPECT(Read.FilterVersion == 3);
        TEST_EXPECT(Read.Sky.Format == ETextureFormat::BC6H_UF16 && Read.Sky.Mips.Size() == 7);
        TEST_EXPECT(Read.Sky.Mips.Size() == 7 && Read.Sky.Mips[6].Width == 1 && Read.Sky.Mips[6].Data.Size() == 16 * 6 && Read.Sky.Mips[6].Data[0] == 16);
        TEST_EXPECT(Read.Specular.Mips.Size() == 5 && Read.Specular.Mips[0].Width == 32 && Read.Specular.Mips[0].Data == Environment.Specular.Mips[0].Data);
        TEST_EXPECT(Read.Diffuse.Format == ETextureFormat::R16G16B16A16_Float && Read.Diffuse.Mips.Size() == 1);
        TEST_EXPECT(Read.Diffuse.Mips.Size() == 1 && Read.Diffuse.Mips[0].Data.Size() == 8 * 8 * 8 * 6 && Read.Diffuse.Mips[0].Data[0] == 30);
    }

    TEST_SECTION(TEXT("Damaged environment data is refused"));
    {
        FEnvironmentAssetData Read;
        FString               Error;

        TArray<uint8> WrongMagic = Payload;
        WrongMagic[0] = 'X';
        TEST_EXPECT(!EnvironmentAssets::Read(WrongMagic, Read, Error) && Error.Contains(TEXT("DXEV")));

        TArray<uint8> Truncated = Payload;
        Truncated.Resize(Payload.Size() - 64);
        TEST_EXPECT(!EnvironmentAssets::Read(Truncated, Read, Error) && Error.Contains(TEXT("past the end")));

        TArray<uint8> WrongSize = Payload;
        WrongSize[16 + 3 * 8] = 65;
        TEST_EXPECT(!EnvironmentAssets::Read(WrongSize, Read, Error) && Error.Contains(TEXT("Mip 0 of cube 0")));

        TArray<uint8> NoMips = Payload;
        NoMips[16 + 4] = 0;
        TEST_EXPECT(!EnvironmentAssets::Read(NoMips, Read, Error) && Error.Contains(TEXT("Cube 0")));
    }

    TEST_END();
}

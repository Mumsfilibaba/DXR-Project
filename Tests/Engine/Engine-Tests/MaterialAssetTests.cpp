#include "MaterialAssetTests.h"

#include <AssetCore/AssetMetadata.h>
#include <AssetCore/AssetRegistry.h>
#include <Core/Filesystem/File.h>
#include <Core/Json/Json.h>
#include <Core/Platform/PlatformFile.h>
#include <Engine/Assets/MaterialAsset.h>

#include "TestCommon/TestMacros.h"

static FGuid MakeGuid(const CHAR_T* Text)
{
    FGuid Guid;
    CHECK(FGuid::Parse(FString(Text), Guid));
    return Guid;
}

static FString ToText(const FMaterialAssetData& Material)
{
    return Json::ToString(MaterialAssets::ToJson(Material));
}

static bool ParseMaterial(const FString& Text, FMaterialAssetData& OutMaterial, FString& OutError)
{
    FJsonValue Document;
    FJsonError ParseError;
    if (!Json::Parse(FStringView(Text), Document, &ParseError))
    {
        OutError = ParseError.ToString();
        return false;
    }

    return MaterialAssets::FromJson(Document, OutMaterial, OutError);
}

static FString ReadText(const FString& Filename)
{
    TArray<uint8>           Bytes;
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
    if (FileHandle)
    {
        File::ReadFile(FileHandle.Get(), Bytes);
    }

    FString Text;
    for (uint8 Byte : Bytes)
    {
        Text += static_cast<CHAR_T>(Byte);
    }

    return Text;
}

static FMaterialAssetData MakeFullMaterial()
{
    FMaterialAssetData Material;
    Material.Guid                    = MakeGuid(TEXT("11111111000000010000000100000001"));
    Material.Info.Albedo             = FFloatColor(0.5f, 0.25f, 1.0f, 1.0f);
    Material.Info.Roughness           = 0.75f;
    Material.Info.Opacity            = 0.5f;
    Material.Info.MaterialFlags      = EMaterialFlags::EnableNormalMapping | EMaterialFlags::DoubleSided;
    Material.Info.IndexOfRefraction  = 1.33f;
    Material.Info.Routes[EMaterialScalar::Roughness] = FMaterialTextureRoute(EMaterialTextureSlot::MaskB, ETextureChannel::G, true);
    Material.Info.Routes[EMaterialScalar::Opacity]   = FMaterialTextureRoute();

    Material.Textures[EMaterialTextureSlot::BaseColor] = TAssetRef<FTexture>(MakeGuid(TEXT("22222222000000010000000100000001")), TEXT("/Project/Textures/Brick.dxrasset"));
    Material.Textures[EMaterialTextureSlot::Normal]    = TAssetRef<FTexture>(MakeGuid(TEXT("33333333000000010000000100000001")), TEXT("/Project/Textures/Brick_Normal.dxrasset"));
    Material.Textures[EMaterialTextureSlot::MaskB]     = TAssetRef<FTexture>(MakeGuid(TEXT("22222222000000010000000100000001")), TEXT("/Project/Textures/Brick.dxrasset"));

    Material.EmissiveColor        = FFloatColor(1.0f, 0.5f, 0.0f, 1.0f);
    Material.EmissiveIntensity    = 4.0f;
    Material.EmissiveTexture      = TAssetRef<FTexture>(MakeGuid(TEXT("44444444000000010000000100000001")), TEXT("/Project/Textures/Glow.dxrasset"));
    Material.bMultiplyVertexColor = true;
    return Material;
}

bool MaterialAsset_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("A material with only defaults writes its header and nothing else"));
    {
        FMaterialAssetData Material;
        Material.Guid = MakeGuid(TEXT("11111111000000010000000100000001"));

        const FString Expected = FString::Printf(TEXT("{\n    \"Version\": 1,\n    \"Guid\": \"%s\",\n    \"Type\": \"Material\"\n}\n"), *Material.Guid.ToString());
        TEST_EXPECT(ToText(Material).Equals(Expected));
    }

    TEST_SECTION(TEXT("Every parameter, flag, route, texture and reserved field survives a round trip"));
    {
        const FMaterialAssetData Material = MakeFullMaterial();
        const FString            Text     = ToText(Material);

        FMaterialAssetData Read;
        FString            Error;
        TEST_EXPECT(ParseMaterial(Text, Read, Error));
        TEST_EXPECT(Read.Guid == Material.Guid);
        TEST_EXPECT(Read.Info.Albedo == Material.Info.Albedo && Read.Info.Roughness == 0.75f && Read.Info.Opacity == 0.5f && Read.Info.IndexOfRefraction == 1.33f);
        TEST_EXPECT(Read.Info.Metallic == FMaterialInfo().Metallic && Read.Info.AmbientOcclusion == FMaterialInfo().AmbientOcclusion);
        TEST_EXPECT(Read.Info.MaterialFlags == (EMaterialFlags::EnableNormalMapping | EMaterialFlags::DoubleSided));

        for (int32 Scalar = 0; Scalar < EMaterialScalar::Count; ++Scalar)
        {
            TEST_EXPECT(Read.Info.Routes[Scalar] == Material.Info.Routes[Scalar]);
        }

        for (int32 Slot = 0; Slot < EMaterialTextureSlot::Count; ++Slot)
        {
            TEST_EXPECT(Read.Textures[Slot] == Material.Textures[Slot] && Read.Textures[Slot].GetPath().Equals(Material.Textures[Slot].GetPath()));
        }

        TEST_EXPECT(Read.EmissiveColor == Material.EmissiveColor && Read.EmissiveIntensity == 4.0f && Read.bMultiplyVertexColor);
        TEST_EXPECT(Read.EmissiveTexture == Material.EmissiveTexture);

        TEST_EXPECT(Text.Contains(TEXT("\"EnableNormalMapping\"")) && Text.Contains(TEXT("\"DoubleSided\"")));
        TEST_EXPECT(Text.Contains(TEXT("\"Slot\": \"MaskB\"")) && Text.Contains(TEXT("\"Channel\": \"G\"")) && Text.Contains(TEXT("\"Invert\": true")));
        TEST_EXPECT(Text.Contains(TEXT("\"Slot\": \"None\"")));
        TEST_EXPECT(!Text.Contains(TEXT("\"Metallic\"")) && !Text.Contains(TEXT("\"Occlusion\"")) && !Text.Contains(TEXT("\"Height\"")));
        TEST_EXPECT(!Text.Contains(TEXT("\"ParallaxHeightScale\"")) && !Text.Contains(TEXT("\"RefractionStrength\"")));

        TEST_EXPECT(ToText(Read).Equals(Text));
    }

    TEST_SECTION(TEXT("A saved file is LF-only with a trailing newline, the same bytes every time, and lists its textures as dependencies"));
    {
        const FString Root     = File::MakeAbsolute(TEXT("MaterialAssetTests.Scratch"));
        const FString Filename = Root + TEXT("/Content/Materials/Brick.dxrmaterial");
        File::DeleteDirectoryTree(Root);

        const FMaterialAssetData Material = MakeFullMaterial();
        FString                  Error;
        TEST_EXPECT(MaterialAssets::SaveToFile(Filename, Material, Error));

        const FString First = ReadText(Filename);
        TEST_EXPECT(MaterialAssets::SaveToFile(Filename, Material, Error));
        TEST_EXPECT(ReadText(Filename).Equals(First));
        TEST_EXPECT(!First.Contains(TEXT("\r")) && First.EndsWith(TEXT("}\n")));

        FMaterialAssetData Read;
        TEST_EXPECT(MaterialAssets::LoadFromFile(Filename, Read, Error) && Read.Guid == Material.Guid);

        TArray<FGuid> Dependencies;
        TEST_EXPECT(AssetMetadata::ReadDependencies(Filename, Dependencies, Error));
        TEST_EXPECT(Dependencies.Size() == 3);
        TEST_EXPECT(Dependencies.Size() == 3 && Dependencies[0] == Material.Textures[EMaterialTextureSlot::BaseColor].GetGuid());
        TEST_EXPECT(Dependencies.Size() == 3 && Dependencies[1] == Material.Textures[EMaterialTextureSlot::Normal].GetGuid());
        TEST_EXPECT(Dependencies.Size() == 3 && Dependencies[2] == Material.EmissiveTexture.GetGuid());

        FContentMounts Mounts;
        TEST_EXPECT(Mounts.Mount(FContentMounts::ProjectRoot, Root + TEXT("/Content")));

        FAssetRegistry Registry;
        Registry.SetMounts(Mounts);
        Registry.Rescan();

        const FAssetData* Indexed = Registry.FindByGuid(Material.Guid);
        TEST_EXPECT(Indexed && Indexed->Type == EAssetType::Material && Indexed->Path.Equals(TEXT("/Project/Materials/Brick.dxrmaterial")));

        File::DeleteDirectoryTree(Root);
    }

    TEST_SECTION(TEXT("A material that isn't valid is refused, naming the field"));
    {
        const FString Guid = MakeGuid(TEXT("11111111000000010000000100000001")).ToString();
        const FString Header = FString::Printf(TEXT("\"Version\": 1, \"Guid\": \"%s\", \"Type\": \"Material\""), *Guid);

        FMaterialAssetData Read;
        FString            Error;

        TEST_EXPECT(!ParseMaterial(FString::Printf(TEXT("{ %s, \"Flags\": [ \"DoubleSided\", \"Glowing\" ] }"), *Header), Read, Error));
        TEST_EXPECT(Error.Contains(TEXT("Flags")) && Error.Contains(TEXT("'Glowing' is not a material flag")));

        TEST_EXPECT(!ParseMaterial(FString::Printf(TEXT("{ %s, \"Routes\": { \"Roughness\": { \"Slot\": \"MaskZ\" } } }"), *Header), Read, Error));
        TEST_EXPECT(Error.Contains(TEXT("Routes.Roughness.Slot")) && Error.Contains(TEXT("MaskZ")));

        TEST_EXPECT(!ParseMaterial(FString::Printf(TEXT("{ %s, \"Roughness\": \"high\" }"), *Header), Read, Error));
        TEST_EXPECT(Error.Contains(TEXT("Roughness")));

        TEST_EXPECT(!ParseMaterial(FString::Printf(TEXT("{ %s, \"Textures\": { \"Normal\": 5 } }"), *Header), Read, Error));
        TEST_EXPECT(Error.Contains(TEXT("Textures.Normal")));

        TEST_EXPECT(!ParseMaterial(FString::Printf(TEXT("{ \"Version\": 1, \"Guid\": \"%s\", \"Type\": \"Scene\" }"), *Guid), Read, Error));
        TEST_EXPECT(Error.Contains(TEXT("not Material")));

        TEST_EXPECT(!ParseMaterial(TEXT("{ \"Version\": 1, \"Type\": \"Material\" }"), Read, Error));
        TEST_EXPECT(Error.Contains(TEXT("no Guid")));

        TEST_EXPECT(!ParseMaterial(FString::Printf(TEXT("{ \"Guid\": \"%s\", \"Type\": \"Material\" }"), *Guid), Read, Error));
        TEST_EXPECT(Error.Contains(TEXT("no Version")));

        TEST_EXPECT(!ParseMaterial(FString::Printf(TEXT("{ \"Version\": 2, \"Guid\": \"%s\", \"Type\": \"Material\" }"), *Guid), Read, Error));
        TEST_EXPECT(Error.Contains(TEXT("version 2")));

        TEST_EXPECT(ParseMaterial(FString::Printf(TEXT("{ %s, \"Sheen\": 0.5, \"Roughness\": 0.25 }"), *Header), Read, Error) && Read.Info.Roughness == 0.25f);
    }

    TEST_END();
}

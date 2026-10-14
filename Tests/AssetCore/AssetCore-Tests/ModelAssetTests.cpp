#include "ModelAssetTests.h"

#include <AssetCore/AssetFile.h>
#include <AssetCore/AssetImporter.h>
#include <AssetCore/TextureAssetFactory.h>
#include <Core/Filesystem/File.h>
#include <Core/Image/PngWriter.h>
#include <Core/Platform/PlatformFile.h>
#include <AssetCore/MaterialAsset.h>
#include <AssetCore/MeshAsset.h>
#include <AssetCore/ModelAssetFactory.h>

#include "TestCommon/TestMacros.h"

static bool WriteText(const FString& Filename, const FString& Text)
{
    File::CreateDirectoryTree(File::GetDirectoryOf(Filename));
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    return FileHandle && File::WriteTextFile(FileHandle.Get(), Text);
}

static bool WritePng(const FString& Filename)
{
    TArray<uint8> Pixels;
    for (int32 Index = 0; Index < 8 * 8; ++Index)
    {
        Pixels.Append({ static_cast<uint8>(Index * 4), 128, static_cast<uint8>(255 - Index * 4), 255 });
    }

    File::CreateDirectoryTree(File::GetDirectoryOf(Filename));

    FPngWriter Writer;
    return Writer.WriteToFile(Filename, FImageView(Pixels.Data(), 8, 8));
}

static bool ReadPayload(const FString& DiskPath, TArray<uint8>& OutData, FGuid& OutGuid)
{
    FAssetFileReader Reader;
    FString          Error;
    if (!Reader.Open(DiskPath, Error) || !Reader.ReadChunk(AssetChunk::Data, OutData, Error))
    {
        return false;
    }

    OutGuid = Reader.GetGuid();
    return true;
}

static bool ReadTexture(const FString& DiskPath, FTextureAssetData& OutTexture, FGuid& OutGuid)
{
    TArray<uint8> Data;
    FString       Error;
    return ReadPayload(DiskPath, Data, OutGuid) && TextureAssets::Read(Data, OutTexture, Error);
}

static bool ReadMesh(const FString& DiskPath, FMeshAssetData& OutMesh, FGuid& OutGuid)
{
    TArray<uint8> Data;
    FString       Error;
    return ReadPayload(DiskPath, Data, OutGuid) && MeshAssets::Read(Data, OutMesh, Error);
}

static FJsonValue MakeSettings(const CHAR_T* Name, FJsonValue&& Value)
{
    FJsonValue Settings = FJsonValue::MakeObject();
    Settings.AddMember(Name, ::Move(Value));
    return Settings;
}

bool ModelAsset_Test()
{
    TEST_BEGIN();

    const FString Root       = File::MakeAbsolute(TEXT("ModelAssetTests.Scratch"));
    const FString ProjectDir = Root + TEXT("/Game");
    const FString Sources    = ProjectDir + TEXT("/Sources");
    const FString Content    = ProjectDir + TEXT("/Content");
    File::DeleteDirectoryTree(Root);

    FContentMounts Mounts;
    TEST_EXPECT(Mounts.Mount(FContentMounts::ProjectRoot, Content));
    TEST_EXPECT(File::CreateDirectoryTree(Content));

    TEST_EXPECT(WritePng(Sources + TEXT("/Textures/wood.png")));
    TEST_EXPECT(WritePng(Sources + TEXT("/Textures/metal.png")));
    TEST_EXPECT(WritePng(Sources + TEXT("/Textures/metal_n.png")));
    TEST_EXPECT(WriteText(Sources + TEXT("/Crate.mtl"),
        TEXT("newmtl Wood\nKd 0.5 0.4 0.3\nmap_Kd Textures/wood.png\nmap_Ns Textures/missing.png\n\n")
        TEXT("newmtl Metal\nKa 1 1 1\nmap_Kd Textures/wood.png\nmap_Ka Textures/metal.png\nmap_bump Textures/metal_n.png\n")));
    TEST_EXPECT(WriteText(Sources + TEXT("/Crate.obj"),
        TEXT("mtllib Crate.mtl\nv 0 0 0\nv 1 0 0\nv 0 1 0\nv 1 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nvt 1 1\n")
        TEXT("o Crate\nusemtl Wood\nf 1/1 2/2 3/3\nusemtl Metal\nf 2/2 4/4 3/3\n")
        TEXT("o Lid\nusemtl Wood\nf 1/1 2/2 4/4\n")));

    TSharedPtr<FTextureAssetFactory> TextureFactory = MakeSharedPtr<FTextureAssetFactory>();
    TSharedPtr<FModelAssetFactory>   ModelFactory   = MakeSharedPtr<FModelAssetFactory>(TextureFactory);

    FAssetImporter Importer(Mounts);
    Importer.RegisterFactory(TextureFactory);
    Importer.RegisterFactory(ModelFactory);

    TEST_SECTION(TEXT("Names become file names, and an OBJ's companions are its .mtl and the textures that exist"));
    {
        TEST_EXPECT(FModelAssetFactory::MakeFileName(TEXT("street_light:lamp_1")).Equals(TEXT("street_light_lamp_1")));
        TEST_EXPECT(FModelAssetFactory::MakeFileName(TEXT("Door <left>. ")).Equals(TEXT("Door _left_")));
        TEST_EXPECT(FModelAssetFactory::MakeFileName(TEXT("")).Equals(TEXT("Unnamed")));

        TArray<FString> Companions;
        ModelFactory->GetCompanionFiles(Sources + TEXT("/Crate.obj"), Companions);
        TEST_EXPECT(Companions.Size() == 4);
        TEST_EXPECT(Companions.Contains(File::CombinePath(Sources, TEXT("Crate.mtl"))));
        TEST_EXPECT(Companions.Contains(Sources + TEXT("/Textures/metal_n.png")) && !Companions.Contains(Sources + TEXT("/Textures/missing.png")));
    }

    TEST_SECTION(TEXT("A model imports into meshes with material slots, materials and textures with the usage the materials give them"));
    {
        const FAssetImportResult Result = Importer.ImportFile(Sources + TEXT("/Crate.obj"), TEXT("/Project/Crate"));
        TEST_EXPECT(Result.Error == EAssetImportError::None && Result.WrittenAssets.Size() == 7);
        TEST_EXPECT(Result.WrittenAssets.Contains(FString(TEXT("/Project/Crate/Textures/wood.dxrasset"))));
        TEST_EXPECT(Result.WrittenAssets.Contains(FString(TEXT("/Project/Crate/Materials/Metal.dxrmaterial"))));
        TEST_EXPECT(Result.WrittenAssets.Contains(FString(TEXT("/Project/Crate/Meshes/Lid.dxrasset"))));
        TEST_EXPECT(Result.Warnings.Size() == 1 && Result.Warnings[0].Contains(TEXT("missing.png")));

        FTextureAssetData Wood;
        FTextureAssetData Metal;
        FTextureAssetData MetalNormal;
        FGuid             WoodGuid;
        FGuid             MetalGuid;
        FGuid             MetalNormalGuid;
        TEST_EXPECT(ReadTexture(Content + TEXT("/Crate/Textures/wood.dxrasset"), Wood, WoodGuid));
        TEST_EXPECT(ReadTexture(Content + TEXT("/Crate/Textures/metal.dxrasset"), Metal, MetalGuid));
        TEST_EXPECT(ReadTexture(Content + TEXT("/Crate/Textures/metal_n.dxrasset"), MetalNormal, MetalNormalGuid));
        TEST_EXPECT(Wood.Usage == ETextureUsage::Color && Wood.bSRGB);
        TEST_EXPECT(Metal.Usage == ETextureUsage::Single && Metal.Format == ETextureFormat::R8_UNorm);
        TEST_EXPECT(MetalNormal.Usage == ETextureUsage::Normal);

        FMaterialAssetData WoodMaterial;
        FMaterialAssetData MetalMaterial;
        FString            Error;
        TEST_EXPECT(MaterialAssets::LoadFromFile(Content + TEXT("/Crate/Materials/Wood.dxrmaterial"), WoodMaterial, Error));
        TEST_EXPECT(MaterialAssets::LoadFromFile(Content + TEXT("/Crate/Materials/Metal.dxrmaterial"), MetalMaterial, Error));

        TEST_EXPECT(WoodMaterial.Textures[EMaterialTextureSlot::BaseColor].GetGuid() == WoodGuid);
        TEST_EXPECT(WoodMaterial.Textures[EMaterialTextureSlot::BaseColor].GetPath().Equals(TEXT("/Project/Crate/Textures/wood.dxrasset")));
        TEST_EXPECT(!WoodMaterial.Textures[EMaterialTextureSlot::MaskA].IsSet() && !WoodMaterial.Info.Routes[EMaterialScalar::Roughness].IsRouted());
        TEST_EXPECT(WoodMaterial.Info.Albedo == FFloatColor(0.5f, 0.4f, 0.3f, 1.0f));

        TEST_EXPECT(MetalMaterial.Textures[EMaterialTextureSlot::BaseColor].GetGuid() == WoodGuid);
        TEST_EXPECT(MetalMaterial.Textures[EMaterialTextureSlot::Normal].GetGuid() == MetalNormalGuid);
        TEST_EXPECT(MetalMaterial.Textures[EMaterialTextureSlot::MaskA].GetGuid() == MetalGuid);
        TEST_EXPECT(MetalMaterial.Info.Routes[EMaterialScalar::Metallic] == FMaterialTextureRoute(EMaterialTextureSlot::MaskA, ETextureChannel::R));
        TEST_EXPECT(IsEnumFlagSet(MetalMaterial.Info.MaterialFlags, EMaterialFlags::EnableNormalMapping) && MetalMaterial.Info.Metallic == 1.0f);

        FMeshAssetData Crate;
        FMeshAssetData Lid;
        FGuid          CrateGuid;
        FGuid          LidGuid;
        TEST_EXPECT(ReadMesh(Content + TEXT("/Crate/Meshes/Crate.dxrasset"), Crate, CrateGuid));
        TEST_EXPECT(ReadMesh(Content + TEXT("/Crate/Meshes/Lid.dxrasset"), Lid, LidGuid));
        TEST_EXPECT(Crate.MaterialSlots.Size() == 2 && Crate.Mesh.SubMeshes.Size() == 2);
        TEST_EXPECT(Crate.MaterialSlots.Size() == 2 && Crate.MaterialSlots[0].Name.Equals(TEXT("Wood")) && Crate.MaterialSlots[0].Material.GetGuid() == WoodMaterial.Guid);
        TEST_EXPECT(Crate.MaterialSlots.Size() == 2 && Crate.MaterialSlots[1].Material.GetPath().Equals(TEXT("/Project/Crate/Materials/Metal.dxrmaterial")));
        TEST_EXPECT(Lid.MaterialSlots.Size() == 1 && Lid.MaterialSlots[0].Material.GetGuid() == WoodMaterial.Guid);

        const FString WoodFile = Content + TEXT("/Crate/Materials/Wood.dxrmaterial");
        WoodMaterial.Info.Roughness = 0.25f;
        TEST_EXPECT(MaterialAssets::SaveToFile(WoodFile, WoodMaterial, Error));

        const FAssetImportResult Reimported = Importer.Reimport(TEXT("/Project/Crate/Meshes/Lid.dxrasset"));
        TEST_EXPECT(Reimported.Error == EAssetImportError::None && Reimported.WrittenAssets.Size() == 6);
        TEST_EXPECT(Reimported.KeptAssets.Size() == 1 && Reimported.KeptAssets[0].Equals(TEXT("/Project/Crate/Materials/Wood.dxrmaterial")));

        FMaterialAssetData Kept;
        FGuid              ReimportedGuid;
        TEST_EXPECT(MaterialAssets::LoadFromFile(WoodFile, Kept, Error) && Kept.Info.Roughness == 0.25f);
        TEST_EXPECT(ReadMesh(Content + TEXT("/Crate/Meshes/Crate.dxrasset"), Crate, ReimportedGuid) && ReimportedGuid == CrateGuid);
        TEST_EXPECT(ReadTexture(Content + TEXT("/Crate/Textures/wood.dxrasset"), Wood, ReimportedGuid) && ReimportedGuid == WoodGuid);
        TEST_EXPECT(Crate.MaterialSlots.Size() == 2 && Crate.MaterialSlots[0].Material.GetGuid() == WoodMaterial.Guid);

        TEST_EXPECT(Importer.Reimport(TEXT("/Project/Crate/Meshes/Lid.dxrasset"), FString(), true).KeptAssets.IsEmpty());
        TEST_EXPECT(MaterialAssets::LoadFromFile(WoodFile, Kept, Error) && Kept.Info.Roughness == 1.0f && Kept.Guid == WoodMaterial.Guid);
    }

    TEST_SECTION(TEXT("Only the selected nodes are imported, with the materials and textures they use"));
    {
        FAssetImportOptions Options;
        Options.Settings = FJsonValue::MakeObject();

        FJsonValue Nodes = FJsonValue::MakeArray();
        Nodes.Add(FJsonValue(TEXT("Lid")));
        Options.Settings.AddMember(TEXT("Nodes"), ::Move(Nodes));

        const FAssetImportResult Result = Importer.ImportFile(Sources + TEXT("/Crate.obj"), TEXT("/Project/LidOnly"), Options);
        TEST_EXPECT(Result.Error == EAssetImportError::None && Result.WrittenAssets.Size() == 3);
        TEST_EXPECT(Result.WrittenAssets.Contains(FString(TEXT("/Project/LidOnly/Meshes/Lid.dxrasset"))) && !FPlatformFile::IsFile(*(Content + TEXT("/LidOnly/Meshes/Crate.dxrasset"))));
        TEST_EXPECT(Result.WrittenAssets.Contains(FString(TEXT("/Project/LidOnly/Materials/Wood.dxrmaterial"))));

        FJsonValue Unknown = FJsonValue::MakeArray();
        Unknown.Add(FJsonValue(TEXT("Crate/Handle")));
        Options.Settings = MakeSettings(TEXT("Nodes"), ::Move(Unknown));

        const FAssetImportResult Refused = Importer.ImportFile(Sources + TEXT("/Crate.obj"), TEXT("/Project/Handle"), Options);
        TEST_EXPECT(Refused.Error == EAssetImportError::ImportFailed && Refused.Message.Contains(TEXT("Crate/Handle")));
    }

    TEST_SECTION(TEXT("Combining bakes the selected nodes into one mesh, a submesh for each part and a slot for each material"));
    {
        FAssetImportOptions Options;
        Options.Settings = MakeSettings(TEXT("Mode"), FJsonValue(TEXT("Combine")));

        const FAssetImportResult Result = Importer.ImportFile(Sources + TEXT("/Crate.obj"), TEXT("/Project/Combined"), Options);
        TEST_EXPECT(Result.Error == EAssetImportError::None && Result.WrittenAssets.Size() == 6);

        FMeshAssetData Combined;
        FGuid          CombinedGuid;
        TEST_EXPECT(ReadMesh(Content + TEXT("/Combined/Meshes/Crate.dxrasset"), Combined, CombinedGuid));
        FMeshAssetData Crate;
        FMeshAssetData Lid;
        FGuid          PartGuid;
        TEST_EXPECT(ReadMesh(Content + TEXT("/Crate/Meshes/Crate.dxrasset"), Crate, PartGuid) && ReadMesh(Content + TEXT("/Crate/Meshes/Lid.dxrasset"), Lid, PartGuid));
        TEST_EXPECT(Combined.Mesh.PackedVertexCount == Crate.Mesh.PackedVertexCount + Lid.Mesh.PackedVertexCount && Combined.Mesh.Indices.Size() == 9);
        TEST_EXPECT(Combined.Mesh.SubMeshes.Size() == 3 && Combined.MaterialSlots.Size() == 2);
        TEST_EXPECT(Combined.Mesh.SubMeshes.Size() == 3 && Combined.Mesh.SubMeshes[0].MaterialIndex == 0 && Combined.Mesh.SubMeshes[1].MaterialIndex == 1 && Combined.Mesh.SubMeshes[2].MaterialIndex == 0);
        TEST_EXPECT(Combined.Mesh.SubMeshes.Size() == 3 && Combined.Mesh.SubMeshes[2].StartIndex == 6 && Combined.Mesh.Indices[6] >= uint32(Crate.Mesh.PackedVertexCount));
        TEST_EXPECT(Combined.MaterialSlots.Size() == 2 && Combined.MaterialSlots[0].Name.Equals(TEXT("Wood")) && Combined.MaterialSlots[1].Name.Equals(TEXT("Metal")));
    }

    File::DeleteDirectoryTree(Root);

    TEST_END();
}

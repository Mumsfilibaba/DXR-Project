#include "AssetCore/AssetFile.h"
#include "AssetCore/AssetImporter.h"
#include "AssetCore/AssetMetadata.h"
#include "AssetCore/ContentMounts.h"
#include "AssetCore/TextureAsset.h"
#include "Core/Filesystem/File.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/Paths.h"
#include "Core/Platform/PlatformFile.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/EngineContent.h"
#include "Engine/Assets/MaterialAsset.h"

static constexpr const CHAR_T* TexturesFolder     = TEXT("Textures");
static constexpr const CHAR_T* MaterialsFolder    = TEXT("Materials");
static constexpr const CHAR_T* EnvironmentsFolder = TEXT("Environments");

#if EDITOR_BUILD
static FAutoConsoleCommand CCmdImportEngineContent(
    TEXT("Engine.ImportEngineContent"),
    TEXT("Writes the engine content made in code and imports the default sky into <Engine>/Content, for the engine to use from its next start"),
    FConsoleCommandDelegate::CreateLambda([](FStringView)
    {
        FAssetManager& AssetManager = FAssetManager::Get();
        const FString  ContentDir   = Paths::GetEngineContentDir();

        FString Error;
        if (!EngineContent::WriteGeneratedContent(ContentDir, Error) || !EngineContent::ImportDefaultSky(AssetManager.GetAssetImporter(), Error))
        {
            LOG_ERROR("[EngineContent]: The engine content couldn't be created: %s", *Error);
            return;
        }

        AssetManager.GetAssetRegistry().Rescan();
        LOG_INFO("[EngineContent]: Wrote the engine content to '%s'; the engine uses it from its next start", *ContentDir);
    }));
#endif

static FString GetVirtualPath(const CHAR_T* Folder, const CHAR_T* Name, const CHAR_T* Extension)
{
    return FString(FContentMounts::EngineRoot) + Folder + TEXT("/") + Name + Extension;
}

static FString GetDiskPath(const FString& ContentDirectory, const CHAR_T* Folder, const CHAR_T* Name, const CHAR_T* Extension)
{
    return File::CombinePath(File::CombinePath(ContentDirectory, Folder), FString(Name) + Extension);
}

static bool WriteBinaryAsset(const FString& Filename, const FGuid& Guid, EAssetType Type, const CHAR_T* Name, TArray<uint8>&& Data, const TArray<FGuid>& Dependencies, FString& OutError)
{
    FAssetMetadata Metadata;
    Metadata.Name         = Name;
    Metadata.Factory      = TEXT("EngineContent");
    Metadata.Dependencies = Dependencies;

    FAssetFileWriter Writer(Guid, Type);
    Writer.SetMetadata(AssetMetadata::ToJson(Metadata));
    Writer.SetChunk(AssetChunk::Data, ::Move(Data));

    if (!File::CreateDirectoryTree(File::GetDirectoryOf(Filename)))
    {
        OutError = FString::Printf(TEXT("Could not create the folder of '%s'"), *Filename);
        return false;
    }

    return Writer.SaveToFile(Filename, OutError);
}

static bool WriteTexture(const FString& ContentDirectory, const FGuid& Guid, const CHAR_T* Name, ETextureUsage Usage, const uint8 (&Pixel)[4], FString& OutError)
{
    FTextureAssetData Texture;
    Texture.Format = ETextureFormat::R8G8B8A8_UNorm;
    Texture.Usage  = Usage;
    Texture.bSRGB  = false;

    FTextureMip& Mip = Texture.Mips.Emplace();
    Mip.Width  = 1;
    Mip.Height = 1;
    Mip.Data   = TArray<uint8>(Pixel, 4);

    TArray<uint8> Payload;
    TextureAssets::Write(Texture, Payload);
    return WriteBinaryAsset(GetDiskPath(ContentDirectory, TexturesFolder, Name, FAssetFile::FileExtension), Guid, EAssetType::Texture, Name, ::Move(Payload), TArray<FGuid>(), OutError);
}

static bool WriteBaseMaterial(const FString& ContentDirectory, FString& OutError)
{
    FMaterialAssetData Material;
    Material.Guid                  = EngineContent::BaseMaterial;
    Material.Info.Albedo           = FFloatColor::White;
    Material.Info.Metallic         = 0.0f;
    Material.Info.AmbientOcclusion = 1.0f;
    Material.Info.Roughness        = 1.0f;

    const TAssetRef<FTexture> BaseTexture(EngineContent::BaseTexture, GetVirtualPath(TexturesFolder, TEXT("BaseTexture"), FAssetFile::FileExtension));
    Material.Textures[EMaterialTextureSlot::BaseColor] = BaseTexture;
    Material.Textures[EMaterialTextureSlot::MaskA]     = BaseTexture;
    Material.Textures[EMaterialTextureSlot::Normal]    = TAssetRef<FTexture>(EngineContent::BaseNormal, GetVirtualPath(TexturesFolder, TEXT("BaseNormal"), FAssetFile::FileExtension));

    const FString Filename = GetDiskPath(ContentDirectory, MaterialsFolder, TEXT("BaseMaterial"), AssetTypes::GetTextExtension(EAssetType::Material));
    if (!File::CreateDirectoryTree(File::GetDirectoryOf(Filename)))
    {
        OutError = FString::Printf(TEXT("Could not create the folder of '%s'"), *Filename);
        return false;
    }

    return MaterialAssets::SaveToFile(Filename, Material, OutError);
}

bool EngineContent::WriteGeneratedContent(const FString& ContentDirectory, FString& OutError)
{
    static constexpr uint8 White[4]      = { 255, 255, 255, 255 };
    static constexpr uint8 FlatNormal[4] = { 127, 127, 255, 255 };

    if (!WriteTexture(ContentDirectory, BaseTexture, TEXT("BaseTexture"), ETextureUsage::Color, White, OutError) ||
        !WriteTexture(ContentDirectory, BaseNormal, TEXT("BaseNormal"), ETextureUsage::Normal, FlatNormal, OutError) ||
        !WriteBaseMaterial(ContentDirectory, OutError))
    {
        return false;
    }

    return true;
}

bool EngineContent::ImportDefaultSky(FAssetImporter& Importer, FString& OutError)
{
    const FString Source = File::CombinePath(File::CombinePath(Paths::GetEngineDir(), FAssetImporter::ContentSourcesFolder), DefaultSkySource);

    FAssetImportOptions Options;
    Options.Factory          = TEXT("Environment");
    Options.bReplaceExisting = true;
    Options.Guids.Add(File::ExtractFilenameWithoutExtension(Source), DefaultSky);

    const FAssetImportResult Result = Importer.ImportFile(Source, FString(FContentMounts::EngineRoot) + EnvironmentsFolder, Options);
    if (Result.Error != EAssetImportError::None)
    {
        OutError = Result.Message;
        return false;
    }

    return true;
}

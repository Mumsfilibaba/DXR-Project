#include "AssetManager.h"
#include "TextureImporterBase.h"
#include "TextureImporterDDS.h"

#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Utilities/StringUtilities.h"

#include "RHI/RHICommandList.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAssetManager

FAssetManager* FAssetManager::GInstance = nullptr;

FAssetManager::FAssetManager()
    : Textures()
    , TextureMap()
    , TexturesCS()
    , TextureImporters()
{
    TextureImporters.Emplace(dbg_new FTextureImporterBase());
    TextureImporters.Emplace(dbg_new FTextureImporterDDS());
}

FAssetManager::~FAssetManager()
{
    for (ITextureImporter* Importer : TextureImporters)
    {
        delete Importer;
    }

    TextureImporters.Clear();
}

bool FAssetManager::Initialize()
{
    if (!GInstance)
    {
        GInstance = dbg_new FAssetManager();
        return true;
    }

    return false;
}

void FAssetManager::Release()
{
    if (GInstance)
    {
        delete GInstance;
        GInstance = nullptr;
    }
}

FAssetManager& FAssetManager::Get()
{
    CHECK(GInstance != nullptr);
    return *GInstance;
}

FTextureResourceRef FAssetManager::LoadTexture(const FString& Filename, bool bGenerateMips)
{
    SCOPED_LOCK(TexturesCS);

    // Convert backslashes
    FString FinalPath = Filename;
    ConvertBackslashes(FinalPath);

    const auto ExistingTexture = TextureMap.find(FinalPath);
    if (ExistingTexture != TextureMap.end())
    {
        const uint32 TextureIndex = ExistingTexture->second;
        return Textures[TextureIndex];
    }

    FTextureResourceRef NewTexture;
    for (ITextureImporter* Importer : TextureImporters)
    {
        FStringView FileNameView(FinalPath);
        if (Importer->MatchExtenstion(FileNameView))
        {
            NewTexture = Importer->ImportFromFile(FileNameView);
        }
    }

    if (!NewTexture)
    {
        LOG_ERROR("[FAssetManager]: Unsupported texture format. Failed to load '%s'.", FinalPath.GetCString());
        return nullptr;
    }

    if (!NewTexture->CreateRHITexture(bGenerateMips))
    {
        return nullptr;
    }

    LOG_INFO("[FAssetManager]: Loaded Texture '%s'", FinalPath.GetCString());

    // Set name 
    NewTexture->SetName(FinalPath);

    // Insert the new texture
    const auto Index = Textures.GetSize();
    Textures.Emplace(NewTexture);
    TextureMap.insert(std::make_pair(FinalPath, Index));
    return NewTexture;
}

void FAssetManager::UnloadTexture(const FTextureResourceRef& Texture)
{
    SCOPED_LOCK(TexturesCS);
    Textures.Remove(Texture);
}

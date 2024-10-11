#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Utilities/StringUtilities.h"
#include "RHI/RHICommandList.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/TextureImporterDDS.h"
#include "Engine/Assets/TextureImporterBase.h"
#include "Engine/Assets/AssetLoaders/ModelImporter.h"

FAssetManager* FAssetManager::GInstance = nullptr;

FAssetManager::FAssetManager()
    : MeshImporters()
    , MeshImportersCS()
    , MeshesMap()
    , Meshes()
    , MeshesCS()
    , TextureImporters()
    , TextureImportersCS()
    , TextureMap()
    , Textures()
    , TexturesCS()
{
}

FAssetManager::~FAssetManager()
{
    {
        SCOPED_LOCK(TextureImportersCS);
        TextureImporters.Clear();
    }

    {
        SCOPED_LOCK(MeshImportersCS);
        MeshImporters.Clear();
    }
}

bool FAssetManager::Initialize()
{
    if (!GInstance)
    {
        GInstance = new FAssetManager();
        CHECK(GInstance != nullptr);
        
        GInstance->RegisterTextureImporter(MakeSharedPtr<FTextureImporterDDS>());
        GInstance->RegisterTextureImporter(MakeSharedPtr<FTextureImporterBase>());
        
        GInstance->RegisterModelImporter(MakeSharedPtr<FModelImporter>());
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

TSharedRef<FTexture> FAssetManager::LoadTexture(const FString& Filename, bool bGenerateMips)
{
    SCOPED_LOCK(TexturesCS);

    // Convert backslashes
    FString FinalPath = Filename;
    ConvertBackslashes(FinalPath);
    
    if (int32* TextureID = TextureMap.Find(FinalPath))
    {
        const int32 TextureIndex = *TextureID;
        return Textures[TextureIndex];
    }

    TSharedRef<FTexture> NewTexture;
    
    {
        SCOPED_LOCK(TextureImportersCS);
        
        for (TSharedPtr<ITextureImporter> Importer : TextureImporters)
        {
            FStringView FileNameView(FinalPath);
            if (Importer->MatchExtenstion(FileNameView))
            {
                NewTexture = Importer->ImportFromFile(FileNameView);
            }
        }
    }

    if (!NewTexture)
    {
        LOG_ERROR("[FAssetManager]: Unsupported texture format. Failed to load '%s'.", FinalPath.GetCString());
        return nullptr;
    }

    if (IsBlockCompressed(NewTexture->GetFormat()))
    {
        bGenerateMips = false;
    }

    if (!NewTexture->CreateRHITexture(bGenerateMips))
    {
        LOG_ERROR("[FAssetManager]: Failed to create RHI texture for image '%s'.", FinalPath.GetCString());
        return nullptr;
    }

    LOG_INFO("[FAssetManager]: Loaded Texture '%s'", FinalPath.GetCString());

    // Set name 
    NewTexture->SetDebugName(FinalPath); // For the RHI
    NewTexture->SetFilename(FinalPath);  // For the AssetSystem

    // Release the data after the texture is loaded
    NewTexture->ReleaseData();

    // Insert the new texture
    const int32 Index = Textures.Size();
    Textures.Emplace(NewTexture);
    TextureMap.Add(FinalPath, Index);
    return NewTexture;
}

TSharedRef<FModel> FAssetManager::LoadModel(const FString& Filename, EMeshImportFlags Flags)
{
    SCOPED_LOCK(MeshesCS);

    // Convert backslashes
    FString FinalPath = Filename;
    ConvertBackslashes(FinalPath);
    
    if (int32* MeshID = MeshesMap.Find(FinalPath))
    {
        const int32 MeshIndex = *MeshID;
        return Meshes[MeshIndex];
    }
    
    TSharedRef<FModel> NewModel;
    
    {
        SCOPED_LOCK(MeshImportersCS);
        
        for (TSharedPtr<IModelImporter> Importer : MeshImporters)
        {
            FStringView FileNameView(FinalPath);
            if (Importer->MatchExtenstion(FileNameView))
            {
                NewModel = Importer->ImportFromFile(FileNameView, Flags);
            }
        }
    }
    
    if (!NewModel)
    {
        LOG_ERROR("[FAssetManager]: Unsupported mesh format. Failed to load '%s'.", FinalPath.GetCString());
        return nullptr;
    }
    
    LOG_INFO("[FAssetManager]: Loaded Mesh '%s'", FinalPath.GetCString());

    // Insert the new texture
    const int32 Index = Meshes.Size();
    Meshes.Emplace(NewModel);
    MeshesMap.Add(FinalPath, Index);
    return NewModel;
}

void FAssetManager::UnloadModel(const TSharedRef<FModel>& InModel)
{
    SCOPED_LOCK(MeshesCS);
        
    Meshes.Remove(InModel);
    // TODO: Remove mesh from map
}

void FAssetManager::UnloadTexture(const FTextureRef& Texture)
{
    SCOPED_LOCK(TexturesCS);
    
    const FString& Filename = Texture->GetFilename();
    TextureMap.Remove(Filename);
    Textures.Remove(Texture);
}

void FAssetManager::RegisterTextureImporter(const TSharedPtr<ITextureImporter>& InImporter)
{
    SCOPED_LOCK(TextureImportersCS);
    TextureImporters.AddUnique(InImporter);
}

void FAssetManager::UnregisterTextureImporter(const TSharedPtr<ITextureImporter>& InImporter)
{
    SCOPED_LOCK(TextureImportersCS);
    TextureImporters.Remove(InImporter);
}

void FAssetManager::RegisterModelImporter(const TSharedPtr<IModelImporter>& InImporter)
{
    SCOPED_LOCK(MeshImportersCS);
    MeshImporters.AddUnique(InImporter);
}

void FAssetManager::UnregisterModelImporter(const TSharedPtr<IModelImporter>& InImporter)
{
    SCOPED_LOCK(MeshImportersCS);
    MeshImporters.Remove(InImporter);
}

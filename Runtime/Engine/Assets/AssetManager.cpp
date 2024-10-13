#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Utilities/StringUtilities.h"
#include "RHI/RHICommandList.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetImporters/TextureImporterDDS.h"
#include "Engine/Assets/AssetImporters/TextureImporterBase.h"
#include "Engine/Assets/AssetImporters/ModelImporter.h"
#include "Engine/Assets/AssetImporters/FBXImporter.h"
#include "Engine/Assets/AssetImporters/OBJImporter.h"

FAssetManager* FAssetManager::GInstance = nullptr;

FAssetManager::FAssetManager()
    : ModelImporters()
    , ModelImportersCS()
    , ModelsMap()
    , Models()
    , ModelsCS()
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
        SCOPED_LOCK(ModelImportersCS);
        ModelImporters.Clear();
    }
}

bool FAssetManager::Initialize()
{
    if (!GInstance)
    {
        GInstance = new FAssetManager();
        CHECK(GInstance != nullptr);
        
        // Importers for textures
        GInstance->RegisterTextureImporter(MakeSharedPtr<FTextureImporterDDS>());
        GInstance->RegisterTextureImporter(MakeSharedPtr<FTextureImporterBase>());
        
        // Importers for models
        GInstance->RegisterModelImporter(MakeSharedPtr<FFBXImporter>());
        GInstance->RegisterModelImporter(MakeSharedPtr<FOBJImporter>());
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
    SCOPED_LOCK(ModelsCS);

    // Convert backslashes
    FString FinalPath = Filename;
    ConvertBackslashes(FinalPath);
    
    if (int32* MeshID = ModelsMap.Find(FinalPath))
    {
        const int32 MeshIndex = *MeshID;
        return Models[MeshIndex];
    }
    
    TSharedRef<FModel> NewModel;
    
    {
        SCOPED_LOCK(ModelImportersCS);
        
        for (TSharedPtr<IModelImporter> Importer : ModelImporters)
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
    const int32 Index = Models.Size();
    Models.Emplace(NewModel);
    ModelsMap.Add(FinalPath, Index);
    return NewModel;
}

void FAssetManager::UnloadModel(const TSharedRef<FModel>& InModel)
{
    SCOPED_LOCK(ModelsCS);
        
    Models.Remove(InModel);
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
    SCOPED_LOCK(ModelImportersCS);
    ModelImporters.AddUnique(InImporter);
}

void FAssetManager::UnregisterModelImporter(const TSharedPtr<IModelImporter>& InImporter)
{
    SCOPED_LOCK(ModelImportersCS);
    ModelImporters.Remove(InImporter);
}

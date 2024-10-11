#pragma once
#include "Core/Containers/Map.h"
#include "Core/Platform/CriticalSection.h"
#include "Engine/Assets/TextureResource.h"
#include "Engine/Assets/ITextureImporter.h"
#include "Engine/Assets/IModelImporter.h"
#include "Engine/Resources/Mesh.h"

class ENGINE_API FAssetManager
{
public:

    /** @brief - Create and initialize the AssetManager */
    static bool Initialize();

    /** @brief - Destroy the AssetManager */
    static void Release();

    /** @brief - Retrieve the AssetManager instance */
    static FAssetManager& Get();

    TSharedRef<FTexture> LoadTexture(const FString& Filename, bool bGenerateMips = true);
    void UnloadTexture(const TSharedRef<FTexture>& Texture);
    
    TSharedRef<FModel> LoadModel(const FString& Filename, EMeshImportFlags Flags = EMeshImportFlags::Default);
    void UnloadModel(const TSharedRef<FModel>& InModel);

    void RegisterTextureImporter(const TSharedPtr<ITextureImporter>& InImporter);
    void UnregisterTextureImporter(const TSharedPtr<ITextureImporter>& InImporter);
    
    void RegisterModelImporter(const TSharedPtr<IModelImporter>& InImporter);
    void UnregisterModelImporter(const TSharedPtr<IModelImporter>& InImporter);

private:
    FAssetManager();
    ~FAssetManager();

    // Meshes
    TArray<TSharedPtr<IModelImporter>>   MeshImporters;
    FCriticalSection                     MeshImportersCS;
    TMap<FString, int32>                 MeshesMap;
    TArray<TSharedRef<FModel>>           Meshes;
    FCriticalSection                     MeshesCS;
    
    // Textures
    TArray<TSharedPtr<ITextureImporter>> TextureImporters;
    FCriticalSection                     TextureImportersCS;
    TMap<FString, int32>                 TextureMap;
    TArray<TSharedRef<FTexture>>         Textures;
    FCriticalSection                     TexturesCS;

    static FAssetManager* GInstance;
};

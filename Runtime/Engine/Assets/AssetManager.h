#pragma once
#include "TextureResource.h"
#include "ITextureImporter.h"
#include "IMeshImporter.h"
#include "Core/Containers/Map.h"
#include "Core/Platform/CriticalSection.h"

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
    
    TSharedRef<FSceneData> LoadMesh(const FString& Filename, EMeshImportFlags Flags = EMeshImportFlags::Default);
    void UnloadMesh(const TSharedRef<FSceneData>& InMesh);

    void RegisterTextureImporter(const TSharedPtr<ITextureImporter>& InImporter);
    void UnregisterTextureImporter(const TSharedPtr<ITextureImporter>& InImporter);
    
    void RegisterMeshImporter(const TSharedPtr<IMeshImporter>& InImporter);
    void UnregisterMeshImporter(const TSharedPtr<IMeshImporter>& InImporter);

private:
    FAssetManager();
    ~FAssetManager();

    // Meshes
    TArray<TSharedPtr<IMeshImporter>>    MeshImporters;
    FCriticalSection                     MeshImportersCS;
    TMap<FString, int32>                 MeshesMap;
    TArray<TSharedRef<FSceneData>>       Meshes;
    FCriticalSection                     MeshesCS;
    
    // Textures
    TArray<TSharedPtr<ITextureImporter>> TextureImporters;
    FCriticalSection                     TextureImportersCS;
    TMap<FString, int32>                 TextureMap;
    TArray<TSharedRef<FTexture>>         Textures;
    FCriticalSection                     TexturesCS;

    static FAssetManager* GInstance;
};

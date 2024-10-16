#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Platform/CriticalSection.h"
#include "Engine/Assets/ITextureImporter.h"
#include "Engine/Assets/IModelImporter.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/TextureResource.h"

struct FModelImporter;
struct FModelSerializer;

class ENGINE_API FAssetRegistry
{
public:
    FAssetRegistry();
    ~FAssetRegistry();
    
    FString* FindFile(const FString& SrcFilename);
    void AddEntry(const FString& SrcFilename, const FString& Filename);
    void RemoveEntry(const FString& SrcFilename);
    void LoadRegistryFile();
    void UpdateRegistryFile();
    
private:
    // Maps the original path to the engine file
    TMap<FString, FString> RegistryMap;
    FString                RegistryFilename;
};

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

    // Registry of where to find any engine file
    TUniquePtr<FAssetRegistry>   AssetRegistry;
    TUniquePtr<FModelSerializer> ModelSerializer;
    TUniquePtr<FModelImporter>   ModelImporter;
    
    // Meshes
    TArray<TSharedPtr<IModelImporter>>   ModelImporters;
    FCriticalSection                     ModelImportersCS;
    TMap<FString, int32>                 ModelsMap;
    TArray<TSharedRef<FModel>>           Models;
    FCriticalSection                     ModelsCS;
    
    // Textures
    TArray<TSharedPtr<ITextureImporter>> TextureImporters;
    FCriticalSection                     TextureImportersCS;
    TMap<FString, int32>                 TextureMap;
    TArray<TSharedRef<FTexture>>         Textures;
    FCriticalSection                     TexturesCS;

    static FAssetManager* GInstance;
};

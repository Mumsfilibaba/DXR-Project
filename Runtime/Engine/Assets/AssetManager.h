#pragma once
#include "AssetCore/AssetImporter.h"
#include "AssetCore/AssetLoader.h"
#include "AssetCore/AssetRegistry.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/CriticalSection.h"
#include "Engine/Assets/ITextureImporter.h"
#include "AssetCore/IModelImporter.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Texture.h"
#include "Core/Templates/Utility/NonCopyable.h"

class ENGINE_API FAssetManager : public FNonCopyable
{
public:

    /** @brief Create and initialize the AssetManager */
    static bool Initialize();

    /** @brief Destroy the AssetManager */
    static void Release();

    /** @brief Retrieve the AssetManager instance */
    static FAssetManager& Get();

public:

    /**
     * @brief Load the asset with Guid as a T through the loader registered for T's asset type, or return it if it is loaded
     * already. Logs why when it can't be loaded. Call from the main thread.
     */
    template<typename T>
    TSharedPtr<T> LoadAsset(const FGuid& Guid);

    template<typename T>
    TSharedPtr<T> LoadAsset(const TAssetRef<T>& Ref);

    // Loading a source file by path is the legacy way, kept until the importers write assets (Phase 3)
    TSharedPtr<FTexture> LoadTexture(const FString& Filename, bool bGenerateMips = true);
    void UnloadTexture(const TSharedPtr<FTexture>& Texture);

    TSharedPtr<FModel> LoadModel(const FString& Filename, EMeshImportFlags Flags = EMeshImportFlags::Default);
    void UnloadModel(const TSharedPtr<FModel>& InModel);

    void RegisterTextureImporter(const TSharedPtr<ITextureImporter>& InImporter);
    void UnregisterTextureImporter(const TSharedPtr<ITextureImporter>& InImporter);

    void RegisterModelImporter(const TSharedPtr<IModelImporter>& InImporter);
    void UnregisterModelImporter(const TSharedPtr<IModelImporter>& InImporter);

    FORCEINLINE FAssetRegistry& GetAssetRegistry()
    {
        return AssetRegistry;
    }

    /** @return Returns the loader of assets by GUID, where loaders for each asset type are registered */
    FORCEINLINE FAssetLoader& GetAssetLoader()
    {
        return AssetLoader;
    }

    /** @return Returns the importer of source files into assets, where factories for each source format are registered */
    FORCEINLINE FAssetImporter& GetAssetImporter()
    {
        return AssetImporter;
    }

private:
    FAssetManager();
    ~FAssetManager();

    FAssetRegistry                       AssetRegistry;
    FAssetLoader                         AssetLoader;
    FAssetImporter                       AssetImporter;
    // Meshes
    TArray<TSharedPtr<IModelImporter>>   ModelImporters;
    FCriticalSection                     ModelImportersCS;
    TMap<FString, TSharedPtr<FModel>>    Models;
    FCriticalSection                     ModelsCS;
    // Textures
    TArray<TSharedPtr<ITextureImporter>> TextureImporters;
    FCriticalSection                     TextureImportersCS;
    TMap<FString, TSharedPtr<FTexture>>  Textures;
    FCriticalSection                     TexturesCS;

    static FAssetManager* AssetManager;
};

template<typename T>
TSharedPtr<T> FAssetManager::LoadAsset(const FGuid& Guid)
{
    FString       Error;
    TSharedPtr<T> Asset = AssetLoader.Load<T>(Guid, &Error);

    if (!Asset.IsValid())
    {
        LOG_ERROR("[FAssetManager]: %s", *Error);
    }

    return Asset;
}

template<typename T>
TSharedPtr<T> FAssetManager::LoadAsset(const TAssetRef<T>& Ref)
{
    return LoadAsset<T>(Ref.GetGuid());
}

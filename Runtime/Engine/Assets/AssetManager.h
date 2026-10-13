#pragma once
#include "AssetCore/AssetRegistry.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Platform/CriticalSection.h"
#include "Engine/Assets/ITextureImporter.h"
#include "Engine/Assets/IModelImporter.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Texture.h"
#include "Core/Templates/Utility/NonCopyable.h"

struct FModelImporter;
struct FModelSerializer;

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
    TSharedRef<FTexture> LoadTexture(const FString& Filename, bool bGenerateMips = true);
    void UnloadTexture(const TSharedRef<FTexture>& Texture);

    TSharedRef<FModel> LoadModel(const FString& Filename, EMeshImportFlags Flags = EMeshImportFlags::Default);
    void UnloadModel(const TSharedRef<FModel>& InModel);

    void RegisterTextureImporter(const TSharedPtr<ITextureImporter>& InImporter);
    void UnregisterTextureImporter(const TSharedPtr<ITextureImporter>& InImporter);

    void RegisterModelImporter(const TSharedPtr<IModelImporter>& InImporter);
    void UnregisterModelImporter(const TSharedPtr<IModelImporter>& InImporter);

    FORCEINLINE FAssetRegistry& GetAssetRegistry()
    {
        return AssetRegistry;
    }

private:
    FAssetManager();
    ~FAssetManager();

    FAssetRegistry                       AssetRegistry;
    TUniquePtr<FModelSerializer>         ModelSerializer;
    TUniquePtr<FModelImporter>           ModelImporter;
    // Meshes
    TArray<TSharedPtr<IModelImporter>>   ModelImporters;
    FCriticalSection                     ModelImportersCS;
    TMap<FString, TSharedRef<FModel>>    Models;
    FCriticalSection                     ModelsCS;
    // Textures
    TArray<TSharedPtr<ITextureImporter>> TextureImporters;
    FCriticalSection                     TextureImportersCS;
    TMap<FString, TSharedRef<FTexture>>  Textures;
    FCriticalSection                     TexturesCS;

    static FAssetManager* AssetManager;
};

#pragma once
#include "TextureResource.h"
#include "ITextureImporter.h"

#include "Core/Containers/Map.h"
#include "Core/Platform/CriticalSection.h"

class ENGINE_API FAssetManager
{
private:
    FAssetManager();
    ~FAssetManager();

public:

    /** @brief - Create and initialize the AssetManager */
    static bool Initialize();

    /** @brief - Destroy the AssetManager */
    static void Release();

    /** @brief - Retrieve the AssetManager instance */
    static FAssetManager& Get();

    /** 
    * @brief          - Load a texture
    * @param Filename - Filename relative to the AssetFolder
    * @return         - Returns the loaded texture
    */
    FTextureResourceRef LoadTexture(const FString& Filename, bool bGenerateMips = true);

    /** @brief - Unload the texture and release the AssetManager's reference to the texture */
    void UnloadTexture(const FTextureResourceRef& Texture);

private:

    // Textures
    TMap<FString, uint32, FStringHasher> TextureMap;

    TArray<FTextureResourceRef> Textures;
    FCriticalSection            TexturesCS;

    TArray<ITextureImporter*>   TextureImporters;

    static FAssetManager* GInstance;
};
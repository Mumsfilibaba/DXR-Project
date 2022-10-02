#pragma once
#include "EngineModule.h"

#include "Core/RefCounted.h"
#include "Core/Containers/Optional.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Map.h"
#include "Core/Platform/CriticalSection.h"

#include "RHI/RHITexture.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class FTexture2D> FTexture2DRef;
typedef TSharedRef<class FMeshModel> FMeshModelRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTexture2D 

class ENGINE_API FTexture2D 
    : public FRefCounted
{
public:
    FTexture2D();
    ~FTexture2D();

    bool CreateRHITexture();

    FRHITexture2DRef GetRHITexture() const;

    EFormat GetFormat() const;

    uint16 GetWidth() const;
    uint16 GetHeight() const;

    void* GetData(int32 MipLevel = 0) const;

private:
    FRHITexture2DRef TextureRHI;

    TArray<void*> TextureData;

    EFormat Format;

    uint16  Width;
    uint16  Height;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMeshModel

class ENGINE_API FMeshModel
    : public FRefCounted
{
public:
    FMeshModel();

private:
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAssetManager

class ENGINE_API FAssetManager
{
private:
    FAssetManager();
    ~FAssetManager() = default;

public:

    /** @brief: Create and initialize the AssetManager */
    static bool Initialize();

    /** @brief: Destroy the AssetManager */
    static void Release();

    /** @brief: Retrieve the AssetManager instance */
    static FAssetManager& Get();

    /** 
    * @brief: Load a texture
    * 
    * @param Filename: Filename relative to the AssetFolder
    * @return: Returns the loaded texture
    */
    FTexture2DRef LoadTexture(const FString& Filename);

    /**
     * @brief: Unload the texture and release the AssetManager's reference to the texture
     */
    void UnloadTexture(const FTexture2DRef& Texture);

private:
    // Textures
    using FTextureMap = TMap<FString, FTexture2DRef, FStringHasher>;

    FTextureMap      Textures;
    FCriticalSection TexturesCS;

    // Models
    using FMeshModelMap = TMap<FString, FTexture2DRef, FStringHasher>;
    
    FMeshModelMap    MeshModels;
    FCriticalSection MeshModelsCS;

    static FAssetManager* GInstance;
};
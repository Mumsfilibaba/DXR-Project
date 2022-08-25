#pragma once
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

class FTexture2D 
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

    void* GetData() const;

private:
    FRHITexture2DRef TextureRHI;

    EFormat Format;

    uint16  Width;
    uint16  Height;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMeshModel

class FMeshModel
    : public FRefCounted
{
public:
    FMeshModel();

private:
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAssetManager

class FAssetManager
{
private:
    friend class TOptional<FAssetManager>;

    FAssetManager();
    ~FAssetManager();

public:

    /** @brief: Create and initialize the AssetManager */
    static bool Initialize();

    /** @brief: Destroy the AssetManager */
    static void Release();

    /** @brief: Retrieve the AssetManager instance */
    static FORCEINLINE FAssetManager& Get() { return Instance.GetValue(); }

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

    static TOptional<FAssetManager> Instance;
};
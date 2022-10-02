#include "AssetManager.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTexture2D 

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAssetManager

FAssetManager* FAssetManager::GInstance = nullptr;

FAssetManager::FAssetManager()
    : Textures()
    , TexturesCS()
    , MeshModels()
    , MeshModelsCS()
{ }

bool FAssetManager::Initialize()
{
    if (!GInstance)
    {
        GInstance = dbg_new FAssetManager();
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
    Check(GInstance != nullptr);
    return *GInstance;
}
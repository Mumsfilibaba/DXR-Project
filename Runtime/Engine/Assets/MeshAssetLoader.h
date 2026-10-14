#pragma once
#include "AssetCore/AssetRegistry.h"
#include "Engine/Resources/Model.h"

struct ENGINE_API MeshAssetLoader
{
    /** @brief Load a mesh asset into an FMesh with its material slots, as the loader FAssetLoader calls for meshes */
    static TSharedPtr<FMesh> Load(const FAssetData& Asset, const FString& DiskPath, FString& OutError);
};

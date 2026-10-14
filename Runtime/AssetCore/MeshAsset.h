#pragma once
#include "AssetCore/AssetRef.h"
#include "AssetCore/MeshData.h"

struct FMeshAssetSlot
{
    /** @brief The source's name for the material, such as "Bricks" */
    FString Name;

    /** @brief The material asset the slot uses by default, or unset */
    FAssetRef Material;
};

struct FMeshAssetData
{
    /** @brief The geometry: packed streams when read, or source vertices or packed streams to write */
    FMeshData Mesh;

    /** @brief The materials the submeshes' MaterialIndex picks from, until a component overrides them */
    TArray<FMeshAssetSlot> MaterialSlots;
};

struct ASSETCORE_API MeshAssets
{
    /**
     * @brief Write a mesh asset's DATA payload: its vertex streams in the GPU layout of its declaration, 32-bit indices,
     * submeshes and material slots. Source vertices are packed unless the streams already are.
     */
    static bool Write(const FMeshAssetData& Mesh, TArray<uint8>& OutPayload, FString& OutError);

    /** @brief Read a mesh asset's DATA payload into packed streams, checking every size and index against the others */
    static bool Read(const TArray<uint8>& Payload, FMeshAssetData& OutMesh, FString& OutError);

    /** @return Returns the materials the slots reference, once each, as the asset's dependencies */
    NODISCARD static TArray<FGuid> GetDependencies(const FMeshAssetData& Mesh);
};

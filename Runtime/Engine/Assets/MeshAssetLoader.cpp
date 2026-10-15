#include "AssetCore/AssetFile.h"
#include "AssetCore/MeshAsset.h"
#include "Engine/Assets/MeshAssetLoader.h"

TSharedPtr<FMesh> MeshAssetLoader::Load(const FAssetData& Asset, const FString& DiskPath, FString& OutError)
{
    FAssetFileReader Reader;
    TArray<uint8>    Payload;
    FMeshAssetData   Data;

    if (!Reader.Open(DiskPath, OutError) || !Reader.ReadChunk(AssetChunk::Data, Payload, OutError))
    {
        return nullptr;
    }

    const FAssetChunkEntry* DataChunk   = Reader.FindChunk(AssetChunk::Data);
    const uint64            ContentHash = (uint64(DataChunk->CRC) << 32) ^ DataChunk->Size;

    if (!MeshAssets::Read(Payload, Data, OutError))
    {
        OutError = FString::Printf(TEXT("The mesh data of '%s' can't be read: %s"), *Asset.Path, *OutError);
        return nullptr;
    }

    Payload.Clear(true);
    Data.Mesh.Name = Asset.Name;

    TSharedPtr<FMesh> Mesh = FMesh::Create(Data.Mesh);
    if (!Mesh)
    {
        OutError = FString::Printf(TEXT("The GPU buffers for '%s' couldn't be created"), *Asset.Path);
        return nullptr;
    }

    TArray<FMeshMaterialSlot> MaterialSlots;
    for (const FMeshAssetSlot& Slot : Data.MaterialSlots)
    {
        FMeshMaterialSlot& MaterialSlot = MaterialSlots.Emplace();
        MaterialSlot.Name     = Slot.Name;
        MaterialSlot.Material = Slot.Material.IsSet() ? TAssetRef<FMaterial>(Slot.Material.GetGuid(), Slot.Material.GetPath()) : TAssetRef<FMaterial>();
    }

    Mesh->SetMaterialSlots(MaterialSlots);
    Mesh->SetContentHash(ContentHash);
    return Mesh;
}

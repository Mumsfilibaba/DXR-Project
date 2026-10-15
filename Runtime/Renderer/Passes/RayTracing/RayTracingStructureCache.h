#pragma once
#include "Core/Containers/UniquePtr.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "RendererCore/RayTracing/AccelerationStructureCache.h"
#include "RendererCore/RayTracing/AccelerationStructureCacheBackend.h"

class FMesh;

class FRayTracingStructureCache : public FNonCopyable
{
public:

    /**
     * @return Returns the key of the mesh's structure: its asset's GUID, the RHI, whose structures the others can't load, and a hash of its data and
     * build settings. Returns an empty string for a mesh that isn't an asset.
     */
    NODISCARD static FString CreateKey(const FMesh& Mesh);

public:
    FRayTracingStructureCache();
    ~FRayTracingStructureCache();

    /** @brief Open the engine's and the project's caches, under their Cache folders */
    void Initialize();
    void Release();

    /**
     * @brief Give the mesh its geometry, deserialized from the structure cached for it
     * @return Returns false when nothing usable is cached, after removing what was cached for the mesh's earlier data
     */
    bool TryLoad(FMesh& Mesh);

    /** @return Returns the cache the mesh's structure belongs in and its key there, or nullptr for a mesh that isn't an asset */
    FAccelerationStructureCache* FindCache(const FMesh& Mesh, FString& OutKey);

private:
    struct FCacheDirectory
    {
        TUniquePtr<FDiskAccelerationStructureCacheBackend> Backend;
        FAccelerationStructureCache                        Cache;
        bool                                               bPruned = false;
    };

    FCacheDirectory* FindDirectory(const FMesh& Mesh);
    void PruneUnregisteredEntries(FCacheDirectory& Directory);

    FCacheDirectory EngineDirectory;
    FCacheDirectory ProjectDirectory;
};

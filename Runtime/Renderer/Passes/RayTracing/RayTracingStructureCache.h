#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Misc/Guid.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "RendererCore/RayTracing/AccelerationStructureCache.h"
#include "RendererCore/RayTracing/AccelerationStructureCacheBackend.h"
#include "RendererCore/RayTracing/AccelerationStructureSerializationHelper.h"

class FMesh;

class FRayTracingStructureCache : public FNonCopyable
{
public:
    FRayTracingStructureCache();
    ~FRayTracingStructureCache();

    /** @brief Open the engine's and the project's caches, under their Cache folders */
    void Initialize();
    void Release();

    /**
     * @return Returns the key of the mesh's structure: its asset's GUID, the RHI and the adapter, whose structures the others can't load, and a hash of
     * its data, build settings, driver and the micromaps it links. Returns an empty string for a mesh that isn't an asset or links a micromap that isn't
     * registered.
     */
    NODISCARD FString CreateKey(const FMesh& Mesh) const;

    /**
     * @brief Give the mesh its geometry, deserialized from the structure cached for it and linked to the micromaps the mesh links now
     * @return Returns false when nothing usable is cached, after removing what was cached for the mesh's earlier data
     */
    bool TryLoad(FMesh& Mesh);

    /** @return Returns the cache the mesh's structure belongs in and its key there, or nullptr for a mesh that isn't an asset */
    FAccelerationStructureCache* FindCache(const FMesh& Mesh, FString& OutKey);

    /** @return Returns what the mesh's structure Geometry is stored with so a load can link it again, or an empty function when it links no micromaps */
    NODISCARD FAccelerationStructureSerializationHelper::FCreateAttachment CreateAttachment(const FMesh& Mesh, FRHIGeometryAccelerationStructure* Geometry) const;

    /**
     * @brief Deserialize the micromap shared between meshes that was cached under Name for this adapter and driver, and register it
     * @return Returns nullptr when nothing usable is cached, after removing what was cached under Name before
     */
    FRHIOpacityMicromapRef TryLoadSharedOpacityMicromap(const CHAR_T* Name);

    /**
     * @brief Register a micromap shared between meshes that was built this run, so structures that link it can be cached
     * @return Returns the cache the micromap belongs in, with its key and what it's stored with, or nullptr when there's no cache
     */
    FAccelerationStructureCache* RegisterSharedOpacityMicromap(const CHAR_T* Name, FRHIOpacityMicromap* Micromap, FString& OutKey, FAccelerationStructureSerializationHelper::FCreateAttachment& OutCreateAttachment);

private:
    struct FCacheDirectory
    {
        TUniquePtr<FDiskAccelerationStructureCacheBackend> Backend;
        FAccelerationStructureCache                        Cache;
        bool                                               bPruned = false;
    };

    FString GetKeyPrefix(const FMesh& Mesh) const;
    FString GetSharedKeyPrefix(const CHAR_T* Name) const;
    FString CreateSharedKey(const CHAR_T* Name) const;

    FCacheDirectory* FindDirectory(const FMesh& Mesh);
    void PruneUnregisteredEntries(FCacheDirectory& Directory);

    FCacheDirectory                   EngineDirectory;
    FCacheDirectory                   ProjectDirectory;
    FRHIAdapterDesc                   AdapterDesc;
    TMap<FRHIOpacityMicromap*, FGuid> SharedOpacityMicromaps;
};

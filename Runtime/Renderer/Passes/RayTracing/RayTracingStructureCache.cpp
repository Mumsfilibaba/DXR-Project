#include "Core/Filesystem/File.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/Paths.h"
#include "RHI/RHI.h"
#include "RHI/RHIDevice.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Resources/Model.h"
#include "Renderer/Passes/RayTracing/RayTracingStructureCache.h"

static int32 GRayTracingASCacheMaxSizeMB = 1024;
static FAutoConsoleVariableRef CVarRayTracingASCacheMaxSizeMB(
    TEXT("Renderer.RayTracing.ASCache.MaxSizeMB"),
    TEXT("The size in MB the engine's and each project's acceleration structure cache may take before the least recently used structures are deleted. 0 for no limit."),
    GRayTracingASCacheMaxSizeMB);

// Bump when the renderer builds structures differently from the same mesh data, so earlier entries miss
static constexpr uint32 CacheKeyVersion = 1;

static constexpr const CHAR_T* CacheFolderName = TEXT("/RayTracing");

static uint64 HashBytes(uint64 Hash, const void* Data, SIZE_T Size)
{
    const uint8* Bytes = reinterpret_cast<const uint8*>(Data);
    for (SIZE_T Index = 0; Index < Size; ++Index)
    {
        Hash = (Hash ^ Bytes[Index]) * 0x100000001b3ull;
    }

    return Hash;
}

template<typename T>
static uint64 HashValue(uint64 Hash, const T& Value)
{
    return HashBytes(Hash, &Value, sizeof(T));
}

static FString GetKeyPrefix(const FMesh& Mesh)
{
    return Mesh.GetAssetRef().GetGuid().ToString() + TEXT("-") + ToString(RHI::Device->GetRHIType()) + TEXT("-");
}

static bool ParseKeyGuid(const FString& Key, FGuid& OutGuid)
{
    const int32 Separator = Key.FindChar('-');
    return Separator > 0 && FGuid::Parse(Key.SubString(0, Separator), OutGuid);
}

FString FRayTracingStructureCache::CreateKey(const FMesh& Mesh)
{
    if (!Mesh.IsAsset() || Mesh.GetContentHash() == 0)
    {
        return FString();
    }

    const FRHIGeometryAccelerationStructureDesc GeometryDesc = Mesh.GetRayTracingGeometryDesc();

    uint64 Hash = 0xcbf29ce484222325ull;
    Hash = HashValue(Hash, CacheKeyVersion);
    Hash = HashValue(Hash, Mesh.GetContentHash());
    Hash = HashValue(Hash, GeometryDesc.NumVertices);
    Hash = HashValue(Hash, GeometryDesc.NumIndices);
    Hash = HashValue(Hash, GeometryDesc.IndexFormat);
    Hash = HashValue(Hash, GeometryDesc.Flags);
    Hash = HashValue(Hash, Mesh.GetVertexDeclaration().GetStreamStride(0));

    return GetKeyPrefix(Mesh) + FString::Printf(TEXT("%016llx"), static_cast<unsigned long long>(Hash));
}

FRayTracingStructureCache::FRayTracingStructureCache()
    : EngineDirectory()
    , ProjectDirectory()
{
}

FRayTracingStructureCache::~FRayTracingStructureCache()
{
    Release();
}

void FRayTracingStructureCache::Initialize()
{
    const uint64 MaxSizeInBytes = static_cast<uint64>(Math::Max(GRayTracingASCacheMaxSizeMB, 0)) * 1024ull * 1024ull;

    const FString EngineCacheDir = File::NormalizePath(Paths::GetEngineCacheDir() + CacheFolderName);
    EngineDirectory.Backend = MakeUniquePtr<FDiskAccelerationStructureCacheBackend>(EngineCacheDir, MaxSizeInBytes);
    EngineDirectory.Cache.SetBackend(EngineDirectory.Backend.Get());

    const FString ProjectCacheDir = File::NormalizePath(Paths::GetProjectCacheDir() + CacheFolderName);
    if (!ProjectCacheDir.Equals(EngineCacheDir, EStringCaseType::NoCase))
    {
        ProjectDirectory.Backend = MakeUniquePtr<FDiskAccelerationStructureCacheBackend>(ProjectCacheDir, MaxSizeInBytes);
        ProjectDirectory.Cache.SetBackend(ProjectDirectory.Backend.Get());
    }
}

void FRayTracingStructureCache::Release()
{
    for (FCacheDirectory* Directory : { &EngineDirectory, &ProjectDirectory })
    {
        Directory->Cache.SetBackend(nullptr);
        Directory->Backend.Reset();
        Directory->bPruned = false;
    }
}

bool FRayTracingStructureCache::TryLoad(FMesh& Mesh)
{
    if (!Mesh.IsRayTracingReady())
    {
        return false;
    }

    FString                      Key;
    FAccelerationStructureCache* Cache = FindCache(Mesh, Key);
    if (!Cache)
    {
        return false;
    }

    TArray<uint8> Bytes;
    if (!Cache->TryLoad(FStringView(Key), Bytes))
    {
        const FString KeyPrefix = GetKeyPrefix(Mesh);
        FindDirectory(Mesh)->Backend->RemoveEntries([&](const FString& EntryKey)
        {
            return EntryKey.StartsWith(*KeyPrefix) && !EntryKey.Equals(Key);
        });

        return false;
    }

    const FRHIGeometryAccelerationStructureDesc GeometryDesc = FRHIGeometryAccelerationStructureDesc::CreateSerialized(
        Bytes.Data() + FAccelerationStructureCache::GetPayloadOffset(),
        uint64(Bytes.Size()) - FAccelerationStructureCache::GetPayloadOffset(),
        Mesh.GetRayTracingGeometryDesc().Flags);

    FRHIGeometryAccelerationStructureRef Geometry = RHI::CreateGeometryAccelerationStructure(GeometryDesc);
    if (!Geometry)
    {
        return false;
    }

    Mesh.SetRayTracingGeometry(Geometry);
    return true;
}

FAccelerationStructureCache* FRayTracingStructureCache::FindCache(const FMesh& Mesh, FString& OutKey)
{
    FCacheDirectory* Directory = FindDirectory(Mesh);
    if (!Directory)
    {
        return nullptr;
    }

    OutKey = CreateKey(Mesh);
    if (OutKey.IsEmpty())
    {
        return nullptr;
    }

    if (!Directory->bPruned)
    {
        PruneUnregisteredEntries(*Directory);
    }

    return &Directory->Cache;
}

FRayTracingStructureCache::FCacheDirectory* FRayTracingStructureCache::FindDirectory(const FMesh& Mesh)
{
    if (!Mesh.IsAsset())
    {
        return nullptr;
    }

    FCacheDirectory* Directory = Mesh.GetAssetRef().GetPath().StartsWith(FContentMounts::EngineRoot) ? &EngineDirectory : &ProjectDirectory;
    if (!Directory->Backend)
    {
        Directory = &EngineDirectory;
    }

    return Directory->Backend ? Directory : nullptr;
}

void FRayTracingStructureCache::PruneUnregisteredEntries(FCacheDirectory& Directory)
{
    const FAssetRegistry& Registry = FAssetManager::Get().GetAssetRegistry();
    if (Registry.GetNumAssets() == 0)
    {
        return;
    }

    Directory.bPruned = true;

    const int32  NumBefore  = Directory.Backend->GetNumEntries();
    const uint64 SizeBefore = Directory.Backend->GetSizeInBytes();

    Directory.Backend->RemoveEntries([&Registry](const FString& Key)
    {
        FGuid Guid;
        return !ParseKeyGuid(Key, Guid) || Registry.FindByGuid(Guid) == nullptr;
    });

    if (Directory.Backend->GetNumEntries() != NumBefore)
    {
        LOG_INFO("[FRayTracingStructureCache]: Removed %d structures (%.1f MB) of meshes no longer registered from '%s'", NumBefore - Directory.Backend->GetNumEntries(),
            double(SizeBefore - Directory.Backend->GetSizeInBytes()) / (1024.0 * 1024.0), *Directory.Backend->GetDirectory());
    }
}

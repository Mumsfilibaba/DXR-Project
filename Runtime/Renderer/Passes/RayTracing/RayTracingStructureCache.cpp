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

// Bump when the renderer builds structures or shared micromaps differently from the same data, so earlier entries miss
static constexpr uint32 CacheKeyVersion = 3;

static constexpr const CHAR_T* CacheFolderName = TEXT("/RayTracing");
static constexpr const CHAR_T* SharedKeyPrefix = TEXT("Shared-");

static constexpr uint32 NullMicromapIndex = ~0u;

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

static FString GetAdapterKey(const FRHIAdapterDesc& AdapterDesc)
{
    return FString::Printf(TEXT("%04x%04x"), AdapterDesc.VendorID, AdapterDesc.DeviceID);
}

static bool ParseKeyGuid(const FString& Key, FGuid& OutGuid)
{
    const int32 Separator = Key.FindChar('-');
    return Separator > 0 && FGuid::Parse(Key.SubString(0, Separator), OutGuid);
}

static bool GetLinkedOpacityMicromaps(const FMesh& Mesh, const TArray<uint8>& Attachment, TArray<FRHIOpacityMicromap*>& OutOpacityMicromaps)
{
    OutOpacityMicromaps.Clear();
    if (Attachment.IsEmpty())
    {
        return true;
    }

    if (Attachment.Size() < int32(sizeof(uint32)))
    {
        return false;
    }

    uint32 NumOpacityMicromaps = 0;
    Memory::Memcpy(&NumOpacityMicromaps, Attachment.Data(), sizeof(uint32));

    if (Attachment.Size() != int32(sizeof(uint32) * (NumOpacityMicromaps + 1)))
    {
        return false;
    }

    const TArray<FRHIOpacityMicromapRef>& MeshMicromaps = Mesh.GetRayTracingOpacityMicromaps();
    for (uint32 Index = 0; Index < NumOpacityMicromaps; ++Index)
    {
        uint32 MicromapIndex = NullMicromapIndex;
        Memory::Memcpy(&MicromapIndex, Attachment.Data() + sizeof(uint32) * (Index + 1), sizeof(uint32));

        if (MicromapIndex != NullMicromapIndex && MicromapIndex >= uint32(MeshMicromaps.Size()))
        {
            return false;
        }

        OutOpacityMicromaps.Add(MicromapIndex != NullMicromapIndex ? MeshMicromaps[MicromapIndex].Get() : nullptr);
    }

    return true;
}

FRayTracingStructureCache::FRayTracingStructureCache()
    : EngineDirectory()
    , ProjectDirectory()
    , AdapterDesc()
    , SharedOpacityMicromaps()
{
}

FRayTracingStructureCache::~FRayTracingStructureCache()
{
    Release();
}

void FRayTracingStructureCache::Initialize()
{
    AdapterDesc = RHI::Device->GetAdapterDesc();

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

    SharedOpacityMicromaps.Clear();
}

FString FRayTracingStructureCache::CreateKey(const FMesh& Mesh) const
{
    if (!Mesh.IsAsset() || Mesh.GetContentHash() == 0)
    {
        return FString();
    }

    const FRHIGeometryAccelerationStructureDesc GeometryDesc = Mesh.GetRayTracingGeometryDesc();

    uint64 Hash = 0xcbf29ce484222325ull;
    Hash = HashValue(Hash, CacheKeyVersion);
    Hash = HashValue(Hash, AdapterDesc.DriverVersion);
    Hash = HashValue(Hash, Mesh.GetContentHash());
    Hash = HashValue(Hash, GeometryDesc.NumVertices);
    Hash = HashValue(Hash, GeometryDesc.NumIndices);
    Hash = HashValue(Hash, GeometryDesc.IndexFormat);
    Hash = HashValue(Hash, GeometryDesc.Flags);
    Hash = HashValue(Hash, Mesh.GetVertexDeclaration().GetStreamStride(0));
    Hash = HashValue(Hash, GeometryDesc.NumSegments);

    for (uint32 SegmentIndex = 0; SegmentIndex < GeometryDesc.NumSegments; ++SegmentIndex)
    {
        const FRHIRayTracingGeometrySegment& Segment = GeometryDesc.Segments[SegmentIndex];
        Hash = HashValue(Hash, Segment.FirstIndex);
        Hash = HashValue(Hash, Segment.NumIndices);

        if (!Segment.HasOpacityMicromap())
        {
            continue;
        }

        FGuid MicromapGeneration;
        if (Segment.OpacityMicromap)
        {
            const FGuid* RegisteredGeneration = SharedOpacityMicromaps.Find(Segment.OpacityMicromap);
            if (!RegisteredGeneration)
            {
                return FString();
            }

            MicromapGeneration = *RegisteredGeneration;
        }

        Hash = HashValue(Hash, MicromapGeneration);
        Hash = HashValue(Hash, Segment.OpacityMicromapIndexOffset);
        Hash = HashValue(Hash, Segment.OpacityMicromapIndexFormat);
        Hash = HashValue(Hash, Segment.OpacityMicromapBaseLocation);
    }

    return GetKeyPrefix(Mesh) + FString::Printf(TEXT("%016llx"), static_cast<unsigned long long>(Hash));
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
    TArray<uint8> Attachment;

    if (!Cache->TryLoad(FStringView(Key), Bytes, Attachment))
    {
        const FString KeyPrefix = GetKeyPrefix(Mesh);
        FindDirectory(Mesh)->Backend->RemoveEntries([&](const FString& EntryKey)
        {
            return EntryKey.StartsWith(*KeyPrefix) && !EntryKey.Equals(Key);
        });

        return false;
    }

    TArray<FRHIOpacityMicromap*> OpacityMicromaps;
    if (!GetLinkedOpacityMicromaps(Mesh, Attachment, OpacityMicromaps))
    {
        return false;
    }

    const FRHIGeometryAccelerationStructureDesc GeometryDesc = FRHIGeometryAccelerationStructureDesc::CreateSerialized(
        Bytes.Data() + FAccelerationStructureCache::GetPayloadOffset(),
        uint64(Bytes.Size()) - FAccelerationStructureCache::GetPayloadOffset(),
        Mesh.GetRayTracingGeometryDesc().Flags,
        TArrayView<FRHIOpacityMicromap* const>(OpacityMicromaps.Data(), OpacityMicromaps.Size()));
    
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

FAccelerationStructureSerializationHelper::FCreateAttachment FRayTracingStructureCache::CreateAttachment(const FMesh& Mesh, FRHIGeometryAccelerationStructure* Geometry) const
{
    const TArray<FRHIOpacityMicromapRef> MeshMicromaps = Mesh.GetRayTracingOpacityMicromaps();
    if (MeshMicromaps.IsEmpty() || !Geometry)
    {
        return FAccelerationStructureSerializationHelper::FCreateAttachment();
    }

    const FRHIGeometryAccelerationStructureRef GeometryRef = MakeSharedRef<FRHIGeometryAccelerationStructure>(Geometry);
    return [MeshMicromaps, GeometryRef](const uint8* SerializedData, uint64 SerializedSizeInBytes, TArray<uint8>& OutAttachment)
    {
        TArray<FRHIOpacityMicromap*> LinkedMicromaps;
        if (!GeometryRef->GetSerializedOpacityMicromaps(SerializedData, SerializedSizeInBytes, LinkedMicromaps) || LinkedMicromaps.IsEmpty())
        {
            LOG_WARNING("[FRayTracingStructureCache]: A structure that links micromaps was serialized without the micromaps it links, so it isn't cached");
            return false;
        }

        const uint32 NumLinkedMicromaps = static_cast<uint32>(LinkedMicromaps.Size());
        OutAttachment.Append(reinterpret_cast<const uint8*>(&NumLinkedMicromaps), sizeof(uint32));

        for (FRHIOpacityMicromap* LinkedMicromap : LinkedMicromaps)
        {
            uint32 MicromapIndex = NullMicromapIndex;
            for (int32 Index = 0; Index < MeshMicromaps.Size() && LinkedMicromap && MicromapIndex == NullMicromapIndex; ++Index)
            {
                if (MeshMicromaps[Index].Get() == LinkedMicromap)
                {
                    MicromapIndex = static_cast<uint32>(Index);
                }
            }

            if (LinkedMicromap && MicromapIndex == NullMicromapIndex)
            {
                LOG_WARNING("[FRayTracingStructureCache]: A serialized structure links a micromap its mesh doesn't, so it isn't cached");
                return false;
            }

            OutAttachment.Append(reinterpret_cast<const uint8*>(&MicromapIndex), sizeof(uint32));
        }

        return true;
    };
}

FRHIOpacityMicromapRef FRayTracingStructureCache::TryLoadSharedOpacityMicromap(const CHAR_T* Name)
{
    if (!EngineDirectory.Backend)
    {
        return nullptr;
    }

    const FString Key = CreateSharedKey(Name);

    TArray<uint8> Bytes;
    TArray<uint8> Attachment;

    if (!EngineDirectory.Cache.TryLoad(FStringView(Key), Bytes, Attachment) || Attachment.Size() != int32(sizeof(FGuid)))
    {
        const FString KeyPrefix = GetSharedKeyPrefix(Name);
        EngineDirectory.Backend->RemoveEntries([&](const FString& EntryKey)
        {
            return EntryKey.StartsWith(*KeyPrefix) && !EntryKey.Equals(Key);
        });

        return nullptr;
    }

    FRHIAccelerationStructureSerializationHeader Header;
    Memory::Memcpy(&Header, Bytes.Data(), sizeof(FRHIAccelerationStructureSerializationHeader));

    if (Header.AccelerationStructureType != ERayTracingAccelerationStructureType::OpacityMicromap)
    {
        return nullptr;
    }

    const FRHIOpacityMicromapDesc MicromapDesc = FRHIOpacityMicromapDesc::CreateSerialized(
        Bytes.Data() + FAccelerationStructureCache::GetPayloadOffset(),
        uint64(Bytes.Size()) - FAccelerationStructureCache::GetPayloadOffset(),
        EAccelerationStructureBuildFlags::None);

    FRHIOpacityMicromapRef Micromap = RHI::CreateOpacityMicromap(MicromapDesc);
    if (!Micromap)
    {
        return nullptr;
    }

    FGuid Generation;
    Memory::Memcpy(&Generation, Attachment.Data(), sizeof(FGuid));
    SharedOpacityMicromaps.Add(Micromap.Get(), Generation);
    return Micromap;
}

FAccelerationStructureCache* FRayTracingStructureCache::RegisterSharedOpacityMicromap(const CHAR_T* Name, FRHIOpacityMicromap* Micromap, FString& OutKey, FAccelerationStructureSerializationHelper::FCreateAttachment& OutCreateAttachment)
{
    const FGuid Generation = FGuid::Create();
    SharedOpacityMicromaps.Add(Micromap, Generation);

    if (!EngineDirectory.Backend)
    {
        return nullptr;
    }

    OutKey = CreateSharedKey(Name);
    OutCreateAttachment = [Generation](const uint8*, uint64, TArray<uint8>& OutAttachment)
    {
        OutAttachment.Append(reinterpret_cast<const uint8*>(&Generation), sizeof(FGuid));
        return true;
    };

    return &EngineDirectory.Cache;
}

FString FRayTracingStructureCache::GetKeyPrefix(const FMesh& Mesh) const
{
    return Mesh.GetAssetRef().GetGuid().ToString() + TEXT("-") + ToString(RHI::Device->GetRHIType()) + TEXT("-") + GetAdapterKey(AdapterDesc) + TEXT("-");
}

FString FRayTracingStructureCache::GetSharedKeyPrefix(const CHAR_T* Name) const
{
    return FString(SharedKeyPrefix) + Name + TEXT("-") + ToString(RHI::Device->GetRHIType()) + TEXT("-") + GetAdapterKey(AdapterDesc) + TEXT("-");
}

FString FRayTracingStructureCache::CreateSharedKey(const CHAR_T* Name) const
{
    uint64 Hash = 0xcbf29ce484222325ull;
    Hash = HashValue(Hash, CacheKeyVersion);
    Hash = HashValue(Hash, AdapterDesc.DriverVersion);

    return GetSharedKeyPrefix(Name) + FString::Printf(TEXT("%016llx"), static_cast<unsigned long long>(Hash));
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
        if (Key.StartsWith(SharedKeyPrefix))
        {
            return false;
        }

        FGuid Guid;
        return !ParseKeyGuid(Key, Guid) || Registry.FindByGuid(Guid) == nullptr;
    });

    if (Directory.Backend->GetNumEntries() != NumBefore)
    {
        LOG_INFO("[FRayTracingStructureCache]: Removed %d structures (%.1f MB) of meshes no longer registered from '%s'", NumBefore - Directory.Backend->GetNumEntries(),
            double(SizeBefore - Directory.Backend->GetSizeInBytes()) / (1024.0 * 1024.0), *Directory.Backend->GetDirectory());
    }
}

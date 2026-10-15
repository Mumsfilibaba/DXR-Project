#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Model.h"
#include "Renderer/Shaders/MaterialBindless.h"
#include "Renderer/Shaders/RayTracingBindless.h"
#include "Renderer/Shaders/RayTracingShaders.h"
#include "Renderer/RendererStats.h"
#include "Renderer/Passes/RayTracing/RayTracingSceneBuilder.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"

static bool GRayTracingCompaction = false;
static FAutoConsoleVariableRef CVarRayTracingCompaction(
    TEXT("Renderer.RayTracing.Compaction"),
    TEXT("When true, each BLAS is compacted once after build (multi-frame read-back + in-place swap). Requires AllowCompaction on the BLAS. "
    "UNVALIDATED on hardware."),
    GRayTracingCompaction);

static bool GRayTracingASCache = false;
static FAutoConsoleVariableRef CVarRayTracingASCache(
    TEXT("Renderer.RayTracing.ASCache"),
    TEXT("When true, the BLAS of a mesh asset is loaded from the acceleration structure cache under the engine's or the project's Cache folder "
    "instead of built, and written there once built and compacted."),
    GRayTracingASCache);

static FMaterial* GetSubMeshMaterial(const FSceneStaticMesh& StaticMesh, int32 SubMeshIndex)
{
    TSharedPtr<FMaterial> Material = StaticMesh.GetMaterial(SubMeshIndex);
    return Material ? Material.Get() : StaticMesh.GetMaterial().Get();
}

FRayTracingSceneBuilder::FRayTracingSceneBuilder(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , GeometryTableCapacity(0)
{
}

FRayTracingSceneBuilder::~FRayTracingSceneBuilder()
{
    Release();
}

bool FRayTracingSceneBuilder::Initialize()
{
    StructureCache.Initialize();
    return true;
}

void FRayTracingSceneBuilder::Release()
{
    CompactionHelper.ReleaseAll();
    SerializationHelper.ReleaseAll();
    HandledStructures.Clear();
    AwaitingCompaction.Clear();
    StructureCache.Release();

    GeometryTableCapacity = 0;
}

void FRayTracingSceneBuilder::ReleaseRayTracingResources(FScene* Scene)
{
    if (Scene)
    {
        for (const FSceneStaticMesh* StaticMesh : Scene->GetStaticMeshes())
        {
            if (StaticMesh && StaticMesh->Mesh)
            {
                StaticMesh->Mesh->ReleaseRayTracingResources();
            }
        }
    }

    HandledStructures.Clear();
}

void FRayTracingSceneBuilder::BuildSceneAccelerationData(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, bool bNeedBindlessData)
{
    TRACE_SCOPE(TEXT("Gather Instances"));

    Resources.RayTracingGeometryInstances.Clear();
    Resources.RayTracingHitGroupBindings.Clear();
    Resources.RayTracingHitGroupShaderIndices.Clear();
    Resources.RayTracingMeshToHitGroupIndex.Clear();
    Resources.RayTracingGeometryTableData.Clear();

    uint32 LazyBLASBuildsThisFrame = 0;
    uint32 CacheLoadsThisFrame     = 0;
    uint32 CacheMissesThisFrame    = 0;
    uint32 SkippedNullGeometry     = 0;

    TSet<FRHIRayTracingAccelerationStructure*> StructuresInScene;
    for (const FSceneStaticMesh* StaticMesh : Scene->GetStaticMeshes())
    {
        FMesh& Mesh = *StaticMesh->Mesh;

        const bool bHadGeometry     = Mesh.GetRayTracingGeometry() != nullptr;
        const bool bLoadedFromCache = !bHadGeometry && GRayTracingASCache && StructureCache.TryLoad(Mesh);
        Mesh.EnsureRayTracingResources();

        FRHIGeometryAccelerationStructure* Geometry = Mesh.GetRayTracingGeometry();
        if (!Geometry)
        {
            ++SkippedNullGeometry;
            continue;
        }

        if (bLoadedFromCache)
        {
            ++CacheLoadsThisFrame;
        }
        else if (!bHadGeometry)
        {
            ++LazyBLASBuildsThisFrame;
            if (GRayTracingASCache && Mesh.IsAsset())
            {
                ++CacheMissesThisFrame;
            }
        }

        StructuresInScene.Add(Geometry);

        if ((GRayTracingCompaction || GRayTracingASCache) && !HandledStructures.Contains(Geometry))
        {
            HandledStructures.Add(Geometry);
            if (!bLoadedFromCache)
            {
                RequestCompactionAndCacheWrite(CommandList, Mesh, Geometry);
            }
        }

        FRHIShaderResourceView* AttributeBufferSRV = StaticMesh->Mesh->GetAttributeBufferSRV();
        FRHIShaderResourceView* IndexBufferSRV     = StaticMesh->Mesh->GetIndexBufferSRV();

        const FMatrix3x4 InstanceTransform = StaticMesh->PerObjectBuffer.Transform;

        FRayTracingHitGroupKey HitGroupKey;
        HitGroupKey.Mesh = &Mesh;

        bool bAnyAlphaTested = false;
        bool bAnyDoubleSided = false;

        for (int32 SubMeshIndex = 0; SubMeshIndex < Mesh.GetNumSubMeshes(); ++SubMeshIndex)
        {
            FMaterial* Material = GetSubMeshMaterial(*StaticMesh, SubMeshIndex);
            HitGroupKey.Materials.Add(Material);

            bAnyAlphaTested |= Material->HasAlphaMask();
            bAnyDoubleSided |= Material->IsDoubleSided();
        }

        uint32 HitGroupIndex = 0;
        if (uint32* ExistingIndex = Resources.RayTracingMeshToHitGroupIndex.Find(HitGroupKey))
        {
            HitGroupIndex = *ExistingIndex;
        }
        else
        {
            HitGroupIndex = Resources.RayTracingHitGroupBindings.Size();
            Resources.RayTracingMeshToHitGroupIndex[HitGroupKey] = HitGroupIndex;

            for (FMaterial* Material : HitGroupKey.Materials)
            {
                Resources.RayTracingHitGroupShaderIndices.Add(Material->HasAlphaMask() ? RAY_TRACING_HIT_GROUP_ALPHA_TESTED : RAY_TRACING_HIT_GROUP_OPAQUE);

                TArray<FRHIHitGroupLocalShaderBinding>& Record = Resources.RayTracingHitGroupBindings.Emplace();
                if (!bNeedBindlessData)
                {
                    if (AttributeBufferSRV)
                    {
                        Record.Add(FRHIHitGroupLocalShaderBinding::CreateShaderResourceView(AttributeBufferSRV, 0));
                    }

                    if (IndexBufferSRV)
                    {
                        Record.Add(FRHIHitGroupLocalShaderBinding::CreateShaderResourceView(IndexBufferSRV, 1));
                    }

                    for (uint32 Slot = 0; Slot < EMaterialTextureSlot::Count; ++Slot)
                    {
                        FRHITexture* SlotTexture = ResolveMaterialSlotTexture(*Material, EMaterialTextureSlot::Type(Slot));
                        Record.Add(FRHIHitGroupLocalShaderBinding::CreateShaderResourceView(SafeGetDefaultSRV(SlotTexture), RAY_TRACING_MATERIAL_SLOT_REGISTER + Slot));
                    }

                    Record.Add(FRHIHitGroupLocalShaderBinding::CreateSamplerState(Material->GetMaterialSampler(), 0));
                }
            }
        }

        const uint32 FirstGeometryRow = Resources.RayTracingGeometryTableData.Size();
        for (int32 SubMeshIndex = 0; SubMeshIndex < Mesh.GetNumSubMeshes(); ++SubMeshIndex)
        {
            const FMaterial* Material = HitGroupKey.Materials[SubMeshIndex];

            FRayTracingGeometryIndicesHLSL& GeometryTableRow = Resources.RayTracingGeometryTableData.Emplace();
            if (AttributeBufferSRV)
            {
                GeometryTableRow.AttributesHandle = AttributeBufferSRV->GetBindlessHandle();
            }

            if (IndexBufferSRV)
            {
                GeometryTableRow.IndicesHandle = IndexBufferSRV->GetBindlessHandle();
            }

            GeometryTableRow.MaterialIndex   = Material->GetBufferIndex();
            GeometryTableRow.DeterminantSign = StaticMesh->PerObjectBuffer.DeterminantSign;
            GeometryTableRow.FirstTriangle   = Mesh.GetSubMesh(SubMeshIndex).StartIndex / 3;
            GeometryTableRow.Flags           = Material->HasAlphaMask() ? RT_GEOMETRY_FLAG_ALPHA_TESTED : 0;
        }

        FRHIGeometryAccelerationStructureInstance Instance;
        Instance.Geometry      = Geometry;
        Instance.Flags         = bAnyDoubleSided ? ERayTracingInstanceFlags::CullDisable : ERayTracingInstanceFlags::None;

        if (bAnyAlphaTested)
        {
            Instance.Flags |= ERayTracingInstanceFlags::ForceNonOpaque;
        }

        Instance.HitGroupIndex = HitGroupIndex;
        Instance.InstanceIndex = FirstGeometryRow;
        Instance.Transform     = InstanceTransform;
        Resources.RayTracingGeometryInstances.Emplace(Instance);
    }

    STAT_SET(STAT_RT_Active,                  1);
    STAT_SET(STAT_RT_InstanceCount,           Resources.RayTracingGeometryInstances.Size());
    STAT_SET(STAT_RT_HitGroupCount,           Resources.RayTracingHitGroupBindings.Size());
    STAT_SET(STAT_RT_GeometryTableRows,       Resources.RayTracingGeometryTableData.Size());
    STAT_SET(STAT_RT_LazyBLASBuildsThisFrame, LazyBLASBuildsThisFrame);
    STAT_SET(STAT_RT_BLASCacheLoadsThisFrame, CacheLoadsThisFrame);
    STAT_SET(STAT_RT_SkippedNullGeometry,     SkippedNullGeometry);

    if (!Resources.RayTracingScene)
    {
        FRHISceneAccelerationStructureDesc SceneDesc(MakeArrayView(Resources.RayTracingGeometryInstances), EAccelerationStructureBuildFlags::None);
        Resources.RayTracingScene = RHI::CreateSceneAccelerationStructure(SceneDesc);

        if (Resources.RayTracingScene)
        {
            Resources.RayTracingScene->SetDebugName(TEXT("RayTracing Scene (TLAS)"));
        }
    }
    else
    {
        FRHISceneAccelerationStructureBuildDesc BuildSceneDesc;
        BuildSceneDesc.Instances    = Resources.RayTracingGeometryInstances.Data();
        BuildSceneDesc.NumInstances = Resources.RayTracingGeometryInstances.Size();
        BuildSceneDesc.bUpdate      = false;

        CommandList.BuildSceneAccelerationStructure(Resources.RayTracingScene.Get(), BuildSceneDesc);
        CommandList.UnorderedAccessBarrier(Resources.RayTracingScene.Get());
    }

    {
        TSet<FRHIRayTracingAccelerationStructure*> StillInScene;
        for (FRHIRayTracingAccelerationStructure* Structure : HandledStructures)
        {
            if (StructuresInScene.Contains(Structure))
            {
                StillInScene.Add(Structure);
            }
        }

        HandledStructures = ::Move(StillInScene);
    }

    TickCompactionAndCacheWrites(CommandList);

    if (CacheLoadsThisFrame > 0 || CacheMissesThisFrame > 0)
    {
        LOG_INFO("[FRayTracingSceneBuilder]: Loaded %u mesh asset structures from the cache and built %u", CacheLoadsThisFrame, CacheMissesThisFrame);
    }

    if (!Resources.RayTracingGeometryTableData.IsEmpty())
    {
        const uint32 RequiredCount = uint32(Resources.RayTracingGeometryTableData.Size());
        if (!Resources.RayTracingGeometryTableBuffer || RequiredCount > GeometryTableCapacity)
        {
            GeometryTableCapacity = RequiredCount;

            const FRHIBufferDesc TableDesc = FRHIBufferDesc::CreateStructuredBuffer(sizeof(FRayTracingGeometryIndicesHLSL), GeometryTableCapacity, EBufferFlags::Dynamic);
            Resources.RayTracingGeometryTableBuffer = RHI::CreateBuffer(TableDesc, ERHIResourceState::GenericRead, nullptr);
            Resources.RayTracingGeometryTableSRV    = nullptr;

            if (Resources.RayTracingGeometryTableBuffer)
            {
                Resources.RayTracingGeometryTableBuffer->SetDebugName(TEXT("RayTracing Geometry Table"));

                FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateBuffer(0, GeometryTableCapacity);
                Resources.RayTracingGeometryTableSRV = RHI::CreateShaderResourceView(Resources.RayTracingGeometryTableBuffer.Get(), SRVDesc);
            }
        }

        if (Resources.RayTracingGeometryTableBuffer)
        {
            CommandList.UpdateBuffer(Resources.RayTracingGeometryTableBuffer.Get(), FBufferRegion(0, sizeof(FRayTracingGeometryIndicesHLSL) * RequiredCount), Resources.RayTracingGeometryTableData.Data());
        }
    }
}

void FRayTracingSceneBuilder::RequestCompactionAndCacheWrite(FRHICommandList& CommandList, const FMesh& Mesh, FRHIGeometryAccelerationStructure* Geometry)
{
    FString                      Key;
    FAccelerationStructureCache* Cache = GRayTracingASCache ? StructureCache.FindCache(Mesh, Key) : nullptr;

    if (GRayTracingCompaction && CompactionHelper.RequestCompaction(CommandList, Geometry))
    {
        if (Cache)
        {
            FPendingCacheWrite& Pending = AwaitingCompaction.Emplace();
            Pending.Structure = Geometry;
            Pending.Cache     = Cache;
            Pending.Key       = ::Move(Key);
        }
    }
    else if (Cache)
    {
        SerializationHelper.RequestSerialize(CommandList, Geometry, *Cache, Key);
    }
}

void FRayTracingSceneBuilder::TickCompactionAndCacheWrites(FRHICommandList& CommandList)
{
    if (CompactionHelper.HasPendingWork())
    {
        TArray<FRHIRayTracingAccelerationStructureRef> Compacted;
        CompactionHelper.Tick(CommandList, Compacted);

        for (const FRHIRayTracingAccelerationStructureRef& Structure : Compacted)
        {
            for (int32 Index = 0; Index < AwaitingCompaction.Size(); ++Index)
            {
                FPendingCacheWrite& Pending = AwaitingCompaction[Index];
                if (Pending.Structure == Structure.Get())
                {
                    SerializationHelper.RequestSerialize(CommandList, Structure.Get(), *Pending.Cache, Pending.Key);
                    AwaitingCompaction.RemoveAt(Index);
                    break;
                }
            }
        }
    }

    if (SerializationHelper.HasPendingWork())
    {
        SerializationHelper.Tick(CommandList);
    }
}

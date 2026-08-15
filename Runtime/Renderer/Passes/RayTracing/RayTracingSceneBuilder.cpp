#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/Paths.h"
#include "Core/Templates/CString.h"
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
    "Renderer.RayTracing.Compaction",
    "When true, each BLAS is compacted once after build (multi-frame read-back + in-place swap). Requires AllowCompaction on the BLAS. "
    "UNVALIDATED on hardware.",
    GRayTracingCompaction);

static bool GRayTracingASCache = false;
static FAutoConsoleVariableRef CVarRayTracingASCache(
    "Renderer.RayTracing.ASCache",
    "When true, each BLAS is serialized once after build into the on-disk acceleration-structure cache. UNVALIDATED on hardware.",
    GRayTracingASCache);

FRayTracingSceneBuilder::FRayTracingSceneBuilder(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
    , GeometryTableCapacity(0)
    , SerializationHelper(&ASCache)
{
}

FRayTracingSceneBuilder::~FRayTracingSceneBuilder()
{
    Release();
}

bool FRayTracingSceneBuilder::Initialize()
{
    ASCacheBackend = MakeUniquePtr<FDiskAccelerationStructureCacheBackend>(Paths::GetAssetDir() + "/RTASCache");
    ASCache.SetBackend(ASCacheBackend.Get());
    return true;
}

void FRayTracingSceneBuilder::Release()
{
    CompactionHelper.ReleaseAll();
    SerializationHelper.ReleaseAll();
    CompactionRequested.Clear();
    SerializationRequested.Clear();
    ASCache.SetBackend(nullptr);
    ASCacheBackend.Reset();
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

    CompactionRequested.Clear();
    SerializationRequested.Clear();
}

void FRayTracingSceneBuilder::BuildSceneAccelerationData(FRHICommandList& CommandList, FFrameResources& Resources, FScene* Scene, bool bNeedBindlessData)
{
    TRACE_SCOPE("Gather Instances");

    Resources.RayTracingGeometryInstances.Clear();
    Resources.RayTracingHitGroupBindings.Clear();
    Resources.RayTracingGeometryTableData.Clear();

    uint32 NextInstanceIndex       = 0;
    uint32 LazyBLASBuildsThisFrame = 0;
    uint32 SkippedNullGeometry     = 0;

    for (const FSceneStaticMesh* StaticMesh : Scene->GetStaticMeshes())
    {
        TSharedPtr<FMaterial> Material = StaticMesh->GetMaterial();
        if (Material->HasAlphaMask())
        {
            continue;
        }

        const bool bHadGeometry = StaticMesh->Mesh->GetRayTracingGeometry() != nullptr;
        StaticMesh->Mesh->EnsureRayTracingResources();

        FRHIGeometryAccelerationStructure* Geometry = StaticMesh->Mesh->GetRayTracingGeometry();
        if (!Geometry)
        {
            ++SkippedNullGeometry;
            continue;
        }

        if (!bHadGeometry)
        {
            ++LazyBLASBuildsThisFrame;
        }

        const uint32 InstanceIndex = NextInstanceIndex++;

        FRHIShaderResourceView* AttributeBufferSRV = StaticMesh->Mesh->GetAttributeBufferSRV();
        FRHIShaderResourceView* IndexBufferSRV     = StaticMesh->Mesh->GetIndexBufferSRV();

        const Matrix3x4 InstanceTransform = StaticMesh->PerObjectBuffer.Transform;

        uint32 HitGroupIndex = 0;
        if (uint32* ExistingIndex = Resources.RayTracingMeshToHitGroupIndex.Find(StaticMesh->Mesh.Get()))
        {
            HitGroupIndex = *ExistingIndex;
        }
        else
        {
            HitGroupIndex = Resources.RayTracingHitGroupBindings.Size();
            Resources.RayTracingMeshToHitGroupIndex[StaticMesh->Mesh.Get()] = HitGroupIndex;

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

        {
            FRayTracingGeometryIndicesHLSL Geo;
            if (AttributeBufferSRV)
            {
                Geo.AttributesHandle = AttributeBufferSRV->GetBindlessHandle();
            }

            if (IndexBufferSRV)
            {
                Geo.IndicesHandle = IndexBufferSRV->GetBindlessHandle();
            }

            Geo.MaterialIndex   = Material->GetBufferIndex();
            Geo.DeterminantSign = StaticMesh->PerObjectBuffer.DeterminantSign;

            if (int32(InstanceIndex) >= Resources.RayTracingGeometryTableData.Size())
            {
                Resources.RayTracingGeometryTableData.Resize(int32(InstanceIndex) + 1);
            }

            Resources.RayTracingGeometryTableData[InstanceIndex] = Geo;
        }

        FRHIGeometryAccelerationStructureInstance Instance;
        Instance.Geometry      = Geometry;
        Instance.Flags         = Material->IsDoubleSided() ? ERayTracingInstanceFlags::CullDisable : ERayTracingInstanceFlags::None;
        Instance.HitGroupIndex = HitGroupIndex;
        Instance.InstanceIndex = InstanceIndex;
        Instance.Mask          = 0xff;
        Instance.Transform     = InstanceTransform;
        Resources.RayTracingGeometryInstances.Emplace(Instance);
    }

    STAT_SET(STAT_RT_Active,                  1);
    STAT_SET(STAT_RT_InstanceCount,           Resources.RayTracingGeometryInstances.Size());
    STAT_SET(STAT_RT_HitGroupCount,           Resources.RayTracingHitGroupBindings.Size());
    STAT_SET(STAT_RT_GeometryTableRows,       Resources.RayTracingGeometryTableData.Size());
    STAT_SET(STAT_RT_LazyBLASBuildsThisFrame, LazyBLASBuildsThisFrame);
    STAT_SET(STAT_RT_SkippedNullGeometry,     SkippedNullGeometry);

    if (!Resources.RayTracingScene)
    {
        FRHISceneAccelerationStructureDesc SceneDesc(MakeArrayView(Resources.RayTracingGeometryInstances), EAccelerationStructureBuildFlags::None);
        Resources.RayTracingScene = RHI::CreateSceneAccelerationStructure(SceneDesc);

        if (Resources.RayTracingScene)
        {
            Resources.RayTracingScene->SetDebugName("RayTracing Scene (TLAS)");
        }
    }
    else
    {
        FRHISceneAccelerationStructureBuildDesc BuildSceneDesc;
        BuildSceneDesc.Instances    = Resources.RayTracingGeometryInstances.Data();
        BuildSceneDesc.NumInstances = Resources.RayTracingGeometryInstances.Size();
        BuildSceneDesc.bUpdate      = false;

        CommandList.BuildSceneAccelerationStructure(Resources.RayTracingScene.Get(), BuildSceneDesc);
    }

    if (GRayTracingCompaction || GRayTracingASCache)
    {
        for (const FRHIGeometryAccelerationStructureInstance& Instance : Resources.RayTracingGeometryInstances)
        {
            FRHIGeometryAccelerationStructure* Geometry = Instance.Geometry;
            if (!Geometry)
            {
                continue;
            }

            if (GRayTracingCompaction && !CompactionRequested.Contains(Geometry))
            {
                CompactionHelper.RequestCompaction(CommandList, Geometry);
                CompactionRequested.Emplace(Geometry);
            }

            if (GRayTracingASCache && ASCache.HasBackend() && !SerializationRequested.Contains(Geometry))
            {
                CHAR KeyBuffer[32] = {};
                CString::Snprintf(KeyBuffer, int32(sizeof(KeyBuffer)), "blas_%llx", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(Geometry)));

                SerializationHelper.RequestSerialize(CommandList, Geometry, String(KeyBuffer));
                SerializationRequested.Emplace(Geometry);
            }
        }
    }

    if (CompactionHelper.HasPendingWork())
    {
        CompactionHelper.Tick(CommandList);
    }

    if (SerializationHelper.HasPendingWork())
    {
        SerializationHelper.Tick(CommandList);
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
                Resources.RayTracingGeometryTableBuffer->SetDebugName("RayTracing Geometry Table");

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

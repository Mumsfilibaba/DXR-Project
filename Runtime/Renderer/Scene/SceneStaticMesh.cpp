#include "Core/Memory/Memory.h"
#include "RHI/RHI.h"
#include "RHI/RHIQuery.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/MeshComponent.h"
#include "Renderer/Scene/SceneStaticMesh.h"

FSceneStaticMesh::FSceneStaticMesh(FScene* InScene, FMeshComponent* MeshComponent)
    : FSceneObject(InScene)
    , Materials()
    , Mesh(nullptr)
    , Actor(nullptr)
    , Geometry(nullptr)
    , CurrentOcclusionQuery(nullptr)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
    , NumVertices(0)
    , NumIndices(0)
    , IndexFormat(EIndexFormat::Unknown)
    , CurrentOcclusionQueryIndex(0)
    , NumFramesOccluded(0)
{
    FMemory::Memzero(OcclusionQueries, sizeof(OcclusionQueries));

    Mesh = MeshComponent->GetMesh();
    CHECK(Mesh != nullptr);

    Geometry     = Mesh->GetRayTracingGeometry();
    VertexBuffer = Mesh->GetVertexBuffer(EVertexStream::Packed);
    NumVertices  = Mesh->GetVertexCount();
    IndexBuffer  = Mesh->GetIndexBuffer();
    NumIndices   = Mesh->GetIndexCount();
    IndexFormat  = Mesh->GetIndexFormat();

    Actor     = MeshComponent->GetActorOwner();
    Materials = MeshComponent->GetMaterials();
}

FSceneStaticMesh::~FSceneStaticMesh()
{
    for (FRHIQuery* Query : OcclusionQueries)
    {
        if (Query)
        {
            Query->Release();
        }
    }
}

void FSceneStaticMesh::Tick()
{
    // Retrieve the transforms for each object so that they are ready for the GPU
    const FActorTransform& Transform = Actor->GetTransform();
    TransformBuffer.Transform    = Transform.GetTransformMatrix();
    TransformBuffer.Transform    = TransformBuffer.Transform.GetTranspose();
    TransformBuffer.TransformInv = Transform.GetTransformMatrixInverse();
}

void FSceneStaticMesh::UpdateOcclusion()
{
    const auto CheckOcclusion = [this]()
    {
        if (!FrustumVisibility.bWasVisible)
        {
            return false;
        }

        if (!CurrentOcclusionQuery)
        {
            return false;
        }

        uint64 NumSamples;
        if (!GetRHI()->RHIGetQueryResult(CurrentOcclusionQuery, NumSamples))
        {
            return false;
        }

        if (!NumSamples)
        {
            return true;
        }

        return false;
    };

    if (CheckOcclusion())
    {
        NumFramesOccluded++;
    }
    else
    {
        NumFramesOccluded = 0;
    }
}

#include "Core/Memory/Memory.h"
#include "RHI/RHI.h"
#include "RHI/RHIQuery.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Renderer/Scene/SceneStaticMesh.h"

FSceneStaticMesh::FSceneStaticMesh(FScene* InScene, FStaticMeshComponent* MeshComponent)
    : FSceneObject(InScene)
    , Materials()
    , Mesh(nullptr)
    , Actor(nullptr)
    , Geometry(nullptr)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
    , NumVertices(0)
    , NumIndices(0)
    , IndexFormat(EIndexFormat::Unknown)
{
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
}

void FSceneStaticMesh::Tick()
{
    // Retrieve the transforms for each object so that they are ready for the GPU
    const FActorTransform& Transform = Actor->GetTransform();
    TransformBuffer.Transform    = Transform.GetTransformMatrix();
    TransformBuffer.Transform    = TransformBuffer.Transform.GetTranspose();
    TransformBuffer.TransformInv = Transform.GetTransformMatrixInverse();

    // Create a world bounding-box
    const FAABB& LocalBounds = Mesh->GetAABB();

    const FVector3 Max = Transform.GetTransformMatrix().Transform(LocalBounds.Max);
    const FVector3 Min = Transform.GetTransformMatrix().Transform(LocalBounds.Min);
    WorldBounds = FAABB(Max, Min); 
}

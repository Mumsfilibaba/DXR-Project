#include "Core/Memory/Memory.h"
#include "RHI/RHI.h"
#include "RHI/RHIQuery.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Renderer/Scene/Scene.h"
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

    TransformBuffer.ObjectID = InScene ? InScene->GetOrCreateObjectID(Actor) : 0;
}

FSceneStaticMesh::~FSceneStaticMesh()
{
}

void FSceneStaticMesh::Tick()
{
    // Retrieve the transforms for each object so that they are ready for the GPU
    const FActorTransform& Transform = Actor->GetTransform();
    const FMatrix4 TransformM = Transform.GetTransformMatrix();
    const FMatrix4 TransformT = TransformM.GetTranspose();

    // Store a row-major float3x4 (3 first rows) for shaders + DXR instance transforms.
    TransformBuffer.Transform = FMatrix3x4(TransformT);

    // For normals/tangents we need inverse-transpose(Transform). Since Transform = transpose(TransformM),
    // we have inverse-transpose(Transform) = inverse(TransformM).
    const FMatrix4 TransformInv = Transform.GetTransformMatrixInverse();
    TransformBuffer.TransformInvT = FMatrix3x4(TransformInv);

    // Create a world bounding-box
    const FAABB& LocalBounds = Mesh->GetAABB();

    const FVector3 Max = Transform.GetTransformMatrix().Transform(LocalBounds.Max);
    const FVector3 Min = Transform.GetTransformMatrix().Transform(LocalBounds.Min);
    WorldBounds = FAABB(Max, Min); 
}

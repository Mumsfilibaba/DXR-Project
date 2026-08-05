#include "Core/Memory/Memory.h"
#include "RHI/RHI.h"
#include "RHI/RHIQuery.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneStaticMesh.h"
#include "Renderer/Scene/SceneProxyData.h"

FSceneStaticMesh::FSceneStaticMesh(FScene* InScene, const FStaticMeshInitData& InitData)
    : FSceneObject(InScene)
    , Mesh(InitData.Mesh)
    , Materials(InitData.Materials)
    , Geometry(nullptr)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
    , NumVertices(0)
    , NumIndices(0)
    , IndexFormat(EIndexFormat::Unknown)
{
    CHECK(Mesh != nullptr);

    Geometry     = Mesh->GetRayTracingGeometry();
    VertexBuffer = Mesh->GetVertexBuffer(EVertexStream::Packed);
    NumVertices  = Mesh->GetVertexCount();
    IndexBuffer  = Mesh->GetIndexBuffer();
    NumIndices   = Mesh->GetIndexCount();
    IndexFormat  = Mesh->GetIndexFormat();

    PerObjectBuffer.ObjectID = InitData.ObjectID;
}

FSceneStaticMesh::~FSceneStaticMesh() = default;

void FSceneStaticMesh::RenderThread_ApplyUpdate(const FStaticMeshProxyUpdate& Update)
{
    // Store a row-major float3x4 (3 first rows).
    const Matrix4 TransformT = Update.TransformMatrix.GetTranspose();
    PerObjectBuffer.Transform = Matrix3x4(TransformT);

    // For normals/tangents we need inverse-transpose(Transform).
    PerObjectBuffer.TransformInvT = Matrix3x4(Update.TransformMatrixInverse);

    // A mirroring transform (any negative scale) reverses the handedness of the tangent basis.
    PerObjectBuffer.DeterminantSign = (Update.TransformMatrix.GetDeterminant() < 0.0f) ? -1.0f : 1.0f;

    // Create a world bounding-box from the mesh's local AABB.
    const FAABB&  LocalBounds = Mesh->GetAABB();
    const Vector3 Max         = Update.TransformMatrix.Transform(LocalBounds.Max);
    const Vector3 Min         = Update.TransformMatrix.Transform(LocalBounds.Min);

    WorldBounds = FAABB(Max, Min);
}

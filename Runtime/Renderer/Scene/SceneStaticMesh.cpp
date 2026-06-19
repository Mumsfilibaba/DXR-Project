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
    , Materials(InitData.Materials)
    , Mesh(InitData.Mesh)
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

    TransformBuffer.ObjectID = InitData.ObjectID;
}

FSceneStaticMesh::~FSceneStaticMesh() = default;

void FSceneStaticMesh::RenderThread_ApplyUpdate(const FStaticMeshProxyUpdate& Update)
{
    // Store a row-major float3x4 (3 first rows) for shaders + DXR instance transforms. The shader
    // treats positions as column vectors, so we upload the transpose of the affine transform.
    const Matrix4 TransformT = Update.TransformMatrix.GetTranspose();
    TransformBuffer.Transform = Matrix3x4(TransformT);

    // For normals/tangents we need inverse-transpose(Transform). Since the uploaded Transform is the
    // transpose of the world matrix, inverse-transpose(Transform) == inverse(world matrix).
    TransformBuffer.TransformInvT = Matrix3x4(Update.TransformMatrixInverse);

    // Create a world bounding-box from the mesh's local AABB.
    const FAABB& LocalBounds = Mesh->GetAABB();
    const Vector3 Max = Update.TransformMatrix.Transform(LocalBounds.Max);
    const Vector3 Min = Update.TransformMatrix.Transform(LocalBounds.Min);
    WorldBounds = FAABB(Max, Min);
}

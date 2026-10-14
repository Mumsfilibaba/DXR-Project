#include "AssetCore/ImportScene.h"

FImportMaterial::FImportMaterial()
    : Name()
    , Routes()
    , Diffuse()
    , AmbientFactor(1.0f)
    , Roughness(1.0f)
    , Metallic(0.0f)
    , MaterialFlags(EMaterialFlags::None)
{
    for (int32& Texture : Textures)
    {
        Texture = -1;
    }

    Routes[EMaterialScalar::Opacity] = FMaterialSourceRoute(EMaterialTexture::Diffuse, ETextureChannel::A);
}

FMatrix4 ImportScenes::GetWorldTransform(const FImportScene& Scene, int32 NodeIndex)
{
    FMatrix4 World = Scene.Nodes[NodeIndex].LocalTransform;
    for (int32 Parent = Scene.Nodes[NodeIndex].Parent; Parent >= 0; Parent = Scene.Nodes[Parent].Parent)
    {
        World = World * Scene.Nodes[Parent].LocalTransform;
    }

    return World;
}

FMatrix4 ImportScenes::MirrorTransform(const FMatrix4& Transform, int32 Axis)
{
    CHECK(Axis >= 0 && Axis < 3);

    FMatrix4 Result = Transform;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Result.M[Axis][Index] = -Result.M[Axis][Index];
        Result.M[Index][Axis] = -Result.M[Index][Axis];
    }

    return Result;
}

FMeshData ImportScenes::BakeNodeMesh(const FImportScene& Scene, int32 NodeIndex)
{
    const FImportNode& Node = Scene.Nodes[NodeIndex];
    CHECK(Node.Mesh >= 0 && Node.Mesh < Scene.Meshes.Size());

    FMeshData Mesh = Scene.Meshes[Node.Mesh];
    Mesh.Name = Node.Name;

    const FMatrix4 World = GetWorldTransform(Scene, NodeIndex);
    if (!(World == FMatrix4::Identity()))
    {
        const FMatrix4 NormalTransform = World.GetInverse().GetTranspose();
        const bool     bMirrors        = World.GetDeterminant() < 0.0f;

        for (FSourceVertex& Vertex : Mesh.Vertices)
        {
            Vertex.Position = World.Transform(Vertex.Position);
            Vertex.Normal   = NormalTransform.TransformNormal(Vertex.Normal).GetNormalized();
            Vertex.Tangent  = World.TransformNormal(Vertex.Tangent).GetNormalized();

            if (bMirrors)
            {
                Vertex.TangentSign = -Vertex.TangentSign;
            }
        }

        if (bMirrors)
        {
            for (int32 Index = 0; Index + 2 < Mesh.Indices.Size(); Index += 3)
            {
                Swap(Mesh.Indices[Index + 1], Mesh.Indices[Index + 2]);
            }
        }
    }

    for (FSubMesh& SubMesh : Mesh.SubMeshes)
    {
        const int32 Slot = SubMesh.MaterialIndex;
        SubMesh.MaterialIndex = (Slot >= 0 && Slot < Node.Materials.Size()) ? Node.Materials[Slot] : -1;
    }

    return Mesh;
}

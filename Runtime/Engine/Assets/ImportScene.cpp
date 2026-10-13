#include "Engine/Assets/ImportScene.h"

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

bool ImportScenes::BakeToModelData(const FImportScene& Scene, FModelData& OutModelData, TFunction<TSharedPtr<FTexture2D>(const FString&)> LoadTexture)
{
    FModelData ModelData;

    TArray<TSharedPtr<FTexture2D>> Textures;
    for (const FImportTexture& Texture : Scene.Textures)
    {
        Textures.Add(LoadTexture ? LoadTexture(Texture.SourcePath) : nullptr);
    }

    for (const FImportMaterial& Source : Scene.Materials)
    {
        FMaterialData& Material = ModelData.Materials.Emplace();
        Material.Name          = Source.Name;
        Material.Diffuse       = Source.Diffuse;
        Material.AmbientFactor = Source.AmbientFactor;
        Material.Roughness     = Source.Roughness;
        Material.Metallic      = Source.Metallic;
        Material.MaterialFlags = Source.MaterialFlags;

        for (int32 Texture = 0; Texture < EMaterialTexture::Count; ++Texture)
        {
            const int32 TextureIndex = Source.Textures[Texture];
            Material.Textures[Texture] = (TextureIndex >= 0 && TextureIndex < Textures.Size()) ? Textures[TextureIndex] : nullptr;
        }

        for (int32 Scalar = 0; Scalar < EMaterialScalar::Count; ++Scalar)
        {
            Material.Routes[Scalar] = Source.Routes[Scalar];
        }
    }

    for (int32 NodeIndex = 0; NodeIndex < Scene.Nodes.Size(); ++NodeIndex)
    {
        const FImportNode& Node = Scene.Nodes[NodeIndex];
        if (Node.Mesh < 0)
        {
            continue;
        }

        if (Node.Mesh >= Scene.Meshes.Size() || Node.Parent >= NodeIndex)
        {
            return false;
        }

        FMeshData& Mesh = ModelData.Meshes.Emplace(Scene.Meshes[Node.Mesh]);
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
    }

    OutModelData = ::Move(ModelData);
    return true;
}

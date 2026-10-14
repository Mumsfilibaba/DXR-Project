#include "Engine/Assets/ImportSceneBaking.h"

bool ImportSceneBaking::BakeToModelData(const FImportScene& Scene, FModelData& OutModelData, TFunction<TSharedPtr<FTexture2D>(const FString&)> LoadTexture)
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

        ModelData.Meshes.Add(ImportScenes::BakeNodeMesh(Scene, NodeIndex));
    }

    OutModelData = ::Move(ModelData);
    return true;
}

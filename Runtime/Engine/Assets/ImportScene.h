#pragma once
#include "Core/Containers/Function.h"
#include "Core/Math/Matrix4.h"
#include "Engine/Assets/ModelData.h"

struct FImportTexture
{
    /** @brief The image file on disk */
    FString SourcePath;
};

struct FImportMaterial
{
    FImportMaterial();

    FString              Name;

    /** @brief The scene texture each source texture is, or -1 for none */
    int32                Textures[EMaterialTexture::Count];
    FMaterialSourceRoute Routes[EMaterialScalar::Count];
    FVector3             Diffuse;
    float                AmbientFactor;
    float                Roughness;
    float                Metallic;
    EMaterialFlags       MaterialFlags;
};

struct FImportNode
{
    FString       Name;

    /** @brief The parent node, which comes before this one in the scene, or -1 for a root */
    int32         Parent = -1;

    /** @brief Relative to the parent, for row vectors like every FMatrix4 */
    FMatrix4      LocalTransform = FMatrix4::Identity();

    /** @brief The scene mesh the node places, or -1 for none */
    int32         Mesh = -1;

    /** @brief The scene material of each of the mesh's material slots, which its submeshes' MaterialIndex picks, or -1 for none */
    TArray<int32> Materials;
};

/** @brief What a model importer reads from a source: the node hierarchy and the meshes, materials and textures it uses */
struct FImportScene
{
    TArray<FImportNode>     Nodes;

    /** @brief In the local space of the nodes that place them, each once however many nodes do */
    TArray<FMeshData>       Meshes;
    TArray<FImportMaterial> Materials;
    TArray<FImportTexture>  Textures;
};

struct ENGINE_API ImportScenes
{
    /** @return Returns the node's transform relative to the scene: its local transform followed by its parents' */
    NODISCARD static FMatrix4 GetWorldTransform(const FImportScene& Scene, int32 NodeIndex);

    /**
     * @return Returns Transform for a scene mirrored along Axis (0, 1 or 2 for X, Y or Z), whose meshes are mirrored the same way,
     * such as by FMeshData::ReverseHandedness, so the mirrored scene places them where the original did, mirrored
     */
    NODISCARD static FMatrix4 MirrorTransform(const FMatrix4& Transform, int32 Axis);

    /**
     * @return Returns the mesh the node places, named after the node, with the node's transform relative to the scene applied and
     * its submeshes' MaterialIndex turned from the node's slots into scene materials
     */
    NODISCARD static FMeshData BakeNodeMesh(const FImportScene& Scene, int32 NodeIndex);

    /**
     * @brief Bake the scene into the FModelData LoadModel uses: a mesh for each node that has one, from BakeNodeMesh
     * @param LoadTexture Loads a scene texture's source file, or returns nullptr to leave it out
     */
    static bool BakeToModelData(const FImportScene& Scene, FModelData& OutModelData, TFunction<TSharedPtr<FTexture2D>(const FString&)> LoadTexture);
};

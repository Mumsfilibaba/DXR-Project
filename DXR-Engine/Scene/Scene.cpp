#include "Scene.h"

#include "Components/MeshComponent.h"

#include "Rendering/TextureFactory.h"
#include "Rendering/MeshFactory.h"
#include "Rendering/Material.h"
#include "Rendering/Mesh.h"

#include "RenderLayer/Resources.h"

#include <tiny_obj_loader.h>

#include <unordered_map>

Scene::Scene()
    : Actors()
{
}

Scene::~Scene()
{
    for (Actor* CurrentActor : Actors)
    {
        SAFEDELETE(CurrentActor);
    }
    Actors.Clear();

    for (Light* CurrentLight : Lights)
    {
        SAFEDELETE(CurrentLight);
    }
    Lights.Clear();

    SAFEDELETE(CurrentCamera);
}

void Scene::Tick(Timestamp DeltaTime)
{
    UNREFERENCED_VARIABLE(DeltaTime);
}

void Scene::AddCamera(Camera* InCamera)
{
    if (CurrentCamera)
    {
        SAFEDELETE(CurrentCamera);
    }

    CurrentCamera = InCamera;
}

void Scene::AddActor(Actor* InActor)
{
    VALIDATE(InActor != nullptr);
    Actors.EmplaceBack(InActor);

    InActor->OnAddedToScene(this);

    MeshComponent* Component = InActor->GetComponentOfType<MeshComponent>();
    if (Component)
    {
        AddMeshComponent(Component);
    }
}

void Scene::AddLight(Light* InLight)
{
    VALIDATE(InLight != nullptr);
    Lights.EmplaceBack(InLight);
}

void Scene::OnAddedComponent(Component* NewComponent)
{
    MeshComponent* Component = Cast<MeshComponent>(NewComponent);
    if (Component)
    {
        AddMeshComponent(Component);
    }
}

Scene* Scene::LoadFromFile(const std::string& Filepath)
{
    // Load Scene File
    std::string Warning;
    std::string Error;
    std::vector<tinyobj::shape_t>    Shapes;
    std::vector<tinyobj::material_t> Materials;
    tinyobj::attrib_t Attributes;

    std::string MTLFiledir = std::string(Filepath.begin(), Filepath.begin() + Filepath.find_last_of('/'));
    if (!tinyobj::LoadObj(&Attributes, &Shapes, &Materials, &Warning, &Error, Filepath.c_str(), MTLFiledir.c_str(), true, false))
    {
        LOG_WARNING("[Scene]: Failed to load Scene '" + Filepath + "'." + " Warning: " + Warning + " Error: " + Error);
        return nullptr;
    }
    else
    {
        LOG_INFO("[Scene]: Loaded Scene'" + Filepath + "'");
    }

    // Create standard textures
    Byte Pixels[] = { 255, 255, 255, 255 };
    TSharedRef<Texture2D> WhiteTexture = TextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
    if (!WhiteTexture)
    {
        return nullptr;
    }
    else
    {
        WhiteTexture->SetName("[Scene] WhiteTexture");
    }

    Pixels[0] = 127;
    Pixels[1] = 127;
    Pixels[2] = 255;

    TSharedRef<Texture2D> NormalMap = TextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
    if (!NormalMap)
    {
        return nullptr;
    }
    else
    {
        NormalMap->SetName("[Scene] NormalMap");
    }

    // Create BaseMaterial
    MaterialProperties Properties;
    Properties.AO        = 1.0f;
    Properties.Metallic  = 0.0f;
    Properties.Roughness = 1.0f;

    TSharedPtr<Material> BaseMaterial = MakeShared<Material>(Properties);
    BaseMaterial->AlbedoMap    = WhiteTexture;
    BaseMaterial->AOMap        = WhiteTexture;
    BaseMaterial->HeightMap    = WhiteTexture;
    BaseMaterial->MetallicMap  = WhiteTexture;
    BaseMaterial->RoughnessMap = WhiteTexture;
    BaseMaterial->NormalMap    = NormalMap;
    BaseMaterial->Init();

    // Create All Materials in scene
    TArray<TSharedPtr<Material>> LoadedMaterials;
    std::unordered_map<std::string, TSharedRef<Texture2D>> MaterialTextures;
    for (tinyobj::material_t& Mat : Materials)
    {
        // Create new material with default properties
        MaterialProperties MatProps;
        MatProps.Metallic  = Mat.ambient[0];
        MatProps.AO        = 1.0f;
        MatProps.Roughness = 1.0f;

        TSharedPtr<Material>& NewMaterial = LoadedMaterials.EmplaceBack(MakeShared<Material>(MatProps));
        LOG_INFO("Loaded materialID=" + std::to_string(LoadedMaterials.Size() - 1));

        NewMaterial->AlbedoMap    = WhiteTexture;
        NewMaterial->AOMap        = WhiteTexture;
        NewMaterial->HeightMap    = WhiteTexture;
        NewMaterial->MetallicMap  = WhiteTexture;
        NewMaterial->RoughnessMap = WhiteTexture;
        NewMaterial->NormalMap    = NormalMap;

        // Metallic
        if (!Mat.ambient_texname.empty())
        {
            ConvertBackslashes(Mat.ambient_texname);
            if (MaterialTextures.count(Mat.ambient_texname) == 0)
            {
                std::string TexName = MTLFiledir + '/' + Mat.ambient_texname;
                TSharedRef<Texture2D> Texture = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
                if (Texture)
                {
                    Texture->SetName(Mat.ambient_texname);
                    MaterialTextures[Mat.ambient_texname] = Texture;
                }
                else
                {
                    MaterialTextures[Mat.ambient_texname] = WhiteTexture;
                }
            }

            NewMaterial->MetallicMap = MaterialTextures[Mat.ambient_texname];
        }

        // Albedo
        if (!Mat.diffuse_texname.empty())
        {
            ConvertBackslashes(Mat.diffuse_texname);
            if (MaterialTextures.count(Mat.diffuse_texname) == 0)
            {
                std::string TexName = MTLFiledir + '/' + Mat.diffuse_texname;
                TSharedRef<Texture2D> Texture = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm); 
                if (Texture)
                {
                    Texture->SetName(Mat.diffuse_texname);
                    MaterialTextures[Mat.diffuse_texname] = Texture;
                }
                else
                {
                    MaterialTextures[Mat.diffuse_texname] = WhiteTexture;
                }
            }

            NewMaterial->AlbedoMap = MaterialTextures[Mat.diffuse_texname];
        }

        // Roughness
        if (!Mat.specular_highlight_texname.empty())
        {
            ConvertBackslashes(Mat.specular_highlight_texname);
            if (MaterialTextures.count(Mat.specular_highlight_texname) == 0)
            {
                std::string TexName = MTLFiledir + '/' + Mat.specular_highlight_texname;
                TSharedRef<Texture2D> Texture = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
                if (Texture)
                {
                    Texture->SetName(Mat.specular_highlight_texname);
                    MaterialTextures[Mat.specular_highlight_texname] = Texture;
                }
                else
                {
                    MaterialTextures[Mat.specular_highlight_texname] = WhiteTexture;
                }
            }

            NewMaterial->RoughnessMap = MaterialTextures[Mat.specular_highlight_texname];
        }

        // Normal
        if (!Mat.bump_texname.empty())
        {
            ConvertBackslashes(Mat.bump_texname);
            if (MaterialTextures.count(Mat.bump_texname) == 0)
            {
                std::string TexName = MTLFiledir + '/' + Mat.bump_texname;
                TSharedRef<Texture2D> Texture = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
                if (Texture)
                {
                    Texture->SetName(Mat.bump_texname);
                    MaterialTextures[Mat.bump_texname] = Texture;
                }
                else
                {
                    MaterialTextures[Mat.bump_texname] = WhiteTexture;
                }
            }

            NewMaterial->NormalMap = MaterialTextures[Mat.bump_texname];
        }

        // Alpha
        if (!Mat.alpha_texname.empty())
        {
            ConvertBackslashes(Mat.alpha_texname);
            if (MaterialTextures.count(Mat.alpha_texname) == 0)
            {
                std::string TexName = MTLFiledir + '/' + Mat.alpha_texname;
                TSharedRef<Texture2D> Texture = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
                if (Texture)
                {
                    Texture->SetName(Mat.alpha_texname);
                    MaterialTextures[Mat.alpha_texname] = Texture;
                }
                else
                {
                    MaterialTextures[Mat.alpha_texname] = WhiteTexture;
                }
            }

            NewMaterial->AlphaMask = MaterialTextures[Mat.alpha_texname];
        }

        NewMaterial->Init();
    }

    // Construct Scene
    MeshData Data;
    TUniquePtr<Scene> LoadedScene = MakeUnique<Scene>();
    std::unordered_map<Vertex, UInt32, VertexHasher> UniqueVertices;

    for (const tinyobj::shape_t& Shape : Shapes)
    {
        // Start at index zero for eaxh mesh and loop until all indices are processed
        UInt32 i = 0;
        while (i < Shape.mesh.indices.size())
        {
            // Start a new mesh
            Data.Indices.Clear();
            Data.Vertices.Clear();

            UniqueVertices.clear();

            UInt32 Face = i / 3;
            const Int32 MaterialID = Shape.mesh.material_ids[Face];
            for (; i < Shape.mesh.indices.size(); i++)
            {
                // Break if material is not the same
                Face = i / 3;
                if (Shape.mesh.material_ids[Face] != MaterialID)
                {
                    break;
                }

                const tinyobj::index_t& Index = Shape.mesh.indices[i];
                Vertex TempVertex;

                // Normals and texcoords are optional, Positions are required
                VALIDATE(Index.vertex_index >= 0);

                size_t PositionIndex = 3 * static_cast<size_t>(Index.vertex_index);
                TempVertex.Position =
                {
                    Attributes.vertices[PositionIndex + 0],
                    Attributes.vertices[PositionIndex + 1],
                    Attributes.vertices[PositionIndex + 2],
                };

                if (Index.normal_index >= 0)
                {
                    size_t NormalIndex = 3 * static_cast<size_t>(Index.normal_index);
                    TempVertex.Normal =
                    {
                        Attributes.normals[NormalIndex + 0],
                        Attributes.normals[NormalIndex + 1],
                        Attributes.normals[NormalIndex + 2],
                    };
                }

                if (Index.texcoord_index >= 0)
                {
                    size_t TexCoordIndex = 2 * static_cast<size_t>(Index.texcoord_index);
                    TempVertex.TexCoord =
                    {
                        Attributes.texcoords[TexCoordIndex + 0],
                        Attributes.texcoords[TexCoordIndex + 1],
                    };
                }

                if (UniqueVertices.count(TempVertex) == 0)
                {
                    UniqueVertices[TempVertex] = static_cast<UInt32>(Data.Vertices.Size());
                    Data.Vertices.PushBack(TempVertex);
                }

                Data.Indices.EmplaceBack(UniqueVertices[TempVertex]);
            }

            // Calculate tangents and create mesh
            MeshFactory::CalculateTangents(Data);
            TSharedPtr<Mesh> NewMesh = Mesh::Make(Data);

            // Setup new actor for this shape
            Actor* NewActor = DBG_NEW Actor();
            NewActor->SetName(Shape.name);
            NewActor->GetTransform().SetScale(0.015f, 0.015f, 0.015f);

            // Add a MeshComponent
            MeshComponent* NewComponent = DBG_NEW MeshComponent(NewActor);
            NewComponent->Mesh = NewMesh;
            if (MaterialID >= 0)
            {
                LOG_INFO(Shape.name + " got materialID=" + std::to_string(MaterialID));
                NewComponent->Material = LoadedMaterials[MaterialID];
            }
            else
            {
                NewComponent->Material = BaseMaterial;
            }

            NewActor->AddComponent(NewComponent);
            LoadedScene->AddActor(NewActor);
        }
    }

    return LoadedScene.Release();
}

void Scene::AddMeshComponent(MeshComponent* Component)
{
    MeshDrawCommand Command;
    Command.CurrentActor = Component->GetOwningActor();
    Command.Geometry     = Component->Mesh->RayTracingGeometry.Get();
    Command.VertexBuffer = Component->Mesh->VertexBuffer.Get();
    Command.VertexCount  = Component->Mesh->VertexCount;
    Command.IndexBuffer  = Component->Mesh->IndexBuffer.Get();
    Command.IndexCount   = Component->Mesh->IndexCount;
    Command.Material     = Component->Material.Get();
    Command.Mesh         = Component->Mesh.Get();
    MeshDrawCommands.PushBack(Command);
}

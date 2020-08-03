#include "Scene.h"

#include "Rendering/Renderer.h"
#include "Rendering/TextureFactory.h"
#include "Rendering/MeshFactory.h"

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

	Actors.clear();

	SAFEDELETE(CurrentCamera);
}

void Scene::AddCamera(Camera* InCamera)
{
	SAFEDELETE(CurrentCamera);
	CurrentCamera = InCamera;
}

void Scene::AddActor(Actor* InActor)
{
	VALIDATE(InActor != nullptr);
	Actors.emplace_back(InActor);
}

Scene* Scene::LoadFromFile(const std::string& Filepath, D3D12Device* Device)
{
	// Load Scene File
	tinyobj::attrib_t Attributes;
	std::string Warning;
	std::string Error;
	std::vector<tinyobj::shape_t>		Shapes;
	std::vector<tinyobj::material_t>	Materials;

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
	std::shared_ptr<D3D12Texture> WhiteTexture = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromMemory(Device, Pixels, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!WhiteTexture)
	{
		return nullptr;
	}
	else
	{
		WhiteTexture->SetDebugName("[Scene] WhiteTexture");
	}

	Pixels[0] = 127;
	Pixels[1] = 127;
	Pixels[2] = 255;

	std::shared_ptr<D3D12Texture> NormalMap = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromMemory(Device, Pixels, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!NormalMap)
	{
		return nullptr;
	}
	else
	{
		NormalMap->SetDebugName("[Scene] NormalMap");
	}

	// Create BaseMaterial
	MaterialProperties Properties;
	Properties.AO			= 1.0f;
	Properties.Metallic		= 0.0f;
	Properties.Roughness	= 1.0f;

	std::shared_ptr<Material> BaseMaterial = std::make_shared<Material>(Properties);
	BaseMaterial->AlbedoMap = WhiteTexture;
	BaseMaterial->AO		= WhiteTexture;
	BaseMaterial->Height	= WhiteTexture;
	BaseMaterial->Metallic	= WhiteTexture;
	BaseMaterial->Roughness = WhiteTexture;
	BaseMaterial->NormalMap = NormalMap;
	BaseMaterial->Initialize(Device);

	// Create All Materials in scene
	std::vector<std::shared_ptr<Material>> LoadedMaterials;
	std::unordered_map<std::string, std::shared_ptr<D3D12Texture>> MaterialTextures;
	for (tinyobj::material_t& Mat : Materials)
	{
		// Create new material with default properties
		MaterialProperties MatProps;
		MatProps.Metallic	= Mat.ambient[0];
		MatProps.AO			= 1.0f;
		MatProps.Roughness	= 1.0f;

		std::shared_ptr<Material>& NewMaterial = LoadedMaterials.emplace_back(std::make_shared<Material>(MatProps));
		NewMaterial->AlbedoMap	= WhiteTexture;
		NewMaterial->AO			= WhiteTexture;
		NewMaterial->Height		= WhiteTexture;
		NewMaterial->Metallic	= WhiteTexture;
		NewMaterial->Roughness	= WhiteTexture;
		NewMaterial->NormalMap	= NormalMap;

		// Metallic
		if (!Mat.ambient_texname.empty())
		{
			ConvertBackslashes(Mat.ambient_texname);
			
			if (MaterialTextures.count(Mat.ambient_texname) == 0)
			{
				std::string TexName = MTLFiledir + '/' + Mat.ambient_texname;
				std::shared_ptr<D3D12Texture> Texture = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Device, TexName, TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
				if (Texture)
				{
					Texture->SetDebugName(Mat.ambient_texname);
					MaterialTextures[Mat.ambient_texname] = Texture;
				}
				else
				{
					MaterialTextures[Mat.ambient_texname] = WhiteTexture;
				}
			}

			NewMaterial->Metallic = MaterialTextures[Mat.ambient_texname];
		}

		// Albedo
		if (!Mat.diffuse_texname.empty())
		{
			ConvertBackslashes(Mat.diffuse_texname);

			if (MaterialTextures.count(Mat.diffuse_texname) == 0)
			{
				std::string TexName = MTLFiledir + '/' + Mat.diffuse_texname;
				std::shared_ptr<D3D12Texture> Texture = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Device, TexName, TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
				if (Texture)
				{
					Texture->SetDebugName(Mat.diffuse_texname);
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
				std::shared_ptr<D3D12Texture> Texture = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Device, TexName, TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
				if (Texture)
				{
					Texture->SetDebugName(Mat.specular_highlight_texname);
					MaterialTextures[Mat.specular_highlight_texname] = Texture;
				}
				else
				{
					MaterialTextures[Mat.specular_highlight_texname] = WhiteTexture;
				}
			}

			NewMaterial->Roughness = MaterialTextures[Mat.specular_highlight_texname];
		}

		// Normal
		if (!Mat.bump_texname.empty())
		{
			ConvertBackslashes(Mat.bump_texname);

			if (MaterialTextures.count(Mat.bump_texname) == 0)
			{
				std::string TexName = MTLFiledir + '/' + Mat.bump_texname;
				std::shared_ptr<D3D12Texture> Texture = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Device, TexName, TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
				if (Texture)
				{
					Texture->SetDebugName(Mat.bump_texname);
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
				std::shared_ptr<D3D12Texture> Texture = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Device, TexName, TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
				if (Texture)
				{
					Texture->SetDebugName(Mat.alpha_texname);
					MaterialTextures[Mat.alpha_texname] = Texture;
				}
				else
				{
					MaterialTextures[Mat.alpha_texname] = WhiteTexture;
				}
			}
		}

		NewMaterial->Initialize(Device);
	}

	// Construct Scene
	MeshData Data;
	std::unique_ptr<Scene>				LoadedScene = std::make_unique<Scene>();
	std::unordered_map<Vertex, Uint32>	UniqueVertices;

	for (const tinyobj::shape_t& Shape : Shapes)
	{
		// Create Mesh
		Data.Indices.clear();
		Data.Vertices.clear();
		UniqueVertices.clear();

		for (const tinyobj::index_t& Index : Shape.mesh.indices)
		{
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
				UniqueVertices[TempVertex] = static_cast<Uint32>(Data.Vertices.size());
				Data.Vertices.push_back(TempVertex);
			}

			Data.Indices.emplace_back(UniqueVertices[TempVertex]);
		}

		// Calculate tangents and create mesh
		MeshFactory::CalculateTangents(Data);
		std::shared_ptr<Mesh> NewMesh = std::shared_ptr<Mesh>(Mesh::Make(Device, Data));

		// Setup new actor for this shape
		Actor* NewActor = new Actor();
		NewActor->SetDebugName(Shape.name);
		NewActor->GetTransform().SetScale(0.015f, 0.015f, 0.015f);

		// Add a MeshComponent
		MeshComponent* NewComponent = new MeshComponent(NewActor);
		NewComponent->Mesh		= NewMesh;
		if (Shape.mesh.material_ids[0] >= 0)
		{
			NewComponent->Material = LoadedMaterials[Shape.mesh.material_ids[0]];
		}
		else
		{
			NewComponent->Material	= BaseMaterial;
		}

		NewActor->AddComponent(NewComponent);
		LoadedScene->AddActor(NewActor);
	}

	return LoadedScene.release();
}

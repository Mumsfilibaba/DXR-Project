#include "Game.h"

#include "Rendering/Renderer.h"
#include "Rendering/DebugUI.h"
#include "Rendering/TextureFactory.h"

#include "Scene/Scene.h"
#include "Scene/PointLight.h"
#include "Scene/DirectionalLight.h"
#include "Scene/Components/MeshComponent.h"

#include "Application/Input.h"
#include "Application/Application.h"

/*
* Game
*/

Game* Game::CurrentGame = nullptr;

Game::Game()
	: CurrentScene(nullptr)
	, CurrentCamera(nullptr)
{
}

Game::~Game()
{
	SAFEDELETE(CurrentScene);
}

bool Game::Initialize()
{
	// Initialize Scene
	constexpr Float	SphereOffset	= 1.25f;
	constexpr UInt32	SphereCountX	= 8;
	constexpr Float	StartPositionX	= (-static_cast<Float>(SphereCountX) * SphereOffset) / 2.0f;
	constexpr UInt32	SphereCountY	= 8;
	constexpr Float	StartPositionY	= (-static_cast<Float>(SphereCountY) * SphereOffset) / 2.0f;
	constexpr Float	MetallicDelta	= 1.0f / SphereCountY;
	constexpr Float	RoughnessDelta	= 1.0f / SphereCountX;

	Actor*			NewActor		= nullptr;
	MeshComponent*	NewComponent	= nullptr;
	CurrentScene = Scene::LoadFromFile("../Assets/Scenes/Sponza/Sponza.obj");

	// Create Spheres
	MeshData SphereMeshData		= MeshFactory::CreateSphere(3);
	TSharedPtr<Mesh> SphereMesh	= Mesh::Make(SphereMeshData);
	SphereMesh->ShadowOffset = 0.05f;

	// Create standard textures
	Byte Pixels[] = { 255, 255, 255, 255 };
	TSharedPtr<D3D12Texture> BaseTexture = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromMemory(Pixels, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!BaseTexture)
	{
		return false;
	}
	else
	{
		BaseTexture->SetDebugName("BaseTexture");
	}

	Pixels[0] = 127;
	Pixels[1] = 127;
	Pixels[2] = 255;

	TSharedPtr<D3D12Texture> BaseNormal = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromMemory(Pixels, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!BaseNormal)
	{
		return false;
	}
	else
	{
		BaseNormal->SetDebugName("BaseNormal");
	}

	Pixels[0] = 255;
	Pixels[1] = 255;
	Pixels[2] = 255;

	TSharedPtr<D3D12Texture> WhiteTexture = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromMemory(Pixels, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!WhiteTexture)
	{
		return false;
	}
	else
	{
		WhiteTexture->SetDebugName("WhiteTexture");
	}

	MaterialProperties MatProperties;
	UInt32 SphereIndex = 0;
	for (UInt32 y = 0; y < SphereCountY; y++)
	{
		for (UInt32 x = 0; x < SphereCountX; x++)
		{
			NewActor = new Actor();
			NewActor->GetTransform().SetPosition(StartPositionX + (x * SphereOffset), 8.0f + StartPositionY + (y * SphereOffset), 0.0f);

			NewActor->SetDebugName("Sphere[" + std::to_string(SphereIndex) + "]");
			SphereIndex++;

			CurrentScene->AddActor(NewActor);

			NewComponent = new MeshComponent(NewActor);
			NewComponent->Mesh		= SphereMesh;
			NewComponent->Material	= MakeShared<Material>(MatProperties);

			NewComponent->Material->AlbedoMap		= BaseTexture;
			NewComponent->Material->NormalMap		= BaseNormal;
			NewComponent->Material->RoughnessMap	= WhiteTexture;
			NewComponent->Material->HeightMap		= WhiteTexture;
			NewComponent->Material->AOMap			= WhiteTexture;
			NewComponent->Material->MetallicMap		= WhiteTexture;
			NewComponent->Material->Initialize();

			NewActor->AddComponent(NewComponent);

			MatProperties.Roughness += RoughnessDelta;
		}

		MatProperties.Roughness = 0.05f;
		MatProperties.Metallic += MetallicDelta;
	}

	// Create Other Meshes
	MeshData CubeMeshData = MeshFactory::CreateCube();

	NewActor = new Actor();
	CurrentScene->AddActor(NewActor);

	NewActor->SetDebugName("Cube");
	NewActor->GetTransform().SetPosition(0.0f, 2.0f, -2.0f);

	MatProperties.AO			= 1.0f;
	MatProperties.Metallic		= 1.0f;
	MatProperties.Roughness		= 1.0f;
	MatProperties.EnableHeight	= 1;

	NewComponent = new MeshComponent(NewActor);
	NewComponent->Mesh		= Mesh::Make(CubeMeshData);
	NewComponent->Material	= MakeShared<Material>(MatProperties);

	TSharedPtr<D3D12Texture> AlbedoMap = TSharedPtr(TextureFactory::LoadFromFile("../Assets/Textures/Gate_Albedo.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!AlbedoMap)
	{
		return false;
	}
	else
	{
		AlbedoMap->SetDebugName("AlbedoMap");
	}

	TSharedPtr<D3D12Texture> NormalMap = TSharedPtr(TextureFactory::LoadFromFile("../Assets/Textures/Gate_Normal.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!NormalMap)
	{
		return false;
	}
	else
	{
		NormalMap->SetDebugName("NormalMap");
	}

	TSharedPtr<D3D12Texture> AOMap = TSharedPtr(TextureFactory::LoadFromFile("../Assets/Textures/Gate_AO.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!AOMap)
	{
		return false;
	}
	else
	{
		AOMap->SetDebugName("AOMap");
	}

	TSharedPtr<D3D12Texture> RoughnessMap = TSharedPtr(TextureFactory::LoadFromFile("../Assets/Textures/Gate_Roughness.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!RoughnessMap)
	{
		return false;
	}
	else
	{
		RoughnessMap->SetDebugName("RoughnessMap");
	}

	TSharedPtr<D3D12Texture> HeightMap = TSharedPtr(TextureFactory::LoadFromFile("../Assets/Textures/Gate_Height.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!HeightMap)
	{
		return false;
	}
	else
	{
		HeightMap->SetDebugName("HeightMap");
	}

	TSharedPtr<D3D12Texture> MetallicMap = TSharedPtr(TextureFactory::LoadFromFile("../Assets/Textures/Gate_Metallic.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!MetallicMap)
	{
		return false;
	}
	else
	{
		MetallicMap->SetDebugName("MetallicMap");
	}

	NewComponent->Material->AlbedoMap		= AlbedoMap;
	NewComponent->Material->NormalMap		= NormalMap;
	NewComponent->Material->RoughnessMap	= RoughnessMap;
	NewComponent->Material->HeightMap		= HeightMap;
	NewComponent->Material->AOMap			= AOMap;
	NewComponent->Material->MetallicMap		= MetallicMap;
	NewComponent->Material->Initialize();
	NewActor->AddComponent(NewComponent);

	CurrentCamera = new Camera();
	CurrentScene->AddCamera(CurrentCamera);

	// Add PointLight- Source
	PointLight* Light0 = new PointLight();
	Light0->SetPosition(14.0f, 1.0f, -0.5f);
	Light0->SetColor(1.0f, 1.0f, 1.0f);
	Light0->SetShadowBias(0.0005f);
	Light0->SetMaxShadowBias(0.009f);
	Light0->SetShadowFarPlane(50.0f);
	Light0->SetIntensity(100.0f);
	CurrentScene->AddLight(Light0);

	// Add DirectionalLight- Source
	DirectionalLight* Light1 = new DirectionalLight();
	Light1->SetShadowBias(0.0008f);
	Light1->SetMaxShadowBias(0.008f);
	Light1->SetShadowNearPlane(0.01f);
	Light1->SetShadowFarPlane(140.0f);
	Light1->SetColor(1.0f, 1.0f, 1.0f);
	Light1->SetIntensity(10.0f);
	CurrentScene->AddLight(Light1);

	Scene::SetCurrentScene(CurrentScene);
	return true;
}

void Game::Destroy()
{
	delete this;
}

void Game::Tick(Timestamp DeltaTime)
{
	// Run app
	const Float Delta = static_cast<Float>(DeltaTime.AsSeconds());
	const Float RotationSpeed = 45.0f;

	Float Speed = 1.0f;
	if (Input::IsKeyDown(EKey::KEY_LEFT_SHIFT))
	{
		Speed = 4.0f;
	}

	if (Input::IsKeyDown(EKey::KEY_RIGHT))
	{
		CurrentCamera->Rotate(0.0f, XMConvertToRadians(RotationSpeed * Delta), 0.0f);
	}
	else if (Input::IsKeyDown(EKey::KEY_LEFT))
	{
		CurrentCamera->Rotate(0.0f, XMConvertToRadians(-RotationSpeed * Delta), 0.0f);
	}

	if (Input::IsKeyDown(EKey::KEY_UP))
	{
		CurrentCamera->Rotate(XMConvertToRadians(-RotationSpeed * Delta), 0.0f, 0.0f);
	}
	else if (Input::IsKeyDown(EKey::KEY_DOWN))
	{
		CurrentCamera->Rotate(XMConvertToRadians(RotationSpeed * Delta), 0.0f, 0.0f);
	}

	if (Input::IsKeyDown(EKey::KEY_W))
	{
		CurrentCamera->Move(0.0f, 0.0f, Speed * Delta);
	}
	else if (Input::IsKeyDown(EKey::KEY_S))
	{
		CurrentCamera->Move(0.0f, 0.0f, -Speed * Delta);
	}

	if (Input::IsKeyDown(EKey::KEY_A))
	{
		CurrentCamera->Move(Speed * Delta, 0.0f, 0.0f);
	}
	else if (Input::IsKeyDown(EKey::KEY_D))
	{
		CurrentCamera->Move(-Speed * Delta, 0.0f, 0.0f);
	}

	if (Input::IsKeyDown(EKey::KEY_Q))
	{
		CurrentCamera->Move(0.0f, Speed * Delta, 0.0f);
	}
	else if (Input::IsKeyDown(EKey::KEY_E))
	{
		CurrentCamera->Move(0.0f, -Speed * Delta, 0.0f);
	}

	CurrentCamera->UpdateMatrices();
}

Game& Game::GetCurrent()
{
	VALIDATE(CurrentGame != nullptr);
	return *CurrentGame;
}

void Game::SetCurrent(Game* InCurrentGame)
{
	CurrentGame = InCurrentGame;
}

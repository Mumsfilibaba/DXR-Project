#include "Sandbox.h"

#include "Math/Math.h"

#include "Rendering/Renderer.h"
#include "Rendering/DebugUI.h"
#include "Rendering/TextureFactory.h"

#include "Scene/Scene.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"
#include "Scene/Components/MeshComponent.h"

#include "Application/Input.h"

/*
* MakeGameInstance
*/

Game* MakeGameInstance()
{
	return DBG_NEW Sandbox();
}

/*
* Sandbox
*/

Bool Sandbox::Init()
{
	// Initialize Scene
	constexpr Float	 SphereOffset		= 1.25f;
	constexpr UInt32 SphereCountX		= 8;
	constexpr Float	 StartPositionX		= (-static_cast<Float>(SphereCountX) * SphereOffset) / 2.0f;
	constexpr UInt32 SphereCountY		= 8;
	constexpr Float	 StartPositionY		= (-static_cast<Float>(SphereCountY) * SphereOffset) / 2.0f;
	constexpr Float	 MetallicDelta		= 1.0f / SphereCountY;
	constexpr Float	 RoughnessDelta		= 1.0f / SphereCountX;

	Actor* NewActor				= nullptr;
	MeshComponent* NewComponent	= nullptr;
	CurrentScene = Scene::LoadFromFile("../Assets/Scenes/Sponza/Sponza.obj");

	// Create Spheres
	MeshData SphereMeshData = MeshFactory::CreateSphere(3);
	TSharedPtr<Mesh> SphereMesh = Mesh::Make(SphereMeshData);
	SphereMesh->ShadowOffset = 0.05f;

	// Create standard textures
	Byte Pixels[] =
	{
		255,
		255,
		255,
		255
	};

	SampledTexture2D BaseTexture = TextureFactory::LoadSampledTextureFromMemory(Pixels, 1, 1, 0, EFormat::Format_R8G8B8A8_Unorm);
	if (!BaseTexture)
	{
		return false;
	}
	else
	{
		BaseTexture.SetName("BaseTexture");
	}

	Pixels[0] = 127;
	Pixels[1] = 127;
	Pixels[2] = 255;

	SampledTexture2D BaseNormal = TextureFactory::LoadSampledTextureFromMemory(Pixels, 1, 1, 0, EFormat::Format_R8G8B8A8_Unorm);
	if (!BaseNormal)
	{
		return false;
	}
	else
	{
		BaseNormal.SetName("BaseNormal");
	}

	Pixels[0] = 255;
	Pixels[1] = 255;
	Pixels[2] = 255;

	SampledTexture2D WhiteTexture = TextureFactory::LoadSampledTextureFromMemory(Pixels, 1, 1, 0, EFormat::Format_R8G8B8A8_Unorm);
	if (!WhiteTexture)
	{
		return false;
	}
	else
	{
		WhiteTexture.SetName("WhiteTexture");
	}

	MaterialProperties MatProperties;
	MatProperties.AO = 1.0f;

	UInt32 SphereIndex = 0;
	for (UInt32 y = 0; y < SphereCountY; y++)
	{
		for (UInt32 x = 0; x < SphereCountX; x++)
		{
			NewActor = DBG_NEW Actor();
			NewActor->GetTransform().SetTranslation(StartPositionX + (x * SphereOffset), 8.0f + StartPositionY + (y * SphereOffset), 0.0f);

			NewActor->SetName("Sphere[" + std::to_string(SphereIndex) + "]");
			SphereIndex++;

			CurrentScene->AddActor(NewActor);

			NewComponent			= DBG_NEW MeshComponent(NewActor);
			NewComponent->Mesh		= SphereMesh;
			NewComponent->Material	= MakeShared<Material>(MatProperties);

			NewComponent->Material->AlbedoMap		= BaseTexture;
			NewComponent->Material->NormalMap		= BaseNormal;
			NewComponent->Material->RoughnessMap	= WhiteTexture;
			NewComponent->Material->HeightMap		= WhiteTexture;
			NewComponent->Material->AOMap			= WhiteTexture;
			NewComponent->Material->MetallicMap		= WhiteTexture;
			NewComponent->Material->Init();

			NewActor->AddComponent(NewComponent);

			MatProperties.Roughness += RoughnessDelta;
		}

		MatProperties.Roughness = 0.05f;
		MatProperties.Metallic += MetallicDelta;
	}

	// Create Other Meshes
	MeshData CubeMeshData = MeshFactory::CreateCube();

	NewActor = DBG_NEW Actor();
	CurrentScene->AddActor(NewActor);

	NewActor->SetName("Cube");
	NewActor->GetTransform().SetTranslation(0.0f, 2.0f, -2.0f);

	MatProperties.AO			= 1.0f;
	MatProperties.Metallic		= 1.0f;
	MatProperties.Roughness		= 1.0f;
	MatProperties.EnableHeight	= 1;

	NewComponent			= DBG_NEW MeshComponent(NewActor);
	NewComponent->Mesh		= Mesh::Make(CubeMeshData);
	NewComponent->Material	= MakeShared<Material>(MatProperties);

	SampledTexture2D AlbedoMap = TextureFactory::LoadSampledTextureFromFile(
		"../Assets/Textures/Gate_Albedo.png",
		TextureFactoryFlag_GenerateMips,
		EFormat::Format_R8G8B8A8_Unorm);
	if (!AlbedoMap)
	{
		return false;
	}
	else
	{
		AlbedoMap.SetName("AlbedoMap");
	}

	SampledTexture2D NormalMap = TextureFactory::LoadSampledTextureFromFile(
		"../Assets/Textures/Gate_Normal.png",
		TextureFactoryFlag_GenerateMips,
		EFormat::Format_R8G8B8A8_Unorm);
	if (!NormalMap)
	{
		return false;
	}
	else
	{
		NormalMap.SetName("NormalMap");
	}

	SampledTexture2D AOMap = TextureFactory::LoadSampledTextureFromFile(
		"../Assets/Textures/Gate_AO.png",
		TextureFactoryFlag_GenerateMips,
		EFormat::Format_R8G8B8A8_Unorm);
	if (!AOMap)
	{
		return false;
	}
	else
	{
		AOMap.SetName("AOMap");
	}

	SampledTexture2D RoughnessMap = TextureFactory::LoadSampledTextureFromFile(
		"../Assets/Textures/Gate_Roughness.png",
		TextureFactoryFlag_GenerateMips,
		EFormat::Format_R8G8B8A8_Unorm);
	if (!RoughnessMap)
	{
		return false;
	}
	else
	{
		RoughnessMap.SetName("RoughnessMap");
	}

	SampledTexture2D HeightMap = TextureFactory::LoadSampledTextureFromFile(
		"../Assets/Textures/Gate_Height.png",
		TextureFactoryFlag_GenerateMips,
		EFormat::Format_R8G8B8A8_Unorm);
	if (!HeightMap)
	{
		return false;
	}
	else
	{
		HeightMap.SetName("HeightMap");
	}

	SampledTexture2D MetallicMap = TextureFactory::LoadSampledTextureFromFile(
		"../Assets/Textures/Gate_Metallic.png"
		, TextureFactoryFlag_GenerateMips,
		EFormat::Format_R8G8B8A8_Unorm);
	if (!MetallicMap)
	{
		return false;
	}
	else
	{
		MetallicMap.SetName("MetallicMap");
	}

	NewComponent->Material->AlbedoMap		= AlbedoMap;
	NewComponent->Material->NormalMap		= NormalMap;
	NewComponent->Material->RoughnessMap	= RoughnessMap;
	NewComponent->Material->HeightMap		= HeightMap;
	NewComponent->Material->AOMap			= AOMap;
	NewComponent->Material->MetallicMap		= MetallicMap;
	NewComponent->Material->Init();
	NewActor->AddComponent(NewComponent);

	CurrentCamera = DBG_NEW Camera();
	CurrentScene->AddCamera(CurrentCamera);

	// Add PointLight- Source
	PointLight* Light0 = DBG_NEW PointLight();
	Light0->SetPosition(16.5f, 1.0f, 0.0f);
	Light0->SetColor(1.0f, 1.0f, 1.0f);
	Light0->SetShadowBias(0.0005f);
	Light0->SetMaxShadowBias(0.009f);
	Light0->SetShadowFarPlane(50.0f);
	Light0->SetIntensity(20.0f);
	CurrentScene->AddLight(Light0);

	PointLight* Light1 = DBG_NEW PointLight();
	Light1->SetPosition(-17.5f, 1.0f, 0.0f);
	Light1->SetColor(1.0f, 1.0f, 1.0f);
	Light1->SetShadowBias(0.0005f);
	Light1->SetMaxShadowBias(0.009f);
	Light1->SetShadowFarPlane(50.0f);
	Light1->SetIntensity(20.0f);
	CurrentScene->AddLight(Light1);

	PointLight* Light2 = DBG_NEW PointLight();
	Light2->SetPosition(16.5f, 11.0f, 0.0f);
	Light2->SetColor(1.0f, 1.0f, 1.0f);
	Light2->SetShadowBias(0.0005f);
	Light2->SetMaxShadowBias(0.009f);
	Light2->SetShadowFarPlane(50.0f);
	Light2->SetIntensity(20.0f);
	CurrentScene->AddLight(Light2);

	PointLight* Light3 = DBG_NEW PointLight();
	Light3->SetPosition(-17.5f, 11.0f, 0.0f);
	Light3->SetColor(1.0f, 1.0f, 1.0f);
	Light3->SetShadowBias(0.0005f);
	Light3->SetMaxShadowBias(0.009f);
	Light3->SetShadowFarPlane(50.0f);
	Light3->SetIntensity(20.0f);
	CurrentScene->AddLight(Light3);

	// Add DirectionalLight- Source
	DirectionalLight* Light4 = DBG_NEW DirectionalLight();
	Light4->SetShadowBias(0.0008f);
	Light4->SetMaxShadowBias(0.008f);
	Light4->SetShadowNearPlane(0.01f);
	Light4->SetShadowFarPlane(140.0f);
	Light4->SetColor(1.0f, 1.0f, 1.0f);
	Light4->SetIntensity(10.0f);
	CurrentScene->AddLight(Light4);

	Scene::SetCurrentScene(CurrentScene);
	return true;
}

void Sandbox::Tick(Timestamp DeltaTime)
{
	const Float Delta = static_cast<Float>(DeltaTime.AsSeconds());
	const Float RotationSpeed = 45.0f;

	if (Input::IsKeyDown(EKey::Key_Right))
	{
		CurrentCamera->Rotate(0.0f, XMConvertToRadians(RotationSpeed * Delta), 0.0f);
	}
	else if (Input::IsKeyDown(EKey::Key_Left))
	{
		CurrentCamera->Rotate(0.0f, XMConvertToRadians(-RotationSpeed * Delta), 0.0f);
	}

	if (Input::IsKeyDown(EKey::Key_Up))
	{
		CurrentCamera->Rotate(XMConvertToRadians(-RotationSpeed * Delta), 0.0f, 0.0f);
	}
	else if (Input::IsKeyDown(EKey::Key_Down))
	{
		CurrentCamera->Rotate(XMConvertToRadians(RotationSpeed * Delta), 0.0f, 0.0f);
	}

	Float Acceleration = 15.0f;
	if (Input::IsKeyDown(EKey::Key_LeftShift))
	{
		Acceleration = Acceleration * 3;
	}

	XMFLOAT3 CameraAcceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
	if (Input::IsKeyDown(EKey::Key_W))
	{
		CameraAcceleration.z = Acceleration;
	}
	else if (Input::IsKeyDown(EKey::Key_S))
	{
		CameraAcceleration.z = -Acceleration;
	}

	if (Input::IsKeyDown(EKey::Key_A))
	{
		CameraAcceleration.x = Acceleration;
	}
	else if (Input::IsKeyDown(EKey::Key_D))
	{
		CameraAcceleration.x = -Acceleration;
	}

	if (Input::IsKeyDown(EKey::Key_Q))
	{
		CameraAcceleration.y = Acceleration;
	}
	else if (Input::IsKeyDown(EKey::Key_E))
	{
		CameraAcceleration.y = -Acceleration;
	}

	const Float Deacceleration = -5.0f;
	CameraSpeed = CameraSpeed + (CameraSpeed * Deacceleration) * Delta;
	CameraSpeed = CameraSpeed + (CameraAcceleration * Delta);

	XMFLOAT3 Speed = CameraSpeed * Delta;
	CurrentCamera->Move(Speed.x, Speed.y, Speed.z);
	CurrentCamera->UpdateMatrices();
}

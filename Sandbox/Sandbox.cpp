#include "Sandbox.h"

#include "Math/Math.h"

#include "Rendering/Renderer.h"
#include "Rendering/DebugUI.h"
#include "Rendering/Resources/TextureFactory.h"

#include "Scene/Scene.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"
#include "Scene/Components/MeshComponent.h"

#include "Application/Input.h"

#include <random>

#define ENABLE_LIGHT_TEST 0

Game* MakeGameInstance()
{
    return DBG_NEW Sandbox();
}

Bool Sandbox::Init()
{
    // Initialize Scene
    Actor* NewActor             = nullptr;
    MeshComponent* NewComponent = nullptr;
    CurrentScene = Scene::LoadFromFile("../Assets/Scenes/Sponza/Sponza.obj");

    // Create Spheres
    MeshData SphereMeshData     = MeshFactory::CreateSphere(3);
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

    TSharedRef<Texture2D> BaseTexture = TextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
    if (!BaseTexture)
    {
        return false;
    }
    else
    {
        BaseTexture->SetName("BaseTexture");
    }

    Pixels[0] = 127;
    Pixels[1] = 127;
    Pixels[2] = 255;

    TSharedRef<Texture2D> BaseNormal = TextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
    if (!BaseNormal)
    {
        return false;
    }
    else
    {
        BaseNormal->SetName("BaseNormal");
    }

    Pixels[0] = 255;
    Pixels[1] = 255;
    Pixels[2] = 255;

    TSharedRef<Texture2D> WhiteTexture = TextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
    if (!WhiteTexture)
    {
        return false;
    }
    else
    {
        WhiteTexture->SetName("WhiteTexture");
    }

    constexpr Float	 SphereOffset   = 1.25f;
    constexpr UInt32 SphereCountX   = 8;
    constexpr Float	 StartPositionX = (-static_cast<Float>(SphereCountX) * SphereOffset) / 2.0f;
    constexpr UInt32 SphereCountY   = 8;
    constexpr Float	 StartPositionY = (-static_cast<Float>(SphereCountY) * SphereOffset) / 2.0f;
    constexpr Float	 MetallicDelta  = 1.0f / SphereCountY;
    constexpr Float	 RoughnessDelta = 1.0f / SphereCountX;

    MaterialProperties MatProperties;
    MatProperties.AO = 1.0f;

    UInt32 SphereIndex = 0;
    for (UInt32 y = 0; y < SphereCountY; y++)
    {
        for (UInt32 x = 0; x < SphereCountX; x++)
        {
            NewActor = DBG_NEW Actor();
            NewActor->GetTransform().SetTranslation(StartPositionX + (x * SphereOffset), 8.0f + StartPositionY + (y * SphereOffset), 40.0f);

            NewActor->SetName("Sphere[" + std::to_string(SphereIndex) + "]");
            SphereIndex++;

            CurrentScene->AddActor(NewActor);

            NewComponent = DBG_NEW MeshComponent(NewActor);
            NewComponent->Mesh     = SphereMesh;
            NewComponent->Material = MakeShared<Material>(MatProperties);

            NewComponent->Material->AlbedoMap    = BaseTexture;
            NewComponent->Material->NormalMap    = BaseNormal;
            NewComponent->Material->RoughnessMap = WhiteTexture;
            NewComponent->Material->HeightMap    = WhiteTexture;
            NewComponent->Material->AOMap        = WhiteTexture;
            NewComponent->Material->MetallicMap  = WhiteTexture;
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
    NewActor->GetTransform().SetTranslation(0.0f, 2.0f, 42.0f);

    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 1.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 1;

    NewComponent = DBG_NEW MeshComponent(NewActor);
    NewComponent->Mesh     = Mesh::Make(CubeMeshData);
    NewComponent->Material = MakeShared<Material>(MatProperties);

    TSharedRef<Texture2D> AlbedoMap = TextureFactory::LoadFromFile("../Assets/Textures/Gate_Albedo.png", TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (!AlbedoMap)
    {
        return false;
    }
    else
    {
        AlbedoMap->SetName("AlbedoMap");
    }

    TSharedRef<Texture2D> NormalMap = TextureFactory::LoadFromFile("../Assets/Textures/Gate_Normal.png", TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (!NormalMap)
    {
        return false;
    }
    else
    {
        NormalMap->SetName("NormalMap");
    }

    TSharedRef<Texture2D> AOMap = TextureFactory::LoadFromFile("../Assets/Textures/Gate_AO.png", TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (!AOMap)
    {
        return false;
    }
    else
    {
        AOMap->SetName("AOMap");
    }

    TSharedRef<Texture2D> RoughnessMap = TextureFactory::LoadFromFile("../Assets/Textures/Gate_Roughness.png", TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (!RoughnessMap)
    {
        return false;
    }
    else
    {
        RoughnessMap->SetName("RoughnessMap");
    }

    TSharedRef<Texture2D> HeightMap = TextureFactory::LoadFromFile("../Assets/Textures/Gate_Height.png", TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (!HeightMap)
    {
        return false;
    }
    else
    {
        HeightMap->SetName("HeightMap");
    }

    TSharedRef<Texture2D> MetallicMap = TextureFactory::LoadFromFile("../Assets/Textures/Gate_Metallic.png" , TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (!MetallicMap)
    {
        return false;
    }
    else
    {
        MetallicMap->SetName("MetallicMap");
    }

    NewComponent->Material->AlbedoMap    = AlbedoMap;
    NewComponent->Material->NormalMap    = NormalMap;
    NewComponent->Material->RoughnessMap = RoughnessMap;
    NewComponent->Material->HeightMap    = HeightMap;
    NewComponent->Material->AOMap        = AOMap;
    NewComponent->Material->MetallicMap  = MetallicMap;
    NewComponent->Material->Init();
    NewActor->AddComponent(NewComponent);

    NewActor = DBG_NEW Actor();
    CurrentScene->AddActor(NewActor);

    NewActor->SetName("Plane");
    NewActor->GetTransform().SetRotation(0.0f, 0.0f, Math::HALF_PI);
    NewActor->GetTransform().SetUniformScale(40.0f);
    NewActor->GetTransform().SetTranslation(0.0f, 0.0f, 42.0f);

    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 0.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 0;
    MatProperties.Albedo       = XMFLOAT3(1.0f, 1.0f, 1.0f);

    NewComponent = DBG_NEW MeshComponent(NewActor);
    NewComponent->Mesh     = Mesh::Make(MeshFactory::CreatePlane(10, 10));
    NewComponent->Material = MakeShared<Material>(MatProperties);
    NewComponent->Material->AlbedoMap    = BaseTexture;
    NewComponent->Material->NormalMap    = BaseNormal;
    NewComponent->Material->RoughnessMap = WhiteTexture;
    NewComponent->Material->HeightMap    = WhiteTexture;
    NewComponent->Material->AOMap        = WhiteTexture;
    NewComponent->Material->MetallicMap  = WhiteTexture;
    NewComponent->Material->Init();
    NewActor->AddComponent(NewComponent);

    CurrentCamera = DBG_NEW Camera();
    CurrentScene->AddCamera(CurrentCamera);

    // Add PointLight- Source
    const Float Intensity = 50.0f;

    PointLight* Light0 = DBG_NEW PointLight();
    Light0->SetPosition(16.5f, 1.0f, 0.0f);
    Light0->SetColor(1.0f, 1.0f, 1.0f);
    Light0->SetShadowBias(0.001f);
    Light0->SetMaxShadowBias(0.009f);
    Light0->SetShadowFarPlane(50.0f);
    Light0->SetIntensity(Intensity);
    Light0->SetShadowCaster(true);
    CurrentScene->AddLight(Light0);

    PointLight* Light1 = DBG_NEW PointLight();
    Light1->SetPosition(-17.5f, 1.0f, 0.0f);
    Light1->SetColor(1.0f, 1.0f, 1.0f);
    Light1->SetShadowBias(0.001f);
    Light1->SetMaxShadowBias(0.009f);
    Light1->SetShadowFarPlane(50.0f);
    Light1->SetIntensity(Intensity);
    Light1->SetShadowCaster(true);
    CurrentScene->AddLight(Light1);

    PointLight* Light2 = DBG_NEW PointLight();
    Light2->SetPosition(16.5f, 11.0f, 0.0f);
    Light2->SetColor(1.0f, 1.0f, 1.0f);
    Light2->SetShadowBias(0.001f);
    Light2->SetMaxShadowBias(0.009f);
    Light2->SetShadowFarPlane(50.0f);
    Light2->SetIntensity(Intensity);
    Light2->SetShadowCaster(true);
    CurrentScene->AddLight(Light2);

    PointLight* Light3 = DBG_NEW PointLight();
    Light3->SetPosition(-17.5f, 11.0f, 0.0f);
    Light3->SetColor(1.0f, 1.0f, 1.0f);
    Light3->SetShadowBias(0.001f);
    Light3->SetMaxShadowBias(0.009f);
    Light3->SetShadowFarPlane(50.0f);
    Light3->SetIntensity(Intensity);
    Light3->SetShadowCaster(true);
    CurrentScene->AddLight(Light3);

#if ENABLE_LIGHT_TEST
    // Add multiple lights
    std::uniform_real_distribution<Float> RandomFloats(0.0f, 1.0f);
    std::default_random_engine Generator;

    for (UInt32 i = 0; i < 256; i++)
    {
        Float x = RandomFloats(Generator) * 35.0f - 17.5f;
        Float y = RandomFloats(Generator) * 22.0f;
        Float z = RandomFloats(Generator) * 16.0f - 8.0f;
        Float Intentsity = RandomFloats(Generator) * 5.0f + 1.0f;

        PointLight* Light = DBG_NEW PointLight();
        Light->SetPosition(x, y, z);
        Light->SetColor(RandomFloats(Generator), RandomFloats(Generator), RandomFloats(Generator));
        Light->SetIntensity(Intentsity);
        CurrentScene->AddLight(Light);
    }
#endif

    // Add DirectionalLight- Source
    DirectionalLight* Light4 = DBG_NEW DirectionalLight();
    Light4->SetShadowBias(0.0008f);
    Light4->SetMaxShadowBias(0.008f);
    Light4->SetShadowNearPlane(0.01f);
    Light4->SetShadowFarPlane(140.0f);
    Light4->SetColor(1.0f, 1.0f, 1.0f);
    Light4->SetIntensity(10.0f);
    CurrentScene->AddLight(Light4);

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

#include "Sandbox.h"

#include "Math/Math.h"

#include "Rendering/Renderer.h"
#include "Rendering/DebugUI.h"
#include "Rendering/Resources/TextureFactory.h"

#include "Scene/Scene.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"
#include "Scene/Components/MeshComponent.h"

#include "Core/Input/InputManager.h"

#include <random>

#define ENABLE_LIGHT_TEST 0

Application* CreateApplication()
{
    return DBG_NEW Sandbox();
}

bool Sandbox::Init()
{
    if (!Application::Init())
    {
        return false;
    }

    // Initialize Scene
    Actor* NewActor             = nullptr;
    MeshComponent* NewComponent = nullptr;
    Scene = Scene::LoadFromFile("../Assets/Scenes/Sponza/Sponza.obj");

    // Create Spheres
    MeshData SphereMeshData     = MeshFactory::CreateSphere(3);
    TSharedPtr<Mesh> SphereMesh = Mesh::Make(SphereMeshData);
    SphereMesh->ShadowOffset = 0.05f; // TODO: Remove

    // Create standard textures
    uint8 Pixels[] =
    {
        255,
        255,
        255,
        255
    };

    TRef<Texture2D> BaseTexture = TextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
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

    TRef<Texture2D> BaseNormal = TextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
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

    TRef<Texture2D> WhiteTexture = TextureFactory::LoadFromMemory(Pixels, 1, 1, 0, EFormat::R8G8B8A8_Unorm);
    if (!WhiteTexture)
    {
        return false;
    }
    else
    {
        WhiteTexture->SetName("WhiteTexture");
    }

    constexpr float  SphereOffset   = 1.25f;
    constexpr uint32 SphereCountX   = 8;
    constexpr float  StartPositionX = (-static_cast<float>(SphereCountX) * SphereOffset) / 2.0f;
    constexpr uint32 SphereCountY   = 8;
    constexpr float  StartPositionY = (-static_cast<float>(SphereCountY) * SphereOffset) / 2.0f;
    constexpr float  MetallicDelta  = 1.0f / SphereCountY;
    constexpr float  RoughnessDelta = 1.0f / SphereCountX;

    MaterialProperties MatProperties;
    MatProperties.AO = 1.0f;

    uint32 SphereIndex = 0;
    for (uint32 y = 0; y < SphereCountY; y++)
    {
        for (uint32 x = 0; x < SphereCountX; x++)
        {
            NewActor = DBG_NEW Actor();
            NewActor->GetTransform().SetTranslation(StartPositionX + (x * SphereOffset), 1.0f, 40.0f + StartPositionY + (y * SphereOffset));

            NewActor->SetName("Sphere[" + std::to_string(SphereIndex) + "]");
            SphereIndex++;

            Scene->AddActor(NewActor);

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
    Scene->AddActor(NewActor);

    NewActor->SetName("Cube");
    NewActor->GetTransform().SetTranslation(0.0f, 2.0f, 50.0f);

    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 1.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 1;

    NewComponent = DBG_NEW MeshComponent(NewActor);
    NewComponent->Mesh     = Mesh::Make(CubeMeshData);
    NewComponent->Material = MakeShared<Material>(MatProperties);

    TRef<Texture2D> AlbedoMap = TextureFactory::LoadFromFile("../Assets/Textures/Gate_Albedo.png", TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (!AlbedoMap)
    {
        return false;
    }
    else
    {
        AlbedoMap->SetName("AlbedoMap");
    }

    TRef<Texture2D> NormalMap = TextureFactory::LoadFromFile("../Assets/Textures/Gate_Normal.png", TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (!NormalMap)
    {
        return false;
    }
    else
    {
        NormalMap->SetName("NormalMap");
    }

    TRef<Texture2D> AOMap = TextureFactory::LoadFromFile("../Assets/Textures/Gate_AO.png", TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
    if (!AOMap)
    {
        return false;
    }
    else
    {
        AOMap->SetName("AOMap");
    }

    TRef<Texture2D> RoughnessMap = TextureFactory::LoadFromFile("../Assets/Textures/Gate_Roughness.png", TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
    if (!RoughnessMap)
    {
        return false;
    }
    else
    {
        RoughnessMap->SetName("RoughnessMap");
    }

    TRef<Texture2D> HeightMap = TextureFactory::LoadFromFile("../Assets/Textures/Gate_Height.png", TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
    if (!HeightMap)
    {
        return false;
    }
    else
    {
        HeightMap->SetName("HeightMap");
    }

    TRef<Texture2D> MetallicMap = TextureFactory::LoadFromFile("../Assets/Textures/Gate_Metallic.png" , TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
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
    Scene->AddActor(NewActor);

    NewActor->SetName("Plane");
    NewActor->GetTransform().SetRotation(0.0f, 0.0f, Math::HALF_PI);
    NewActor->GetTransform().SetUniformScale(50.0f);
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

    AlbedoMap = TextureFactory::LoadFromFile("../Assets/Textures/StreetLight/BaseColor.jpg", TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (!AlbedoMap)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        AlbedoMap->SetName("AlbedoMap");
    }

    NormalMap = TextureFactory::LoadFromFile("../Assets/Textures/StreetLight/Normal.jpg", TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (!NormalMap)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        NormalMap->SetName("NormalMap");
    }

    RoughnessMap = TextureFactory::LoadFromFile("../Assets/Textures/StreetLight/Roughness.jpg", TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
    if (!RoughnessMap)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RoughnessMap->SetName("RoughnessMap");
    }

    MetallicMap = TextureFactory::LoadFromFile("../Assets/Textures/StreetLight/Metallic.jpg", TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
    if (!MetallicMap)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        MetallicMap->SetName("MetallicMap");
    }

    TSharedPtr<Mesh>     StreetLight    = Mesh::Make(MeshFactory::CreateFromFile("../Assets/Models/Street_Light.obj"));
    TSharedPtr<Material> StreetLightMat = MakeShared<Material>(MatProperties);

    for (uint32 i = 0; i < 4; i++)
    {
        NewActor = DBG_NEW Actor();
        Scene->AddActor(NewActor);

        NewActor->SetName("Street Light " + std::to_string(i));
        NewActor->GetTransform().SetUniformScale(0.25f);
        NewActor->GetTransform().SetTranslation(15.0f, 0.0f, 55.0f - ((float)i * 3.0f));

        MatProperties.AO           = 1.0f;
        MatProperties.Metallic     = 1.0f;
        MatProperties.Roughness    = 1.0f;
        MatProperties.EnableHeight = 0;
        MatProperties.Albedo       = XMFLOAT3(1.0f, 1.0f, 1.0f);

        NewComponent = DBG_NEW MeshComponent(NewActor);
        NewComponent->Mesh                   = StreetLight;
        NewComponent->Material               = StreetLightMat;
        NewComponent->Material->AlbedoMap    = AlbedoMap;
        NewComponent->Material->NormalMap    = NormalMap;
        NewComponent->Material->RoughnessMap = RoughnessMap;
        NewComponent->Material->HeightMap    = WhiteTexture;
        NewComponent->Material->AOMap        = WhiteTexture;
        NewComponent->Material->MetallicMap  = MetallicMap;
        NewComponent->Material->Init();
        NewActor->AddComponent(NewComponent);
    }

    CurrentCamera = DBG_NEW Camera();
    Scene->AddCamera(CurrentCamera);

    // Add PointLight- Source
    const float Intensity = 50.0f;

    PointLight* Light0 = DBG_NEW PointLight();
    Light0->SetPosition(16.5f, 1.0f, 0.0f);
    Light0->SetColor(1.0f, 1.0f, 1.0f);
    Light0->SetShadowBias(0.001f);
    Light0->SetMaxShadowBias(0.009f);
    Light0->SetShadowFarPlane(50.0f);
    Light0->SetIntensity(Intensity);
    Light0->SetShadowCaster(true);
    Scene->AddLight(Light0);

    PointLight* Light1 = DBG_NEW PointLight();
    Light1->SetPosition(-17.5f, 1.0f, 0.0f);
    Light1->SetColor(1.0f, 1.0f, 1.0f);
    Light1->SetShadowBias(0.001f);
    Light1->SetMaxShadowBias(0.009f);
    Light1->SetShadowFarPlane(50.0f);
    Light1->SetIntensity(Intensity);
    Light1->SetShadowCaster(true);
    Scene->AddLight(Light1);

    PointLight* Light2 = DBG_NEW PointLight();
    Light2->SetPosition(16.5f, 11.0f, 0.0f);
    Light2->SetColor(1.0f, 1.0f, 1.0f);
    Light2->SetShadowBias(0.001f);
    Light2->SetMaxShadowBias(0.009f);
    Light2->SetShadowFarPlane(50.0f);
    Light2->SetIntensity(Intensity);
    Light2->SetShadowCaster(true);
    Scene->AddLight(Light2);

    PointLight* Light3 = DBG_NEW PointLight();
    Light3->SetPosition(-17.5f, 11.0f, 0.0f);
    Light3->SetColor(1.0f, 1.0f, 1.0f);
    Light3->SetShadowBias(0.001f);
    Light3->SetMaxShadowBias(0.009f);
    Light3->SetShadowFarPlane(50.0f);
    Light3->SetIntensity(Intensity);
    Light3->SetShadowCaster(true);
    Scene->AddLight(Light3);

#if ENABLE_LIGHT_TEST
    // Add multiple lights
    std::uniform_real_distribution<float> RandomFloats(0.0f, 1.0f);
    std::default_random_engine Generator;

    for (uint32 i = 0; i < 256; i++)
    {
        float x = RandomFloats(Generator) * 35.0f - 17.5f;
        float y = RandomFloats(Generator) * 22.0f;
        float z = RandomFloats(Generator) * 16.0f - 8.0f;
        float Intentsity = RandomFloats(Generator) * 5.0f + 1.0f;

        PointLight* Light = DBG_NEW PointLight();
        Light->SetPosition(x, y, z);
        Light->SetColor(RandomFloats(Generator), RandomFloats(Generator), RandomFloats(Generator));
        Light->SetIntensity(Intentsity);
        Scene->AddLight(Light);
    }
#endif

    // Add DirectionalLight- Source
    DirectionalLight* Light4 = DBG_NEW DirectionalLight();
    Light4->SetShadowBias(0.0001f);
    Light4->SetMaxShadowBias(0.001f);
    Light4->SetColor(1.0f, 1.0f, 1.0f);
    Light4->SetIntensity(10.0f);
    Light4->SetRotation(Math::ToRadians(45.0f), Math::ToRadians(145.0f), 0.0f);
    Scene->AddLight(Light4);

    return true;
}

void Sandbox::Tick(Timestamp DeltaTime)
{
    Application::Tick(DeltaTime);

    const float Delta = static_cast<float>(DeltaTime.AsSeconds());
    const float RotationSpeed = 45.0f;

    if (InputManager::Get().IsKeyDown(EKey::Key_Right))
    {
        CurrentCamera->Rotate(0.0f, XMConvertToRadians(RotationSpeed * Delta), 0.0f);
    }
    else if (InputManager::Get().IsKeyDown(EKey::Key_Left))
    {
        CurrentCamera->Rotate(0.0f, XMConvertToRadians(-RotationSpeed * Delta), 0.0f);
    }

    if (InputManager::Get().IsKeyDown(EKey::Key_Up))
    {
        CurrentCamera->Rotate(XMConvertToRadians(-RotationSpeed * Delta), 0.0f, 0.0f);
    }
    else if (InputManager::Get().IsKeyDown(EKey::Key_Down))
    {
        CurrentCamera->Rotate(XMConvertToRadians(RotationSpeed * Delta), 0.0f, 0.0f);
    }

    float Acceleration = 15.0f;
    if (InputManager::Get().IsKeyDown(EKey::Key_LeftShift))
    {
        Acceleration = Acceleration * 3;
    }

    XMFLOAT3 CameraAcceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
    if (InputManager::Get().IsKeyDown(EKey::Key_W))
    {
        CameraAcceleration.z = Acceleration;
    }
    else if (InputManager::Get().IsKeyDown(EKey::Key_S))
    {
        CameraAcceleration.z = -Acceleration;
    }

    if (InputManager::Get().IsKeyDown(EKey::Key_A))
    {
        CameraAcceleration.x = Acceleration;
    }
    else if (InputManager::Get().IsKeyDown(EKey::Key_D))
    {
        CameraAcceleration.x = -Acceleration;
    }

    if (InputManager::Get().IsKeyDown(EKey::Key_Q))
    {
        CameraAcceleration.y = Acceleration;
    }
    else if (InputManager::Get().IsKeyDown(EKey::Key_E))
    {
        CameraAcceleration.y = -Acceleration;
    }

    const float Deacceleration = -5.0f;
    CameraSpeed = CameraSpeed + (CameraSpeed * Deacceleration) * Delta;
    CameraSpeed = CameraSpeed + (CameraAcceleration * Delta);

    XMFLOAT3 Speed = CameraSpeed * Delta;
    CurrentCamera->Move(Speed.x, Speed.y, Speed.z);
    CurrentCamera->UpdateMatrices();
}

#include "Sandbox.h"

#include "Core/Math/Math.h"

#include "Renderer/Renderer.h"

#include "Engine/Engine.h"
#include "Engine/Assets/Loaders/OBJLoader.h"
#include "Engine/Assets/Loaders/FBXLoader.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Lights/PointLight.h"
#include "Engine/Scene/Lights/DirectionalLight.h"
#include "Engine/Scene/Components/MeshComponent.h"
#include "Engine/Resources/TextureFactory.h"

#include "Core/Logging/Log.h"

#include "Application/ApplicationInstance.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

// TODO: Custom random
#include <random>

#define ENABLE_LIGHT_TEST 0

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

IMPLEMENT_ENGINE_MODULE(CSandbox, Sandbox);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

bool CSandbox::Init()
{
    if (!CApplicationModule::Init())
    {
        return false;
    }

    // Initialize Scene
    CActor*         NewActor     = nullptr;
    CMeshComponent* NewComponent = nullptr;

    TSharedPtr<CScene> CurrentScene = GEngine->Scene;

    // Load Scene
    SSceneData SceneData;
#if 0
    COBJLoader::LoadFile((ENGINE_LOCATION"/Assets/Scenes/Sponza/Sponza.obj"), SceneData);
    SceneData.Scale = 0.015f;
#else
    CFBXLoader::LoadFile((ENGINE_LOCATION"/Assets/Scenes/Bistro/BistroInterior.fbx"), SceneData);
    SceneData.AddToScene(CurrentScene.Get());

    CFBXLoader::LoadFile((ENGINE_LOCATION"/Assets/Scenes/Bistro/BistroExterior.fbx"), SceneData);
#endif
    SceneData.AddToScene(CurrentScene.Get());

    // Create Spheres
    SMeshData SphereMeshData = CMeshFactory::CreateSphere(3);

    TSharedPtr<CMesh> SphereMesh = CMesh::Make(SphereMeshData);

    constexpr float  SphereOffset   = 1.25f;
    constexpr uint32 SphereCountX   = 8;
    constexpr float  StartPositionX = (-static_cast<float>(SphereCountX) * SphereOffset) / 2.0f;
    constexpr uint32 SphereCountY   = 8;
    constexpr float  StartPositionY = (-static_cast<float>(SphereCountY) * SphereOffset) / 2.0f;
    constexpr float  MetallicDelta  = 1.0f / SphereCountY;
    constexpr float  RoughnessDelta = 1.0f / SphereCountX;

    SMaterialDesc MatProperties;
    MatProperties.AO = 1.0f;

    uint32 SphereIndex = 0;
    for (uint32 y = 0; y < SphereCountY; y++)
    {
        for (uint32 x = 0; x < SphereCountX; x++)
        {
            NewActor = CurrentScene->MakeActor();
            NewActor->GetTransform().SetTranslation(StartPositionX + (x * SphereOffset), 0.6f, 40.0f + StartPositionY + (y * SphereOffset));

            NewActor->SetName("Sphere[" + ToString(SphereIndex) + "]");
            SphereIndex++;

            NewComponent = dbg_new CMeshComponent(NewActor);
            NewComponent->Mesh = SphereMesh;
            NewComponent->Material = MakeShared<CMaterial>(MatProperties);

            NewComponent->Material->AlbedoMap    = GEngine->BaseTexture;
            NewComponent->Material->NormalMap    = GEngine->BaseNormal;
            NewComponent->Material->RoughnessMap = GEngine->BaseTexture;
            NewComponent->Material->AOMap        = GEngine->BaseTexture;
            NewComponent->Material->MetallicMap  = GEngine->BaseTexture;
            NewComponent->Material->Init();

            NewActor->AddComponent(NewComponent);

            MatProperties.Roughness += RoughnessDelta;
        }

        MatProperties.Roughness = 0.05f;
        MatProperties.Metallic += MetallicDelta;
    }

    // Create Other Meshes
    SMeshData CubeMeshData = CMeshFactory::CreateCube();

    NewActor = CurrentScene->MakeActor();

    NewActor->SetName("Cube");
    NewActor->GetTransform().SetTranslation(0.0f, 2.0f, 50.0f);

    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 1.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 1;

    NewComponent = dbg_new CMeshComponent(NewActor);
    NewComponent->Mesh     = CMesh::Make(CubeMeshData);
    NewComponent->Material = MakeShared<CMaterial>(MatProperties);

    TSharedRef<CRHITexture2D> AlbedoMap = CTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/Gate_Albedo.png"), TextureFactoryFlag_GenerateMips, ERHIFormat::R8G8B8A8_Unorm);
    if (AlbedoMap)
    {
        AlbedoMap->SetName("AlbedoMap");
    }

    TSharedRef<CRHITexture2D> NormalMap = CTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/Gate_Normal.png"), TextureFactoryFlag_GenerateMips, ERHIFormat::R8G8B8A8_Unorm);
    if (NormalMap)
    {
        NormalMap->SetName("NormalMap");
    }

    TSharedRef<CRHITexture2D> AOMap = CTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/Gate_AO.png"), TextureFactoryFlag_GenerateMips, ERHIFormat::R8_Unorm);
    if (AOMap)
    {
        AOMap->SetName("AOMap");
    }

    TSharedRef<CRHITexture2D> RoughnessMap = CTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/Gate_Roughness.png"), TextureFactoryFlag_GenerateMips, ERHIFormat::R8_Unorm);
    if (RoughnessMap)
    {
        RoughnessMap->SetName("RoughnessMap");
    }

    TSharedRef<CRHITexture2D> HeightMap = CTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/Gate_Height.png"), TextureFactoryFlag_GenerateMips, ERHIFormat::R8_Unorm);
    if (HeightMap)
    {
        HeightMap->SetName("HeightMap");
    }

    TSharedRef<CRHITexture2D> MetallicMap = CTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/Gate_Metallic.png"), TextureFactoryFlag_GenerateMips, ERHIFormat::R8_Unorm);
    if (MetallicMap)
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

    NewActor = CurrentScene->MakeActor();

    NewActor->SetName("Plane");
    NewActor->GetTransform().SetRotation(NMath::HALF_PI_F, 0.0f, 0.0f);
    NewActor->GetTransform().SetUniformScale(50.0f);
    NewActor->GetTransform().SetTranslation(0.0f, 0.0f, 42.0f);

    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 0.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 0;
    MatProperties.Albedo       = CVector3(1.0f);

    NewComponent = dbg_new CMeshComponent(NewActor);
    NewComponent->Mesh                   = CMesh::Make(CMeshFactory::CreatePlane(10, 10));
    NewComponent->Material               = MakeShared<CMaterial>(MatProperties);
    NewComponent->Material->AlbedoMap    = GEngine->BaseTexture;
    NewComponent->Material->NormalMap    = GEngine->BaseNormal;
    NewComponent->Material->RoughnessMap = GEngine->BaseTexture;
    NewComponent->Material->AOMap        = GEngine->BaseTexture;
    NewComponent->Material->MetallicMap  = GEngine->BaseTexture;
    NewComponent->Material->Init();
    NewActor->AddComponent(NewComponent);

    AlbedoMap = CTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/StreetLight/BaseColor.jpg"), TextureFactoryFlag_GenerateMips, ERHIFormat::R8G8B8A8_Unorm);
    if (AlbedoMap)
    {
        AlbedoMap->SetName("AlbedoMap");
    }

    NormalMap = CTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/StreetLight/Normal.jpg"), TextureFactoryFlag_GenerateMips, ERHIFormat::R8G8B8A8_Unorm);
    if (NormalMap)
    {
        NormalMap->SetName("NormalMap");
    }

    RoughnessMap = CTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/StreetLight/Roughness.jpg"), TextureFactoryFlag_GenerateMips, ERHIFormat::R8_Unorm);
    if (RoughnessMap)
    {
        RoughnessMap->SetName("RoughnessMap");
    }

    MetallicMap = CTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/StreetLight/Metallic.jpg"), TextureFactoryFlag_GenerateMips, ERHIFormat::R8_Unorm);
    if (MetallicMap)
    {
        MetallicMap->SetName("MetallicMap");
    }

    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 1.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 0;
    MatProperties.Albedo       = CVector3(1.0f, 1.0f, 1.0f);

    SSceneData StreetLightData;
    COBJLoader::LoadFile((ENGINE_LOCATION"/Assets/Models/Street_Light.obj"), StreetLightData);

    TSharedPtr<CMesh>     StreetLight = StreetLightData.HasModelData() ? CMesh::Make(StreetLightData.Models.FirstElement().Mesh) : nullptr;
    TSharedPtr<CMaterial> StreetLightMat = MakeShared<CMaterial>(MatProperties);

    for (uint32 i = 0; i < 4; i++)
    {
        NewActor = CurrentScene->MakeActor();

        NewActor->SetName("Street Light " + ToString(i));
        NewActor->GetTransform().SetUniformScale(0.25f);
        NewActor->GetTransform().SetTranslation(15.0f, 0.0f, 55.0f - ((float)i * 3.0f));

        NewComponent = dbg_new CMeshComponent(NewActor);
        NewComponent->Mesh                   = StreetLight;
        NewComponent->Material               = StreetLightMat;
        NewComponent->Material->AlbedoMap    = AlbedoMap;
        NewComponent->Material->NormalMap    = NormalMap;
        NewComponent->Material->RoughnessMap = RoughnessMap;
        NewComponent->Material->AOMap        = GEngine->BaseTexture;
        NewComponent->Material->MetallicMap  = MetallicMap;
        NewComponent->Material->Init();
        NewActor->AddComponent(NewComponent);
    }

    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 0.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 0;
    MatProperties.Albedo       = CVector3(0.4f);

    SSceneData PillarData;
    COBJLoader::LoadFile((ENGINE_LOCATION"/Assets/Models/Pillar.obj"), PillarData);

    TSharedPtr<CMesh>     Pillar = PillarData.HasModelData() ? CMesh::Make(PillarData.Models.FirstElement().Mesh) : nullptr;
    TSharedPtr<CMaterial> PillarMat = MakeShared<CMaterial>(MatProperties);

    for (uint32 i = 0; i < 8; i++)
    {
        NewActor = CurrentScene->MakeActor();

        NewActor->SetName("Pillar " + ToString(i));
        NewActor->GetTransform().SetUniformScale(0.25f);
        NewActor->GetTransform().SetTranslation(-15.0f + ((float)i * 1.75f), 0.0f, 60.0f);

        NewComponent = dbg_new CMeshComponent(NewActor);
        NewComponent->Mesh                   = Pillar;
        NewComponent->Material               = PillarMat;
        NewComponent->Material->AlbedoMap    = GEngine->BaseTexture;
        NewComponent->Material->NormalMap    = GEngine->BaseNormal;
        NewComponent->Material->RoughnessMap = GEngine->BaseTexture;
        NewComponent->Material->AOMap        = GEngine->BaseTexture;
        NewComponent->Material->MetallicMap  = MetallicMap;
        NewComponent->Material->Init();
        NewActor->AddComponent(NewComponent);
    }

    CurrentCamera = dbg_new CCamera();
    CurrentScene->AddCamera(CurrentCamera);

    // Add PointLight- Source
    const float Intensity = 50.0f;

    CPointLight* Light0 = dbg_new CPointLight();
    Light0->SetPosition(16.5f, 1.0f, 0.0f);
    Light0->SetColor(1.0f, 1.0f, 1.0f);
    Light0->SetShadowBias(0.001f);
    Light0->SetMaxShadowBias(0.009f);
    Light0->SetShadowFarPlane(50.0f);
    Light0->SetIntensity(Intensity);
    Light0->SetShadowCaster(true);
    CurrentScene->AddLight(Light0);

    CPointLight* Light1 = dbg_new CPointLight();
    Light1->SetPosition(-17.5f, 1.0f, 0.0f);
    Light1->SetColor(1.0f, 1.0f, 1.0f);
    Light1->SetShadowBias(0.001f);
    Light1->SetMaxShadowBias(0.009f);
    Light1->SetShadowFarPlane(50.0f);
    Light1->SetIntensity(Intensity);
    Light1->SetShadowCaster(true);
    CurrentScene->AddLight(Light1);

    CPointLight* Light2 = dbg_new CPointLight();
    Light2->SetPosition(16.5f, 11.0f, 0.0f);
    Light2->SetColor(1.0f, 1.0f, 1.0f);
    Light2->SetShadowBias(0.001f);
    Light2->SetMaxShadowBias(0.009f);
    Light2->SetShadowFarPlane(50.0f);
    Light2->SetIntensity(Intensity);
    Light2->SetShadowCaster(true);
    CurrentScene->AddLight(Light2);

    CPointLight* Light3 = dbg_new CPointLight();
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
    std::uniform_real_distribution<float> RandomFloats(0.0f, 1.0f);
    std::default_random_engine Generator;

    for (uint32 i = 0; i < 256; i++)
    {
        float x = RandomFloats(Generator) * 35.0f - 17.5f;
        float y = RandomFloats(Generator) * 22.0f;
        float z = RandomFloats(Generator) * 16.0f - 8.0f;
        float Intentsity = RandomFloats(Generator) * 5.0f + 1.0f;

        CPointLight* Light = dbg_new CPointLight();
        Light->SetPosition(x, y, z);
        Light->SetColor(RandomFloats(Generator), RandomFloats(Generator), RandomFloats(Generator));
        Light->SetIntensity(Intentsity);
        CScene->AddLight(Light);
    }
#endif

    // Add DirectionalLight- Source
    CDirectionalLight* Light4 = dbg_new CDirectionalLight();
    Light4->SetShadowBias(0.0002f);
    Light4->SetMaxShadowBias(0.0015f);
    Light4->SetColor(1.0f, 1.0f, 1.0f);
    Light4->SetIntensity(10.0f);
    Light4->SetRotation(NMath::ToRadians(35.0f), NMath::ToRadians(135.0f), 0.0f);
    //Light4->SetRotation(0.0f, 0.0f, 0.0f);
    CurrentScene->AddLight(Light4);

    return true;
}

void CSandbox::Tick(CTimestamp DeltaTime)
{
    CApplicationModule::Tick(DeltaTime);

    //LOG_INFO("Tick: " + ToString(DeltaTime.AsMilliSeconds()));

    const float Delta = static_cast<float>(DeltaTime.AsSeconds());
    const float RotationSpeed = 45.0f;

    TSharedPtr<CApplicationUser> User = CApplicationInstance::Get().GetFirstUser();
    if (User->IsKeyDown(EKey::Key_Right))
    {
        CurrentCamera->Rotate(0.0f, NMath::ToRadians(RotationSpeed * Delta), 0.0f);
    }
    else if (User->IsKeyDown(EKey::Key_Left))
    {
        CurrentCamera->Rotate(0.0f, NMath::ToRadians(-RotationSpeed * Delta), 0.0f);
    }

    if (User->IsKeyDown(EKey::Key_Up))
    {
        CurrentCamera->Rotate(NMath::ToRadians(-RotationSpeed * Delta), 0.0f, 0.0f);
    }
    else if (User->IsKeyDown(EKey::Key_Down))
    {
        CurrentCamera->Rotate(NMath::ToRadians(RotationSpeed * Delta), 0.0f, 0.0f);
    }

    float Acceleration = 15.0f;
    if (User->IsKeyDown(EKey::Key_LeftShift))
    {
        Acceleration = Acceleration * 3;
    }

    CVector3 CameraAcceleration;
    if (User->IsKeyDown(EKey::Key_W))
    {
        CameraAcceleration.z = Acceleration;
    }
    else if (User->IsKeyDown(EKey::Key_S))
    {
        CameraAcceleration.z = -Acceleration;
    }

    if (User->IsKeyDown(EKey::Key_A))
    {
        CameraAcceleration.x = Acceleration;
    }
    else if (User->IsKeyDown(EKey::Key_D))
    {
        CameraAcceleration.x = -Acceleration;
    }

    if (User->IsKeyDown(EKey::Key_Q))
    {
        CameraAcceleration.y = Acceleration;
    }
    else if (User->IsKeyDown(EKey::Key_E))
    {
        CameraAcceleration.y = -Acceleration;
    }

    const float Deacceleration = -5.0f;
    CameraSpeed = CameraSpeed + (CameraSpeed * Deacceleration) * Delta;
    CameraSpeed = CameraSpeed + (CameraAcceleration * Delta);

    CVector3 Speed = CameraSpeed * Delta;
    CurrentCamera->Move(Speed.x, Speed.y, Speed.z);
    CurrentCamera->UpdateMatrices();
}

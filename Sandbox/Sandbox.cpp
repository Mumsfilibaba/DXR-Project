#include "Sandbox.h"
#include "GameComponents.h"

#include <Core/Math/Math.h>
#include <Core/Misc/OutputDeviceLogger.h>

#include <Renderer/Renderer.h>

#include <Engine/Engine.h>
#include <Engine/Assets/Loaders/OBJLoader.h>
#include <Engine/Assets/Loaders/FBXLoader.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Lights/PointLight.h>
#include <Engine/Scene/Lights/DirectionalLight.h>
#include <Engine/Scene/Components/MeshComponent.h>
#include <Engine/Resources/TextureFactory.h>

#include <Application/ApplicationInterface.h>

#include <InterfaceRenderer/InterfaceRenderer.h>

// TODO: Custom random
#include <random>

#define LOAD_SPONZA         (1)
#define ENABLE_LIGHT_TEST   (0)
#define ENABLE_MANY_SPHERES (0)

IMPLEMENT_ENGINE_MODULE(FSandbox, Sandbox);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FSandbox

bool FSandbox::Init()
{
    if (!FApplicationModule::Init())
    {
        return false;
    }

    // Initialize Scene
    FActor*         NewActor     = nullptr;
    FMeshComponent* NewComponent = nullptr;

    TSharedPtr<FScene> CurrentScene = GEngine->Scene;

    // Load Scene
    FSceneData SceneData;
#if LOAD_SPONZA
    FOBJLoader::LoadFile((ENGINE_LOCATION"/Assets/Scenes/Sponza/Sponza.obj"), SceneData);
    SceneData.Scale = 0.015f;
#else
    FFBXLoader::LoadFile((ENGINE_LOCATION"/Assets/Scenes/Bistro/BistroInterior.fbx"), SceneData);
    SceneData.AddToScene(CurrentScene.Get());

    FFBXLoader::LoadFile((ENGINE_LOCATION"/Assets/Scenes/Bistro/BistroExterior.fbx"), SceneData);
#endif
    SceneData.AddToScene(CurrentScene.Get());

    // Create Spheres
    FMeshData SphereMeshData = FMeshFactory::CreateSphere(3);

    TSharedPtr<FMesh> SphereMesh = FMesh::Make(SphereMeshData);

    constexpr float  SphereOffset   = 1.25f;
    constexpr uint32 SphereCountX   = 8;
    constexpr float  StartPositionX = (-static_cast<float>(SphereCountX) * SphereOffset) / 2.0f;
    constexpr uint32 SphereCountY   = 8;
    constexpr float  StartPositionY = (-static_cast<float>(SphereCountY) * SphereOffset) / 2.0f;
    constexpr float  MetallicDelta  = 1.0f / SphereCountY;
    constexpr float  RoughnessDelta = 1.0f / SphereCountX;

    FMaterialDesc MatProperties;
    MatProperties.AO = 1.0f;

    uint32 SphereIndex = 0;
    for (uint32 y = 0; y < SphereCountY; y++)
    {
        for (uint32 x = 0; x < SphereCountX; x++)
        {
            NewActor = CurrentScene->CreateActor();
            NewActor->GetTransform().SetTranslation(StartPositionX + (x * SphereOffset), 0.6f, 40.0f + StartPositionY + (y * SphereOffset));

            NewActor->SetName("Sphere[" + ToString(SphereIndex) + "]");
            SphereIndex++;

            NewComponent = dbg_new FMeshComponent(NewActor);
            NewComponent->Mesh = SphereMesh;
            NewComponent->Material = MakeShared<FMaterial>(MatProperties);

            NewComponent->Material->AlbedoMap    = GEngine->BaseTexture;
            NewComponent->Material->NormalMap    = GEngine->BaseNormal;
            NewComponent->Material->RoughnessMap = GEngine->BaseTexture;
            NewComponent->Material->AOMap        = GEngine->BaseTexture;
            NewComponent->Material->MetallicMap  = GEngine->BaseTexture;
            NewComponent->Material->Initialize();

            NewActor->AddComponent(NewComponent);

            MatProperties.Roughness += RoughnessDelta;
        }

        MatProperties.Roughness = 0.05f;
        MatProperties.Metallic += MetallicDelta;
    }

#if ENABLE_MANY_SPHERES
    {
        constexpr uint32 kNumSpheres = 4096 * 8;
        constexpr float  kMaxRadius  = 32.0f;

        std::default_random_engine            Generator;

        std::uniform_real_distribution<float> Random0(0.0f, NMath::kTwoPI_f);
        std::uniform_real_distribution<float> Random1(0.05f, 1.0);
        std::uniform_real_distribution<float> Random2(0.05f, 0.7);
        std::uniform_real_distribution<float> Random3(0.5f, 1.0);
        std::uniform_real_distribution<float> Random4(1.0f, 10.0f);

        for (uint32 i = 0; i < kNumSpheres; ++i)
        {
            const float Radius = Random1(Generator) * kMaxRadius;
            const float Alpha  = Random0(Generator);
            const float Theta  = Random0(Generator);

            const float CosAlpha = NMath::Cos(Alpha);
            const float SinAlpha = NMath::Sin(Alpha);
            const float CosTheta = NMath::Cos(Theta);
            const float SinTheta = NMath::Sin(Theta);

            const float PositionX = Radius * CosAlpha * SinTheta;
            const float PositionY = Radius * SinAlpha * SinTheta;
            const float PositionZ = Radius * CosTheta;

            const float Offset = -60.0f;

            NewActor = CurrentScene->CreateActor();
            NewActor->GetTransform().SetTranslation(PositionX, PositionY + 10.0f, Offset + PositionZ);

            // MovingBallComponent
            NewActor->AddComponent(dbg_new FMovingBallComponent(NewActor, Random4(Generator)));

            NewActor->SetName("Random Sphere[" + ToString(i) + "]");

            // MeshComponent
            NewComponent           = dbg_new FMeshComponent(NewActor);
            NewComponent->Mesh     = SphereMesh;
            NewComponent->Material = MakeShared<FMaterial>(MatProperties);

            NewComponent->Material->AlbedoMap    = GEngine->BaseTexture;
            NewComponent->Material->NormalMap    = GEngine->BaseNormal;
            NewComponent->Material->RoughnessMap = GEngine->BaseTexture;
            NewComponent->Material->AOMap        = GEngine->BaseTexture;
            NewComponent->Material->MetallicMap  = GEngine->BaseTexture;
            NewComponent->Material->Initialize();

            NewActor->AddComponent(NewComponent);

            MatProperties.Roughness = Random2(Generator);
            MatProperties.Metallic  = Random3(Generator);
        }
    }
#endif

    // Create Other Meshes
    FMeshData CubeMeshData = FMeshFactory::CreateCube();

    NewActor = CurrentScene->CreateActor();

    NewActor->SetName("Cube");
    NewActor->GetTransform().SetTranslation(0.0f, 2.0f, 50.0f);

    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 1.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 1;

    NewComponent           = dbg_new FMeshComponent(NewActor);
    NewComponent->Mesh     = FMesh::Make(CubeMeshData);
    NewComponent->Material = MakeShared<FMaterial>(MatProperties);

    FRHITexture2DRef AlbedoMap = FTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/Gate_Albedo.png"), TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (AlbedoMap)
    {
        AlbedoMap->SetName("AlbedoMap");
    }

    FRHITexture2DRef NormalMap = FTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/Gate_Normal.png"), TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (NormalMap)
    {
        NormalMap->SetName("NormalMap");
    }

    FRHITexture2DRef AOMap = FTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/Gate_AO.png"), TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
    if (AOMap)
    {
        AOMap->SetName("AOMap");
    }

    FRHITexture2DRef RoughnessMap = FTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/Gate_Roughness.png"), TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
    if (RoughnessMap)
    {
        RoughnessMap->SetName("RoughnessMap");
    }

    FRHITexture2DRef HeightMap = FTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/Gate_Height.png"), TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
    if (HeightMap)
    {
        HeightMap->SetName("HeightMap");
    }

    FRHITexture2DRef MetallicMap = FTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/Gate_Metallic.png"), TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
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
    NewComponent->Material->Initialize();
    NewActor->AddComponent(NewComponent);

    NewActor = CurrentScene->CreateActor();

    NewActor->SetName("Plane");
    NewActor->GetTransform().SetRotation(NMath::kHalfPI_f, 0.0f, 0.0f);
    NewActor->GetTransform().SetUniformScale(50.0f);
    NewActor->GetTransform().SetTranslation(0.0f, 0.0f, 42.0f);

    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 1.0f;
    MatProperties.Roughness    = 0.5f;
    MatProperties.EnableHeight = 0;
    MatProperties.Albedo       = FVector3(1.0f);

    NewComponent                         = dbg_new FMeshComponent(NewActor);
    NewComponent->Mesh                   = FMesh::Make(FMeshFactory::CreatePlane(10, 10));
    NewComponent->Material               = MakeShared<FMaterial>(MatProperties);
    NewComponent->Material->AlbedoMap    = GEngine->BaseTexture;
    NewComponent->Material->NormalMap    = GEngine->BaseNormal;
    NewComponent->Material->RoughnessMap = GEngine->BaseTexture;
    NewComponent->Material->AOMap        = GEngine->BaseTexture;
    NewComponent->Material->MetallicMap  = GEngine->BaseTexture;
    NewComponent->Material->Initialize();

    NewActor->AddComponent(NewComponent);

    AlbedoMap = FTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/StreetLight/BaseColor.jpg"), TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (AlbedoMap)
    {
        AlbedoMap->SetName("AlbedoMap");
    }

    NormalMap = FTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/StreetLight/Normal.jpg"), TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
    if (NormalMap)
    {
        NormalMap->SetName("NormalMap");
    }

    RoughnessMap = FTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/StreetLight/Roughness.jpg"), TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
    if (RoughnessMap)
    {
        RoughnessMap->SetName("RoughnessMap");
    }

    MetallicMap = FTextureFactory::LoadFromFile((ENGINE_LOCATION"/Assets/Textures/StreetLight/Metallic.jpg"), TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
    if (MetallicMap)
    {
        MetallicMap->SetName("MetallicMap");
    }

    MatProperties.Albedo       = FVector3(1.0f);
    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 1.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 0;
    MatProperties.EnableMask   = 0;

    FSceneData StreetLightData;
    FOBJLoader::LoadFile((ENGINE_LOCATION"/Assets/Models/Street_Light.obj"), StreetLightData);

    TSharedPtr<FMesh>     StreetLight = StreetLightData.HasModelData() ? FMesh::Make(StreetLightData.Models.FirstElement().Mesh) : nullptr;
    TSharedPtr<FMaterial> StreetLightMat = MakeShared<FMaterial>(MatProperties);

    for (uint32 i = 0; i < 4; i++)
    {
        NewActor = CurrentScene->CreateActor();

        NewActor->SetName("Street Light " + ToString(i));
        NewActor->GetTransform().SetUniformScale(0.25f);
        NewActor->GetTransform().SetTranslation(15.0f, 0.0f, 55.0f - ((float)i * 3.0f));

        NewComponent                         = dbg_new FMeshComponent(NewActor);
        NewComponent->Mesh                   = StreetLight;
        NewComponent->Material               = StreetLightMat;
        NewComponent->Material->AlbedoMap    = AlbedoMap;
        NewComponent->Material->NormalMap    = NormalMap;
        NewComponent->Material->RoughnessMap = RoughnessMap;
        NewComponent->Material->AOMap        = GEngine->BaseTexture;
        NewComponent->Material->MetallicMap  = MetallicMap;
        NewComponent->Material->Initialize();
        NewActor->AddComponent(NewComponent);
    }

    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 0.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 0;
    MatProperties.Albedo       = FVector3(0.4f);

    FSceneData PillarData;
    FOBJLoader::LoadFile((ENGINE_LOCATION"/Assets/Models/Pillar.obj"), PillarData);

    TSharedPtr<FMesh>     Pillar         = PillarData.HasModelData() ? FMesh::Make(PillarData.Models.FirstElement().Mesh) : nullptr;
    TSharedPtr<FMaterial> PillarMaterial = MakeShared<FMaterial>(MatProperties);

    for (uint32 i = 0; i < 8; i++)
    {
        NewActor = CurrentScene->CreateActor();

        NewActor->SetName("Pillar " + ToString(i));
        NewActor->GetTransform().SetUniformScale(0.25f);
        NewActor->GetTransform().SetTranslation(-15.0f + ((float)i * 1.75f), 0.0f, 60.0f);

        NewComponent                         = dbg_new FMeshComponent(NewActor);
        NewComponent->Mesh                   = Pillar;
        NewComponent->Material               = PillarMaterial;
        NewComponent->Material->AlbedoMap    = GEngine->BaseTexture;
        NewComponent->Material->NormalMap    = GEngine->BaseNormal;
        NewComponent->Material->RoughnessMap = GEngine->BaseTexture;
        NewComponent->Material->AOMap        = GEngine->BaseTexture;
        NewComponent->Material->MetallicMap  = MetallicMap;
        NewComponent->Material->Initialize();

        NewActor->AddComponent(NewComponent);
    }

    CurrentCamera = dbg_new FCamera();
    CurrentScene->AddCamera(CurrentCamera);

    // Add PointLight- Source
    const float Intensity = 50.0f;

    FPointLight* Light0 = dbg_new FPointLight();
    Light0->SetPosition(FVector3(16.5f, 1.0f, 0.0f));
    Light0->SetColor(FVector3(1.0f, 1.0f, 1.0f));
    Light0->SetShadowBias(0.001f);
    Light0->SetMaxShadowBias(0.009f);
    Light0->SetShadowFarPlane(50.0f);
    Light0->SetIntensity(Intensity);
    Light0->SetShadowCaster(true);
    CurrentScene->AddLight(Light0);

    FPointLight* Light1 = dbg_new FPointLight();
    Light1->SetPosition(FVector3(-17.5f, 1.0f, 0.0f));
    Light1->SetColor(FVector3(1.0f, 1.0f, 1.0f));
    Light1->SetShadowBias(0.001f);
    Light1->SetMaxShadowBias(0.009f);
    Light1->SetShadowFarPlane(50.0f);
    Light1->SetIntensity(Intensity);
    Light1->SetShadowCaster(true);
    CurrentScene->AddLight(Light1);

    FPointLight* Light2 = dbg_new FPointLight();
    Light2->SetPosition(FVector3(16.5f, 11.0f, 0.0f));
    Light2->SetColor(FVector3(1.0f, 1.0f, 1.0f));
    Light2->SetShadowBias(0.001f);
    Light2->SetMaxShadowBias(0.009f);
    Light2->SetShadowFarPlane(50.0f);
    Light2->SetIntensity(Intensity);
    Light2->SetShadowCaster(true);
    CurrentScene->AddLight(Light2);

    FPointLight* Light3 = dbg_new FPointLight();
    Light3->SetPosition(FVector3(-17.5f, 11.0f, 0.0f));
    Light3->SetColor(FVector3(1.0f, 1.0f, 1.0f));
    Light3->SetShadowBias(0.001f);
    Light3->SetMaxShadowBias(0.009f);
    Light3->SetShadowFarPlane(50.0f);
    Light3->SetIntensity(Intensity);
    Light3->SetShadowCaster(true);
    CurrentScene->AddLight(Light3);

#if ENABLE_LIGHT_TEST
    {
        // Add multiple lights
        std::uniform_real_distribution<float> RandomFloats(0.0f, 1.0f);
        std::default_random_engine            Generator;

        for (uint32 i = 0; i < 256; i++)
        {
            float x = RandomFloats(Generator) * 35.0f - 17.5f;
            float y = RandomFloats(Generator) * 22.0f;
            float z = RandomFloats(Generator) * 16.0f - 8.0f;
            float Intentsity = RandomFloats(Generator) * 5.0f + 1.0f;

            FPointLight* Light = dbg_new FPointLight();
            Light->SetPosition(x, y, z);
            Light->SetColor(RandomFloats(Generator), RandomFloats(Generator), RandomFloats(Generator));
            Light->SetIntensity(Intentsity);
            FScene->AddLight(Light);
        }
    }
#endif

    // Add DirectionalLight- Source
    FDirectionalLight* Light4 = dbg_new FDirectionalLight();
    Light4->SetShadowBias(0.0005f);
    Light4->SetMaxShadowBias(0.0025f);
    Light4->SetColor(FVector3(1.0f, 1.0f, 1.0f));
    Light4->SetIntensity(10.0f);
    Light4->SetRotation(FVector3(NMath::ToRadians(35.0f), NMath::ToRadians(135.0f), 0.0f));
    Light4->SetCascadeSplitLambda(0.9f);
    CurrentScene->AddLight(Light4);

    FLightProbe* LightProbe = dbg_new FLightProbe();
    LightProbe->SetPosition(FVector3(0.0f));
    CurrentScene->AddLightProbe(LightProbe);

    LOG_INFO("Finished loading game");
    return true;
}

void FSandbox::Tick(FTimespan DeltaTime)
{
    FApplicationModule::Tick(DeltaTime);

    // LOG_INFO("Tick: %f", DeltaTime.AsMilliseconds());

    const float Delta         = static_cast<float>(DeltaTime.AsSeconds());
    const float RotationSpeed = 45.0f;

    TSharedPtr<FUser> User = FApplicationInterface::Get().GetFirstUser();
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

    FVector3 CameraAcceleration;
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

    FVector3 Speed = CameraSpeed * Delta;
    CurrentCamera->Move(Speed.x, Speed.y, Speed.z);
    CurrentCamera->UpdateMatrices();
}

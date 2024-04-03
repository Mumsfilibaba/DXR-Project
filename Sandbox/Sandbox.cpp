#include "Sandbox.h"
#include "SandboxPlayer.h"
#include "GameComponents.h"
#include <Core/Math/Math.h>
#include <Core/Misc/OutputDeviceLogger.h>
#include <Engine/Engine.h>
#include <Engine/Assets/AssetManager.h>
#include <Engine/Assets/AssetLoaders/MeshImporter.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Lights/PointLight.h>
#include <Engine/Scene/Lights/DirectionalLight.h>
#include <Engine/Scene/Actors/PlayerController.h>
#include <Engine/Scene/Components/MeshComponent.h>
#include <Application/Application.h>

// TODO: Custom random
#include <random>

#define LOAD_SPONZA         (1)
#define LOAD_BISTRO         (0)
#define LOAD_SUN_TEMPLE     (0)
#define LOAD_EMERALD_SQUARE (0)

#define ENABLE_LIGHT_TEST   (0)
#define ENABLE_MANY_SPHERES (0)

IMPLEMENT_ENGINE_MODULE(FSandbox, Sandbox);

bool FSandbox::Init()
{
    if (!FGameModule::Init())
    {
        return false;
    }

    // Initialize Scene
    MAYBE_UNUSED FActor*         NewActor     = nullptr;
    MAYBE_UNUSED FMeshComponent* NewComponent = nullptr;

    // Store the Engine's scene pointer 
    FScene* CurrentScene = GEngine->Scene;

    // Load Scene
    {
        FSceneData SceneData;

    #if LOAD_SPONZA
        FMeshImporter::Get().LoadMesh((ENGINE_LOCATION"/Assets/Scenes/Sponza/Sponza.obj"), SceneData);
        SceneData.Scale = 0.015f;
    #elif LOAD_BISTRO
        FMeshImporter::Get().LoadMesh((ENGINE_LOCATION"/Assets/Scenes/Bistro/BistroInterior.fbx"), SceneData);
        for (auto& Material : SceneData.Materials)
        {
            if (Material.Name.Contains("DoubleSided"))
            {
                Material.bAlphaDiffuseCombined = true;
                Material.bIsDoubleSided        = true;
            }
        }
        
        SceneData.AddToScene(CurrentScene);

        FMeshImporter::Get().LoadMesh((ENGINE_LOCATION"/Assets/Scenes/Bistro/BistroExterior.fbx"), SceneData);
        for (auto& Material : SceneData.Materials)
        {
            if (Material.Name.Contains("DoubleSided"))
            {
                Material.bAlphaDiffuseCombined = true;
                Material.bIsDoubleSided        = true;
            }
        }
     #elif LOAD_SUN_TEMPLE
        FMeshImporter::Get().LoadMesh((ENGINE_LOCATION"/Assets/Scenes/SunTemple/SunTemple.fbx"), SceneData);
        for (auto& Material : SceneData.Materials)
        {
            if (Material.Name.Contains("DoubleSided"))
            {
                Material.bAlphaDiffuseCombined = true;
                Material.bIsDoubleSided        = true;
            }
        }
     #elif LOAD_EMERALD_SQUARE
        FMeshImporter::Get().LoadMesh((ENGINE_LOCATION"/Assets/Scenes/EmeraldSquare/EmeraldSquare_Day.fbx"), SceneData);
        for (auto& Material : SceneData.Materials)
        {
            if (Material.Name.Contains("DoubleSided"))
            {
                Material.bAlphaDiffuseCombined = true;
                Material.bIsDoubleSided        = true;
            }
        }
    #endif

        SceneData.AddToScene(CurrentScene);
    }

#if LOAD_SPONZA
    // Create Spheres
    FMeshData SphereMeshData = FMeshFactory::CreateSphere(3);

    TSharedPtr<FMesh> SphereMesh = FMesh::Create(SphereMeshData);

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
            if (NewActor)
            {
                NewActor->GetTransform().SetTranslation(StartPositionX + (x * SphereOffset), 0.6f, 40.0f + StartPositionY + (y * SphereOffset));
                NewActor->SetName("Sphere[" + TTypeToString<uint32>::ToString(SphereIndex) + "]");
                SphereIndex++;

                NewComponent = NewObject<FMeshComponent>();
                if (NewComponent)
                {
                    TSharedPtr<FMaterial> NewMaterial = MakeShared<FMaterial>(MatProperties);
                    NewMaterial->AlbedoMap    = GEngine->BaseTexture;
                    NewMaterial->NormalMap    = GEngine->BaseNormal;
                    NewMaterial->RoughnessMap = GEngine->BaseTexture;
                    NewMaterial->AOMap        = GEngine->BaseTexture;
                    NewMaterial->MetallicMap  = GEngine->BaseTexture;
                    NewMaterial->Initialize();

                    NewComponent->SetMesh(SphereMesh);
                    NewComponent->SetMaterial(NewMaterial);
                    
                    NewActor->AddComponent(NewComponent);
                }
            }

            MatProperties.Roughness += RoughnessDelta;
        }

        MatProperties.Roughness = 0.05f;
        MatProperties.Metallic += MetallicDelta;
    }

#if ENABLE_MANY_SPHERES
    {
        constexpr uint32 kNumSpheres = 4096 * 8;
        constexpr float  kMaxRadius  = 32.0f;

        std::default_random_engine Generator;

        std::uniform_real_distribution<float> Random0(0.0f, FMath::kTwoPI_f);
        std::uniform_real_distribution<float> Random1(0.05f, 1.0);
        std::uniform_real_distribution<float> Random2(0.05f, 0.7);
        std::uniform_real_distribution<float> Random3(0.5f, 1.0);
        std::uniform_real_distribution<float> Random4(1.0f, 10.0f);

        for (uint32 i = 0; i < kNumSpheres; ++i)
        {
            const float Radius = Random1(Generator) * kMaxRadius;
            const float Alpha  = Random0(Generator);
            const float Theta  = Random0(Generator);

            const float CosAlpha = FMath::Cos(Alpha);
            const float SinAlpha = FMath::Sin(Alpha);
            const float CosTheta = FMath::Cos(Theta);
            const float SinTheta = FMath::Sin(Theta);

            const float PositionX = Radius * CosAlpha * SinTheta;
            const float PositionY = Radius * SinAlpha * SinTheta;
            const float PositionZ = Radius * CosTheta;

            const float Offset = -60.0f;

            NewActor = CurrentScene->CreateActor();
            if (NewActor)
            {
                NewActor->GetTransform().SetTranslation(PositionX, PositionY + 10.0f, Offset + PositionZ);

                // MovingBallComponent
                FMovingBallComponent* NewBall = NewObject<FMovingBallComponent>();
                if (NewBall)
                {
                    NewBall->Initialize(NewActor, Random4(Generator));
                    NewActor->AddComponent(NewBall);
                }

                NewActor->SetName("Random Sphere[" + ToString(i) + "]");

                // MeshComponent
                NewComponent = NewObject<FMeshComponent>();
                if (NewComponent)
                {
                    NewComponent->Initialize(NewActor, MakeShared<FMaterial>(MatProperties), SphereMesh);

                    NewComponent->Material->AlbedoMap    = GEngine->BaseTexture;
                    NewComponent->Material->NormalMap    = GEngine->BaseNormal;
                    NewComponent->Material->RoughnessMap = GEngine->BaseTexture;
                    NewComponent->Material->AOMap        = GEngine->BaseTexture;
                    NewComponent->Material->MetallicMap  = GEngine->BaseTexture;
                    NewComponent->Material->Initialize();

                    NewActor->AddComponent(NewComponent);
                }
            }

            MatProperties.Roughness = Random2(Generator);
            MatProperties.Metallic  = Random3(Generator);
        }
    }
#endif

    // Create Other Meshes
    FMeshData CubeMeshData = FMeshFactory::CreateCube();

    NewActor = CurrentScene->CreateActor();
    if (NewActor)
    {
        NewActor->SetName("Cube");
        NewActor->GetTransform().SetTranslation(0.0f, 2.0f, 50.0f);

        MatProperties.AO           = 1.0f;
        MatProperties.Metallic     = 1.0f;
        MatProperties.Roughness    = 1.0f;
        MatProperties.EnableHeight = 1;

        NewComponent = NewObject<FMeshComponent>();
        if (NewComponent)
        {
            FTextureResource2DRef AlbedoMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Albedo.png")));
            FTextureResource2DRef NormalMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Normal.png")));
            FTextureResource2DRef AOMap        = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_AO.png")));
            FTextureResource2DRef RoughnessMap = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Roughness.png")));
            FTextureResource2DRef HeightMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Height.png")));
            FTextureResource2DRef MetallicMap  = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Metallic.png")));
            
            TSharedPtr<FMaterial> NewMaterial = MakeShared<FMaterial>(MatProperties);
            NewMaterial->AlbedoMap    = AlbedoMap->GetRHITexture();
            NewMaterial->NormalMap    = NormalMap->GetRHITexture();
            NewMaterial->RoughnessMap = RoughnessMap->GetRHITexture();
            NewMaterial->HeightMap    = HeightMap->GetRHITexture();
            NewMaterial->AOMap        = AOMap->GetRHITexture();
            NewMaterial->MetallicMap  = MetallicMap->GetRHITexture();
            NewMaterial->Initialize();

            NewComponent->SetMesh(FMesh::Create(CubeMeshData));
            NewComponent->SetMaterial(NewMaterial);

            NewActor->AddComponent(NewComponent);
        }
    }

    NewActor = CurrentScene->CreateActor();
    if (NewActor)
    {
        NewActor->SetName("Plane");
        NewActor->GetTransform().SetRotation(FMath::kHalfPI_f, 0.0f, 0.0f);
        NewActor->GetTransform().SetUniformScale(50.0f);
        NewActor->GetTransform().SetTranslation(0.0f, 0.0f, 42.0f);

        MatProperties.AO           = 1.0f;
        MatProperties.Metallic     = 1.0f;
        MatProperties.Roughness    = 0.5f;
        MatProperties.EnableHeight = 0;
        MatProperties.Albedo       = FVector3(1.0f);

        NewComponent = NewObject<FMeshComponent>();
        if (NewComponent)
        {
            TSharedPtr<FMaterial> NewMaterial = MakeShared<FMaterial>(MatProperties);
            NewMaterial->AlbedoMap    = GEngine->BaseTexture;
            NewMaterial->NormalMap    = GEngine->BaseNormal;
            NewMaterial->RoughnessMap = GEngine->BaseTexture;
            NewMaterial->AOMap        = GEngine->BaseTexture;
            NewMaterial->MetallicMap  = GEngine->BaseTexture;
            NewMaterial->Initialize();

            NewComponent->SetMesh(FMesh::Create(FMeshFactory::CreatePlane(10, 10)));
            NewComponent->SetMaterial(NewMaterial);

            NewActor->AddComponent(NewComponent);
        }
    }

    FTextureResource2DRef AlbedoMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/StreetLight/BaseColor.jpg")));
    FTextureResource2DRef NormalMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/StreetLight/Normal.jpg")));
    FTextureResource2DRef RoughnessMap = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/StreetLight/Roughness.jpg")));
    FTextureResource2DRef MetallicMap  = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/StreetLight/Metallic.jpg")));

    MatProperties.Albedo       = FVector3(1.0f);
    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 1.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 0;
    MatProperties.EnableMask   = 0;

    FSceneData StreetLightData;
    FMeshImporter::Get().LoadMesh((ENGINE_LOCATION"/Assets/Models/Street_Light.obj"), StreetLightData);

    TSharedPtr<FMesh>     StreetLight = StreetLightData.HasModelData() ? FMesh::Create(StreetLightData.Models.FirstElement().Mesh) : nullptr;
    TSharedPtr<FMaterial> StreetLightMat = MakeShared<FMaterial>(MatProperties);

    for (uint32 i = 0; i < 4; i++)
    {
        NewActor = CurrentScene->CreateActor();
        if (NewActor)
        {
            NewActor->SetName("Street Light " + TTypeToString<uint32>::ToString(i));
            NewActor->GetTransform().SetUniformScale(0.25f);
            NewActor->GetTransform().SetTranslation(15.0f, 0.0f, 55.0f - float(i) * 3.0f);

            NewComponent = NewObject<FMeshComponent>();
            if (NewComponent)
            {
                StreetLightMat->AlbedoMap    = AlbedoMap->GetRHITexture();;
                StreetLightMat->NormalMap    = NormalMap->GetRHITexture();;
                StreetLightMat->RoughnessMap = RoughnessMap->GetRHITexture();;
                StreetLightMat->AOMap        = GEngine->BaseTexture;
                StreetLightMat->MetallicMap  = MetallicMap->GetRHITexture();
                StreetLightMat->Initialize();

                NewComponent->SetMesh(StreetLight);
                NewComponent->SetMaterial(StreetLightMat);

                NewActor->AddComponent(NewComponent);
            }
        }
    }

    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 0.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 0;
    MatProperties.Albedo       = FVector3(0.4f);

    FSceneData PillarData;
    FMeshImporter::Get().LoadMesh((ENGINE_LOCATION"/Assets/Models/Pillar.obj"), PillarData);

    TSharedPtr<FMesh>     Pillar         = PillarData.HasModelData() ? FMesh::Create(PillarData.Models.FirstElement().Mesh) : nullptr;
    TSharedPtr<FMaterial> PillarMaterial = MakeShared<FMaterial>(MatProperties);

    for (uint32 i = 0; i < 8; i++)
    {
        NewActor = CurrentScene->CreateActor();
        if (NewActor)
        {
            NewActor->SetName("Pillar " + TTypeToString<uint32>::ToString(i));
            NewActor->GetTransform().SetUniformScale(0.25f);
            NewActor->GetTransform().SetTranslation(-15.0f + float(i) * 1.75f, 0.0f, 60.0f);

            NewComponent = NewObject<FMeshComponent>();
            if (NewComponent)
            {
                PillarMaterial->AlbedoMap    = GEngine->BaseTexture;
                PillarMaterial->NormalMap    = GEngine->BaseNormal;
                PillarMaterial->RoughnessMap = GEngine->BaseTexture;
                PillarMaterial->AOMap        = GEngine->BaseTexture;
                PillarMaterial->MetallicMap  = GEngine->BaseTexture;
                PillarMaterial->Initialize();

                NewComponent->SetMesh(Pillar);
                NewComponent->SetMaterial(PillarMaterial);

                NewActor->AddComponent(NewComponent);
            }
        }
    }
#endif

    if (FSandboxPlayerController* Player = NewObject<FSandboxPlayerController>())
    {
        // TODO: Camera should be a component
        CurrentScene->AddCamera(Player->GetCamera());
        CurrentScene->AddActor(Player);
    }

    // Add PointLights
#if LOAD_SPONZA
    const float Intensity      = 100.0f;
    const float ShadowFarPlane = 20.0f;
    if (FPointLight* Light0 = NewObject<FPointLight>())
    {
        Light0->SetPosition(FVector3(16.5f, 1.0f, 0.0f));
        Light0->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        Light0->SetShadowBias(0.001f);
        Light0->SetMaxShadowBias(0.009f);
        Light0->SetShadowFarPlane(ShadowFarPlane);
        Light0->SetIntensity(Intensity);
        Light0->SetShadowCaster(true);
        CurrentScene->AddLight(Light0);
    }
    if (FPointLight* Light1 = NewObject<FPointLight>())
    {
        Light1->SetPosition(FVector3(-17.5f, 1.0f, 0.0f));
        Light1->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        Light1->SetShadowBias(0.001f);
        Light1->SetMaxShadowBias(0.009f);
        Light1->SetShadowFarPlane(ShadowFarPlane);
        Light1->SetIntensity(Intensity);
        Light1->SetShadowCaster(true);
        CurrentScene->AddLight(Light1);
    }
    if (FPointLight* Light2 = NewObject<FPointLight>())
    {
        Light2->SetPosition(FVector3(16.5f, 11.0f, 0.0f));
        Light2->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        Light2->SetShadowBias(0.001f);
        Light2->SetMaxShadowBias(0.009f);
        Light2->SetShadowFarPlane(ShadowFarPlane);
        Light2->SetIntensity(Intensity);
        Light2->SetShadowCaster(true);
        CurrentScene->AddLight(Light2);
    }
    if (FPointLight* Light3 = NewObject<FPointLight>())
    {
        Light3->SetPosition(FVector3(-17.5f, 11.0f, 0.0f));
        Light3->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        Light3->SetShadowBias(0.001f);
        Light3->SetMaxShadowBias(0.009f);
        Light3->SetShadowFarPlane(ShadowFarPlane);
        Light3->SetIntensity(Intensity);
        Light3->SetShadowCaster(true);
        CurrentScene->AddLight(Light3);
    }
#endif

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

            FPointLight* Light = NewObject<FPointLight>();
            if (Light)
            {
                Light->SetPosition(x, y, z);
                Light->SetColor(RandomFloats(Generator), RandomFloats(Generator), RandomFloats(Generator));
                Light->SetIntensity(Intentsity);
                FScene->AddLight(Light);
            }
        }
    }
#endif

    // Add DirectionalLight- Source
    if (FDirectionalLight* Light4 = NewObject<FDirectionalLight>())
    {
        Light4->SetShadowBias(0.0005f);
        Light4->SetMaxShadowBias(0.0009f);
        Light4->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        Light4->SetIntensity(50.0f);
    #if LOAD_SUN_TEMPLE
        Light4->SetRotation(FVector3(FMath::ToRadians(35.0f), FMath::ToRadians(-55.0f), 0.0f));
    #else
        Light4->SetRotation(FVector3(FMath::ToRadians(35.0f), FMath::ToRadians(135.0f), 0.0f));
    #endif
        Light4->SetCascadeSplitLambda(0.9f);
        CurrentScene->AddLight(Light4);
    }

    if (FLightProbe* LightProbe = NewObject<FLightProbe>())
    {
        LightProbe->SetPosition(FVector3(0.0f));
        CurrentScene->AddLightProbe(LightProbe);
    }

    LOG_INFO("Finished loading game");
    return true;
}

void FSandbox::Tick(FTimespan DeltaTime)
{
    FGameModule::Tick(DeltaTime);
}

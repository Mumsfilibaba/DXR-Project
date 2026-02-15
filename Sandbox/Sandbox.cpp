#include "Sandbox.h"
#include "SandboxPlayer.h"
#include "GameComponents.h"
#include <Core/Math/Math.h>
#include <Core/Misc/OutputDeviceLogger.h>
#include <Engine/Engine.h>
#include <Engine/Assets/AssetManager.h>
#include <Engine/World/World.h>
#include <Engine/World/Lights/PointLight.h>
#include <Engine/World/Lights/DirectionalLight.h>
#include <Engine/World/Lights/SkyLight.h>
#include <Engine/World/Actors/PlayerController.h>
#include <Engine/World/Components/StaticMeshComponent.h>
#include <Engine/World/Components/SkyboxComponent.h>
#include <RendererCore/TextureFactory.h>
#include <Renderer/FrameResources.h>
#include <Application/Application.h>

// TODO: Custom random
#include <random>

#define LOAD_LIGHT_SANDBOX (0)
#define LOAD_SPONZA (0)
#define LOAD_BISTRO (0)
#define LOAD_SUN_TEMPLE (0)
#define LOAD_EMERALD_SQUARE (1)

#define ENABLE_LIGHT_TEST (0)
#define ENABLE_SPHERES_TEST (0)

IMPLEMENT_ENGINE_MODULE(FSandbox, Sandbox);

FSandbox::FSandbox()
    : FGameModule()
{
}

FSandbox::~FSandbox()
{
}

bool FSandbox::Init()
{
    // Store the Engine's world pointer 
    FWorld* CurrentWorld = FEngine::Get()->GetWorld();

    bool bResult = false;

#if LOAD_LIGHT_SANDBOX
    bResult = CreateLightSandbox(CurrentWorld);
#elif LOAD_SPONZA
    bResult = CreateSponza(CurrentWorld);
#elif LOAD_BISTRO
    bResult = CreateBistro(CurrentWorld);
#elif LOAD_SUN_TEMPLE
    bResult = CreateSunTemple(CurrentWorld);
#elif LOAD_EMERALD_SQUARE
    bResult = CreateEmeraldSquare(CurrentWorld);
#endif

    if (!bResult)
    {
        DEBUG_BREAK();
    }

    LOG_INFO("Finished loading game");
    return true;
}

void FSandbox::Tick(float)
{
}

bool FSandbox::CreateSponza(FWorld* InWorld)
{
    // Load Scene
    TSharedRef<FModel> Sponza = FAssetManager::Get().LoadModel((ENGINE_LOCATION"/Assets/Scenes/Sponza/Sponza.obj"));
    if (!Sponza)
    {
        return false;
    }

    Sponza->SetUniformScale(0.015f);

    const int32 NumMaterials = Sponza->GetNumMaterials();
    for (int32 Index = 0; Index < NumMaterials; Index++)
    {
        const TSharedPtr<FMaterial>& Material = Sponza->GetMaterial(Index);
        if (Material->AlphaMask)
        {
            const EMaterialFlags Flags = EMaterialFlags::EnableAlpha | EMaterialFlags::DoubleSided;
            Material->SetMaterialFlags(Flags, true);
        }
    }

    Sponza->AddToWorld(InWorld);

    // Create Spheres
    FMeshCreateInfo SphereMeshInfo = FMeshFactory::CreateSphere(3);

    TSharedPtr<FMesh> SphereMesh = MakeSharedPtr<FMesh>();
    SphereMesh->Init(SphereMeshInfo);

    constexpr float  SphereOffset   = 1.25f;
    constexpr uint32 SphereCountX   = 8;
    constexpr float  StartPositionX = (-static_cast<float>(SphereCountX) * SphereOffset) / 2.0f;
    constexpr uint32 SphereCountY   = 8;
    constexpr float  StartPositionY = (-static_cast<float>(SphereCountY) * SphereOffset) / 2.0f;
    constexpr float  MetallicDelta  = 1.0f / SphereCountY;
    constexpr float  RoughnessDelta = 1.0f / SphereCountX;

    FMaterialInfo MaterialInfo;
    MaterialInfo.Albedo           = FFloatColor::White;
    MaterialInfo.AmbientOcclusion = 1.0f;
    MaterialInfo.MaterialFlags    = EMaterialFlags::None;

    uint32 SphereIndex = 0;
    for (uint32 y = 0; y < SphereCountY; y++)
    {
        for (uint32 x = 0; x < SphereCountX; x++)
        {
            FActor* NewActor = InWorld->CreateActor();
            if (NewActor)
            {
                NewActor->GetTransform().SetTranslation(StartPositionX + (x * SphereOffset), 0.6f, 40.0f + StartPositionY + (y * SphereOffset));
                NewActor->SetName(FString::CreateFormatted("Sphere[%d]", SphereIndex));
                SphereIndex++;

                FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
                if (NewComponent)
                {
                    TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
                    NewMaterial->AlbedoMap    = FEngine::Get()->BaseTexture;
                    NewMaterial->RoughnessMap = FEngine::Get()->BaseTexture;
                    NewMaterial->AOMap        = FEngine::Get()->BaseTexture;
                    NewMaterial->MetallicMap  = FEngine::Get()->BaseTexture;

                    NewMaterial->Initialize();
                    NewMaterial->SetName(FString::CreateFormatted("Sphere Material %d", SphereIndex));

                    NewComponent->SetMesh(SphereMesh);
                    NewComponent->SetMaterial(NewMaterial);

                    NewActor->AddComponent(NewComponent);
                }
            }

            MaterialInfo.Roughness += RoughnessDelta;
        }

        MaterialInfo.Roughness = 0.05f;
        MaterialInfo.Metallic += MetallicDelta;
    }

#if ENABLE_SPHERES_TEST
    {
        constexpr uint32 kNumSpheres = 4096 * 8;
        constexpr float  kMaxRadius = 32.0f;

        std::default_random_engine Generator;

        std::uniform_real_distribution<float> Random0(0.0f, Math::Constants::TwoPI);
        std::uniform_real_distribution<float> Random1(0.05f, 1.0);
        std::uniform_real_distribution<float> Random2(0.05f, 0.7);
        std::uniform_real_distribution<float> Random3(0.5f, 1.0);
        std::uniform_real_distribution<float> Random4(1.0f, 10.0f);

        for (uint32 i = 0; i < kNumSpheres; ++i)
        {
            const float Radius = Random1(Generator) * kMaxRadius;
            const float Alpha = Random0(Generator);
            const float Theta = Random0(Generator);

            const float CosAlpha = Math::Cos(Alpha);
            const float SinAlpha = Math::Sin(Alpha);
            const float CosTheta = Math::Cos(Theta);
            const float SinTheta = Math::Sin(Theta);

            const float PositionX = Radius * CosAlpha * SinTheta;
            const float PositionY = Radius * SinAlpha * SinTheta;
            const float PositionZ = Radius * CosTheta;

            const float Offset = -60.0f;

            NewActor = CurrentWorld->CreateActor();
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
                NewComponent = NewObject<FStaticMeshComponent>();
                if (NewComponent)
                {
                    NewComponent->Initialize(NewActor, MakeShared<FMaterial>(MaterialInfo), SphereMesh);

                    NewComponent->Material->AlbedoMap = FEngine::Get()->BaseTexture;
                    NewComponent->Material->NormalMap = FEngine::Get()->BaseNormal;
                    NewComponent->Material->RoughnessMap = FEngine::Get()->BaseTexture;
                    NewComponent->Material->AOMap = FEngine::Get()->BaseTexture;
                    NewComponent->Material->MetallicMap = FEngine::Get()->BaseTexture;
                    NewComponent->Material->Initialize();

                    NewActor->AddComponent(NewComponent);
                }
            }

            MaterialInfo.Roughness = Random2(Generator);
            MaterialInfo.Metallic = Random3(Generator);
        }
    }
#endif

    // Create Other Meshes
    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Cube");
        NewActor->GetTransform().SetTranslation(0.0f, 2.0f, 50.0f);

        MaterialInfo.Albedo           = FFloatColor::White;
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 1.0f;
        MaterialInfo.Roughness        = 1.0f;
        MaterialInfo.MaterialFlags   |= EMaterialFlags::EnableHeight | EMaterialFlags::EnableNormalMapping;

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            FTexture2DRef AlbedoMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Albedo.png")));
            FTexture2DRef NormalMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Normal.png")));
            FTexture2DRef AOMap        = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_AO.png")));
            FTexture2DRef RoughnessMap = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Roughness.png")));
            FTexture2DRef HeightMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Height.png")));
            FTexture2DRef MetallicMap  = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Metallic.png")));

            TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
            NewMaterial->AlbedoMap    = AlbedoMap->GetRHITexture();
            NewMaterial->NormalMap    = NormalMap->GetRHITexture();
            NewMaterial->RoughnessMap = RoughnessMap->GetRHITexture();
            NewMaterial->HeightMap    = HeightMap->GetRHITexture();
            NewMaterial->AOMap        = AOMap->GetRHITexture();
            NewMaterial->MetallicMap  = MetallicMap->GetRHITexture();

            NewMaterial->Initialize();
            NewMaterial->SetName("GateMaterial");

            FMeshCreateInfo CubeMeshData = FMeshFactory::CreateCube();
            TSharedPtr<FMesh> CubeMesh = MakeSharedPtr<FMesh>();
            CubeMesh->Init(CubeMeshData);

            NewComponent->SetMesh(CubeMesh);
            NewComponent->SetMaterial(NewMaterial);

            NewActor->AddComponent(NewComponent);
        }
    }

    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Plane");
        NewActor->GetTransform().SetRotation(Math::Constants::HalfPI, 0.0f, 0.0f);
        NewActor->GetTransform().SetUniformScale(50.0f);
        NewActor->GetTransform().SetTranslation(0.0f, 0.0f, 42.0f);

        MaterialInfo.Albedo           = FFloatColor::White;
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 1.0f;
        MaterialInfo.Roughness        = 0.5f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::None;

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
            NewMaterial->AlbedoMap    = FEngine::Get()->BaseTexture;
            NewMaterial->RoughnessMap = FEngine::Get()->BaseTexture;
            NewMaterial->AOMap        = FEngine::Get()->BaseTexture;
            NewMaterial->MetallicMap  = FEngine::Get()->BaseTexture;

            NewMaterial->Initialize();
            NewMaterial->SetName("PlaneMaterial");

            FMeshCreateInfo PlaneMeshData = FMeshFactory::CreatePlane(10, 10);
            TSharedPtr<FMesh> PlaneMesh = MakeSharedPtr<FMesh>();
            PlaneMesh->Init(PlaneMeshData);

            NewComponent->SetMesh(PlaneMesh);
            NewComponent->SetMaterial(NewMaterial);

            NewActor->AddComponent(NewComponent);
        }
    }

    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Cone");
        NewActor->GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
        NewActor->GetTransform().SetUniformScale(3.0f);
        NewActor->GetTransform().SetTranslation(-15.0f, 0.5f, 50.0f);

        MaterialInfo.Albedo           = FFloatColor::Green;
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 0.0f;
        MaterialInfo.Roughness        = 1.0f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::None;

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
            NewMaterial->AlbedoMap    = FEngine::Get()->BaseTexture;
            NewMaterial->RoughnessMap = FEngine::Get()->BaseTexture;
            NewMaterial->AOMap        = FEngine::Get()->BaseTexture;
            NewMaterial->MetallicMap  = FEngine::Get()->BaseTexture;

            NewMaterial->Initialize();
            NewMaterial->SetName("ConeMaterial");

            FMeshCreateInfo ConeMeshData = FMeshFactory::CreateCone(32, 0.5f);

            TSharedPtr<FMesh> ConeMesh = MakeSharedPtr<FMesh>();
            ConeMesh->Init(ConeMeshData);

            NewComponent->SetMesh(ConeMesh);
            NewComponent->SetMaterial(NewMaterial);

            NewActor->AddComponent(NewComponent);
        }
    }

    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Torus");
        NewActor->GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
        NewActor->GetTransform().SetUniformScale(1.0f);
        NewActor->GetTransform().SetTranslation(-15.0f, 1.0f, 45.0f);

        MaterialInfo.Albedo           = FFloatColor::Red;
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 0.0f;
        MaterialInfo.Roughness        = 1.0f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::None;

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
            NewMaterial->AlbedoMap    = FEngine::Get()->BaseTexture;
            NewMaterial->RoughnessMap = FEngine::Get()->BaseTexture;
            NewMaterial->AOMap        = FEngine::Get()->BaseTexture;
            NewMaterial->MetallicMap  = FEngine::Get()->BaseTexture;

            NewMaterial->Initialize();
            NewMaterial->SetName("TorusMaterial");

            FMeshCreateInfo TorusMeshData = FMeshFactory::CreateTorus(1.0f, 0.4f, 48, 32);

            TSharedPtr<FMesh> TorusMesh = MakeSharedPtr<FMesh>();
            TorusMesh->Init(TorusMeshData);

            NewComponent->SetMesh(TorusMesh);
            NewComponent->SetMaterial(NewMaterial);

            NewActor->AddComponent(NewComponent);
        }
    }

    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Teapot");
        NewActor->GetTransform().SetRotation(-Math::Constants::HalfPI, Math::Constants::HalfPI, 0.0f);
        NewActor->GetTransform().SetUniformScale(1.0f);
        NewActor->GetTransform().SetTranslation(-15.0f, 1.0f, 37.5f);

        // Gold: 0.944, 0.776, 0.373
        MaterialInfo.Albedo           = FFloatColor(0.944f, 0.776f, 0.373f, 1.0f);
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 1.0f;
        MaterialInfo.Roughness        = 0.2f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::DoubleSided;

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
            NewMaterial->AlbedoMap    = FEngine::Get()->BaseTexture;
            NewMaterial->RoughnessMap = FEngine::Get()->BaseTexture;
            NewMaterial->AOMap        = FEngine::Get()->BaseTexture;
            NewMaterial->MetallicMap  = FEngine::Get()->BaseTexture;

            NewMaterial->Initialize();
            NewMaterial->SetName("TeapotMaterial");

            FMeshCreateInfo TeapotMeshData = FMeshFactory::CreateTeapot(12);

            TSharedPtr<FMesh> TeapotMesh = MakeSharedPtr<FMesh>();
            TeapotMesh->Init(TeapotMeshData);

            NewComponent->SetMesh(TeapotMesh);
            NewComponent->SetMaterial(NewMaterial);

            NewActor->AddComponent(NewComponent);
        }
    }

    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Pyramid");
        NewActor->GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
        NewActor->GetTransform().SetUniformScale(1.0f);
        NewActor->GetTransform().SetTranslation(-15.0f, 1.0f, 30.0f);

        MaterialInfo.Albedo           = FFloatColor::Blue;
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 0.0f;
        MaterialInfo.Roughness        = 1.0f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::None;

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
            NewMaterial->AlbedoMap    = FEngine::Get()->BaseTexture;
            NewMaterial->RoughnessMap = FEngine::Get()->BaseTexture;
            NewMaterial->AOMap        = FEngine::Get()->BaseTexture;
            NewMaterial->MetallicMap  = FEngine::Get()->BaseTexture;

            NewMaterial->Initialize();
            NewMaterial->SetName("PyramidMaterial");

            FMeshCreateInfo PyramidMeshData = FMeshFactory::CreatePyramid(2.0f, 2.0f, 2.0f);

            TSharedPtr<FMesh> PyramidMesh = MakeSharedPtr<FMesh>();
            PyramidMesh->Init(PyramidMeshData);

            NewComponent->SetMesh(PyramidMesh);
            NewComponent->SetMaterial(NewMaterial);

            NewActor->AddComponent(NewComponent);
        }
    }

    TSharedRef<FModel> StreetLightModel = FAssetManager::Get().LoadModel((ENGINE_LOCATION"/Assets/Models/Street_Light.obj"));
    if (StreetLightModel)
    {
        FTexture2DRef AlbedoMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/StreetLight/BaseColor.jpg")));
        FTexture2DRef NormalMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/StreetLight/Normal.jpg")));
        FTexture2DRef RoughnessMap = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/StreetLight/Roughness.jpg")));
        FTexture2DRef MetallicMap  = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/StreetLight/Metallic.jpg")));

        MaterialInfo.Albedo           = FFloatColor::White;
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 1.0f;
        MaterialInfo.Roughness        = 1.0f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::EnableNormalMapping;

        TSharedPtr<FMaterial> StreetLightMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
        StreetLightMaterial->AlbedoMap    = AlbedoMap->GetRHITexture();
        StreetLightMaterial->NormalMap    = NormalMap->GetRHITexture();
        StreetLightMaterial->RoughnessMap = RoughnessMap->GetRHITexture();
        StreetLightMaterial->AOMap        = FEngine::Get()->BaseTexture;
        StreetLightMaterial->MetallicMap  = MetallicMap->GetRHITexture();

        StreetLightMaterial->Initialize();
        StreetLightMaterial->SetName("StreetLightMaterial");

        const int32 NumMeshes = StreetLightModel->GetNumMeshes();
        for (uint32 i = 0; i < 4; i++)
        {
            for (int32 MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++)
            {
                if (FActor* NewActor = InWorld->CreateActor())
                {
                    const TSharedPtr<FMesh>& Mesh = StreetLightModel->GetMesh(MeshIndex);
                    NewActor->SetName(FString::CreateFormatted("Street Light (%s) %d", *Mesh->GetName(), i));
                    NewActor->GetTransform().SetUniformScale(0.25f);
                    NewActor->GetTransform().SetTranslation(15.0f, 0.0f, 55.0f - float(i) * 3.0f);

                    FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
                    if (NewComponent)
                    {
                        NewComponent->SetMesh(Mesh);
                        NewComponent->SetMaterial(StreetLightMaterial);
                        NewActor->AddComponent(NewComponent);
                    }
                }
            }
        }
    }

    MaterialInfo.Albedo           = FFloatColor(0.4f, 0.4f, 0.4f, 1.0f);
    MaterialInfo.AmbientOcclusion = 1.0f;
    MaterialInfo.Metallic         = 0.0f;
    MaterialInfo.Roughness        = 1.0f;
    MaterialInfo.MaterialFlags    = EMaterialFlags::None;

    TSharedPtr<FMaterial> CylinderMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
    CylinderMaterial->AlbedoMap    = FEngine::Get()->BaseTexture;
    CylinderMaterial->RoughnessMap = FEngine::Get()->BaseTexture;
    CylinderMaterial->AOMap        = FEngine::Get()->BaseTexture;
    CylinderMaterial->MetallicMap  = FEngine::Get()->BaseTexture;

    CylinderMaterial->Initialize();
    CylinderMaterial->SetName("CylinderMaterial");

    FMeshCreateInfo CylinderMeshData = FMeshFactory::CreateCylinder(32, 0.4f, 5.0f);

    TSharedPtr<FMesh> CylinderMesh = MakeSharedPtr<FMesh>();
    CylinderMesh->Init(CylinderMeshData);

    constexpr uint32 NumCylinders = 8;
    for (uint32 i = 0; i < NumCylinders; i++)
    {
        if (FActor* NewActor = InWorld->CreateActor())
        {
            NewActor->SetName(FString::CreateFormatted("Cylinder %d", i));
            NewActor->GetTransform().SetUniformScale(1.0f);
            NewActor->GetTransform().SetTranslation(-15.0f + float(i) * 1.75f, 2.5f, 60.0f);

            FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
            if (NewComponent)
            {
                NewComponent->SetMesh(CylinderMesh);
                NewComponent->SetMaterial(CylinderMaterial);
                NewActor->AddComponent(NewComponent);
            }
        }
    }

    // Load Skybox
    FRHITextureRef Skybox = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Textures/arches.hdr");
    if (!Skybox)
    {
        DEBUG_BREAK();
        return false;
    }

    // Add Camera
    if (FSandboxPlayerController* Player = NewObject<FSandboxPlayerController>())
    {
        // Add camera to the world
        InWorld->AddCamera(Player->GetCamera());
        InWorld->AddActor(Player);

        // Add Skybox
        if (FSkyboxComponent* SkyboxComponent = NewObject<FSkyboxComponent>())
        {
            // Set skybox cube-map
            SkyboxComponent->SetCubeMap(Skybox);
            Player->AddComponent(SkyboxComponent);
        }
    }

    // Add PointLights
    const float Intensity      = 80.0f;
    const float ShadowFarPlane = 35.0f;
    if (FPointLight* PointLight0 = NewObject<FPointLight>())
    {
        PointLight0->SetPosition(FVector3(15.0f, 2.5f, 0.0f));
        PointLight0->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        PointLight0->SetShadowBias(0.02f);
        PointLight0->SetShadowFarPlane(ShadowFarPlane);
        PointLight0->SetIntensity(Intensity);
        PointLight0->SetShadowCaster(true);

        InWorld->AddLight(PointLight0);
    }

    if (FPointLight* PointLight1 = NewObject<FPointLight>())
    {
        PointLight1->SetPosition(FVector3(-15.0f, 2.5f, 0.0f));
        PointLight1->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        PointLight1->SetShadowBias(0.02f);
        PointLight1->SetShadowFarPlane(ShadowFarPlane);
        PointLight1->SetIntensity(Intensity);
        PointLight1->SetShadowCaster(true);

        InWorld->AddLight(PointLight1);
    }

    if (FPointLight* PointLight2 = NewObject<FPointLight>())
    {
        PointLight2->SetPosition(FVector3(17.0f, 10.0f, 6.0f));
        PointLight2->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        PointLight2->SetShadowBias(0.02f);
        PointLight2->SetShadowFarPlane(ShadowFarPlane);
        PointLight2->SetIntensity(Intensity);
        PointLight2->SetShadowCaster(true);

        InWorld->AddLight(PointLight2);
    }

    if (FPointLight* PointLight3 = NewObject<FPointLight>())
    {
        PointLight3->SetPosition(FVector3(-18.0f, 10.0f, 6.0f));
        PointLight3->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        PointLight3->SetShadowBias(0.02f);
        PointLight3->SetShadowFarPlane(ShadowFarPlane);
        PointLight3->SetIntensity(Intensity);
        PointLight3->SetShadowCaster(true);

        InWorld->AddLight(PointLight3);
    }

    if (FPointLight* PointLight4 = NewObject<FPointLight>())
    {
        PointLight4->SetPosition(FVector3(17.0f, 10.0f, -7.0f));
        PointLight4->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        PointLight4->SetShadowBias(0.02f);
        PointLight4->SetShadowFarPlane(ShadowFarPlane);
        PointLight4->SetIntensity(Intensity);
        PointLight4->SetShadowCaster(true);

        InWorld->AddLight(PointLight4);
    }

    if (FPointLight* PointLight5 = NewObject<FPointLight>())
    {
        PointLight5->SetPosition(FVector3(-18.0f, 10.0f, -7.0f));
        PointLight5->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        PointLight5->SetShadowBias(0.02f);
        PointLight5->SetShadowFarPlane(ShadowFarPlane);
        PointLight5->SetIntensity(Intensity);
        PointLight5->SetShadowCaster(true);

        InWorld->AddLight(PointLight5);
    }

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
                FWorld->AddLight(Light);
            }
        }
    }
#endif

    // Add SkyLight
    if (FSkyLight* SkyLight = NewObject<FSkyLight>())
    {
        SkyLight->SetCubeMap(Skybox);
        InWorld->AddLight(SkyLight);
    }

    // Add DirectionalLight
    if (FDirectionalLight* DirectionalLight = NewObject<FDirectionalLight>())
    {
        DirectionalLight->SetShadowBias(0.0005f);
        DirectionalLight->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        DirectionalLight->SetIntensity(50.0f);
        DirectionalLight->SetRotation(FVector3(Math::DegreesToRadians(35.0f), Math::DegreesToRadians(135.0f), 0.0f));

        InWorld->AddLight(DirectionalLight);
    }

    return true;
}

bool FSandbox::CreateBistro(FWorld* InWorld)
{
    TSharedRef<FModel> BistroInterior = FAssetManager::Get().LoadModel((ENGINE_LOCATION"/Assets/Scenes/Bistro/BistroInterior.fbx"));
    if (!BistroInterior)
    {
        return false;
    }

    int32 NumMaterials = BistroInterior->GetNumMaterials();
    for (int32 Index = 0; Index < NumMaterials; Index++)
    {
        const TSharedPtr<FMaterial>& Material = BistroInterior->GetMaterial(Index);

        EMaterialFlags Flags = EMaterialFlags::PackedParams;
        if (Material->GetName().Contains("DoubleSided"))
        {
            Flags |= EMaterialFlags::EnableAlpha | EMaterialFlags::DoubleSided | EMaterialFlags::PackedDiffuseAlpha;
        }

        Material->SetMaterialFlags(Flags, true);
    }

    BistroInterior->AddToWorld(InWorld);

    TSharedRef<FModel> BistroExterior = FAssetManager::Get().LoadModel((ENGINE_LOCATION"/Assets/Scenes/Bistro/BistroExterior.fbx"));
    if (!BistroExterior)
    {
        return false;
    }

    NumMaterials = BistroExterior->GetNumMaterials();
    for (int32 Index = 0; Index < NumMaterials; Index++)
    {
        const TSharedPtr<FMaterial>& Material = BistroExterior->GetMaterial(Index);

        EMaterialFlags Flags = EMaterialFlags::PackedParams;
        if (Material->GetName().Contains("DoubleSided"))
        {
            Flags |= EMaterialFlags::EnableAlpha | EMaterialFlags::DoubleSided | EMaterialFlags::PackedDiffuseAlpha;
        }

        Material->SetMaterialFlags(Flags, true);
    }

    BistroExterior->AddToWorld(InWorld);

    // Load Skybox
    FRHITextureRef Skybox = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Scenes/Bistro/san_giuseppe_bridge_4k.hdr");
    if (!Skybox)
    {
        DEBUG_BREAK();
        return false;
    }

    // Add Camera
    if (FSandboxPlayerController* Player = NewObject<FSandboxPlayerController>())
    {
        // Add camera to the world
        InWorld->AddCamera(Player->GetCamera());
        InWorld->AddActor(Player);

        // Add Skybox
        if (FSkyboxComponent* SkyboxComponent = NewObject<FSkyboxComponent>())
        {
            // Set skybox cube-map
            SkyboxComponent->SetCubeMap(Skybox);
            Player->AddComponent(SkyboxComponent);
        }
    }

    // Add SkyLight
    if (FSkyLight* SkyLight = NewObject<FSkyLight>())
    {
        SkyLight->SetCubeMap(Skybox);
        InWorld->AddLight(SkyLight);
    }

    // Add DirectionalLight
    if (FDirectionalLight* DirectionalLight = NewObject<FDirectionalLight>())
    {
        DirectionalLight->SetShadowBias(0.0005f);
        DirectionalLight->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        DirectionalLight->SetIntensity(50.0f);
        DirectionalLight->SetRotation(FVector3(Math::DegreesToRadians(35.0f), Math::DegreesToRadians(135.0f), 0.0f));

        InWorld->AddLight(DirectionalLight);
    }

    return true;
}

bool FSandbox::CreateSunTemple(FWorld* InWorld)
{
    const EMeshImportFlags ImportFlags = EMeshImportFlags::InvertAxisX;
    TSharedRef<FModel> SunTemple = FAssetManager::Get().LoadModel(ENGINE_LOCATION"/Assets/Scenes/SunTemple/SunTemple.fbx", ImportFlags);
    if (!SunTemple)
    {
        return false;
    }

    int32 NumMaterials = SunTemple->GetNumMaterials();
    for (int32 Index = 0; Index < NumMaterials; Index++)
    {
        const TSharedPtr<FMaterial>& Material = SunTemple->GetMaterial(Index);

        EMaterialFlags Flags = EMaterialFlags::PackedParams;
        if (Material->GetName().Contains("DoubleSided"))
        {
            Flags |= EMaterialFlags::EnableAlpha | EMaterialFlags::DoubleSided | EMaterialFlags::PackedDiffuseAlpha;
        }

        Material->SetMaterialFlags(Flags, true);
    }

    SunTemple->AddToWorld(InWorld);

    // Load Skybox
    FRHITextureRef Skybox = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Scenes/SunTemple/SunTemple_Skybox.hdr");
    if (!Skybox)
    {
        DEBUG_BREAK();
        return false;
    }

    // Add Camera
    if (FSandboxPlayerController* Player = NewObject<FSandboxPlayerController>())
    {
        // Add camera to the world
        InWorld->AddCamera(Player->GetCamera());
        InWorld->AddActor(Player);

        // Add Skybox
        if (FSkyboxComponent* SkyboxComponent = NewObject<FSkyboxComponent>())
        {
            // Set skybox cube-map
            SkyboxComponent->SetCubeMap(Skybox);
            Player->AddComponent(SkyboxComponent);
        }
    }

    // Add SkyLight
    if (FSkyLight* SkyLight = NewObject<FSkyLight>())
    {
        SkyLight->SetCubeMap(Skybox);
        InWorld->AddLight(SkyLight);
    }

    // Add Light-Probes
    if (FLightProbe* LightProbe = NewObject<FLightProbe>())
    {
        // Load Reflection Probe
        FRHITextureRef ReflectionProbe = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Scenes/SunTemple/SunTemple_Reflection.hdr");
        if (!ReflectionProbe)
        {
            DEBUG_BREAK();
            return false;
        }

        LightProbe->SetPosition(FVector3(0.0f, 13.0f, 0.5f));
        LightProbe->SetBoxExtent(FVector3(19.0f, 21.5f, 22.0f));
        LightProbe->SetBoxOffset(FVector3(0.0f, 0.0f, 0.0f));
        LightProbe->SetCubeMap(ReflectionProbe);
        LightProbe->SetBoxProjection(true);

        InWorld->AddLightProbe(LightProbe);
    }

    // Add Light-Probes
    if (FLightProbe* LightProbe = NewObject<FLightProbe>())
    {
        // Load Reflection Probe
        FRHITextureRef ReflectionProbe = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Scenes/SunTemple/SunTemple_Reflection_Interior.hdr");
        if (!ReflectionProbe)
        {
            DEBUG_BREAK();
            return false;
        }

        LightProbe->SetPosition(FVector3(0.0f, 6.0f, 30.0f));
        LightProbe->SetBoxExtent(FVector3(19.0f, 21.5f, 22.0f));
        LightProbe->SetBoxOffset(FVector3(0.0f, 0.0f, 0.0f));
        LightProbe->SetCubeMap(ReflectionProbe);
        LightProbe->SetBoxProjection(true);

        InWorld->AddLightProbe(LightProbe);
    }

    // Add DirectionalLight
    if (FDirectionalLight* DirectionalLight = NewObject<FDirectionalLight>())
    {
        DirectionalLight->SetShadowBias(0.0005f);
        DirectionalLight->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        DirectionalLight->SetIntensity(50.0f);
        DirectionalLight->SetRotation(FVector3(Math::DegreesToRadians(-55.0f), Math::DegreesToRadians(325.0f), 0.0f));

        InWorld->AddLight(DirectionalLight);
    }

    return true;
}

bool FSandbox::CreateEmeraldSquare(FWorld* InWorld)
{
    const EMeshImportFlags ImportFlags = EMeshImportFlags::RecalculateTangents | EMeshImportFlags::Default;
    TSharedRef<FModel> EmeraldSquare_Day = FAssetManager::Get().LoadModel((ENGINE_LOCATION"/Assets/Scenes/EmeraldSquare/EmeraldSquare_Day.fbx"), ImportFlags);
    if (!EmeraldSquare_Day)
    {
        return false;
    }

    int32 NumMaterials = EmeraldSquare_Day->GetNumMaterials();
    for (int32 Index = 0; Index < NumMaterials; Index++)
    {
        const TSharedPtr<FMaterial>& Material = EmeraldSquare_Day->GetMaterial(Index);

        EMaterialFlags Flags = EMaterialFlags::PackedParams;
        if (Material->GetName().Contains("DoubleSided"))
        {
            Flags |= EMaterialFlags::EnableAlpha | EMaterialFlags::DoubleSided | EMaterialFlags::PackedDiffuseAlpha;
        }

        Material->SetMaterialFlags(Flags, true);
    }

    EmeraldSquare_Day->AddToWorld(InWorld);

    // Load Skybox
    FRHITextureRef Skybox = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Scenes/EmeraldSquare/symmetrical_garden_4k.hdr");
    if (!Skybox)
    {
        DEBUG_BREAK();
        return false;
    }

    // Add Camera
    if (FSandboxPlayerController* Player = NewObject<FSandboxPlayerController>())
    {
        // Add camera to the world
        InWorld->AddCamera(Player->GetCamera());
        InWorld->AddActor(Player);

        // Add Skybox
        if (FSkyboxComponent* SkyboxComponent = NewObject<FSkyboxComponent>())
        {
            // Set skybox cube-map
            SkyboxComponent->SetCubeMap(Skybox);
            Player->AddComponent(SkyboxComponent);
        }
    }

    // Add SkyLight
    if (FSkyLight* SkyLight = NewObject<FSkyLight>())
    {
        SkyLight->SetCubeMap(Skybox);
        InWorld->AddLight(SkyLight);
    }

    // Add DirectionalLight
    if (FDirectionalLight* DirectionalLight = NewObject<FDirectionalLight>())
    {
        DirectionalLight->SetShadowBias(0.0005f);
        DirectionalLight->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        DirectionalLight->SetIntensity(50.0f);
        DirectionalLight->SetRotation(FVector3(Math::DegreesToRadians(35.0f), Math::DegreesToRadians(135.0f), 0.0f));

        InWorld->AddLight(DirectionalLight);
    }

    return true;
}

bool FSandbox::CreateLightSandbox(FWorld* InWorld)
{
    FMaterialInfo MaterialInfo;
    MaterialInfo.Albedo           = FFloatColor::White;
    MaterialInfo.AmbientOcclusion = 1.0f;
    MaterialInfo.Metallic         = 0.0f;
    MaterialInfo.Roughness        = 1.0f;
    MaterialInfo.MaterialFlags    = EMaterialFlags::None;

    TSharedPtr<FMaterial> BasicMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
    BasicMaterial->AlbedoMap    = FEngine::Get()->BaseTexture;
    BasicMaterial->RoughnessMap = FEngine::Get()->BaseTexture;
    BasicMaterial->AOMap        = FEngine::Get()->BaseTexture;
    BasicMaterial->MetallicMap  = FEngine::Get()->BaseTexture;

    BasicMaterial->Initialize();
    BasicMaterial->SetName("Basic-Material");

    // Create Plane
    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Plane");
        NewActor->GetTransform().SetRotation(Math::Constants::HalfPI, 0.0f, 0.0f);
        NewActor->GetTransform().SetUniformScale(30.0f);
        NewActor->GetTransform().SetTranslation(0.0f, 0.0f, 0.0f);

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            FMeshCreateInfo PlaneMeshData = FMeshFactory::CreatePlane(10, 10);

            TSharedPtr<FMesh> PlaneMesh = MakeSharedPtr<FMesh>();
            PlaneMesh->Init(PlaneMeshData);

            NewComponent->SetMesh(PlaneMesh);
            NewComponent->SetMaterial(BasicMaterial);

            NewActor->AddComponent(NewComponent);
        }
    }

    // Create plane
    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Cylinder");
        NewActor->GetTransform().SetScale(1.0f, 4.0f, 1.0f);
        NewActor->GetTransform().SetTranslation(0.0f, 4.0f, 10.0f);

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            FMeshCreateInfo CylinderMeshData = FMeshFactory::CreateCylinder(16, 0.5f, 2.0f);

            TSharedPtr<FMesh> CylinderMesh = MakeSharedPtr<FMesh>();
            CylinderMesh->Init(CylinderMeshData);

            NewComponent->SetMesh(CylinderMesh);
            NewComponent->SetMaterial(BasicMaterial);
            NewActor->AddComponent(NewComponent);
        }
    }

    // Create small hut
    FMeshCreateInfo CubeMeshData = FMeshFactory::CreateCube();

    TSharedPtr<FMesh> CubeMesh = MakeSharedPtr<FMesh>();
    CubeMesh->Init(CubeMeshData);

    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Wall 1");
        NewActor->GetTransform().SetScale(0.05f, 2.0f, 4.0f);
        NewActor->GetTransform().SetTranslation(9.975f, 1.0f, -8.0f);

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            NewComponent->SetMesh(CubeMesh);
            NewComponent->SetMaterial(BasicMaterial);
            NewActor->AddComponent(NewComponent);
        }
    }

    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Wall 2");
        NewActor->GetTransform().SetScale(4.0f, 2.0f, 0.05f);
        NewActor->GetTransform().SetTranslation(8.0f, 1.0f, -9.975f);

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            NewComponent->SetMesh(CubeMesh);
            NewComponent->SetMaterial(BasicMaterial);
            NewActor->AddComponent(NewComponent);
        }
    }

    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Wall 3");
        NewActor->GetTransform().SetScale(4.0f, 2.0f, 0.05f);
        NewActor->GetTransform().SetTranslation(8.0f, 1.0f, -5.975f);

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            NewComponent->SetMesh(CubeMesh);
            NewComponent->SetMaterial(BasicMaterial);
            NewActor->AddComponent(NewComponent);
        }
    }

    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Roof");
        NewActor->GetTransform().SetScale(4.0f, 0.05f, 4.0f);
        NewActor->GetTransform().SetTranslation(8.0f, 1.975f, -8.0f);

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            NewComponent->SetMesh(CubeMesh);
            NewComponent->SetMaterial(BasicMaterial);
            NewActor->AddComponent(NewComponent);
        }
    }

    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Cube 1");
        NewActor->GetTransform().SetScale(0.5f, 1.0f, 0.5f);
        NewActor->GetTransform().SetTranslation(8.0f, 0.5f, -7.0f);

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            NewComponent->SetMesh(CubeMesh);
            NewComponent->SetMaterial(BasicMaterial);
            NewActor->AddComponent(NewComponent);
        }
    }

    if (FActor* NewActor = InWorld->CreateActor())
    {
        NewActor->SetName("Cube 2");
        NewActor->GetTransform().SetScale(0.5f, 1.0f, 0.5f);
        NewActor->GetTransform().SetTranslation(8.0f, 0.5f, -9.0f);

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            NewComponent->SetMesh(CubeMesh);
            NewComponent->SetMaterial(BasicMaterial);
            NewActor->AddComponent(NewComponent);
        }
    }

    // Add PointLight
    const float Intensity = 10.0f;
    if (FPointLight* PointLight = NewObject<FPointLight>())
    {
        PointLight->SetPosition(FVector3(8.0f, 1.0f, -8.0f));
        PointLight->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        PointLight->SetShadowBias(0.006f);
        PointLight->SetShadowNearPlane(0.01f);
        PointLight->SetShadowFarPlane(30.0f);
        PointLight->SetIntensity(Intensity);
        PointLight->SetShadowCaster(true);

        InWorld->AddLight(PointLight);
    }

    // Load Skybox
    FRHITextureRef Skybox = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Textures/arches.hdr");
    if (!Skybox)
    {
        DEBUG_BREAK();
        return false;
    }

    // Add Camera
    if (FSandboxPlayerController* Player = NewObject<FSandboxPlayerController>())
    {
        Player->SetName("PlayerController");

        // Add camera to the world
        InWorld->AddCamera(Player->GetCamera());
        InWorld->AddActor(Player);

        // Add Skybox
        if (FSkyboxComponent* SkyboxComponent = NewObject<FSkyboxComponent>())
        {
            // Set skybox cube-map
            SkyboxComponent->SetCubeMap(Skybox);
            Player->AddComponent(SkyboxComponent);
        }
    }

    // Add SkyLight
    if (FSkyLight* SkyLight = NewObject<FSkyLight>())
    {
        SkyLight->SetCubeMap(Skybox);
        InWorld->AddLight(SkyLight);
    }

    // Add DirectionalLight
    if (FDirectionalLight* DirectionalLight = NewObject<FDirectionalLight>())
    {
        DirectionalLight->SetShadowBias(0.0005f);
        DirectionalLight->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        DirectionalLight->SetIntensity(50.0f);
        DirectionalLight->SetRotation(FVector3(Math::DegreesToRadians(45.0f), 0.0f, 0.0f));

        InWorld->AddLight(DirectionalLight);
    }

    return true;
}

FRHITextureRef FSandbox::LoadCubeMapFromPanorama(const FString& Filename)
{
    FTexture2DRef Panorama = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture(Filename, false));
    if (!Panorama)
    {
        DEBUG_BREAK();
        return nullptr;
    }
    else
    {
        Panorama->SetDebugName(Filename);
    }

    // Convert the Panorama into a cube-map
    FRHITextureRef PanoramaRHI = Panorama->GetRHITexture();
    if (!PanoramaRHI)
    {
        DEBUG_BREAK();
        return nullptr;
    }

    const uint32 SkyboxSize   = 1024;
    const uint32 NumMiplevels = FTextureFactoryHelpers::TextureSizeToMiplevels(SkyboxSize);

    constexpr ETextureUsageFlags TextureFlags = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture;
    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTextureCube(EFormat::R16G16B16A16_Float, SkyboxSize, NumMiplevels, 1, TextureFlags);

    FRHITextureRef Skybox = FRHI::Get()->CreateTexture(TextureInfo);
    if (!Skybox)
    {
        DEBUG_BREAK();
        return nullptr;
    }

    const bool bResult = FTextureFactory::Get().TextureCubeFromPanorma(PanoramaRHI.Get(), Skybox.Get(), ETextureFactoryFlags::GenerateMips);
    if (!bResult)
    {
        return nullptr;
    }
    else
    {
        Skybox->SetDebugName("Skybox Uncompressed");
    }

    // Unload the panorama
    FAssetManager::Get().UnloadTexture(Panorama);

    // Compress the CubeMap
    FRHITextureRef CompressedSkybox;

    FTextureCompressor& TextureCompressor = FTextureFactory::Get().GetTextureCompressor();
    TextureCompressor.CompressCubeMapBC6(Skybox, CompressedSkybox);

    if (CompressedSkybox)
    {
        CompressedSkybox->SetDebugName("Skybox Compressed");
    }

    return CompressedSkybox;
}

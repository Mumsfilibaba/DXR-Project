#include "Sandbox.h"
#include "SandboxPlayer.h"
#include "GameComponents.h"
#include <Core/Math/Math.h>
#include <Core/Misc/OutputDeviceLogger.h>
#include <Engine/Engine.h>
#include <Engine/Assets/AssetManager.h>
#include <Engine/World/World.h>
#include <Engine/World/Actors/Actors.h>
#include <Engine/World/Components/Components.h>
#include <RendererCore/TextureFactory.h>
#include <RendererCore/TextureHelpers.h>
#include <Renderer/FrameResources.h>
#include <Application/Application.h>

// TODO: Custom random
#include <random>

#define LOAD_LIGHT_SANDBOX (0)
#define LOAD_SPONZA (1)
#define LOAD_BISTRO (0)
#define LOAD_SUN_TEMPLE (0)
#define LOAD_EMERALD_SQUARE (0)

#define ENABLE_LIGHT_TEST (0)
#define ENABLE_SPHERES_TEST (0)

static void TryCompressBC1(FTextureCompressor& Compressor, FRHITextureRef& Texture)
{
    if (Texture && !IsBlockCompressed(Texture->GetDesc().Format) && IsBlockCompressedAligned(Texture->GetDesc().Extent.X) && 
        IsBlockCompressedAligned(Texture->GetDesc().Extent.Y))
    {
        FRHITextureRef Compressed;
        if (Compressor.CompressBC1(Texture, Compressed))
        {
            Texture = Compressed;
        }
    }
}

static void TryCompressBC5(FTextureCompressor& Compressor, FRHITextureRef& Texture)
{
    if (Texture && !IsBlockCompressed(Texture->GetDesc().Format) && IsBlockCompressedAligned(Texture->GetDesc().Extent.X) && 
        IsBlockCompressedAligned(Texture->GetDesc().Extent.Y))
    {
        FRHITextureRef Compressed;
        if (Compressor.CompressBC5(Texture, Compressed))
        {
            Texture = Compressed;
        }
    }
}

static bool AddSandboxPlayer(FWorld* World, const FRHITextureRef& Skybox)
{
    FCameraActor* CameraActor = World->SpawnActor<FCameraActor>(Vector3(0.0f, 10.0f, -2.0f), Vector3(0.0f, 0.0f, 0.0f));
    FSandboxPlayerController* Player = World->SpawnActor<FSandboxPlayerController>();

    if (!CameraActor || !Player)
    {
        return false;
    }

    CameraActor->SetName("Main Camera");
    Player->SetName("PlayerController");
    Player->SetCameraActor(CameraActor);
    World->SetActiveCamera(CameraActor->GetCameraComponent());

    if (FSkyboxComponent* SkyboxComponent = NewObject<FSkyboxComponent>())
    {
        SkyboxComponent->SetCubeMap(Skybox);
        Player->AddComponent(SkyboxComponent);
    }

    return true;
}

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
        if (Material->HasAlphaMask())
        {
            Material->EnableDoubleSided(true);
        }
    }

    Sponza->AddToWorld(InWorld);

    // Create Spheres
    FMeshCreateInfo SphereMeshInfo = MeshFactory::CreateSphere(3);

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
                NewActor->SetName(String::CreateFormatted("Sphere[%d]", SphereIndex));
                SphereIndex++;

                FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
                if (NewComponent)
                {
                    TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
                    NewMaterial->AlbedoMap    = FEngine::Get()->BaseTexture;
                    NewMaterial->MaterialMap  = FEngine::Get()->BaseTexture;

                    NewMaterial->Initialize();
                    NewMaterial->SetName(String::CreateFormatted("Sphere Material %d", SphereIndex));

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
                    NewComponent->Material->MaterialMap = FEngine::Get()->BaseTexture;
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
        MaterialInfo.MaterialFlags   |= EMaterialFlags::EnableHeight | EMaterialFlags::EnableNormalMapping | EMaterialFlags::EnableParallaxClipping | EMaterialFlags::NormalMapPositiveY;

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            FTexture2DRef AlbedoMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Albedo.png")));
            FTexture2DRef NormalMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Normal.png")));
            FTexture2DRef AOMap        = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_AO.png")));
            FTexture2DRef RoughnessMap = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Roughness.png")));
            FTexture2DRef HeightMap    = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Height.png")));
            FTexture2DRef MetallicMap  = StaticCastSharedRef<FTexture2D>(FAssetManager::Get().LoadTexture((ENGINE_LOCATION"/Assets/Textures/Gate_Metallic.png")));

            FRHITextureRef PackedMaterial;
            FTextureFactory::Get().PackMaterialParamsTexture(AOMap->GetRHITexture(), RoughnessMap->GetRHITexture(), MetallicMap->GetRHITexture(), PackedMaterial);

            TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
            NewMaterial->AlbedoMap    = AlbedoMap->GetRHITexture();
            NewMaterial->NormalMap    = NormalMap->GetRHITexture();
            NewMaterial->HeightMap    = HeightMap->GetRHITexture();
            NewMaterial->MaterialMap  = PackedMaterial;

            FTextureCompressor& Compressor = FTextureFactory::Get().GetTextureCompressor();
            TryCompressBC1(Compressor, NewMaterial->AlbedoMap);
            TryCompressBC5(Compressor, NewMaterial->NormalMap);
            TryCompressBC1(Compressor, NewMaterial->MaterialMap);

            NewMaterial->Initialize();
            NewMaterial->SetName("GateMaterial");

            FMeshCreateInfo CubeMeshData = MeshFactory::CreateCube();
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
        MaterialInfo.Roughness        = 0.2f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::None;

        FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
        if (NewComponent)
        {
            TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
            NewMaterial->AlbedoMap    = FEngine::Get()->BaseTexture;
            NewMaterial->MaterialMap  = FEngine::Get()->BaseTexture;

            NewMaterial->Initialize();
            NewMaterial->SetName("PlaneMaterial");

            FMeshCreateInfo PlaneMeshData = MeshFactory::CreatePlane(10, 10);
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
            NewMaterial->MaterialMap  = FEngine::Get()->BaseTexture;

            NewMaterial->Initialize();
            NewMaterial->SetName("ConeMaterial");

            FMeshCreateInfo ConeMeshData = MeshFactory::CreateCone(32, 0.5f);

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
            NewMaterial->MaterialMap  = FEngine::Get()->BaseTexture;

            NewMaterial->Initialize();
            NewMaterial->SetName("TorusMaterial");

            FMeshCreateInfo TorusMeshData = MeshFactory::CreateTorus(1.0f, 0.4f, 48, 32);

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
            NewMaterial->MaterialMap  = FEngine::Get()->BaseTexture;

            NewMaterial->Initialize();
            NewMaterial->SetName("TeapotMaterial");

            FMeshCreateInfo TeapotMeshData = MeshFactory::CreateTeapot(12);

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
            NewMaterial->MaterialMap  = FEngine::Get()->BaseTexture;

            NewMaterial->Initialize();
            NewMaterial->SetName("PyramidMaterial");

            FMeshCreateInfo PyramidMeshData = MeshFactory::CreatePyramid(2.0f, 2.0f, 2.0f);

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

        FRHITextureRef StreetLightPackedMaterial;
        FTextureFactory::Get().PackMaterialParamsTexture(FEngine::Get()->BaseTexture, RoughnessMap->GetRHITexture(), MetallicMap->GetRHITexture(), StreetLightPackedMaterial);

        MaterialInfo.Albedo           = FFloatColor::White;
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 1.0f;
        MaterialInfo.Roughness        = 1.0f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::EnableNormalMapping;

        TSharedPtr<FMaterial> StreetLightMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
        StreetLightMaterial->AlbedoMap    = AlbedoMap->GetRHITexture();
        StreetLightMaterial->NormalMap   = NormalMap->GetRHITexture();
        StreetLightMaterial->MaterialMap = StreetLightPackedMaterial;

        FTextureCompressor& Compressor = FTextureFactory::Get().GetTextureCompressor();
        TryCompressBC1(Compressor, StreetLightMaterial->AlbedoMap);
        TryCompressBC5(Compressor, StreetLightMaterial->NormalMap);
        TryCompressBC1(Compressor, StreetLightMaterial->MaterialMap);

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
                    NewActor->SetName(String::CreateFormatted("Street Light (%s) %d", *Mesh->GetName(), i));
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
    CylinderMaterial->MaterialMap  = FEngine::Get()->BaseTexture;

    CylinderMaterial->Initialize();
    CylinderMaterial->SetName("CylinderMaterial");

    FMeshCreateInfo CylinderMeshData = MeshFactory::CreateCylinder(32, 0.4f, 5.0f);

    TSharedPtr<FMesh> CylinderMesh = MakeSharedPtr<FMesh>();
    CylinderMesh->Init(CylinderMeshData);

    constexpr uint32 NumCylinders = 8;
    for (uint32 i = 0; i < NumCylinders; i++)
    {
        if (FActor* NewActor = InWorld->CreateActor())
        {
            NewActor->SetName(String::CreateFormatted("Cylinder %d", i));
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

    if (!AddSandboxPlayer(InWorld, Skybox))
    {
        return false;
    }

    // Add PointLights
    const float Intensity      = 80.0f;
    const float ShadowFarPlane = 35.0f;

    if (FPointLightActor* PointLightActor0 = InWorld->SpawnActor<FPointLightActor>(Vector3(15.0f, 2.5f, 0.0f), true))
    {
        FPointLightComponent* PointLight0 = PointLightActor0->GetLightComponent();
        PointLight0->SetColor(Vector3(1.0f, 1.0f, 1.0f));
        PointLight0->SetShadowBias(0.02f);
        PointLight0->SetShadowFarPlane(ShadowFarPlane);
        PointLight0->SetIntensity(Intensity);
    }

    if (FPointLightActor* PointLightActor1 = InWorld->SpawnActor<FPointLightActor>(Vector3(-15.0f, 2.5f, 0.0f), true))
    {
        FPointLightComponent* PointLight1 = PointLightActor1->GetLightComponent();
        PointLight1->SetColor(Vector3(1.0f, 1.0f, 1.0f));
        PointLight1->SetShadowBias(0.02f);
        PointLight1->SetShadowFarPlane(ShadowFarPlane);
        PointLight1->SetIntensity(Intensity);
    }

    if (FPointLightActor* PointLightActor2 = InWorld->SpawnActor<FPointLightActor>(Vector3(17.0f, 10.0f, 6.0f), true))
    {
        FPointLightComponent* PointLight2 = PointLightActor2->GetLightComponent();
        PointLight2->SetColor(Vector3(1.0f, 1.0f, 1.0f));
        PointLight2->SetShadowBias(0.02f);
        PointLight2->SetShadowFarPlane(ShadowFarPlane);
        PointLight2->SetIntensity(Intensity);
    }

    if (FPointLightActor* PointLightActor3 = InWorld->SpawnActor<FPointLightActor>(Vector3(-18.0f, 10.0f, 6.0f), true))
    {
        FPointLightComponent* PointLight3 = PointLightActor3->GetLightComponent();
        PointLight3->SetColor(Vector3(1.0f, 1.0f, 1.0f));
        PointLight3->SetShadowBias(0.02f);
        PointLight3->SetShadowFarPlane(ShadowFarPlane);
        PointLight3->SetIntensity(Intensity);
    }

    if (FPointLightActor* PointLightActor4 = InWorld->SpawnActor<FPointLightActor>(Vector3(17.0f, 10.0f, -7.0f), true))
    {
        FPointLightComponent* PointLight4 = PointLightActor4->GetLightComponent();
        PointLight4->SetColor(Vector3(1.0f, 1.0f, 1.0f));
        PointLight4->SetShadowBias(0.02f);
        PointLight4->SetShadowFarPlane(ShadowFarPlane);
        PointLight4->SetIntensity(Intensity);
    }

    if (FPointLightActor* PointLightActor5 = InWorld->SpawnActor<FPointLightActor>(Vector3(-18.0f, 10.0f, -7.0f), true))
    {
        FPointLightComponent* PointLight5 = PointLightActor5->GetLightComponent();
        PointLight5->SetColor(Vector3(1.0f, 1.0f, 1.0f));
        PointLight5->SetShadowBias(0.02f);
        PointLight5->SetShadowFarPlane(ShadowFarPlane);
        PointLight5->SetIntensity(Intensity);
    }

#if ENABLE_LIGHT_TEST
    {
        // Add multiple lights
        std::uniform_real_distribution<float> RandomFloats(0.0f, 1.0f);
        std::default_random_engine            Generator;

        for (uint32 i = 0; i < 256; i++)
        {
            float x          = RandomFloats(Generator) * 35.0f - 17.5f;
            float y          = RandomFloats(Generator) * 22.0f;
            float z          = RandomFloats(Generator) * 16.0f - 8.0f;
            float Intentsity = RandomFloats(Generator) * 5.0f + 1.0f;

            if (FPointLightActor* LightActor = InWorld->SpawnActor<FPointLightActor>(Vector3(x, y, z), false))
            {
                FPointLightComponent* Light = LightActor->GetLightComponent();
                Light->SetColor(RandomFloats(Generator), RandomFloats(Generator), RandomFloats(Generator));
                Light->SetIntensity(Intentsity);
            }
        }
    }
#endif

    // Add SkyLight
    InWorld->SpawnActor<FSkyLightActor>(Skybox);

    // Add DirectionalLight
    if (FDirectionalLightActor* DirectionalLightActor = InWorld->SpawnActor<FDirectionalLightActor>(
        Vector3(Math::DegreesToRadians(35.0f), Math::DegreesToRadians(135.0f), 0.0f)))
    {
        FDirectionalLightComponent* DirectionalLight = DirectionalLightActor->GetLightComponent();
        DirectionalLight->SetShadowBias(0.0005f);
        DirectionalLight->SetColor(Vector3(1.0f, 1.0f, 1.0f));
        DirectionalLight->SetIntensity(50.0f);
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
        if (Material->GetName().Contains("DoubleSided"))
        {
            Material->SetMaterialFlags(EMaterialFlags::EnableAlpha | EMaterialFlags::DoubleSided, true);
        }
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
        if (Material->GetName().Contains("DoubleSided"))
        {
            Material->SetMaterialFlags(EMaterialFlags::EnableAlpha | EMaterialFlags::DoubleSided, true);
        }
    }

    BistroExterior->AddToWorld(InWorld);

    // Load Skybox
    FRHITextureRef Skybox = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Scenes/Bistro/san_giuseppe_bridge_4k.hdr");
    if (!Skybox)
    {
        DEBUG_BREAK();
        return false;
    }

    if (!AddSandboxPlayer(InWorld, Skybox))
    {
        return false;
    }

    // Add SkyLight
    InWorld->SpawnActor<FSkyLightActor>(Skybox);

    // Add DirectionalLight
    if (FDirectionalLightActor* DirectionalLightActor = InWorld->SpawnActor<FDirectionalLightActor>(
        Vector3(Math::DegreesToRadians(35.0f), Math::DegreesToRadians(135.0f), 0.0f)))
    {
        FDirectionalLightComponent* DirectionalLight = DirectionalLightActor->GetLightComponent();
        DirectionalLight->SetShadowBias(0.0005f);
        DirectionalLight->SetColor(Vector3(1.0f, 1.0f, 1.0f));
        DirectionalLight->SetIntensity(50.0f);
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
        if (Material->GetName().Contains("DoubleSided"))
        {
            Material->SetMaterialFlags(EMaterialFlags::EnableAlpha | EMaterialFlags::DoubleSided, true);
        }
    }

    SunTemple->AddToWorld(InWorld);

    // Load Skybox
    FRHITextureRef Skybox = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Scenes/SunTemple/SunTemple_Skybox.hdr");
    if (!Skybox)
    {
        DEBUG_BREAK();
        return false;
    }

    if (!AddSandboxPlayer(InWorld, Skybox))
    {
        return false;
    }

    // Add SkyLight
    InWorld->SpawnActor<FSkyLightActor>(Skybox);

    // Load Reflection Probe
    FRHITextureRef ReflectionProbeExterior = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Scenes/SunTemple/SunTemple_Reflection.hdr");
    if (!ReflectionProbeExterior)
    {
        DEBUG_BREAK();
        return false;
    }

    // Add Light-Probes
    if (FLightProbeActor* LightProbeActor = InWorld->SpawnActor<FLightProbeActor>(Vector3(0.0f, 13.0f, 0.5f), ReflectionProbeExterior))
    {
        FLightProbeComponent* LightProbe = LightProbeActor->GetLightProbeComponent();
        LightProbe->SetBoxExtent(Vector3(19.0f, 21.5f, 22.0f));
        LightProbe->SetBoxOffset(Vector3(0.0f, 0.0f, 0.0f));
        LightProbe->SetBoxProjection(true);
    }

    // Load Reflection Probe
    FRHITextureRef ReflectionProbeInterior = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Scenes/SunTemple/SunTemple_Reflection_Interior.hdr");
    if (!ReflectionProbeInterior)
    {
        DEBUG_BREAK();
        return false;
    }

    // Add Light-Probes
    if (FLightProbeActor* LightProbeActor = InWorld->SpawnActor<FLightProbeActor>(Vector3(0.0f, 6.0f, 30.0f), ReflectionProbeInterior))
    {
        FLightProbeComponent* LightProbe = LightProbeActor->GetLightProbeComponent();
        LightProbe->SetBoxExtent(Vector3(19.0f, 21.5f, 22.0f));
        LightProbe->SetBoxOffset(Vector3(0.0f, 0.0f, 0.0f));
        LightProbe->SetBoxProjection(true);
    }

    // Add DirectionalLight
    if (FDirectionalLightActor* DirectionalLightActor = InWorld->SpawnActor<FDirectionalLightActor>(
        Vector3(Math::DegreesToRadians(-55.0f), Math::DegreesToRadians(325.0f), 0.0f)))
    {
        FDirectionalLightComponent* DirectionalLight = DirectionalLightActor->GetLightComponent();
        DirectionalLight->SetShadowBias(0.0005f);
        DirectionalLight->SetColor(Vector3(1.0f, 1.0f, 1.0f));
        DirectionalLight->SetIntensity(50.0f);
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
        if (Material->GetName().Contains("DoubleSided"))
        {
            Material->SetMaterialFlags(EMaterialFlags::EnableAlpha | EMaterialFlags::DoubleSided, true);
        }
    }

    EmeraldSquare_Day->AddToWorld(InWorld);

    // Load Skybox
    FRHITextureRef Skybox = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Scenes/EmeraldSquare/symmetrical_garden_4k.hdr");
    if (!Skybox)
    {
        DEBUG_BREAK();
        return false;
    }

    if (!AddSandboxPlayer(InWorld, Skybox))
    {
        return false;
    }

    // Add SkyLight
    InWorld->SpawnActor<FSkyLightActor>(Skybox);

    // Add DirectionalLight
    if (FDirectionalLightActor* DirectionalLightActor = InWorld->SpawnActor<FDirectionalLightActor>(
        Vector3(Math::DegreesToRadians(35.0f), Math::DegreesToRadians(135.0f), 0.0f)))
    {
        FDirectionalLightComponent* DirectionalLight = DirectionalLightActor->GetLightComponent();
        DirectionalLight->SetShadowBias(0.0005f);
        DirectionalLight->SetColor(Vector3(1.0f, 1.0f, 1.0f));
        DirectionalLight->SetIntensity(50.0f);
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
    BasicMaterial->MaterialMap  = FEngine::Get()->BaseTexture;

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
            FMeshCreateInfo PlaneMeshData = MeshFactory::CreatePlane(10, 10);

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
            FMeshCreateInfo CylinderMeshData = MeshFactory::CreateCylinder(16, 0.5f, 2.0f);

            TSharedPtr<FMesh> CylinderMesh = MakeSharedPtr<FMesh>();
            CylinderMesh->Init(CylinderMeshData);

            NewComponent->SetMesh(CylinderMesh);
            NewComponent->SetMaterial(BasicMaterial);
            NewActor->AddComponent(NewComponent);
        }
    }

    // Create small hut
    FMeshCreateInfo CubeMeshData = MeshFactory::CreateCube();

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
    if (FPointLightActor* PointLightActor = InWorld->SpawnActor<FPointLightActor>(Vector3(8.0f, 1.0f, -8.0f), true))
    {
        FPointLightComponent* PointLight = PointLightActor->GetLightComponent();
        PointLight->SetColor(Vector3(1.0f, 1.0f, 1.0f));
        PointLight->SetShadowBias(0.006f);
        PointLight->SetShadowNearPlane(0.01f);
        PointLight->SetShadowFarPlane(30.0f);
        PointLight->SetIntensity(Intensity);
    }

    // Load Skybox
    FRHITextureRef Skybox = LoadCubeMapFromPanorama(ENGINE_LOCATION"/Assets/Textures/arches.hdr");
    if (!Skybox)
    {
        DEBUG_BREAK();
        return false;
    }

    if (!AddSandboxPlayer(InWorld, Skybox))
    {
        return false;
    }

    // Add SkyLight
    InWorld->SpawnActor<FSkyLightActor>(Skybox);

    // Add DirectionalLight
    if (FDirectionalLightActor* DirectionalLightActor = InWorld->SpawnActor<FDirectionalLightActor>(Vector3(Math::DegreesToRadians(45.0f), 0.0f, 0.0f)))
    {
        FDirectionalLightComponent* DirectionalLight = DirectionalLightActor->GetLightComponent();
        DirectionalLight->SetShadowBias(0.0005f);
        DirectionalLight->SetColor(Vector3(1.0f, 1.0f, 1.0f));
        DirectionalLight->SetIntensity(50.0f);
    }

    return true;
}

FRHITextureRef FSandbox::LoadCubeMapFromPanorama(const String& Filename)
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
    const uint32 NumMiplevels = TextureHelpers::TextureSizeToMiplevels(SkyboxSize);

    constexpr ETextureUsageFlags TextureFlags = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture;
    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTextureCube(EFormat::R16G16B16A16_Float, SkyboxSize, NumMiplevels, 1, TextureFlags);

    FRHITextureRef Skybox = RHI::CreateTexture(TextureDesc);
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

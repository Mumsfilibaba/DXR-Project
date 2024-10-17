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
#include <Engine/World/Actors/PlayerController.h>
#include <Engine/World/Components/MeshComponent.h>
#include <Application/Application.h>

// TODO: Custom random
#include <random>

#define LOAD_SPONZA (1)
#define LOAD_BISTRO (0)
#define LOAD_SUN_TEMPLE (0)
#define LOAD_EMERALD_SQUARE (0)

#define ENABLE_LIGHT_TEST (0)
#define ENABLE_MANY_SPHERES (0)

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
    // Initialize World
    MAYBE_UNUSED FActor*         NewActor     = nullptr;
    MAYBE_UNUSED FMeshComponent* NewComponent = nullptr;

    // Store the Engine's world pointer 
    FWorld* CurrentWorld = GEngine->GetWorld();

    // Load Scene
    {
    #if LOAD_SPONZA
        TSharedRef<FModel> Sponza = FAssetManager::Get().LoadModel((ENGINE_LOCATION"/Assets/Scenes/Sponza/Sponza.obj"));
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

        Sponza->AddToWorld(CurrentWorld);
    #elif LOAD_BISTRO
        TSharedRef<FModel> BistroInterior = FAssetManager::Get().LoadModel((ENGINE_LOCATION"/Assets/Scenes/Bistro/BistroInterior.fbx"));
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
        
        BistroInterior->AddToWorld(CurrentWorld);

        TSharedRef<FModel> BistroExterior = FAssetManager::Get().LoadModel((ENGINE_LOCATION"/Assets/Scenes/Bistro/BistroExterior.fbx"));
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
        
        BistroExterior->AddToWorld(CurrentWorld);
     #elif LOAD_SUN_TEMPLE
        TSharedRef<FModel> SunTemple = FAssetManager::Get().LoadModel((ENGINE_LOCATION"/Assets/Scenes/SunTemple/SunTemple.fbx"));
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
        
        SunTemple->AddToWorld(CurrentWorld);
     #elif LOAD_EMERALD_SQUARE
        TSharedRef<FModel> EmeraldSquare_Day = FAssetManager::Get().LoadModel((ENGINE_LOCATION"/Assets/Scenes/EmeraldSquare/EmeraldSquare_Day.fbx"));
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
        
        EmeraldSquare_Day->AddToWorld(CurrentWorld);
    #endif
    }

#if LOAD_SPONZA
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
    MaterialInfo.Albedo           = FVector3(1.0f);
    MaterialInfo.AmbientOcclusion = 1.0f;
    MaterialInfo.MaterialFlags    = EMaterialFlags::None;

    uint32 SphereIndex = 0;
    for (uint32 y = 0; y < SphereCountY; y++)
    {
        for (uint32 x = 0; x < SphereCountX; x++)
        {
            NewActor = CurrentWorld->CreateActor();
            if (NewActor)
            {
                NewActor->GetTransform().SetTranslation(StartPositionX + (x * SphereOffset), 0.6f, 40.0f + StartPositionY + (y * SphereOffset));
                NewActor->SetName(FString::CreateFormatted("Sphere[%d]", SphereIndex));
                SphereIndex++;

                NewComponent = NewObject<FMeshComponent>();
                if (NewComponent)
                {
                    TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
                    NewMaterial->AlbedoMap    = GEngine->BaseTexture;
                    NewMaterial->RoughnessMap = GEngine->BaseTexture;
                    NewMaterial->AOMap        = GEngine->BaseTexture;
                    NewMaterial->MetallicMap  = GEngine->BaseTexture;
                    
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
                NewComponent = NewObject<FMeshComponent>();
                if (NewComponent)
                {
                    NewComponent->Initialize(NewActor, MakeShared<FMaterial>(MaterialInfo), SphereMesh);

                    NewComponent->Material->AlbedoMap    = GEngine->BaseTexture;
                    NewComponent->Material->NormalMap    = GEngine->BaseNormal;
                    NewComponent->Material->RoughnessMap = GEngine->BaseTexture;
                    NewComponent->Material->AOMap        = GEngine->BaseTexture;
                    NewComponent->Material->MetallicMap  = GEngine->BaseTexture;
                    NewComponent->Material->Initialize();

                    NewActor->AddComponent(NewComponent);
                }
            }

            MaterialInfo.Roughness = Random2(Generator);
            MaterialInfo.Metallic  = Random3(Generator);
        }
    }
#endif

    // Create Other Meshes
    NewActor = CurrentWorld->CreateActor();
    if (NewActor)
    {
        NewActor->SetName("Cube");
        NewActor->GetTransform().SetTranslation(0.0f, 2.0f, 50.0f);

        MaterialInfo.Albedo           = FVector3(1.0f);
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 1.0f;
        MaterialInfo.Roughness        = 1.0f;
        MaterialInfo.MaterialFlags   |= EMaterialFlags::EnableHeight | EMaterialFlags::EnableNormalMapping;

        NewComponent = NewObject<FMeshComponent>();
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

    NewActor = CurrentWorld->CreateActor();
    if (NewActor)
    {
        NewActor->SetName("Plane");
        NewActor->GetTransform().SetRotation(FMath::kHalfPI_f, 0.0f, 0.0f);
        NewActor->GetTransform().SetUniformScale(50.0f);
        NewActor->GetTransform().SetTranslation(0.0f, 0.0f, 42.0f);

        MaterialInfo.Albedo           = FVector3(1.0f);
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 1.0f;
        MaterialInfo.Roughness        = 0.5f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::None;

        NewComponent = NewObject<FMeshComponent>();
        if (NewComponent)
        {
            TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
            NewMaterial->AlbedoMap    = GEngine->BaseTexture;
            NewMaterial->RoughnessMap = GEngine->BaseTexture;
            NewMaterial->AOMap        = GEngine->BaseTexture;
            NewMaterial->MetallicMap  = GEngine->BaseTexture;
            
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
    
    NewActor = CurrentWorld->CreateActor();
    if (NewActor)
    {
        NewActor->SetName("Cone");
        NewActor->GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
        NewActor->GetTransform().SetUniformScale(3.0f);
        NewActor->GetTransform().SetTranslation(-15.0f, 0.5f, 50.0f);

        MaterialInfo.Albedo           = FVector3(0.0f, 1.0f, 0.0f);
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 0.0f;
        MaterialInfo.Roughness        = 1.0f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::None;

        NewComponent = NewObject<FMeshComponent>();
        if (NewComponent)
        {
            TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
            NewMaterial->AlbedoMap    = GEngine->BaseTexture;
            NewMaterial->RoughnessMap = GEngine->BaseTexture;
            NewMaterial->AOMap        = GEngine->BaseTexture;
            NewMaterial->MetallicMap  = GEngine->BaseTexture;
            
            NewMaterial->Initialize();
            NewMaterial->SetName("ConeMaterial");

            FMeshCreateInfo ConeMeshData = FMeshFactory::CreateCone(16, 0.5f);
            
            TSharedPtr<FMesh> ConeMesh = MakeSharedPtr<FMesh>();
            ConeMesh->Init(ConeMeshData);
            
            NewComponent->SetMesh(ConeMesh);
            NewComponent->SetMaterial(NewMaterial);

            NewActor->AddComponent(NewComponent);
        }
    }
    
    NewActor = CurrentWorld->CreateActor();
    if (NewActor)
    {
        NewActor->SetName("Torus");
        NewActor->GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
        NewActor->GetTransform().SetUniformScale(1.0f);
        NewActor->GetTransform().SetTranslation(-15.0f, 1.0f, 45.0f);

        MaterialInfo.Albedo           = FVector3(1.0f, 0.0f, 0.0f);
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 0.0f;
        MaterialInfo.Roughness        = 1.0f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::None;

        NewComponent = NewObject<FMeshComponent>();
        if (NewComponent)
        {
            TSharedPtr<FMaterial> NewMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
            NewMaterial->AlbedoMap    = GEngine->BaseTexture;
            NewMaterial->RoughnessMap = GEngine->BaseTexture;
            NewMaterial->AOMap        = GEngine->BaseTexture;
            NewMaterial->MetallicMap  = GEngine->BaseTexture;
            
            NewMaterial->Initialize();
            NewMaterial->SetName("TorusMaterial");

            FMeshCreateInfo TorusMeshData = FMeshFactory::CreateTorus();
            
            TSharedPtr<FMesh> TorusMesh = MakeSharedPtr<FMesh>();
            TorusMesh->Init(TorusMeshData);
            
            NewComponent->SetMesh(TorusMesh);
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
        
        MaterialInfo.Albedo           = FVector3(1.0f);
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 1.0f;
        MaterialInfo.Roughness        = 1.0f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::EnableNormalMapping;

        TSharedPtr<FMaterial> StreetLightMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
        StreetLightMaterial->AlbedoMap    = AlbedoMap->GetRHITexture();
        StreetLightMaterial->NormalMap    = NormalMap->GetRHITexture();
        StreetLightMaterial->RoughnessMap = RoughnessMap->GetRHITexture();
        StreetLightMaterial->AOMap        = GEngine->BaseTexture;
        StreetLightMaterial->MetallicMap  = MetallicMap->GetRHITexture();

        StreetLightMaterial->Initialize();
        StreetLightMaterial->SetName("StreetLightMaterial");

        const int32 NumMeshes = StreetLightModel->GetNumMeshes();
        for (uint32 i = 0; i < 4; i++)
        {
            for (int32 MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++)
            {
                NewActor = CurrentWorld->CreateActor();
                if (NewActor)
                {
                    const TSharedPtr<FMesh>& Mesh = StreetLightModel->GetMesh(MeshIndex);
                    NewActor->SetName(FString::CreateFormatted("Street Light (%s) %d", Mesh->GetName().GetCString(), i));
                    NewActor->GetTransform().SetUniformScale(0.25f);
                    NewActor->GetTransform().SetTranslation(15.0f, 0.0f, 55.0f - float(i) * 3.0f);

                    NewComponent = NewObject<FMeshComponent>();
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

    TSharedRef<FModel> PillarModel = FAssetManager::Get().LoadModel((ENGINE_LOCATION"/Assets/Models/Pillar.obj"));
    if (PillarModel)
    {
        MaterialInfo.Albedo           = FVector3(0.4f);
        MaterialInfo.AmbientOcclusion = 1.0f;
        MaterialInfo.Metallic         = 0.0f;
        MaterialInfo.Roughness        = 1.0f;
        MaterialInfo.MaterialFlags    = EMaterialFlags::None;

        TSharedPtr<FMaterial> PillarMaterial = MakeSharedPtr<FMaterial>(MaterialInfo);
        PillarMaterial->AlbedoMap    = GEngine->BaseTexture;
        PillarMaterial->RoughnessMap = GEngine->BaseTexture;
        PillarMaterial->AOMap        = GEngine->BaseTexture;
        PillarMaterial->MetallicMap  = GEngine->BaseTexture;
        
        PillarMaterial->Initialize();
        PillarMaterial->SetName("PillarMaterial");

        TSharedPtr<FMesh> Pillar = PillarModel->GetMesh(0);
        for (uint32 i = 0; i < 8; i++)
        {
            NewActor = CurrentWorld->CreateActor();
            if (NewActor)
            {
                NewActor->SetName(FString::CreateFormatted("Pillar %d", i));
                NewActor->GetTransform().SetUniformScale(0.25f);
                NewActor->GetTransform().SetTranslation(-15.0f + float(i) * 1.75f, 0.0f, 60.0f);

                NewComponent = NewObject<FMeshComponent>();
                if (NewComponent)
                {
                    NewComponent->SetMesh(Pillar);
                    NewComponent->SetMaterial(PillarMaterial);
                    NewActor->AddComponent(NewComponent);
                }
            }
        }
    }
#endif

    if (FSandboxPlayerController* Player = NewObject<FSandboxPlayerController>())
    {
        // TODO: Camera should be a component
        CurrentWorld->AddCamera(Player->GetCamera());
        CurrentWorld->AddActor(Player);
    }

    // Add PointLights
#if LOAD_SPONZA
    const float Intensity      = 100.0f;
    const float ShadowFarPlane = 40.0f;
    if (FPointLight* Light0 = NewObject<FPointLight>())
    {
        Light0->SetPosition(FVector3(15.0f, 1.5f, 0.0f));
        Light0->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        Light0->SetShadowBias(0.001f);
        Light0->SetMaxShadowBias(0.009f);
        Light0->SetShadowFarPlane(ShadowFarPlane);
        Light0->SetIntensity(Intensity);
        Light0->SetShadowCaster(true);
        CurrentWorld->AddLight(Light0);
    }
    if (FPointLight* Light1 = NewObject<FPointLight>())
    {
        Light1->SetPosition(FVector3(-15.0f, 1.5f, 0.0f));
        Light1->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        Light1->SetShadowBias(0.001f);
        Light1->SetMaxShadowBias(0.009f);
        Light1->SetShadowFarPlane(ShadowFarPlane);
        Light1->SetIntensity(Intensity);
        Light1->SetShadowCaster(true);
        CurrentWorld->AddLight(Light1);
    }
    if (FPointLight* Light2 = NewObject<FPointLight>())
    {
        Light2->SetPosition(FVector3(17.0f, 10.0f, 6.0f));
        Light2->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        Light2->SetShadowBias(0.001f);
        Light2->SetMaxShadowBias(0.009f);
        Light2->SetShadowFarPlane(ShadowFarPlane);
        Light2->SetIntensity(Intensity);
        Light2->SetShadowCaster(true);
        CurrentWorld->AddLight(Light2);
    }
    if (FPointLight* Light3 = NewObject<FPointLight>())
    {
        Light3->SetPosition(FVector3(-18.0f, 10.0f, 6.0f));
        Light3->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        Light3->SetShadowBias(0.001f);
        Light3->SetMaxShadowBias(0.009f);
        Light3->SetShadowFarPlane(ShadowFarPlane);
        Light3->SetIntensity(Intensity);
        Light3->SetShadowCaster(true);
        CurrentWorld->AddLight(Light3);
    }
    if (FPointLight* Light4 = NewObject<FPointLight>())
    {
        Light4->SetPosition(FVector3(17.0f, 10.0f, -7.0f));
        Light4->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        Light4->SetShadowBias(0.001f);
        Light4->SetMaxShadowBias(0.009f);
        Light4->SetShadowFarPlane(ShadowFarPlane);
        Light4->SetIntensity(Intensity);
        Light4->SetShadowCaster(true);
        CurrentWorld->AddLight(Light4);
    }
    if (FPointLight* Light5 = NewObject<FPointLight>())
    {
        Light5->SetPosition(FVector3(-18.0f, 10.0f, -7.0f));
        Light5->SetColor(FVector3(1.0f, 1.0f, 1.0f));
        Light5->SetShadowBias(0.001f);
        Light5->SetMaxShadowBias(0.009f);
        Light5->SetShadowFarPlane(ShadowFarPlane);
        Light5->SetIntensity(Intensity);
        Light5->SetShadowCaster(true);
        CurrentWorld->AddLight(Light5);
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
                FWorld->AddLight(Light);
            }
        }
    }
#endif

    // Add DirectionalLight
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
        CurrentWorld->AddLight(Light4);
    }

    // if (FLightProbe* LightProbe = NewObject<FLightProbe>())
    // {
    //     LightProbe->SetPosition(FVector3(0.0f));
    //     CurrentWorld->AddLightProbe(LightProbe);
    // }

    LOG_INFO("Finished loading game");
    return true;
}

void FSandbox::Tick(float DeltaTime)
{
}

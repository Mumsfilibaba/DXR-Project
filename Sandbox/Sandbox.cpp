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

// Scene 0 - Sponza
// Scene 1 - SunTemple
// Scene 2 - Bistro
#define SCENE 0

Game* MakeGameInstance()
{
    return DBG_NEW Sandbox();
}

Bool Sandbox::Init()
{
    SceneData SceneBuildData;
#if SCENE == 0
    MeshFactory::LoadSceneFromFile(SceneBuildData, "../Assets/Scenes/Sponza/Sponza.obj");
#elif SCENE == 1
    MeshFactory::LoadSceneFromFile(SceneBuildData, "../Assets/Scenes/SunTemple/SunTemple.fbx");
#elif SCENE == 2
    SceneData Exterior;
    MeshFactory::LoadSceneFromFile(Exterior, "../Assets/Scenes/Bistro/BistroExterior.fbx");
    MeshFactory::CombineScenes(SceneBuildData, Exterior);

    SceneData Interior;
    MeshFactory::LoadSceneFromFile(Interior, "../Assets/Scenes/Bistro/BistroInterior.fbx");
    MeshFactory::CombineScenes(SceneBuildData, Interior);
#endif

    // In order to create fewer meshes and in turn fewer descriptors we bind together all these models
    MeshFactory::MergeSimilarMaterials(SceneBuildData);

    // Initialize Scene
    Actor* NewActor             = nullptr;
    MeshComponent* NewComponent = nullptr;
    CurrentScene = DBG_NEW Scene();

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

    MaterialProperties MatProperties;
    MatProperties.Albedo = XMFLOAT3(1.0f, 1.0f, 1.0f);
    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 1.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 0;

    TSharedPtr<class Material> BaseMaterial = MakeShared<Material>(MatProperties);
    BaseMaterial->AlbedoMap    = BaseTexture;
    BaseMaterial->NormalMap    = BaseNormal;
    BaseMaterial->RoughnessMap = BaseTexture;
    BaseMaterial->HeightMap    = BaseTexture;
    BaseMaterial->AOMap        = BaseTexture;
    BaseMaterial->MetallicMap  = BaseTexture;
    BaseMaterial->Init();

    TArray<TSharedPtr<Material>> LoadedMaterials;
    std::unordered_map<std::string, TRef<Texture2D>> MaterialTextures;

    for (const MaterialData& MaterialData : SceneBuildData.Materials)
    {
        MaterialProperties MaterialProperties;
        MaterialProperties.Metallic  = MaterialData.Metallic;
        MaterialProperties.AO        = MaterialData.AO;
        MaterialProperties.Roughness = MaterialData.Roughness;

        TSharedPtr<Material>& NewMaterial = LoadedMaterials.EmplaceBack(MakeShared<Material>(MaterialProperties));
        LOG_INFO("Loaded materialID=" + std::to_string(LoadedMaterials.Size() - 1));

        NewMaterial->AOMap     = BaseTexture;
        NewMaterial->HeightMap = BaseTexture;

        // Metallic
        if (!MaterialData.MetallicTexname.empty())
        {
            if (MaterialTextures.count(MaterialData.MetallicTexname) == 0)
            {
                std::string TexName = MaterialData.TexPath + '/' + MaterialData.MetallicTexname;
                TRef<Texture2D> Texture = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
                if (Texture)
                {
                    Texture->SetName(MaterialData.MetallicTexname);
                    MaterialTextures[MaterialData.MetallicTexname] = Texture;
                }
                else
                {
                    MaterialTextures[MaterialData.MetallicTexname] = BaseTexture;
                }
            }

            NewMaterial->MetallicMap = MaterialTextures[MaterialData.MetallicTexname];
        }
        else
        {
            NewMaterial->MetallicMap = BaseTexture;
        }

        // Diffuse
        if (!MaterialData.DiffTexName.empty())
        {
            if (MaterialTextures.count(MaterialData.DiffTexName) == 0)
            {
                std::string TexName = MaterialData.TexPath + '/' + MaterialData.DiffTexName;
                TRef<Texture2D> Texture = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
                if (Texture)
                {
                    Texture->SetName(MaterialData.DiffTexName);
                    MaterialTextures[MaterialData.DiffTexName] = Texture;
                }
                else
                {
                    MaterialTextures[MaterialData.DiffTexName] = BaseTexture;
                }
            }

            NewMaterial->AlbedoMap = MaterialTextures[MaterialData.DiffTexName];
        }
        else
        {
            NewMaterial->AlbedoMap = BaseTexture;
        }

        // Roughness
        if (!MaterialData.RoughnessTexname.empty())
        {
            if (MaterialTextures.count(MaterialData.RoughnessTexname) == 0)
            {
                std::string TexName = MaterialData.TexPath + '/' + MaterialData.RoughnessTexname;
                TRef<Texture2D> Texture = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
                if (Texture)
                {
                    Texture->SetName(MaterialData.RoughnessTexname);
                    MaterialTextures[MaterialData.RoughnessTexname] = Texture;
                }
                else
                {
                    MaterialTextures[MaterialData.RoughnessTexname] = BaseTexture;
                }
            }

            NewMaterial->RoughnessMap = MaterialTextures[MaterialData.RoughnessTexname];
        }
        else
        {
            NewMaterial->RoughnessMap = BaseTexture;
        }

        // Normal
        if (!MaterialData.NormalTexname.empty())
        {
            if (MaterialTextures.count(MaterialData.NormalTexname) == 0)
            {
                std::string TexName = MaterialData.TexPath + '/' + MaterialData.NormalTexname;
                TRef<Texture2D> Texture = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
                if (Texture)
                {
                    Texture->SetName(MaterialData.NormalTexname);
                    MaterialTextures[MaterialData.NormalTexname] = Texture;
                }
                else
                {
                    MaterialTextures[MaterialData.NormalTexname] = BaseNormal;
                }
            }

            NewMaterial->NormalMap = MaterialTextures[MaterialData.NormalTexname];
        }
        else
        {
            NewMaterial->NormalMap = BaseNormal;
        }

        // Alpha
        if (!MaterialData.AlphaTexname.empty())
        {
            if (MaterialTextures.count(MaterialData.AlphaTexname) == 0)
            {
                std::string TexName = MaterialData.TexPath + '/' + MaterialData.AlphaTexname;
                TRef<Texture2D> Texture = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
                if (Texture)
                {
                    Texture->SetName(MaterialData.AlphaTexname);
                    MaterialTextures[MaterialData.AlphaTexname] = Texture;
                }
                else
                {
                    MaterialTextures[MaterialData.AlphaTexname] = BaseTexture;
                }
            }

            NewMaterial->AlphaMask = MaterialTextures[MaterialData.AlphaTexname];
        }

        NewMaterial->Init();
    }

    for (const ModelData& Model : SceneBuildData.Models)
    {
        NewActor = DBG_NEW Actor();
        CurrentScene->AddActor(NewActor);

        NewActor->SetName(Model.Name);
        NewActor->GetTransform().SetUniformScale(0.015f);

#if SCENE == 2
        NewActor->GetTransform().SetRotation(0.0f, 0.0f, XMConvertToRadians(-90.0f));
#endif

        NewComponent = DBG_NEW MeshComponent(NewActor);
        NewComponent->Mesh = Mesh::Make(Model.Mesh);
        
        if (Model.MaterialIndex != -1)
        {
            NewComponent->Material = LoadedMaterials[Model.MaterialIndex];
        }
        else
        {
            NewComponent->Material = BaseMaterial;
        }

        NewActor->AddComponent(NewComponent);
    }

    constexpr Float  SphereOffset   = 1.25f;
    constexpr UInt32 SphereCountX   = 8;
    constexpr Float  StartPositionX = (-static_cast<Float>(SphereCountX) * SphereOffset) / 2.0f;
    constexpr UInt32 SphereCountY   = 8;
    constexpr Float  StartPositionY = (-static_cast<Float>(SphereCountY) * SphereOffset) / 2.0f;
    constexpr Float  MetallicDelta  = 1.0f / SphereCountY;
    constexpr Float  RoughnessDelta = 1.0f / SphereCountX;

    MatProperties.Albedo    = XMFLOAT3(1.0f, 1.0f, 1.0f);
    MatProperties.AO        = 1.0f;
    MatProperties.Metallic  = 0.0f;

    UInt32 SphereIndex = 0;
    for (UInt32 y = 0; y < SphereCountY; y++)
    {
        MatProperties.Roughness = 0.05f;

        for (UInt32 x = 0; x < SphereCountX; x++)
        {
            NewActor = DBG_NEW Actor();
            NewActor->GetTransform().SetTranslation(StartPositionX + (x * SphereOffset), 1.0f, 40.0f + StartPositionY + (y * SphereOffset));

            NewActor->SetName("Sphere[" + std::to_string(SphereIndex) + "]");
            SphereIndex++;

            CurrentScene->AddActor(NewActor);

            NewComponent = DBG_NEW MeshComponent(NewActor);
            NewComponent->Mesh     = SphereMesh;
            NewComponent->Material = MakeShared<Material>(MatProperties);

            NewComponent->Material->AlbedoMap    = BaseTexture;
            NewComponent->Material->NormalMap    = BaseNormal;
            NewComponent->Material->RoughnessMap = BaseTexture;
            NewComponent->Material->HeightMap    = BaseTexture;
            NewComponent->Material->AOMap        = BaseTexture;
            NewComponent->Material->MetallicMap  = BaseTexture;
            NewComponent->Material->Init();
            NewActor->AddComponent(NewComponent);

            MatProperties.Roughness += RoughnessDelta;
        }

        MatProperties.Metallic += MetallicDelta;
    }

    // Create Other Meshes
    MeshData CubeMeshData = MeshFactory::CreateCube();

    NewActor = DBG_NEW Actor();
    CurrentScene->AddActor(NewActor);

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
    CurrentScene->AddActor(NewActor);

    NewActor->SetName("Plane");
    NewActor->GetTransform().SetRotation(0.0f, 0.0f, Math::HALF_PI);
    NewActor->GetTransform().SetUniformScale(40.0f);
    NewActor->GetTransform().SetTranslation(0.0f, 0.0f, 42.0f);

    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 1.0f;
    MatProperties.Roughness    = 0.25f;
    MatProperties.EnableHeight = 0;
    MatProperties.Albedo       = XMFLOAT3(1.0f, 1.0f, 1.0f);

    NewComponent = DBG_NEW MeshComponent(NewActor);
    NewComponent->Mesh     = Mesh::Make(MeshFactory::CreatePlane(10, 10));
    NewComponent->Material = MakeShared<Material>(MatProperties);
    NewComponent->Material->AlbedoMap    = BaseTexture;
    NewComponent->Material->NormalMap    = BaseNormal;
    NewComponent->Material->RoughnessMap = BaseTexture;
    NewComponent->Material->HeightMap    = BaseTexture;
    NewComponent->Material->AOMap        = BaseTexture;
    NewComponent->Material->MetallicMap  = BaseTexture;
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

    // Add DirectionalLight- Source
    DirectionalLight* Light4 = DBG_NEW DirectionalLight();
    Light4->SetShadowBias(0.0008f);
    Light4->SetMaxShadowBias(0.008f);
    Light4->SetShadowNearPlane(0.01f);
    Light4->SetShadowFarPlane(140.0f);
    Light4->SetColor(1.0f, 1.0f, 1.0f);
    Light4->SetIntensity(10.0f);
    CurrentScene->AddLight(Light4);

    Camera* Camera = CurrentScene->GetCamera();
    Camera->SetPosition(0.0f, 1.0f, 0.0f);
    Camera->Rotate(0.0f, 90.0f, 0.0f);

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
    CameraSpeed = CameraSpeed + CameraSpeed * Deacceleration * Delta;
    CameraSpeed = CameraSpeed + CameraAcceleration * Delta;

    XMFLOAT3 Speed = CameraSpeed * Delta;
    CurrentCamera->Move(Speed.x, Speed.y, Speed.z);
}

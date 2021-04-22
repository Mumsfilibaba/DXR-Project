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
#define SCENE 2

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
    MeshFactory::LoadSceneFromFile(SceneBuildData, "../Assets/Scenes/Bistro/BistroExterior.fbx");
#endif

    // In order to create fewer meshes and in turn fewer descriptors we bind together all these models
    MeshFactory::MergeSimilarMaterials(SceneBuildData);

    // Initialize Scene
    Actor* NewActor             = nullptr;
    MeshComponent* NewComponent = nullptr;
    CurrentScene = DBG_NEW Scene();

    CurrentCamera = DBG_NEW Camera();
    CurrentScene->AddCamera(CurrentCamera);

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

        // Specular
        if (!MaterialData.SpecTexName.empty())
        {
            if (MaterialTextures.count(MaterialData.SpecTexName) == 0)
            {
                std::string TexName = MaterialData.TexPath + '/' + MaterialData.SpecTexName;
                TRef<Texture2D> Texture = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
                if (Texture)
                {
                    Texture->SetName(MaterialData.SpecTexName);
                    MaterialTextures[MaterialData.SpecTexName] = Texture;
                }
                else
                {
                    MaterialTextures[MaterialData.SpecTexName] = BaseTexture;
                }
            }

            NewMaterial->MetallicMap = MaterialTextures[MaterialData.SpecTexName];
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

        // Emissive
        if (!MaterialData.EmissiveTexName.empty())
        {
            bool EnableEmissive = false;
            if (MaterialTextures.count(MaterialData.EmissiveTexName) == 0)
            {
                std::string TexName = MaterialData.TexPath + '/' + MaterialData.EmissiveTexName;
                TRef<Texture2D> Texture = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
                if (Texture)
                {
                    Texture->SetName(MaterialData.EmissiveTexName);
                    MaterialTextures[MaterialData.EmissiveTexName] = Texture;
                    EnableEmissive = true;
                }
                else
                {
                    MaterialTextures[MaterialData.EmissiveTexName] = BaseTexture;
                }
            }

            NewMaterial->EmissiveMap = MaterialTextures[MaterialData.EmissiveTexName];
            if (EnableEmissive)
            {
                NewMaterial->EnableEmissiveMap(true);
            }
        }

        NewMaterial->Init();
    }

    for (const ModelData& Model : SceneBuildData.Models)
    {
        NewActor = DBG_NEW Actor();
        CurrentScene->AddActor(NewActor);

        NewActor->SetName(Model.Name);
        NewActor->GetTransform().SetUniformScale(1.0f);

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

#if SCENE == 0
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
#elif SCENE == 1
    Camera* Camera = CurrentScene->GetCamera();
    Camera->SetPosition(-4.5f, 9.3f, 5.8f);
    Camera->Rotate(0.0f, 47.0f, 0.0f);

    const Float Intensity = 7.5f;

    // Add DirectionalLight- Source
    DirectionalLight* DirLight = DBG_NEW DirectionalLight();
    DirLight->SetShadowBias(0.0008f);
    DirLight->SetMaxShadowBias(0.008f);
    DirLight->SetShadowNearPlane(0.01f);
    DirLight->SetShadowFarPlane(140.0f);
    DirLight->SetColor(1.0f, 1.0f, 1.0f);
    DirLight->SetIntensity(10.0f);
    CurrentScene->AddLight(DirLight);

    PointLight* Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(0));
    Light->SetPosition(-2.0f, 4.5f, 7.9f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(1));
    Light->SetPosition(2.0f, 4.5f, 7.8f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(2));
    Light->SetPosition(5.7f, 4.5f, 5.7f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(3));
    Light->SetPosition(7.9f, 4.5f, 2.2f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(4));
    Light->SetPosition(-5.7f, 4.5f, 5.8f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(5));
    Light->SetPosition(-8.0f, 4.5f, 2.1f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(6));
    Light->SetPosition(-3.1f, 4.5f, -7.8f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(7));
    Light->SetPosition(7.5f, 4.5f, -18.0f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(8));
    Light->SetPosition(2.4f, 2.5f, -28.6f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(9));
    Light->SetPosition(0.0f, 1.5f, -31.6f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(10));
    Light->SetPosition(7.2f, 2.5f, -43.2f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(11));
    Light->SetPosition(-7.2f, 2.5f, -43.2f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(12));
    Light->SetPosition(-2.1f, 2.5f, -50.8f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(13));
    Light->SetPosition(0.0f, 1.5f, -58.0f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(14));
    Light->SetPosition(1.7f, 2.5f, -70.8f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(15));
    Light->SetPosition(-1.4f, 2.5f, -70.8f);
    Light->SetColor(0.94f, 0.46f, 0.32f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

#elif SCENE == 2
    Camera* Camera = CurrentScene->GetCamera();
    Camera->SetPosition(15.5f, 7.0f, 68.0f);
    Camera->Rotate(0.0f, 46.1f, 0.0f);

    const Float Intensity  = 20.0f;
    const Float Intensity2 = 10.0f;

    // Add DirectionalLight- Source
    DirectionalLight* DirLight = DBG_NEW DirectionalLight();
    DirLight->SetShadowBias(0.0008f);
    DirLight->SetMaxShadowBias(0.008f);
    DirLight->SetShadowNearPlane(0.01f);
    DirLight->SetShadowFarPlane(140.0f);
    DirLight->SetColor(1.0f, 1.0f, 1.0f);
    DirLight->SetIntensity(2.0f);
    CurrentScene->AddLight(DirLight);

    // PointLights
    PointLight* Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(0));
    Light->SetPosition(28.8f, 7.0f, 66.5f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(1));
    Light->SetPosition(28.3f, 7.0f, 55.1f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    // Green Sign
    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(2));
    Light->SetPosition(17.4f, 6.7f, 54.1f);
    Light->SetColor(0.44f, 0.94f, 0.32f);
    Light->SetIntensity(5.0f);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    // Red Sign
    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(3));
    Light->SetPosition(20.3f, 5.0f, 40.9f);
    Light->SetColor(0.94f, 0.32f, 0.32f);
    Light->SetIntensity(5.0f);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(4));
    Light->SetPosition(14.4f, 7.2f, 32.1f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(5));
    Light->SetPosition(6.0f, 7.0f, 34.0f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(6));
    Light->SetPosition(-3.4f, 6.6f, 29.3f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(7));
    Light->SetPosition(-13.5f, 6.3f, 18.3f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(8));
    Light->SetPosition(-6.9f, 7.0f, 6.7f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(9));
    Light->SetPosition(-15.4f, 7.0f, -3.3f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(10));
    Light->SetPosition(-21.4f, 6.3f, -1.9f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(11));
    Light->SetPosition(-27.6f, 6.3f, 6.0f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(12));
    Light->SetPosition(-28.4f, 6.3f, 23.9f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(14));
    Light->SetPosition(-39.3f, 6.3f, 18.9f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(13));
    Light->SetPosition(-33.3f, 7.0f, 29.2f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(15));
    Light->SetPosition(-45.6f, 5.2f, 27.5f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(16));
    Light->SetPosition(-2.8f, 7.0f, -16.0f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(17));
    Light->SetPosition(-20.1f, 15.2f, -4.0f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(18));
    Light->SetPosition(-26.5f, 15.1f, 3.2f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(19));
    Light->SetPosition(-29.8f, 15.1f, 9.1f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(20));
    Light->SetPosition(-35.6f, 15.1f, 13.6f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(21));
    Light->SetPosition(-41.4f, 15.1f, 20.5f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity2);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(22));
    Light->SetPosition(1.0f, 5.8f, 13.3f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity2);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(23));
    Light->SetPosition(-0.2f, 5.8f, 10.3f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity2);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(24));
    Light->SetPosition(-2.1f, 5.8f, 6.9f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity2);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(25));
    Light->SetPosition(-3.1f, 3.5f, 3.4f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity2);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(26));
    Light->SetPosition(-1.0f, 3.5f, -1.3f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity2);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(27));
    Light->SetPosition(2.0f, 5.7f, -3.4f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity2);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(28));
    Light->SetPosition(5.6f, 5.7f, -4.9f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity2);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(29));
    Light->SetPosition(2.9f, 5.7f, 16.9f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity2);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(30));
    Light->SetPosition(12.2f, 5.7f, -7.6f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity2);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(31));
    Light->SetPosition(8.7f, 5.7f, -6.5f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity2);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(32));
    Light->SetPosition(33.3f, 6.3f, -20.5f);
    Light->SetColor(1.0f, 1.0f, 1.0f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(33));
    Light->SetPosition(56.2f, 7.0f, -29.3f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(34));
    Light->SetPosition(39.1f, 7.2f, -37.1f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(35));
    Light->SetPosition(53.3f, 7.0f, -38.5f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(36));
    Light->SetPosition(62.1f, 7.0f, -54.3f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(37));
    Light->SetPosition(78.9f, 7.0f, -55.0f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(38));
    Light->SetPosition(34.5f, 7.0f, -30.1f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(39));
    Light->SetPosition(-32.3f, 6.3f, 11.5f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(40));
    Light->SetPosition(-43.0f, 5.2f, 33.6f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(41));
    Light->SetPosition(-19.1f, 6.3f, 15.4f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(42));
    Light->SetPosition(-3.3f, 7.0f, -7.9f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(43));
    Light->SetPosition(12.5f, 7.0f, -13.9f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);

    Light = DBG_NEW PointLight();
    Light->SetName("PointLight " + std::to_string(44));
    Light->SetPosition(48.0f, 6.3f, -21.6f);
    Light->SetColor(0.88f, 0.64f, 0.36f);
    Light->SetIntensity(Intensity);
    Light->SetShadowCaster(false);
    CurrentScene->AddLight(Light);
#endif

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

#include "Sandbox.h"

#include "Math/Math.h"

#include "Rendering/Renderer.h"
#include "Rendering/DebugUI.h"
#include "Rendering/Resources/TextureFactory.h"
#include "Rendering/RayTracer.h"

#include "Scene/Scene.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"
#include "Scene/Components/MeshComponent.h"

#include "Application/Input.h"

#include "Debug/Profiler.h"

#include <random>
#include <fstream>

// Scene 0 - Sponza
// Scene 1 - SunTemple
// Scene 2 - Bistro
#define SCENE 0

#define NUM_FRAMES 5000

#define ENABLE_TRACK        1
#define ENABLE_POINT_LIGHTS 1

Sandbox* gSandBox = nullptr;

struct TrackHeader
{
    UInt32 NumPoints;
    UInt32 NumRotations;
};

Game* MakeGameInstance()
{
    gSandBox = DBG_NEW Sandbox();
    return gSandBox;
}

Bool Sandbox::Init()
{
    Memory::Memzero(Filename, sizeof(Filename));
    strcpy(Filename, "TrackPoints.pdata");

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
    MatProperties.Diffuse = XMFLOAT3(1.0f, 1.0f, 1.0f);
    MatProperties.AO           = 1.0f;
    MatProperties.Metallic     = 1.0f;
    MatProperties.Roughness    = 1.0f;
    MatProperties.EnableHeight = 0;

    TSharedPtr<class Material> BaseMaterial = MakeShared<Material>(MatProperties);
    BaseMaterial->DiffuseMap  = BaseTexture;
    BaseMaterial->NormalMap   = BaseNormal;
    BaseMaterial->SpecularMap = BaseTexture;
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

        // Diffuse
        TRef<Texture2D> Diffuse;
        if (!MaterialData.DiffTexName.empty())
        {
            if (MaterialTextures.count(MaterialData.DiffTexName) == 0)
            {
                std::string TexName = MaterialData.TexPath + '/' + MaterialData.DiffTexName;
                Diffuse = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
                if (Diffuse)
                {
                    Diffuse->SetName(MaterialData.DiffTexName);
                    MaterialTextures[MaterialData.DiffTexName] = Diffuse;
                }
            }
            else
            {
                Diffuse = MaterialTextures[MaterialData.DiffTexName];
            }
        }

        // Metallic
        TRef<Texture2D> Metallic;
        if (!MaterialData.MetallicTexname.empty())
        {
            if (MaterialTextures.count(MaterialData.MetallicTexname) == 0)
            {
                std::string TexName = MaterialData.TexPath + '/' + MaterialData.MetallicTexname;
                Metallic = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
                if (Metallic)
                {
                    Metallic->SetName(MaterialData.MetallicTexname);
                    MaterialTextures[MaterialData.MetallicTexname] = Metallic;
                }
            }
            else
            {
                Metallic = MaterialTextures[MaterialData.MetallicTexname];
            }
        }

        // Roughness
        TRef<Texture2D> Roughness;
        if (!MaterialData.RoughnessTexname.empty())
        {
            if (MaterialTextures.count(MaterialData.RoughnessTexname) == 0)
            {
                std::string TexName = MaterialData.TexPath + '/' + MaterialData.RoughnessTexname;
                Roughness = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
                if (Roughness)
                {
                    Roughness->SetName(MaterialData.RoughnessTexname);
                    MaterialTextures[MaterialData.RoughnessTexname] = Roughness;
                }
            }
            else
            {
                Roughness = MaterialTextures[MaterialData.RoughnessTexname];
            }
        }

        // Alpha
        TRef<Texture2D> Alpha;
        if (!MaterialData.AlphaTexname.empty())
        {
            if (MaterialTextures.count(MaterialData.AlphaTexname) == 0)
            {
                std::string TexName = MaterialData.TexPath + '/' + MaterialData.AlphaTexname;
                Alpha = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8_Unorm);
                if (Alpha)
                {
                    Alpha->SetName(MaterialData.AlphaTexname);
                    MaterialTextures[MaterialData.AlphaTexname] = Alpha;
                }
            }
            else
            {
                Alpha = MaterialTextures[MaterialData.AlphaTexname];
            }
        }

        // Specular
        TRef<Texture2D> Specular;
        if (!MaterialData.SpecTexName.empty())
        {
            if (MaterialTextures.count(MaterialData.SpecTexName) == 0)
            {
                std::string TexName = MaterialData.TexPath + '/' + MaterialData.SpecTexName;
                Specular = TextureFactory::LoadFromFile(TexName, TextureFactoryFlag_GenerateMips, EFormat::R8G8B8A8_Unorm);
                if (Specular)
                {
                    Specular->SetName(MaterialData.SpecTexName);
                    MaterialTextures[MaterialData.SpecTexName] = Specular;
                }
            }
            else
            {
                Specular = MaterialTextures[MaterialData.SpecTexName];
            }
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

        // Generate Diffuse and Alpha
        if (Diffuse && Alpha)
        {
            Diffuse = TextureFactory::AddAlphaChannel(Diffuse.Get(), Alpha.Get());
        }

        if (!Diffuse)
        {
            Diffuse = BaseTexture;
        }

        NewMaterial->DiffuseMap = Diffuse;

        // Generate Specular
        if (!Specular)
        {
            if (!Metallic)
            {
                Metallic = BaseTexture;
            }

            if (!Roughness)
            {
                Roughness = BaseTexture;
            }

            Specular = TextureFactory::CombineTextureChannels(BaseTexture.Get(), Roughness.Get(), Metallic.Get(), nullptr);
            if (Specular)
            {
                Specular->SetName(Roughness->GetName() + "_Combined");
            }
        }

        NewMaterial->SpecularMap = Specular;

        NewMaterial->Init();
    }

    for (const ModelData& Model : SceneBuildData.Models)
    {
        NewActor = DBG_NEW Actor();
        CurrentScene->AddActor(NewActor);

        NewActor->SetName(Model.Name);
#if SCENE == 0 // Sponza should be smaller
        NewActor->GetTransform().SetUniformScale(0.01f);
#else
        NewActor->GetTransform().SetUniformScale(1.0f);
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

#if SCENE == 0
    Camera* Camera = CurrentScene->GetCamera();
    if (TryLoadTrackFile("SponzaTrack.pdata"))
    {
        ControlPoints.Resize(SavedPoints.Size());
        ControlRotations.Resize(SavedRotations.Size());

        Assert(ControlPoints.SizeInBytes() == SavedPoints.SizeInBytes());
        Assert(ControlRotations.SizeInBytes() == SavedRotations.SizeInBytes());

        Memory::Memcpy(ControlPoints.Data(), SavedPoints.Data(), SavedPoints.SizeInBytes());
        Memory::Memcpy(ControlRotations.Data(), SavedRotations.Data(), SavedRotations.SizeInBytes());

        Camera->SetPosition(ControlPoints[0]);
        Camera->SetRotation(ControlRotations[0]);
    }
    else
    {
        Camera->SetPosition(11.5f, 6.6f, 0.6f);
        Camera->Rotate(0.0f, XMConvertToRadians(270.0f), 0.0f);
    }

    // Add DirectionalLight- Source
    DirectionalLight* Light4 = DBG_NEW DirectionalLight();
    Light4->SetShadowBias(0.0008f);
    Light4->SetMaxShadowBias(0.008f);
    Light4->SetShadowNearPlane(0.01f);
    Light4->SetShadowFarPlane(140.0f);
    Light4->SetColor(1.0f, 1.0f, 1.0f);
    Light4->SetRotation(XMConvertToRadians(-19.0f), XMConvertToRadians(75.0f), XMConvertToRadians(-4.0f));
    Light4->SetIntensity(10.0f);
    CurrentScene->AddLight(Light4);

#elif SCENE == 1
    Camera* Camera = CurrentScene->GetCamera();
    if (TryLoadTrackFile("SunTempleTrack.pdata"))
    {
        ControlPoints.Resize(SavedPoints.Size());
        ControlRotations.Resize(SavedRotations.Size());

        Assert(ControlPoints.SizeInBytes() == SavedPoints.SizeInBytes());
        Assert(ControlRotations.SizeInBytes() == SavedRotations.SizeInBytes());

        Memory::Memcpy(ControlPoints.Data(), SavedPoints.Data(), SavedPoints.SizeInBytes());
        Memory::Memcpy(ControlRotations.Data(), SavedRotations.Data(), SavedRotations.SizeInBytes());

        Camera->SetPosition(ControlPoints[0]);
        Camera->SetRotation(ControlRotations[0]);
    }
    else
    {
        Camera->SetPosition(-4.5f, 9.3f, 5.8f);
        Camera->Rotate(0.0f, XMConvertToRadians(180.0f), 0.0f);
    }

    const Float Intensity = 7.5f;

    // Add DirectionalLight- Source
    DirectionalLight* DirLight = DBG_NEW DirectionalLight();
    DirLight->SetShadowBias(0.0008f);
    DirLight->SetMaxShadowBias(0.008f);
    DirLight->SetShadowNearPlane(0.01f);
    DirLight->SetShadowFarPlane(140.0f);
    DirLight->SetColor(1.0f, 1.0f, 1.0f);
    DirLight->SetRotation(XMConvertToRadians(85.0f), XMConvertToRadians(-60.0f), 0.0f);
    DirLight->SetIntensity(10.0f);
    CurrentScene->AddLight(DirLight);

#if ENABLE_POINT_LIGHTS
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
#endif
#elif SCENE == 2
    Camera* Camera = CurrentScene->GetCamera();
    if (TryLoadTrackFile("BistroTrack.pdata"))
    {
        ControlPoints.Resize(SavedPoints.Size());
        ControlRotations.Resize(SavedRotations.Size());

        Assert(ControlPoints.SizeInBytes() == SavedPoints.SizeInBytes());
        Assert(ControlRotations.SizeInBytes() == SavedRotations.SizeInBytes());

        Memory::Memcpy(ControlPoints.Data(), SavedPoints.Data(), SavedPoints.SizeInBytes());
        Memory::Memcpy(ControlRotations.Data(), SavedRotations.Data(), SavedRotations.SizeInBytes());

        Camera->SetPosition(ControlPoints[0]);
        Camera->SetRotation(ControlRotations[0]);
    }
    else
    {
        Camera->SetPosition(15.5f, 7.0f, 68.0f);
        Camera->Rotate(0.0f, XMConvertToRadians(140.0f), 0.0f);
    }

    const Float Intensity  = 20.0f;
    const Float Intensity2 = 10.0f;

    // Add DirectionalLight- Source
    DirectionalLight* DirLight = DBG_NEW DirectionalLight();
    DirLight->SetShadowBias(0.0008f);
    DirLight->SetMaxShadowBias(0.008f);
    DirLight->SetShadowNearPlane(0.01f);
    DirLight->SetShadowFarPlane(140.0f);
    DirLight->SetColor(1.0f, 1.0f, 1.0f);
    DirLight->SetRotation(XMConvertToRadians(-25.0f), XMConvertToRadians(23.0f), XMConvertToRadians(0.0f));
    DirLight->SetIntensity(20.0f);
    CurrentScene->AddLight(DirLight);

#if ENABLE_POINT_LIGHTS
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
#endif

    // Calculate total length of path
    UInt32 NumPoints = Math::Max(ControlPoints.Size(), 1u) - 1;
    for (UInt32 i = 0; i < NumPoints; i++)
    {
        XMVECTOR Point0 = XMLoadFloat3(&ControlPoints[i]);
        XMVECTOR Point1 = XMLoadFloat3(&ControlPoints[i + 1]);
        XMVECTOR Distance = XMVectorSubtract(Point1, Point0);
        TotalLength += XMVectorGetX(XMVector3Length(Distance));
    }

#if ENABLE_TRACK
    Profiler::Reset();

    std::string SceneName;
#if SCENE == 0
    SceneName = "Sponza/Sponza";
#elif SCENE == 1
    SceneName = "SunTemple/SunTemple";
#elif SCENE == 2
    SceneName = "Bistro/Bistro";
#else
    #error No scene defined
#endif

    std::string NameOnTest;
#if TEST == 0
    NameOnTest = "Reference";
#elif TEST == 1
    NameOnTest = "Half_RayGen";
#elif TEST == 2
    NameOnTest = "Inline_RayGen_Full";
#elif TEST == 3
    NameOnTest = "Inline_RayGen_Half";
#elif TEST == 4
    NameOnTest = "Inline_RayGen_VRS_Rough";
#elif TEST == 5
    NameOnTest = "Inline_RayGen_Grazing_Angles";
#else
#error No test defined
#endif

    TestName = SceneName + '_' + NameOnTest + ".txt";

#endif

    return true;
}

void Sandbox::Tick(Timestamp DeltaTime)
{
    if (Input::IsKeyDown(EKey::Key_1))
    {
        if (!EnablePressed)
        {
            CaptureModeEnabled = !CaptureModeEnabled;
            EnablePressed = true;
        }
    }
    else
    {
        EnablePressed = false;
    }

    if (CaptureModeEnabled)
    {
        FreeCamMode(DeltaTime);
        CurrentPoint = 0;
    }
    else
    {
#if ENABLE_TRACK
        if (!TestFinished)
        {
            TrackMode();
        }
        else
        {
            FreeCamMode(DeltaTime);
        }
#else
        FreeCamMode(DeltaTime);
#endif
    }
}

void Sandbox::FreeCamMode(Timestamp DeltaTime)
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

    // Capture camera position
    if (CaptureModeEnabled)
    {
        if (Input::IsKeyDown(EKey::Key_2))
        {
            SavedPoints.Clear();
            SavedRotations.Clear();
        }

        if (Input::IsKeyDown(EKey::Key_3))
        {
            if (!Saved)
            {
                FILE* File = fopen(Filename, "wb");
                if (File)
                {
                    TrackHeader Header;
                    Header.NumPoints    = SavedPoints.Size();
                    Header.NumRotations = SavedRotations.Size();

                    fwrite(&Header, sizeof(TrackHeader), 1, File);
                    fwrite(SavedPoints.Data(), sizeof(XMFLOAT3), Header.NumPoints, File);
                    fwrite(SavedRotations.Data(), sizeof(XMFLOAT4), Header.NumRotations, File);

                    fclose(File);

                    LOG_INFO("Saved trackfile '" + std::string(Filename) + "' NumPoins=" + std::to_string(Header.NumPoints) + "' NumRotations=" + std::to_string(Header.NumRotations));
                }
                else
                {
                    LOG_ERROR("Failed to open file '" + std::string(Filename) + "'");
                }

                Saved = true;
            }
        }
        else
        {
            Saved = false;
        }

        if (Input::IsKeyDown(EKey::Key_4))
        {
            if (!Loaded)
            {
                TryLoadTrackFile(Filename);
            }
        }
        else
        {
            Loaded = false;
        }

        if (Input::IsKeyDown(EKey::Key_Space))
        {
            if (!Captured)
            {
                // Position
                SavedPoints.EmplaceBack(CurrentCamera->GetPosition());
                
                // Rotation
                XMFLOAT4 Rotation = CurrentCamera->GetRotation();
                XMFLOAT4 Quaternion;
                XMStoreFloat4(&Quaternion, XMQuaternionNormalize(XMLoadFloat4(&Rotation)));
                SavedRotations.EmplaceBack(Quaternion);

                Captured = true;
            }
        }
        else
        {
            Captured = false;
        }

        DebugUI::DrawUI([]() 
            {
                if (ImGui::Begin("Captured Points"))
                {
                    ImGui::Text("Output Filename");
                    
                    ImGui::SameLine();

                    ImGui::InputText("###Input", gSandBox->Filename, sizeof(gSandBox->Filename));

                    UInt32 NumCaptures = gSandBox->SavedPoints.Size();
                    for (UInt32 i = 0; i < NumCaptures; i++)
                    {
                        const XMFLOAT3& Point = gSandBox->SavedPoints[i];
                        const XMFLOAT4& Quaternion = gSandBox->SavedRotations[i];
                        ImGui::Text("%d Position: %.2f, %.2f, %.2f Rotation: %.2f, %.2f, %.2f , %.2f", 
                            i+1, Point.x, Point.y, Point.z, Quaternion.x, Quaternion.y, Quaternion.z, Quaternion.w);
                    }

                    ImGui::End();
                }
            });
    }
}

void Sandbox::TrackMode()
{
    constexpr Float Radius = 0.25f;

    const UInt32 NumPaths = ControlPoints.Size() - 1;

    const Float NumFrames = Float(NUM_FRAMES);

    const Float FramePerPath = NumFrames / Float(NumPaths);
    const Float Delta        = 1.0f / FramePerPath;

    XMVECTOR Point0 = XMLoadFloat3(&ControlPoints[CurrentPoint + 0]);
    XMVECTOR Point1 = XMLoadFloat3(&ControlPoints[CurrentPoint + 1]);
    XMVECTOR CurrentPosition = XMVectorLerp(Point0, Point1, SlerpT);

    XMFLOAT3 Position;
    XMStoreFloat3(&Position, CurrentPosition);

    CurrentCamera->SetPosition(Position);

    XMVECTOR Rotation0 = XMQuaternionNormalize(XMLoadFloat4(&ControlRotations[CurrentPoint + 0]));
    XMVECTOR Rotation1 = XMQuaternionNormalize(XMLoadFloat4(&ControlRotations[CurrentPoint + 1]));
    XMVECTOR Rotation  = XMQuaternionSlerp(Rotation0, Rotation1, SlerpT);

    XMFLOAT4 Quaternion;
    XMStoreFloat4(&Quaternion, Rotation);
    
    SlerpT += Delta;

    CurrentCamera->SetRotation(Quaternion);

    if (SlerpT >= 1.0f)
    {
        CurrentPoint++;
        SlerpT = 0.0f;
    }

    FrameCount++;
    if (FrameCount > NUM_FRAMES)
    {
        TestFinished = true;

        Profiler::Disable();

        FILE* File = fopen(TestName.c_str(), "w");
        if (File)
        {
            const GPUProfileSample* RayTracingSample   = Profiler::GetGPUSample("Ray Tracing");
            const GPUProfileSample* RTBuildSample      = Profiler::GetGPUSample("RT Build Scene");
            const GPUProfileSample* InlineRayGenSample = Profiler::GetGPUSample("Inline RayGen");
            const GPUProfileSample* VRSSample          = Profiler::GetGPUSample("Generate VRS Image");
            const GPUProfileSample* DispatchRaysSample = Profiler::GetGPUSample("Dispatch Rays");
            const GPUProfileSample* ReconstructSample  = Profiler::GetGPUSample("RT Reconstruction Filter");
            const GPUProfileSample* BilateralSample    = Profiler::GetGPUSample("RT Bilateral Filter");

            const GPUProfileSample* FrameTime = Profiler::GetGPUFrameTimeSamples();

            const auto PrintGPUSample = [](FILE* File, const Char* SampleName, const GPUProfileSample* Sample, bool Convert = true)
            {
                const Float NanoToMilli = Convert ? 1.0f / (1000.0f * 1000.0f) : 1.0f;

                Float Avg = 0.0f;
                Float Min = 0.0f;
                Float Max = 0.0f;
                Float Median = 0.0f;
                if (Sample)
                {
                    Avg = Sample->GetAverage() * NanoToMilli;
                    Min = Sample->GetMin() * NanoToMilli;
                    Max = Sample->GetMax() * NanoToMilli;
                    Median = Sample->GetMedian() * NanoToMilli;
                }

                fprintf(File, "%s: Avg=%.8f, Median=%.8f, Min=%.8f, Max=%.8f\n", SampleName, Avg, Median, Min, Max);
            };

            PrintGPUSample(File, "FrameTime", FrameTime, false);
            PrintGPUSample(File, "Ray Tracing", RayTracingSample);
            PrintGPUSample(File, "RT Build Scene", RTBuildSample);
            PrintGPUSample(File, "Inline RayGen", InlineRayGenSample);
            PrintGPUSample(File, "Generate VRS Image", VRSSample);
            PrintGPUSample(File, "Dispatch Rays", DispatchRaysSample);
            PrintGPUSample(File, "RT Reconstruction Filter", ReconstructSample);
            PrintGPUSample(File, "RT Bilateral Filter", BilateralSample);

            fclose(File);
        }
        else
        {
            LOG_ERROR("Failed to open file '" + TestName + "'");
        }

#if SCENE == 0
        // Set camera for taking screenshots
        CurrentCamera->SetPosition(-1.7f, 6.8f, -2.3f);
        CurrentCamera->SetRotation(XMConvertToRadians(30.5f), XMConvertToRadians(14.2f), XMConvertToRadians(299.3f));
#elif SCENE == 1
        // Set camera for taking screenshots
        CurrentCamera->SetPosition(-4.0f, 10.1f, 5.4f);
        CurrentCamera->SetRotation(XMConvertToRadians(18.5f), XMConvertToRadians(166.7f), XMConvertToRadians(353.5f));
#elif SCENE == 2
        // Set camera for taking screenshots
        CurrentCamera->SetPosition(-11.6f, 4.3f, 0.8f);
        CurrentCamera->SetRotation(XMConvertToRadians(41.8f), XMConvertToRadians(95.4f), XMConvertToRadians(357.0f));
#endif
    }
}

bool Sandbox::TryLoadTrackFile(const char* InFilename)
{
    SavedPoints.Clear();
    SavedRotations.Clear();

    FILE* File = fopen(InFilename, "rb");
    if (File)
    {
        TrackHeader Header;
        fread(&Header, sizeof(TrackHeader), 1, File);

        // TODO: Check errors

        SavedPoints.Resize(Header.NumPoints);
        SavedRotations.Resize(Header.NumRotations);

        fread(SavedPoints.Data(), sizeof(XMFLOAT3), SavedPoints.Size(), File);
        fread(SavedRotations.Data(), sizeof(XMFLOAT4), SavedRotations.Size(), File);

        fclose(File);

        LOG_INFO("Load trackfile '" + std::string(InFilename) + "' NumPoins=" + std::to_string(Header.NumPoints) + "' NumRotations=" + std::to_string(Header.NumRotations));

        return true;
    }
    else
    {
        LOG_INFO("Failed to load trackfile '" + std::string(InFilename) + "'");
        return false;
    }
}

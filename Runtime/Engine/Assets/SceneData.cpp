#include "SceneData.h"

#include "Engine/Engine.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Components/MeshComponent.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/TextureFactory.h"

void FSceneData::AddToScene(FScene* Scene)
{
    if (!HasModelData())
    {
        return;
    }

    LOG_INFO("AddToScene");

    TArray<FRHITexture2DRef> RHITextures;
    for (FImage2DPtr Image : Textures)
    {
        FRHITexture2DRef NewTexture = FTextureFactory::LoadFromImage2D(Image.Get(), TextureFactoryFlag_GenerateMips);
        RHITextures.Emplace(NewTexture);
    }

    TArray<TSharedPtr<FMaterial>> CreatedMaterials;
    if (!Materials.IsEmpty())
    {
        for (const FMaterialData& MaterialData : Materials)
        {
            FMaterialDesc Desc;
            Desc.Albedo    = MaterialData.Diffuse;
            Desc.AO        = MaterialData.AO;
            Desc.Metallic  = MaterialData.Metallic;
            Desc.Roughness = MaterialData.Roughness;

            // TODO: Should probably have a better connection between RHITexture and a Texture

            TSharedPtr<FMaterial> Material = MakeShared<FMaterial>(Desc);
            Material->AlbedoMap = (MaterialData.DiffuseTexture >= 0) ? RHITextures[MaterialData.DiffuseTexture] : GEngine->BaseTexture;
            Material->AlbedoMap = (Material->AlbedoMap)              ? Material->AlbedoMap                      : GEngine->BaseTexture;

            Material->AlphaMask = (MaterialData.AlphaMaskTexture >= 0) ? RHITextures[MaterialData.AlphaMaskTexture] : GEngine->BaseTexture;
            Material->AlphaMask = (Material->AlphaMask)                ? Material->AlphaMask                        : GEngine->BaseTexture;

            Material->AOMap = (MaterialData.AOTexture >= 0) ? RHITextures[MaterialData.AOTexture] : GEngine->BaseTexture;
            Material->AOMap = (Material->AOMap)             ? Material->AOMap                     : GEngine->BaseTexture;

            Material->MetallicMap = (MaterialData.MetallicTexture >= 0) ? RHITextures[MaterialData.MetallicTexture] : GEngine->BaseTexture;
            Material->MetallicMap = (Material->MetallicMap)             ? Material->MetallicMap                     : GEngine->BaseTexture;

            Material->NormalMap = (MaterialData.NormalTexture >= 0) ? RHITextures[MaterialData.NormalTexture] : GEngine->BaseNormal;
            Material->NormalMap = (Material->NormalMap)             ? Material->NormalMap                     : GEngine->BaseNormal;

            Material->RoughnessMap = (MaterialData.RoughnessTexture >= 0) ? RHITextures[MaterialData.RoughnessTexture] : GEngine->BaseTexture;
            Material->RoughnessMap = (Material->RoughnessMap)             ? Material->RoughnessMap                     : GEngine->BaseTexture;

            Material->HeightMap = GEngine->BaseTexture;
            Material->Initialize();

            CreatedMaterials.Push(Material);
        }
    }

    Check(Materials.Size() == CreatedMaterials.Size());

    for (const FModelData& ModelData : Models)
    {
        if (ModelData.Mesh.Hasdata())
        {
            FActor* NewActor = Scene->MakeActor();
            NewActor->SetName(ModelData.Name.CStr());
            NewActor->GetTransform().SetUniformScale(Scale);

            FMeshComponent* MeshComponent = dbg_new FMeshComponent(NewActor);
            MeshComponent->Mesh = FMesh::Make(ModelData.Mesh);

            if (ModelData.MaterialIndex >= 0)
            {
                MeshComponent->Material = CreatedMaterials[ModelData.MaterialIndex];
            }
            else
            {
                MeshComponent->Material = GEngine->BaseMaterial;
            }

            NewActor->AddComponent(MeshComponent);
        }
    }
}

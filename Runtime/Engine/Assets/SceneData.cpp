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
            Material->AlbedoMap    = (MaterialData.DiffuseTexture)   ? MaterialData.DiffuseTexture->GetRHITexture()   : GEngine->BaseTexture;
            Material->AOMap        = (MaterialData.AOTexture)        ? MaterialData.AOTexture->GetRHITexture()        : GEngine->BaseTexture;
            Material->MetallicMap  = (MaterialData.MetallicTexture)  ? MaterialData.MetallicTexture->GetRHITexture()  : GEngine->BaseTexture;
            Material->NormalMap    = (MaterialData.NormalTexture)    ? MaterialData.NormalTexture->GetRHITexture()    : GEngine->BaseNormal;
            Material->RoughnessMap = (MaterialData.RoughnessTexture) ? MaterialData.RoughnessTexture->GetRHITexture() : GEngine->BaseTexture;
            Material->HeightMap    = GEngine->BaseTexture;

            Material->AlphaMask = (MaterialData.AlphaMaskTexture) ? MaterialData.AlphaMaskTexture->GetRHITexture(): GEngine->BaseTexture;
            Material->EnableAlphaMask(Material->AlphaMask != GEngine->BaseTexture);

            Material->Initialize();

            CreatedMaterials.Push(Material);
        }
    }

    Check(Materials.GetSize() == CreatedMaterials.GetSize());

    for (const FModelData& ModelData : Models)
    {
        if (ModelData.Mesh.Hasdata())
        {
            FActor* NewActor = Scene->CreateActor();
            NewActor->SetName(ModelData.Name.GetCString());
            NewActor->GetTransform().SetUniformScale(Scale);

            FMeshComponent* MeshComponent = dbg_new FMeshComponent(NewActor);
            MeshComponent->Mesh = FMesh::Create(ModelData.Mesh);

            if (!CreatedMaterials.IsEmpty() && ModelData.MaterialIndex >= 0)
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

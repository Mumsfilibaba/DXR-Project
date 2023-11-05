#include "SceneData.h"
#include "Engine/Engine.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Components/MeshComponent.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

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

            if (MaterialData.bAlphaDiffuseCombined)
            {
                Desc.EnableMask = AlphaMaskMode_DiffuseCombined;
            }

            if (MaterialData.SpecularTexture)
            {
                Desc.EnableHeight = 2;
            }

            TSharedPtr<FMaterial> Material = MakeShared<FMaterial>(Desc);
            Material->AlbedoMap    = MaterialData.DiffuseTexture   ? MaterialData.DiffuseTexture->GetRHITexture()   : GEngine->BaseTexture;
            Material->AOMap        = MaterialData.AOTexture        ? MaterialData.AOTexture->GetRHITexture()        : GEngine->BaseTexture;
            Material->SpecularMap  = MaterialData.SpecularTexture  ? MaterialData.SpecularTexture->GetRHITexture()  : nullptr;
            Material->MetallicMap  = MaterialData.MetallicTexture  ? MaterialData.MetallicTexture->GetRHITexture()  : GEngine->BaseTexture;
            Material->NormalMap    = MaterialData.NormalTexture    ? MaterialData.NormalTexture->GetRHITexture()    : GEngine->BaseNormal;
            Material->RoughnessMap = MaterialData.RoughnessTexture ? MaterialData.RoughnessTexture->GetRHITexture() : GEngine->BaseTexture;
            Material->HeightMap    = GEngine->BaseTexture;

            if (!MaterialData.bAlphaDiffuseCombined)
            {
                Material->AlphaMask = MaterialData.AlphaMaskTexture ? MaterialData.AlphaMaskTexture->GetRHITexture() : nullptr;
                Material->EnableAlphaMask(Material->AlphaMask != nullptr);
            }

            if (MaterialData.bIsDoubleSided)
            {
                Material->EnableDoubleSided(MaterialData.bIsDoubleSided);
            }

            Material->Initialize();
            Material->SetDebugName(MaterialData.Name);

            CreatedMaterials.Add(Material);
        }
    }

    CHECK(Materials.Size() == CreatedMaterials.Size());

    for (const FModelData& ModelData : Models)
    {
        if (ModelData.Mesh.Hasdata())
        {
            FActor* NewActor = Scene->CreateActor();
            NewActor->SetName(ModelData.Name.GetCString());
            NewActor->GetTransform().SetUniformScale(Scale);

            FMeshComponent* MeshComponent = new FMeshComponent(NewActor);
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

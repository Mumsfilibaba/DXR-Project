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
            FMaterialCreateInfo CreateInfo;
            CreateInfo.Albedo           = MaterialData.Diffuse;
            CreateInfo.AmbientOcclusion = MaterialData.AO;
            CreateInfo.Metallic         = MaterialData.Metallic;
            CreateInfo.Roughness        = MaterialData.Roughness;
            CreateInfo.MaterialFlags    = MaterialData.MaterialFlags;

            TSharedPtr<FMaterial> Material = MakeShared<FMaterial>(CreateInfo);
            Material->AlbedoMap    = MaterialData.DiffuseTexture   ? MaterialData.DiffuseTexture->GetRHITexture()   : GEngine->BaseTexture;
            Material->AOMap        = MaterialData.AOTexture        ? MaterialData.AOTexture->GetRHITexture()        : GEngine->BaseTexture;
            Material->SpecularMap  = MaterialData.SpecularTexture  ? MaterialData.SpecularTexture->GetRHITexture()  : GEngine->BaseTexture;
            Material->MetallicMap  = MaterialData.MetallicTexture  ? MaterialData.MetallicTexture->GetRHITexture()  : GEngine->BaseTexture;
            Material->NormalMap    = MaterialData.NormalTexture    ? MaterialData.NormalTexture->GetRHITexture()    : GEngine->BaseNormal;
            Material->RoughnessMap = MaterialData.RoughnessTexture ? MaterialData.RoughnessTexture->GetRHITexture() : GEngine->BaseTexture;
            Material->AlphaMask    = MaterialData.AlphaMaskTexture ? MaterialData.AlphaMaskTexture->GetRHITexture() : GEngine->BaseTexture;
            Material->HeightMap    = GEngine->BaseTexture;

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
            NewActor->SetName(ModelData.Name);
            NewActor->GetTransform().SetUniformScale(Scale);

            FMeshComponent* MeshComponent = NewObject<FMeshComponent>();
            if (MeshComponent)
            {
                if (!CreatedMaterials.IsEmpty() && ModelData.MaterialIndex >= 0)
                {
                    MeshComponent->SetMaterial(CreatedMaterials[ModelData.MaterialIndex]);
                }
                else
                {
                    MeshComponent->SetMaterial(GEngine->BaseMaterial);
                }
                
                MeshComponent->SetMesh(FMesh::Create(ModelData.Mesh));
                NewActor->AddComponent(MeshComponent);
            }
        }
    }
}

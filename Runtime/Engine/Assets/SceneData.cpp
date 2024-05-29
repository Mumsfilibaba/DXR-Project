#include "SceneData.h"
#include "Engine/Engine.h"
#include "Engine/World/World.h"
#include "Engine/World/Components/MeshComponent.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

void FSceneData::AddToWorld(FWorld* World)
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
            FMaterialInfo CreateInfo;
            CreateInfo.Albedo           = MaterialData.Diffuse;
            CreateInfo.AmbientOcclusion = MaterialData.AO;
            CreateInfo.Metallic         = MaterialData.Metallic;
            CreateInfo.Roughness        = MaterialData.Roughness;
            CreateInfo.MaterialFlags    = MaterialData.MaterialFlags;
            
            if (MaterialData.NormalTexture)
            {
                CreateInfo.MaterialFlags |= MaterialFlag_EnableNormalMapping;
            }

            TSharedPtr<FMaterial> Material = MakeShared<FMaterial>(CreateInfo);
            Material->AlbedoMap    = MaterialData.DiffuseTexture   ? MaterialData.DiffuseTexture->GetRHITexture()   : GEngine->BaseTexture;
            Material->AOMap        = MaterialData.AOTexture        ? MaterialData.AOTexture->GetRHITexture()        : GEngine->BaseTexture;
            Material->SpecularMap  = MaterialData.SpecularTexture  ? MaterialData.SpecularTexture->GetRHITexture()  : GEngine->BaseTexture;
            Material->MetallicMap  = MaterialData.MetallicTexture  ? MaterialData.MetallicTexture->GetRHITexture()  : GEngine->BaseTexture;
            Material->RoughnessMap = MaterialData.RoughnessTexture ? MaterialData.RoughnessTexture->GetRHITexture() : GEngine->BaseTexture;
            Material->AlphaMask    = MaterialData.AlphaMaskTexture ? MaterialData.AlphaMaskTexture->GetRHITexture() : GEngine->BaseTexture;

            if (MaterialData.NormalTexture)
            {
                Material->NormalMap = MaterialData.NormalTexture->GetRHITexture();
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
            FActor* NewActor = World->CreateActor();
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

#include "SceneData.h"
#include "Engine/Engine.h"
#include "Engine/World/World.h"
#include "Engine/World/Components/MeshComponent.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

void FImportedModel::AddToWorld(FWorld* World)
{
    if (!HasModelData())
    {
        return;
    }

    TArray<TSharedPtr<FMaterial>> CreatedMaterials;
    if (!Materials.IsEmpty())
    {
        for (const FImportedMaterial& MaterialData : Materials)
        {
            FMaterialInfo CreateInfo;
            CreateInfo.Albedo           = MaterialData.Diffuse;
            CreateInfo.AmbientOcclusion = MaterialData.AO;
            CreateInfo.Metallic         = MaterialData.Metallic;
            CreateInfo.Roughness        = MaterialData.Roughness;
            CreateInfo.MaterialFlags    = MaterialData.MaterialFlags;
            
            if (MaterialData.NormalTexture)
            {
                CreateInfo.MaterialFlags |= EMaterialFlags::EnableNormalMapping;
            }

            TSharedPtr<FMaterial> Material = MakeSharedPtr<FMaterial>(CreateInfo);
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

    for (const FMeshData& ModelData : Models)
    {
        if (ModelData.Hasdata())
        {
            FActor* NewActor = World->CreateActor();
            NewActor->SetName(ModelData.Name);
            NewActor->GetTransform().SetUniformScale(Scale);

            FMeshComponent* MeshComponent = NewObject<FMeshComponent>();
            if (MeshComponent)
            {
                MeshComponent->SetMesh(FMesh::Create(ModelData));

                if (ModelData.Partitions.IsEmpty())
                {
                    if (!CreatedMaterials.IsEmpty())
                    {
                        MeshComponent->SetMaterial(CreatedMaterials[0]);
                    }
                    else
                    {
                        MeshComponent->SetMaterial(GEngine->BaseMaterial);
                    }
                }
                else
                {
                    const int32 NumSubMeshes = ModelData.Partitions.Size();
                    for (int32 Index = 0; Index < NumSubMeshes; Index++)
                    {
                        const FMeshPartition& MeshPartition = ModelData.Partitions[Index];
                        if (MeshPartition.MaterialIndex >= 0 && CreatedMaterials.Size() >= MeshPartition.MaterialIndex)
                        {
                            const TSharedPtr<FMaterial>& Material = CreatedMaterials[MeshPartition.MaterialIndex];
                            MeshComponent->SetMaterial(Material, Index);
                            CHECK(MeshComponent->GetMaterial(Index) == Material);
                        }
                        else
                        {
                            MeshComponent->SetMaterial(GEngine->BaseMaterial, Index);
                        }
                    }
                }
                
                NewActor->AddComponent(MeshComponent);
            }
        }
    }
}

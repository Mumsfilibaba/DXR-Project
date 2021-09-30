#include "SceneData.h"

#include "Scene/Scene.h"

#include "Core/Engine/Engine.h"

#include "Scene/Components/MeshComponent.h"

#include "Rendering/Resources/Mesh.h"
#include "Rendering/Resources/Material.h"

void SSceneData::AddToScene( CScene* Scene )
{
    if ( !HasData() )
    {
        return;
    }

    for ( const SModelData& ModelData : Models )
    {
        CActor* NewActor = Scene->MakeActor();
        NewActor->SetName( ModelData.Name.CStr() );
        NewActor->GetTransform().SetUniformScale( Scale );

        CMeshComponent* MeshComponent = new CMeshComponent( NewActor );
        MeshComponent->Mesh = Mesh::Make( ModelData.Mesh );
        MeshComponent->Material = GEngine->BaseMaterial;

        if (ModelData.MaterialIndex >= 0)
        {
            const SMaterialData& MaterialData = Materials[ModelData.MaterialIndex];

            SMaterialDesc Desc;
            Desc.Albedo    = MaterialData.Diffuse;
            Desc.AO        = MaterialData.AO;
            Desc.Metallic  = MaterialData.Metallic;
            Desc.Roughness = MaterialData.Roughness;

            // TODO: Should probably have a better separation between RHITexture and a Texture

            //TSharedPtr<CMaterial> Material = MeshComponent->Material = DBG_NEW CMaterial( Desc );
            //Material->AlbedoMap = MaterialData.DiffuseTexture;
            //Material->AlbedoMap = MaterialData.DiffuseTexture;
            //Material->AlbedoMap = MaterialData.DiffuseTexture;
            //Material->AlbedoMap = MaterialData.DiffuseTexture;
            //Material->AlbedoMap = MaterialData.DiffuseTexture;
        }

        NewActor->AddComponent( MeshComponent );
    }
}

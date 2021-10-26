#include "SceneData.h"

#include "Engine/Engine.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Components/MeshComponent.h"

#include "Renderer/Resources/Mesh.h"
#include "Renderer/Resources/Material.h"
#include "Renderer/Resources/TextureFactory.h"

void SSceneData::AddToScene( CScene* Scene )
{
    if ( !HasModelData() )
    {
        return;
    }

    TArray<TSharedPtr<CMaterial>> CreatedMaterials;
    if ( !Materials.IsEmpty() )
    {
        for ( const SMaterialData& MaterialData : Materials )
        {
            SMaterialDesc Desc;
            Desc.Albedo = MaterialData.Diffuse;
            Desc.AO = MaterialData.AO;
            Desc.Metallic = MaterialData.Metallic;
            Desc.Roughness = MaterialData.Roughness;

            // TODO: Should probably have a better connection between RHITexture and a Texture

            TSharedPtr<CMaterial> Material = MakeShared<CMaterial>( Desc );
            Material->AlbedoMap = CTextureFactory::LoadFromImage2D( MaterialData.DiffuseTexture.Get(), TextureFactoryFlag_GenerateMips );
            Material->AlbedoMap = Material->AlbedoMap ? Material->AlbedoMap : GEngine->BaseTexture;

            Material->AlphaMask = CTextureFactory::LoadFromImage2D( MaterialData.AlphaMaskTexture.Get(), TextureFactoryFlag_GenerateMips );
            Material->AlphaMask = Material->AlphaMask ? Material->AlphaMask : GEngine->BaseTexture;

            Material->AOMap = CTextureFactory::LoadFromImage2D( MaterialData.AOTexture.Get(), TextureFactoryFlag_GenerateMips );
            Material->AOMap = Material->AOMap ? Material->AOMap : GEngine->BaseTexture;

            Material->MetallicMap = CTextureFactory::LoadFromImage2D( MaterialData.MetallicTexture.Get(), TextureFactoryFlag_GenerateMips );
            Material->MetallicMap = Material->MetallicMap ? Material->MetallicMap : GEngine->BaseTexture;

            Material->NormalMap = CTextureFactory::LoadFromImage2D( MaterialData.NormalTexture.Get(), TextureFactoryFlag_GenerateMips );
            Material->NormalMap = Material->NormalMap ? Material->NormalMap : GEngine->BaseNormal;

            Material->RoughnessMap = CTextureFactory::LoadFromImage2D( MaterialData.RoughnessTexture.Get(), TextureFactoryFlag_GenerateMips );
            Material->RoughnessMap = Material->RoughnessMap ? Material->RoughnessMap : GEngine->BaseTexture;

            Material->HeightMap = GEngine->BaseTexture;

            Material->Init();

            CreatedMaterials.Push( Material );
        }
    }

    Assert( Materials.Size() == CreatedMaterials.Size() );

    for ( const SModelData& ModelData : Models )
    {
        if ( ModelData.Mesh.Hasdata() )
        {
            CActor* NewActor = Scene->MakeActor();
            NewActor->SetName( ModelData.Name.CStr() );
            NewActor->GetTransform().SetUniformScale( Scale );

            CMeshComponent* MeshComponent = dbg_new CMeshComponent( NewActor );
            MeshComponent->Mesh = CMesh::Make( ModelData.Mesh );

            if ( ModelData.MaterialIndex >= 0 )
            {
                MeshComponent->Material = CreatedMaterials[ModelData.MaterialIndex];
            }
            else
            {
                MeshComponent->Material = GEngine->BaseMaterial;
            }

            NewActor->AddComponent( MeshComponent );
        }
    }
}

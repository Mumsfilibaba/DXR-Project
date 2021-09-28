#include "Scene.h"

#include "Components/MeshComponent.h"

#include "Assets/MeshFactory.h"

#include "Rendering/Resources/TextureFactory.h"
#include "Rendering/Resources/Material.h"
#include "Rendering/Resources/Mesh.h"

#include "RenderLayer/Resources.h"

#include <tiny_obj_loader.h>

#include <unordered_map>

Scene::Scene()
    : Actors()
{
}

Scene::~Scene()
{
    for ( CActor* CurrentActor : Actors )
    {
        SafeDelete( CurrentActor );
    }
    Actors.Clear();

    for ( Light* CurrentLight : Lights )
    {
        SafeDelete( CurrentLight );
    }
    Lights.Clear();

    SafeDelete( CurrentCamera );
}

void Scene::Tick( CTimestamp DeltaTime )
{
	for ( CActor* Actor : Actors )
	{
		if ( Actor->IsTickable() )
		{
			Actor->Tick( DeltaTime );
		}
	}
}

void Scene::AddCamera( Camera* InCamera )
{
    if ( CurrentCamera )
    {
        SafeDelete( CurrentCamera );
    }

    CurrentCamera = InCamera;
}

void Scene::AddActor( CActor* InActor )
{
    Assert( InActor != nullptr );
    Actors.Emplace( InActor );

    InActor->OnAddedToScene( this );

    MeshComponent* Component = InActor->GetComponentOfType<MeshComponent>();
    if ( Component )
    {
        AddMeshComponent( Component );
    }
}

void Scene::AddLight( Light* InLight )
{
    Assert( InLight != nullptr );
    Lights.Emplace( InLight );
}

void Scene::OnAddedComponent( CComponent* NewComponent )
{
    MeshComponent* Component = Cast<MeshComponent>( NewComponent );
    if ( Component && Component->Mesh )
    {
        AddMeshComponent( Component );
    }
}

void Scene::AddMeshComponent( MeshComponent* Component )
{
    MeshDrawCommand Command;
    Command.CurrentActor = Component->GetOwningActor();
    Command.Geometry = Component->Mesh->RTGeometry.Get();
    Command.VertexBuffer = Component->Mesh->VertexBuffer.Get();
    Command.IndexBuffer = Component->Mesh->IndexBuffer.Get();
    Command.Material = Component->Material.Get();
    Command.Mesh = Component->Mesh.Get();
    MeshDrawCommands.Push( Command );
}

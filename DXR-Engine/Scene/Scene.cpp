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
    for ( Actor* CurrentActor : Actors )
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

void Scene::Tick( Timestamp DeltaTime )
{
    UNREFERENCED_VARIABLE( DeltaTime );
}

void Scene::AddCamera( Camera* InCamera )
{
    if ( CurrentCamera )
    {
        SafeDelete( CurrentCamera );
    }

    CurrentCamera = InCamera;
}

void Scene::AddActor( Actor* InActor )
{
    Assert( InActor != nullptr );
    Actors.EmplaceBack( InActor );

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
    Lights.EmplaceBack( InLight );
}

void Scene::OnAddedComponent( Component* NewComponent )
{
    MeshComponent* Component = Cast<MeshComponent>( NewComponent );
    if ( Component )
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
    MeshDrawCommands.PushBack( Command );
}

#include "SceneData.h"

#include "Scene/Scene.h"

void SSceneData::AddToScene( Scene* Scene )
{
    Actor* NewActor = DBG_NEW Actor();
    Scene->AddActor( NewActor );


    NewActor->AddComponent();
}

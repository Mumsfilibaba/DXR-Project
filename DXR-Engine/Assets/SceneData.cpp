#include "SceneData.h"

#include "Scene/Scene.h"

void SSceneData::AddToScene( Scene* Scene )
{
    CActor* NewActor = DBG_NEW CActor();
    Scene->AddActor( NewActor );


    //NewActor->AddComponent();
}
